/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
    Copyright (C) 2008 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "wtf/Platform.h"

#if ENABLE(SVG_FONTS)
#include "SVGAltGlyphElement.h"

#include "ExceptionCode.h"
#include "RenderInline.h"
#include "RenderSVGTSpan.h"
#include "SVGGlyphElement.h"
#include "SVGNames.h"
#include "XLinkNames.h"

#include "dom/dom_exception.h"

namespace WebCore {

SVGAltGlyphElement::SVGAltGlyphElement(const QualifiedName& tagName, Document* doc)
    : SVGTextPositioningElement(tagName, doc)
{
}

SVGAltGlyphElement::~SVGAltGlyphElement()
{
}

void SVGAltGlyphElement::setGlyphRef(const DOMString& type, ExceptionCode& ec)
{
    Q_UNUSED(type);
    ec = DOMException::NO_MODIFICATION_ALLOWED_ERR;
}

DOMString SVGAltGlyphElement::glyphRef() const
{
    return getAttribute(SVGNames::glyphRefAttr);
}

void SVGAltGlyphElement::setFormat(const DOMString& type, ExceptionCode& ec)
{
    Q_UNUSED(type);
    ec = DOMException::NO_MODIFICATION_ALLOWED_ERR;
}

DOMString SVGAltGlyphElement::format() const
{
    return getAttribute(SVGNames::formatAttr);
}

bool SVGAltGlyphElement::childShouldCreateRenderer(Node* child) const
{
    if (child->isTextNode())
        return true;
    return false;
}

RenderObject* SVGAltGlyphElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGTSpan(this);
}

SVGGlyphElement* SVGAltGlyphElement::glyphElement() const
{
    Element* elt = document()->getElementById(getTarget(getAttribute(XLinkNames::hrefAttr)));
    if (!elt || !elt->hasTagName(SVGNames::glyphTag))
        return 0;
    return static_cast<SVGGlyphElement*>(elt);
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
