/*
    Copyright 2009 Pino Toscano <pino@kde.org>
    Copyright 2009-2012 Lukáš Tinkl <ltinkl@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "udisksstorageaccess.h"
#include "udisks2.h"

#include <QtCore/QProcess>
#include <QtDBus/QtDBus>
#include <QtGui/QApplication>
#include <QtGui/QWidget>

using namespace Solid::Backends::UDisks2;

StorageAccess::StorageAccess(Device *device)
    : DeviceInterface(device), m_setupInProgress(false), m_teardownInProgress(false), m_passphraseRequested(false)
{
    connect(device, SIGNAL(changed()), this, SLOT(slotChanged()));
    updateCache();

    // Delay connecting to DBus signals to avoid the related time penalty
    // in hot paths such as predicate matching
    QTimer::singleShot(0, this, SLOT(connectDBusSignals()));
}

StorageAccess::~StorageAccess()
{
}

void StorageAccess::connectDBusSignals()
{
    m_device->registerAction("setup", this,
                             SLOT(slotSetupRequested()),
                             SLOT(slotSetupDone(int, const QString&)));

    m_device->registerAction("teardown", this,
                             SLOT(slotTeardownRequested()),
                             SLOT(slotTeardownDone(int, const QString&)));
}

bool StorageAccess::isLuksDevice() const
{
    // FIXME check
    return m_device->isEncrypted(); // encrypted (and unlocked) device
}

bool StorageAccess::isAccessible() const
{
    if (isLuksDevice()) { // check if the cleartext slave is mounted
        Device holderDevice(m_device->prop("CryptoBackingDevice").value<QDBusObjectPath>().path());
        return holderDevice.isMounted();
    }

    return m_device->isMounted();
}

QString StorageAccess::filePath() const
{
    if (!isAccessible())
        return QString();

    QByteArrayList mntPoints;

    if (isLuksDevice()) {  // encrypted (and unlocked) device
        QString path = m_device->prop("CryptoBackingDevice").value<QDBusObjectPath>().path();
        if (path.isEmpty() || path == "/")
            return QString();
        Device holderDevice(path);
        mntPoints = holderDevice.prop("MountPoints").value<QByteArrayList>();
        if (!mntPoints.isEmpty())
            return QFile::decodeName(mntPoints.first()); // FIXME Solid doesn't support multiple mount points
        else
            return QString();
    }

    mntPoints = m_device->prop("MountPoints").value<QByteArrayList>();

    if (!mntPoints.isEmpty())
        return QFile::decodeName(mntPoints.first()); // FIXME Solid doesn't support multiple mount points
    else
        return QString();
}

bool StorageAccess::isIgnored() const
{
    return m_device->prop("HintIgnore").toBool(); // FIXME tune
}

bool StorageAccess::setup()
{
    if ( m_teardownInProgress || m_setupInProgress )
        return false;
    m_setupInProgress = true;
    m_device->broadcastActionRequested("setup");

    if (m_device->isEncrypted())
        return requestPassphrase();
    else
        return mount();
}

bool StorageAccess::teardown()
{
    if ( m_teardownInProgress || m_setupInProgress )
        return false;
    m_teardownInProgress = true;
    m_device->broadcastActionRequested("teardown");

    return unmount();
}

void StorageAccess::slotChanged()
{
    const bool old_isAccessible = m_isAccessible;
    updateCache();

    if (old_isAccessible != m_isAccessible)
    {
        Q_EMIT accessibilityChanged(m_isAccessible, m_device->udi());  // FIXME account for encrypted devices
    }
}

void StorageAccess::updateCache()
{
    m_isAccessible = isAccessible();
}

void StorageAccess::slotDBusReply( const QDBusMessage & reply )
{
    Q_UNUSED(reply);

    if (m_setupInProgress)
    {
        if (isLuksDevice() && !isAccessible())  // unlocked device, now mount it
            mount();

        else // Don't broadcast setupDone unless the setup is really done. (Fix kde#271156)
        {
            m_setupInProgress = false;
            m_device->broadcastActionDone("setup");

            slotChanged();
        }
    }
    else if (m_teardownInProgress)  // FIXME
    {
        QString clearTextPath = m_device->prop("LuksHolder").value<QDBusObjectPath>().path();
        if (isLuksDevice() && clearTextPath != "/") // unlocked device, lock it
        {
            callCryptoTeardown();
        }
        else if (m_device->prop("DeviceIsLuksCleartext").toBool()) {
            callCryptoTeardown(true); // Lock crypted parent
        }
        else
        {
            if (m_device->prop("Ejectable").toBool() && !m_device->prop("Optical").toBool()) // optical drives have their Eject method
            {
                // try to "eject" (aka safely remove) from the (parent) drive, e.g. SD card from a reader
                QString drivePath = m_device->prop("Drive").value<QDBusObjectPath>().path();
                if (!drivePath.isEmpty() || drivePath != "/")
                {
                    QDBusConnection c = QDBusConnection::systemBus();
                    QDBusMessage msg = QDBusMessage::createMethodCall(UD2_DBUS_SERVICE, drivePath, UD2_DBUS_INTERFACE_DRIVE, "Eject");
                    msg << QVariantMap();   // options, unused now
                    c.call(msg, QDBus::NoBlock);
                }

            }

            m_teardownInProgress = false;
            m_device->broadcastActionDone("teardown");

            slotChanged();
        }
    }
}

void StorageAccess::slotDBusError( const QDBusError & error )
{
    if (m_setupInProgress)
    {
        m_setupInProgress = false;
        m_device->broadcastActionDone("setup", m_device->errorToSolidError(error.name()),
                                      m_device->errorToString(error.name()) + ": " +error.message());

        slotChanged();
    }
    else if (m_teardownInProgress)
    {
        m_teardownInProgress = false;
        m_device->broadcastActionDone("teardown", m_device->errorToSolidError(error.name()),
                                      m_device->errorToString(error.name()) + ": " + error.message());
        slotChanged();
    }
}

void StorageAccess::slotSetupRequested()
{
    m_setupInProgress = true;
    Q_EMIT setupRequested(m_device->udi());
}

void StorageAccess::slotSetupDone(int error, const QString &errorString)
{
    m_setupInProgress = false;
    Q_EMIT setupDone(static_cast<Solid::ErrorType>(error), errorString, m_device->udi());
}

void StorageAccess::slotTeardownRequested()
{
    m_teardownInProgress = true;
    Q_EMIT teardownRequested(m_device->udi());
}

void StorageAccess::slotTeardownDone(int error, const QString &errorString)
{
    m_teardownInProgress = false;
    Q_EMIT teardownDone(static_cast<Solid::ErrorType>(error), errorString, m_device->udi());
}

bool StorageAccess::mount()
{
    QString path = m_device->udi();
    QString fstype;

    if (isLuksDevice()) { // mount options for the cleartext volume
        path = m_device->prop("LuksHolder").value<QDBusObjectPath>().path();
        Device holderDevice(path);
        fstype = holderDevice.prop("IdType").toString();
    }

    QDBusConnection c = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(UD2_DBUS_SERVICE, path, UD2_DBUS_INTERFACE_FILESYSTEM, "Mount");

    if (m_device->prop("IdUsage").toString() == "filesystem")
        fstype = m_device->prop("IdType").toString();

    msg << fstype;
    msg << QVariantMap();   // options, unused now

    return c.callWithCallback(msg, this,
                              SLOT(slotDBusReply(const QDBusMessage &)),
                              SLOT(slotDBusError(const QDBusError &)));
}

bool StorageAccess::unmount()
{
    QString path = m_device->udi();

    if (isLuksDevice()) { // unmount options for the cleartext volume
        path = m_device->prop("LuksHolder").value<QDBusObjectPath>().path();
    }

    QDBusConnection c = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(UD2_DBUS_SERVICE, path, UD2_DBUS_INTERFACE_FILESYSTEM, "Unmount");

    msg << QVariantMap();   // options, unused now

    return c.callWithCallback(msg, this,
                              SLOT(slotDBusReply(const QDBusMessage &)),
                              SLOT(slotDBusError(const QDBusError &)),
                              s_unmountTimeout);
}

QString StorageAccess::generateReturnObjectPath()
{
    static int number = 1;

    return "/org/kde/solid/UDisks2StorageAccess_"+QString::number(number++);
}

bool StorageAccess::requestPassphrase()
{
    QString udi = m_device->udi();
    QString returnService = QDBusConnection::sessionBus().baseService();
    m_lastReturnObject = generateReturnObjectPath();

    QDBusConnection::sessionBus().registerObject(m_lastReturnObject, this, QDBusConnection::ExportScriptableSlots);

    QWidget *activeWindow = QApplication::activeWindow();
    uint wId = 0;
    if (activeWindow!=0)
        wId = (uint)activeWindow->winId();

    QString appId = QCoreApplication::applicationName();

    QDBusInterface soliduiserver("org.kde.kded", "/modules/soliduiserver", "org.kde.SolidUiServer");
    QDBusReply<void> reply = soliduiserver.call("showPassphraseDialog", udi, returnService,
                                                m_lastReturnObject, wId, appId);
    m_passphraseRequested = reply.isValid();
    if (!m_passphraseRequested)
        qWarning() << "Failed to call the SolidUiServer, D-Bus said:" << reply.error();

    return m_passphraseRequested;
}

void StorageAccess::passphraseReply( const QString & passphrase )
{
    if (m_passphraseRequested)
    {
        QDBusConnection::sessionBus().unregisterObject(m_lastReturnObject);
        m_passphraseRequested = false;
        if (!passphrase.isEmpty())
            callCryptoSetup(passphrase);
        else
        {
            m_setupInProgress = false;
            m_device->broadcastActionDone("setup");
        }
    }
}

void StorageAccess::callCryptoSetup( const QString & passphrase )
{
    QDBusConnection c = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(UD2_DBUS_SERVICE, m_device->udi(), UD2_DBUS_INTERFACE_ENCRYPTED, "Unlock");

    msg << passphrase.toUtf8();  // QByteArray
    msg << QVariantMap();   // options, unused now

    c.callWithCallback(msg, this,
                       SLOT(slotDBusReply(const QDBusMessage &)),
                       SLOT(slotDBusError(const QDBusError &)));
}

bool StorageAccess::callCryptoTeardown(bool actOnParent)
{
    QDBusConnection c = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(UD2_DBUS_SERVICE,
                                                      actOnParent ? (m_device->prop("CryptoBackingDevice").value<QDBusObjectPath>().path()) : m_device->udi(),
                                                      UD2_DBUS_INTERFACE_ENCRYPTED, "Lock");
    msg << QVariantMap();   // options, unused now

    return c.callWithCallback(msg, this,
                              SLOT(slotDBusReply(const QDBusMessage &)),
                              SLOT(slotDBusError(const QDBusError &)));
}
