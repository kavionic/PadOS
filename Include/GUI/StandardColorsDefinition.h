// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 12.11.2025 23:00

#pragma once

#include <Utils/combsort.h>
#include <GUI/NamedColors.h>

struct PNamedColorNode
{
    constexpr PNamedColorNode(PNamedColors colorID, PColor color) : NameID(colorID), ColorValue(color) {}

    constexpr bool operator < (const PNamedColorNode& rhs) { return NameID < rhs.NameID; }
    constexpr bool operator > (const PNamedColorNode& rhs) { return NameID > rhs.NameID; }
    constexpr bool operator == (const PNamedColorNode& rhs) { return NameID == rhs.NameID; }

    PNamedColors NameID;
    PColor       ColorValue;
};

// Generate an array sorted on NamedColorNode::NameID to make it possible to use binary-search when looking up colors.
static constexpr auto PStandardColorsTable = combsort_immutable(std::array<PNamedColorNode, 147>
{
    PNamedColorNode(PNamedColors::aliceblue,            PColor::FromRGB32A(240, 248, 255)),  //  #F0F8FF
    PNamedColorNode(PNamedColors::antiquewhite,         PColor::FromRGB32A(250, 235, 215)),  //  #FAEBD7
    PNamedColorNode(PNamedColors::aqua,                 PColor::FromRGB32A(0,   255, 255)),  //  #00FFFF
    PNamedColorNode(PNamedColors::aquamarine,           PColor::FromRGB32A(127, 255, 212)),  //  #7FFFD4
    PNamedColorNode(PNamedColors::beige,                PColor::FromRGB32A(245, 245, 220)),  //  #F5F5DC
    PNamedColorNode(PNamedColors::azure,                PColor::FromRGB32A(240, 255, 255)),  //  #F0FFFF
    PNamedColorNode(PNamedColors::bisque,               PColor::FromRGB32A(255, 228, 196)),  //  #FFE4C4
    PNamedColorNode(PNamedColors::black,                PColor::FromRGB32A(0,   0,   0)),    //  #000000
    PNamedColorNode(PNamedColors::blanchedalmond,       PColor::FromRGB32A(255, 235, 205)),  //  #FFEBCD
    PNamedColorNode(PNamedColors::blue,                 PColor::FromRGB32A(0,   0,   255)),  //  #0000FF
    PNamedColorNode(PNamedColors::blueviolet,           PColor::FromRGB32A(138, 43,  226)),  //  #8A2BE2
    PNamedColorNode(PNamedColors::brown,                PColor::FromRGB32A(165, 42,  42)),   //  #A52A2A
    PNamedColorNode(PNamedColors::burlywood,            PColor::FromRGB32A(222, 184, 135)),  //  #DEB887
    PNamedColorNode(PNamedColors::cadetblue,            PColor::FromRGB32A(95,  158, 160)),  //  #5F9EA0
    PNamedColorNode(PNamedColors::chartreuse,           PColor::FromRGB32A(127, 255, 0)),    //  #7FFF00
    PNamedColorNode(PNamedColors::chocolate,            PColor::FromRGB32A(210, 105, 30)),   //  #D2691E
    PNamedColorNode(PNamedColors::coral,                PColor::FromRGB32A(255, 127, 80)),   //  #FF7F50
    PNamedColorNode(PNamedColors::cornflowerblue,       PColor::FromRGB32A(100, 149, 237)),  //  #6495ED
    PNamedColorNode(PNamedColors::cornsilk,             PColor::FromRGB32A(255, 248, 220)),  //  #FFF8DC
    PNamedColorNode(PNamedColors::crimson,              PColor::FromRGB32A(220, 20,  60)),   //  #DC143C
    PNamedColorNode(PNamedColors::cyan,                 PColor::FromRGB32A(0,   255, 255)),  //  #00FFFF
    PNamedColorNode(PNamedColors::darkblue,             PColor::FromRGB32A(0,   0,   139)),  //  #00008B
    PNamedColorNode(PNamedColors::darkcyan,             PColor::FromRGB32A(0,   139, 139)),  //  #008B8B
    PNamedColorNode(PNamedColors::darkgoldenrod,        PColor::FromRGB32A(184, 134, 11)),   //  #B8860B
    PNamedColorNode(PNamedColors::darkgray,             PColor::FromRGB32A(169, 169, 169)),  //  #A9A9A9
    PNamedColorNode(PNamedColors::darkgreen,            PColor::FromRGB32A(0,   100, 0)),    //  #006400
    PNamedColorNode(PNamedColors::darkgrey,             PColor::FromRGB32A(169, 169, 169)),  //  #A9A9A9
    PNamedColorNode(PNamedColors::darkkhaki,            PColor::FromRGB32A(189, 183, 107)),  //  #BDB76B
    PNamedColorNode(PNamedColors::darkmagenta,          PColor::FromRGB32A(139, 0,   139)),  //  #8B008B
    PNamedColorNode(PNamedColors::darkolivegreen,       PColor::FromRGB32A(85,  107, 47)),   //  #556B2F
    PNamedColorNode(PNamedColors::darkorange,           PColor::FromRGB32A(255, 140, 0)),    //  #FF8C00
    PNamedColorNode(PNamedColors::darkorchid,           PColor::FromRGB32A(153, 50,  204)),  //  #9932CC
    PNamedColorNode(PNamedColors::darkred,              PColor::FromRGB32A(139, 0,   0)),    //  #8B0000
    PNamedColorNode(PNamedColors::darksalmon,           PColor::FromRGB32A(233, 150, 122)),  //  #E9967A
    PNamedColorNode(PNamedColors::darkseagreen,         PColor::FromRGB32A(143, 188, 143)),  //  #8FBC8F
    PNamedColorNode(PNamedColors::darkslateblue,        PColor::FromRGB32A(72,  61,  139)),  //  #483D8B
    PNamedColorNode(PNamedColors::darkslategray,        PColor::FromRGB32A(47,  79,  79)),   //  #2F4F4F
    PNamedColorNode(PNamedColors::darkslategrey,        PColor::FromRGB32A(47,  79,  79)),   //  #2F4F4F
    PNamedColorNode(PNamedColors::darkturquoise,        PColor::FromRGB32A(0,   206, 209)),  //  #00CED1
    PNamedColorNode(PNamedColors::darkviolet,           PColor::FromRGB32A(148, 0,   211)),  //  #9400D3
    PNamedColorNode(PNamedColors::deeppink,             PColor::FromRGB32A(255, 20,  147)),  //  #FF1493
    PNamedColorNode(PNamedColors::deepskyblue,          PColor::FromRGB32A(0,   191, 255)),  //  #00BFFF
    PNamedColorNode(PNamedColors::dimgray,              PColor::FromRGB32A(105, 105, 105)),  //  #696969
    PNamedColorNode(PNamedColors::dimgrey,              PColor::FromRGB32A(105, 105, 105)),  //  #696969
    PNamedColorNode(PNamedColors::dodgerblue,           PColor::FromRGB32A(30,  144, 255)),  //  #1E90FF
    PNamedColorNode(PNamedColors::firebrick,            PColor::FromRGB32A(178, 34,  34)),   //  #B22222
    PNamedColorNode(PNamedColors::floralwhite,          PColor::FromRGB32A(255, 250, 240)),  //  #FFFAF0
    PNamedColorNode(PNamedColors::forestgreen,          PColor::FromRGB32A(34,  139, 34)),   //  #228B22
    PNamedColorNode(PNamedColors::fuchsia,              PColor::FromRGB32A(255, 0,   255)),  //  #FF00FF
    PNamedColorNode(PNamedColors::gainsboro,            PColor::FromRGB32A(220, 220, 220)),  //  #DCDCDC
    PNamedColorNode(PNamedColors::ghostwhite,           PColor::FromRGB32A(248, 248, 255)),  //  #F8F8FF
    PNamedColorNode(PNamedColors::gold,                 PColor::FromRGB32A(255, 215, 0)),    //  #FFD700
    PNamedColorNode(PNamedColors::goldenrod,            PColor::FromRGB32A(218, 165, 32)),   //  #DAA520
    PNamedColorNode(PNamedColors::gray,                 PColor::FromRGB32A(128, 128, 128)),  //  #808080
    PNamedColorNode(PNamedColors::green,                PColor::FromRGB32A(0,   128, 0)),    //  #008000
    PNamedColorNode(PNamedColors::greenyellow,          PColor::FromRGB32A(173, 255, 47)),   //  #ADFF2F
    PNamedColorNode(PNamedColors::grey,                 PColor::FromRGB32A(128, 128, 128)),  //  #808080
    PNamedColorNode(PNamedColors::honeydew,             PColor::FromRGB32A(240, 255, 240)),  //  #F0FFF0
    PNamedColorNode(PNamedColors::hotpink,              PColor::FromRGB32A(255, 105, 180)),  //  #FF69B4
    PNamedColorNode(PNamedColors::indianred,            PColor::FromRGB32A(205, 92,  92)),   //  #CD5C5C
    PNamedColorNode(PNamedColors::indigo,               PColor::FromRGB32A(75,  0,   130)),  //  #4B0082
    PNamedColorNode(PNamedColors::ivory,                PColor::FromRGB32A(255, 255, 240)),  //  #FFFFF0
    PNamedColorNode(PNamedColors::khaki,                PColor::FromRGB32A(240, 230, 140)),  //  #F0E68C
    PNamedColorNode(PNamedColors::lavender,             PColor::FromRGB32A(230, 230, 250)),  //  #E6E6FA
    PNamedColorNode(PNamedColors::lavenderblush,        PColor::FromRGB32A(255, 240, 245)),  //  #FFF0F5
    PNamedColorNode(PNamedColors::lawngreen,            PColor::FromRGB32A(124, 252, 0)),    //  #7CFC00
    PNamedColorNode(PNamedColors::lemonchiffon,         PColor::FromRGB32A(255, 250, 205)),  //  #FFFACD
    PNamedColorNode(PNamedColors::lightblue,            PColor::FromRGB32A(173, 216, 230)),  //  #ADD8E6
    PNamedColorNode(PNamedColors::lightcoral,           PColor::FromRGB32A(240, 128, 128)),  //  #F08080
    PNamedColorNode(PNamedColors::lightcyan,            PColor::FromRGB32A(224, 255, 255)),  //  #E0FFFF
    PNamedColorNode(PNamedColors::lightgoldenrodyellow, PColor::FromRGB32A(250, 250, 210)),  //  #FAFAD2
    PNamedColorNode(PNamedColors::lightgray,            PColor::FromRGB32A(211, 211, 211)),  //  #D3D3D3
    PNamedColorNode(PNamedColors::lightgreen,           PColor::FromRGB32A(144, 238, 144)),  //  #90EE90
    PNamedColorNode(PNamedColors::lightgrey,            PColor::FromRGB32A(211, 211, 211)),  //  #D3D3D3
    PNamedColorNode(PNamedColors::lightpink,            PColor::FromRGB32A(255, 182, 193)),  //  #FFB6C1
    PNamedColorNode(PNamedColors::lightsalmon,          PColor::FromRGB32A(255, 160, 122)),  //  #FFA07A
    PNamedColorNode(PNamedColors::lightseagreen,        PColor::FromRGB32A(32,  178, 170)),  //  #20B2AA
    PNamedColorNode(PNamedColors::lightskyblue,         PColor::FromRGB32A(135, 206, 250)),  //  #87CEFA
    PNamedColorNode(PNamedColors::lightslategray,       PColor::FromRGB32A(119, 136, 153)),  //  #778899
    PNamedColorNode(PNamedColors::lightslategrey,       PColor::FromRGB32A(119, 136, 153)),  //  #778899
    PNamedColorNode(PNamedColors::lightsteelblue,       PColor::FromRGB32A(176, 196, 222)),  //  #B0C4DE
    PNamedColorNode(PNamedColors::lightyellow,          PColor::FromRGB32A(255, 255, 224)),  //  #FFFFE0
    PNamedColorNode(PNamedColors::lime,                 PColor::FromRGB32A(0,   255, 0)),    //  #00FF00
    PNamedColorNode(PNamedColors::limegreen,            PColor::FromRGB32A(50,  205, 50)),   //  #32CD32
    PNamedColorNode(PNamedColors::linen,                PColor::FromRGB32A(250, 240, 230)),  //  #FAF0E6
    PNamedColorNode(PNamedColors::magenta,              PColor::FromRGB32A(255, 0,   255)),  //  #FF00FF
    PNamedColorNode(PNamedColors::maroon,               PColor::FromRGB32A(128, 0,   0)),    //  #800000
    PNamedColorNode(PNamedColors::mediumaquamarine,     PColor::FromRGB32A(102, 205, 170)),  //  #66CDAA
    PNamedColorNode(PNamedColors::mediumblue,           PColor::FromRGB32A(0,   0,   205)),  //  #0000CD
    PNamedColorNode(PNamedColors::mediumorchid,         PColor::FromRGB32A(186, 85,  211)),  //  #BA55D3
    PNamedColorNode(PNamedColors::mediumpurple,         PColor::FromRGB32A(147, 112, 219)),  //  #9370DB
    PNamedColorNode(PNamedColors::mediumseagreen,       PColor::FromRGB32A(60,  179, 113)),  //  #3CB371
    PNamedColorNode(PNamedColors::mediumslateblue,      PColor::FromRGB32A(123, 104, 238)),  //  #7B68EE
    PNamedColorNode(PNamedColors::mediumspringgreen,    PColor::FromRGB32A(0,   250, 154)),  //  #00FA9A
    PNamedColorNode(PNamedColors::mediumturquoise,      PColor::FromRGB32A(72,  209, 204)),  //  #48D1CC
    PNamedColorNode(PNamedColors::mediumvioletred,      PColor::FromRGB32A(199, 21,  133)),  //  #C71585
    PNamedColorNode(PNamedColors::midnightblue,         PColor::FromRGB32A(25,  25,  112)),  //  #191970
    PNamedColorNode(PNamedColors::mintcream,            PColor::FromRGB32A(245, 255, 250)),  //  #F5FFFA
    PNamedColorNode(PNamedColors::mistyrose,            PColor::FromRGB32A(255, 228, 225)),  //  #FFE4E1
    PNamedColorNode(PNamedColors::moccasin,             PColor::FromRGB32A(255, 228, 181)),  //  #FFE4B5
    PNamedColorNode(PNamedColors::navajowhite,          PColor::FromRGB32A(255, 222, 173)),  //  #FFDEAD
    PNamedColorNode(PNamedColors::navy,                 PColor::FromRGB32A(0,   0,   128)),  //  #000080
    PNamedColorNode(PNamedColors::oldlace,              PColor::FromRGB32A(253, 245, 230)),  //  #FDF5E6
    PNamedColorNode(PNamedColors::olive,                PColor::FromRGB32A(128, 128, 0)),    //  #808000
    PNamedColorNode(PNamedColors::olivedrab,            PColor::FromRGB32A(107, 142, 35)),   //  #6B8E23
    PNamedColorNode(PNamedColors::orange,               PColor::FromRGB32A(255, 165, 0)),    //  #FFA500
    PNamedColorNode(PNamedColors::orangered,            PColor::FromRGB32A(255, 69,  0)),    //  #FF4500
    PNamedColorNode(PNamedColors::orchid,               PColor::FromRGB32A(218, 112, 214)),  //  #DA70D6
    PNamedColorNode(PNamedColors::palegoldenrod,        PColor::FromRGB32A(238, 232, 170)),  //  #EEE8AA
    PNamedColorNode(PNamedColors::palegreen,            PColor::FromRGB32A(152, 251, 152)),  //  #98FB98
    PNamedColorNode(PNamedColors::paleturquoise,        PColor::FromRGB32A(175, 238, 238)),  //  #AFEEEE
    PNamedColorNode(PNamedColors::palevioletred,        PColor::FromRGB32A(219, 112, 147)),  //  #DB7093
    PNamedColorNode(PNamedColors::papayawhip,           PColor::FromRGB32A(255, 239, 213)),  //  #FFEFD5
    PNamedColorNode(PNamedColors::peachpuff,            PColor::FromRGB32A(255, 218, 185)),  //  #FFDAB9
    PNamedColorNode(PNamedColors::peru,                 PColor::FromRGB32A(205, 133, 63)),   //  #CD853F
    PNamedColorNode(PNamedColors::pink,                 PColor::FromRGB32A(255, 192, 203)),  //  #FFC0CB
    PNamedColorNode(PNamedColors::plum,                 PColor::FromRGB32A(221, 160, 221)),  //  #DDA0DD
    PNamedColorNode(PNamedColors::powderblue,           PColor::FromRGB32A(176, 224, 230)),  //  #B0E0E6
    PNamedColorNode(PNamedColors::purple,               PColor::FromRGB32A(128, 0,   128)),  //  #800080
    PNamedColorNode(PNamedColors::red,                  PColor::FromRGB32A(255, 0,   0)),    //  #FF0000
    PNamedColorNode(PNamedColors::rosybrown,            PColor::FromRGB32A(188, 143, 143)),  //  #BC8F8F
    PNamedColorNode(PNamedColors::royalblue,            PColor::FromRGB32A(65,  105, 225)),  //  #4169E1
    PNamedColorNode(PNamedColors::saddlebrown,          PColor::FromRGB32A(139, 69,  19)),   //  #8B4513
    PNamedColorNode(PNamedColors::salmon,               PColor::FromRGB32A(250, 128, 114)),  //  #FA8072
    PNamedColorNode(PNamedColors::sandybrown,           PColor::FromRGB32A(244, 164, 96)),   //  #F4A460
    PNamedColorNode(PNamedColors::seagreen,             PColor::FromRGB32A(46,  139, 87)),   //  #2E8B57
    PNamedColorNode(PNamedColors::seashell,             PColor::FromRGB32A(255, 245, 238)),  //  #FFF5EE
    PNamedColorNode(PNamedColors::sienna,               PColor::FromRGB32A(160, 82,  45)),   //  #A0522D
    PNamedColorNode(PNamedColors::silver,               PColor::FromRGB32A(192, 192, 192)),  //  #C0C0C0
    PNamedColorNode(PNamedColors::skyblue,              PColor::FromRGB32A(135, 206, 235)),  //  #87CEEB
    PNamedColorNode(PNamedColors::slateblue,            PColor::FromRGB32A(106, 90,  205)),  //  #6A5ACD
    PNamedColorNode(PNamedColors::slategray,            PColor::FromRGB32A(112, 128, 144)),  //  #708090
    PNamedColorNode(PNamedColors::slategrey,            PColor::FromRGB32A(112, 128, 144)),  //  #708090
    PNamedColorNode(PNamedColors::snow,                 PColor::FromRGB32A(255, 250, 250)),  //  #FFFAFA
    PNamedColorNode(PNamedColors::springgreen,          PColor::FromRGB32A(0,   255, 127)),  //  #00FF7F
    PNamedColorNode(PNamedColors::steelblue,            PColor::FromRGB32A(70,  130, 180)),  //  #4682B4
    PNamedColorNode(PNamedColors::tan,                  PColor::FromRGB32A(210, 180, 140)),  //  #D2B48C
    PNamedColorNode(PNamedColors::teal,                 PColor::FromRGB32A(0,   128, 128)),  //  #008080
    PNamedColorNode(PNamedColors::thistle,              PColor::FromRGB32A(216, 191, 216)),  //  #D8BFD8
    PNamedColorNode(PNamedColors::tomato,               PColor::FromRGB32A(255, 99,  71)),   //  #FF6347
    PNamedColorNode(PNamedColors::turquoise,            PColor::FromRGB32A(64,  224, 208)),  //  #40E0D0
    PNamedColorNode(PNamedColors::violet,               PColor::FromRGB32A(238, 130, 238)),  //  #EE82EE
    PNamedColorNode(PNamedColors::wheat,                PColor::FromRGB32A(245, 222, 179)),  //  #F5DEB3
    PNamedColorNode(PNamedColors::white,                PColor::FromRGB32A(255, 255, 255)),  //  #FFFFFF
    PNamedColorNode(PNamedColors::whitesmoke,           PColor::FromRGB32A(245, 245, 245)),  //  #F5F5F5
    PNamedColorNode(PNamedColors::yellow,               PColor::FromRGB32A(255, 255, 0)),    //  #FFFF00
    PNamedColorNode(PNamedColors::yellowgreen,          PColor::FromRGB32A(154, 205, 50)),   //  #9ACD32
});
