/* This file is part of the KDE libraries
    Copyright (C) 2005, 2006 KJSEmbed Authors
    See included AUTHORS file.

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

println("Start color test");
var color = new QColor("red");
println("Color: " + color);
for( x in color )
	println( x );
if( color.red() != 255 )
	println("Error red color is " + color.red() );
else
	println("Red okay");
if( color.green() != 0 )
	println("Error green color is " + color.green() );
else
	println("Green okay");

color.setRed( 155 );
if( color.red() != 155 )
	println("Error red color is " + color.red() );
else
	println("Red okay");

println( "Color " + color );
color.setGreen( 250 );
color.setBlue( 100 );

var darkColor = color.dark();
println( "Dark color " + darkColor );
