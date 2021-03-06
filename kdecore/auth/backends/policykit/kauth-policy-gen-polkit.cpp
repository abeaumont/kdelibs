/*
*   Copyright (C) 2008 Nicola Gigante <nicola.gigante@gmail.com>
*   Copyright (C) 2009-2010 Dario Freddi <drf@kde.org>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation; either version 2.1 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public License
*   along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .
*/

#include <auth/policy-gen/policy-gen.h>

#include <cstdio>
#include <QDebug>
#include <QTextStream>

const char header[] = ""
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                      "<!DOCTYPE policyconfig PUBLIC\n"
                      "\"-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN\"\n"
                      "\"http://www.freedesktop.org/standards/PolicyKit/1.0/policyconfig.dtd\">\n"
                      "<policyconfig>\n";

const char policy_tag[] = ""
                          "      <defaults>\n"
                          "         <allow_inactive>%1</allow_inactive>\n"
                          "         <allow_active>%2</allow_active>\n"
                          "      </defaults>\n";

const char dent[] = "   ";

void output(QList<Action> actions, QHash<QString, QString> domain)
{
    QTextStream out(stdout);
    out.setCodec("UTF-8");

    out << header;

    // Blacklisted characters + replacements
    QHash< QChar, QString > blacklist;
    blacklist.insert(QChar::fromLatin1('&'), QString::fromLatin1("&amp;"));

    if (domain.contains(QLatin1String("vendor"))) {
        QHash< QChar, QString >::const_iterator blI;
        QString vendor = domain[QLatin1String("vendor")];
        for (blI = blacklist.constBegin(); blI != blacklist.constEnd(); ++blI) {
            vendor.replace(blI.key(), blI.value());
        }
        out << "<vendor>" << vendor << "</vendor>\n";
    }
    if (domain.contains(QLatin1String("vendorurl"))) {
        out << "<vendor_url>" << domain[QLatin1String("vendorurl")] << "</vendor_url>\n";
    }
    if (domain.contains(QLatin1String("icon"))) {
        out << "<icon_name>" << domain[QLatin1String("icon")] << "</icon_name>\n";
    }

    foreach (const Action &action, actions) {
        out << dent << "<action id=\"" << action.name << "\" >\n";

        for (QHash< QString, QString >::const_iterator i = action.messages.constBegin(); i != action.messages.constEnd(); ++i) {
            out << dent << dent << "<description";
            if (i.key() != QLatin1String("en")) {
                out << " xml:lang=\"" << i.key() << '"';
            }

            QHash< QChar, QString >::const_iterator blI;
            QString description = i.value();
            for (blI = blacklist.constBegin(); blI != blacklist.constEnd(); ++blI) {
                description.replace(blI.key(), blI.value());
            }

            out << '>' << description << "</description>\n";
        }

        for (QHash< QString, QString>::const_iterator i = action.descriptions.constBegin();
             i != action.descriptions.constEnd();
             ++i) {
            out << dent << dent << "<message";
            if (i.key() != QLatin1String("en")) {
                out << " xml:lang=\"" << i.key() << '"';
            }

            QHash< QChar, QString >::const_iterator blI;
            QString message = i.value();
            for (blI = blacklist.constBegin(); blI != blacklist.constEnd(); ++blI) {
                message.replace(blI.key(), blI.value());
            }

            out << '>' << message << "</message>\n";
        }

        QString policy = action.policy;
        QString policyInactive = action.policyInactive.isEmpty() ? QLatin1String("no") : action.policyInactive;
        if (!action.persistence.isEmpty() && policy != QLatin1String("yes") && policy != QLatin1String("no")) {
            policy += QLatin1String("_keep_") + action.persistence;
        }
        if (!action.persistence.isEmpty() && policyInactive != QLatin1String("yes") &&
            policyInactive != QLatin1String("no")) {
            policyInactive += QLatin1String("_keep_") + action.persistence;
        }

        out << QString(QLatin1String(policy_tag)).arg(policyInactive).arg(policy);

        out << dent << "</action>\n";
    }

    out << "</policyconfig>\n";
}
