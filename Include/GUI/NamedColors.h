// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 09.02.2020 21:20

#pragma once

#include <stdint.h>

#include <Utils/String.h>

namespace os
{

// Named colors from SVG (https://www.w3.org/TR/css-color-3/)
#define DEF_NAMED_COLOR_ENUM(NAME) NAME = PString::hash_string_literal(#NAME)
enum class NamedColors : uint32_t
{
    DEF_NAMED_COLOR_ENUM(aliceblue),
    DEF_NAMED_COLOR_ENUM(antiquewhite),
    DEF_NAMED_COLOR_ENUM(aqua),
    DEF_NAMED_COLOR_ENUM(aquamarine),
    DEF_NAMED_COLOR_ENUM(azure),
    DEF_NAMED_COLOR_ENUM(beige),
    DEF_NAMED_COLOR_ENUM(bisque),
    DEF_NAMED_COLOR_ENUM(black),
    DEF_NAMED_COLOR_ENUM(blanchedalmond),
    DEF_NAMED_COLOR_ENUM(blue),
    DEF_NAMED_COLOR_ENUM(blueviolet),
    DEF_NAMED_COLOR_ENUM(brown),
    DEF_NAMED_COLOR_ENUM(burlywood),
    DEF_NAMED_COLOR_ENUM(cadetblue),
    DEF_NAMED_COLOR_ENUM(chartreuse),
    DEF_NAMED_COLOR_ENUM(chocolate),
    DEF_NAMED_COLOR_ENUM(coral),
    DEF_NAMED_COLOR_ENUM(cornflowerblue),
    DEF_NAMED_COLOR_ENUM(cornsilk),
    DEF_NAMED_COLOR_ENUM(crimson),
    DEF_NAMED_COLOR_ENUM(cyan),
    DEF_NAMED_COLOR_ENUM(darkblue),
    DEF_NAMED_COLOR_ENUM(darkcyan),
    DEF_NAMED_COLOR_ENUM(darkgoldenrod),
    DEF_NAMED_COLOR_ENUM(darkgray),
    DEF_NAMED_COLOR_ENUM(darkgreen),
    DEF_NAMED_COLOR_ENUM(darkgrey),
    DEF_NAMED_COLOR_ENUM(darkkhaki),
    DEF_NAMED_COLOR_ENUM(darkmagenta),
    DEF_NAMED_COLOR_ENUM(darkolivegreen),
    DEF_NAMED_COLOR_ENUM(darkorange),
    DEF_NAMED_COLOR_ENUM(darkorchid),
    DEF_NAMED_COLOR_ENUM(darkred),
    DEF_NAMED_COLOR_ENUM(darksalmon),
    DEF_NAMED_COLOR_ENUM(darkseagreen),
    DEF_NAMED_COLOR_ENUM(darkslateblue),
    DEF_NAMED_COLOR_ENUM(darkslategray),
    DEF_NAMED_COLOR_ENUM(darkslategrey),
    DEF_NAMED_COLOR_ENUM(darkturquoise),
    DEF_NAMED_COLOR_ENUM(darkviolet),
    DEF_NAMED_COLOR_ENUM(deeppink),
    DEF_NAMED_COLOR_ENUM(deepskyblue),
    DEF_NAMED_COLOR_ENUM(dimgray),
    DEF_NAMED_COLOR_ENUM(dimgrey),
    DEF_NAMED_COLOR_ENUM(dodgerblue),
    DEF_NAMED_COLOR_ENUM(firebrick),
    DEF_NAMED_COLOR_ENUM(floralwhite),
    DEF_NAMED_COLOR_ENUM(forestgreen),
    DEF_NAMED_COLOR_ENUM(fuchsia),
    DEF_NAMED_COLOR_ENUM(gainsboro),
    DEF_NAMED_COLOR_ENUM(ghostwhite),
    DEF_NAMED_COLOR_ENUM(gold),
    DEF_NAMED_COLOR_ENUM(goldenrod),
    DEF_NAMED_COLOR_ENUM(gray),
    DEF_NAMED_COLOR_ENUM(grey),
    DEF_NAMED_COLOR_ENUM(green),
    DEF_NAMED_COLOR_ENUM(greenyellow),
    DEF_NAMED_COLOR_ENUM(honeydew),
    DEF_NAMED_COLOR_ENUM(hotpink),
    DEF_NAMED_COLOR_ENUM(indianred),
    DEF_NAMED_COLOR_ENUM(indigo),
    DEF_NAMED_COLOR_ENUM(ivory),
    DEF_NAMED_COLOR_ENUM(khaki),
    DEF_NAMED_COLOR_ENUM(lavender),
    DEF_NAMED_COLOR_ENUM(lavenderblush),
    DEF_NAMED_COLOR_ENUM(lawngreen),
    DEF_NAMED_COLOR_ENUM(lemonchiffon),
    DEF_NAMED_COLOR_ENUM(lightblue),
    DEF_NAMED_COLOR_ENUM(lightcoral),
    DEF_NAMED_COLOR_ENUM(lightcyan),
    DEF_NAMED_COLOR_ENUM(lightgoldenrodyellow),
    DEF_NAMED_COLOR_ENUM(lightgray),
    DEF_NAMED_COLOR_ENUM(lightgreen),
    DEF_NAMED_COLOR_ENUM(lightgrey),
    DEF_NAMED_COLOR_ENUM(lightpink),
    DEF_NAMED_COLOR_ENUM(lightsalmon),
    DEF_NAMED_COLOR_ENUM(lightseagreen),
    DEF_NAMED_COLOR_ENUM(lightskyblue),
    DEF_NAMED_COLOR_ENUM(lightslategray),
    DEF_NAMED_COLOR_ENUM(lightslategrey),
    DEF_NAMED_COLOR_ENUM(lightsteelblue),
    DEF_NAMED_COLOR_ENUM(lightyellow),
    DEF_NAMED_COLOR_ENUM(lime),
    DEF_NAMED_COLOR_ENUM(limegreen),
    DEF_NAMED_COLOR_ENUM(linen),
    DEF_NAMED_COLOR_ENUM(magenta),
    DEF_NAMED_COLOR_ENUM(maroon),
    DEF_NAMED_COLOR_ENUM(mediumaquamarine),
    DEF_NAMED_COLOR_ENUM(mediumblue),
    DEF_NAMED_COLOR_ENUM(mediumorchid),
    DEF_NAMED_COLOR_ENUM(mediumpurple),
    DEF_NAMED_COLOR_ENUM(mediumseagreen),
    DEF_NAMED_COLOR_ENUM(mediumslateblue),
    DEF_NAMED_COLOR_ENUM(mediumspringgreen),
    DEF_NAMED_COLOR_ENUM(mediumturquoise),
    DEF_NAMED_COLOR_ENUM(mediumvioletred),
    DEF_NAMED_COLOR_ENUM(midnightblue),
    DEF_NAMED_COLOR_ENUM(mintcream),
    DEF_NAMED_COLOR_ENUM(mistyrose),
    DEF_NAMED_COLOR_ENUM(moccasin),
    DEF_NAMED_COLOR_ENUM(navajowhite),
    DEF_NAMED_COLOR_ENUM(navy),
    DEF_NAMED_COLOR_ENUM(oldlace),
    DEF_NAMED_COLOR_ENUM(olive),
    DEF_NAMED_COLOR_ENUM(olivedrab),
    DEF_NAMED_COLOR_ENUM(orange),
    DEF_NAMED_COLOR_ENUM(orangered),
    DEF_NAMED_COLOR_ENUM(orchid),
    DEF_NAMED_COLOR_ENUM(palegoldenrod),
    DEF_NAMED_COLOR_ENUM(palegreen),
    DEF_NAMED_COLOR_ENUM(paleturquoise),
    DEF_NAMED_COLOR_ENUM(palevioletred),
    DEF_NAMED_COLOR_ENUM(papayawhip),
    DEF_NAMED_COLOR_ENUM(peachpuff),
    DEF_NAMED_COLOR_ENUM(peru),
    DEF_NAMED_COLOR_ENUM(pink),
    DEF_NAMED_COLOR_ENUM(plum),
    DEF_NAMED_COLOR_ENUM(powderblue),
    DEF_NAMED_COLOR_ENUM(purple),
    DEF_NAMED_COLOR_ENUM(red),
    DEF_NAMED_COLOR_ENUM(rosybrown),
    DEF_NAMED_COLOR_ENUM(royalblue),
    DEF_NAMED_COLOR_ENUM(saddlebrown),
    DEF_NAMED_COLOR_ENUM(salmon),
    DEF_NAMED_COLOR_ENUM(sandybrown),
    DEF_NAMED_COLOR_ENUM(seagreen),
    DEF_NAMED_COLOR_ENUM(seashell),
    DEF_NAMED_COLOR_ENUM(sienna),
    DEF_NAMED_COLOR_ENUM(silver),
    DEF_NAMED_COLOR_ENUM(skyblue),
    DEF_NAMED_COLOR_ENUM(slateblue),
    DEF_NAMED_COLOR_ENUM(slategray),
    DEF_NAMED_COLOR_ENUM(slategrey),
    DEF_NAMED_COLOR_ENUM(snow),
    DEF_NAMED_COLOR_ENUM(springgreen),
    DEF_NAMED_COLOR_ENUM(steelblue),
    DEF_NAMED_COLOR_ENUM(tan),
    DEF_NAMED_COLOR_ENUM(teal),
    DEF_NAMED_COLOR_ENUM(thistle),
    DEF_NAMED_COLOR_ENUM(tomato),
    DEF_NAMED_COLOR_ENUM(turquoise),
    DEF_NAMED_COLOR_ENUM(violet),
    DEF_NAMED_COLOR_ENUM(wheat),
    DEF_NAMED_COLOR_ENUM(white),
    DEF_NAMED_COLOR_ENUM(whitesmoke),
    DEF_NAMED_COLOR_ENUM(yellow),
    DEF_NAMED_COLOR_ENUM(yellowgreen),
};
#undef DEF_NAMED_COLOR_ENUM


} // namespace os

using PNamedColors = os::NamedColors;
