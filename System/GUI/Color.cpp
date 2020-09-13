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

#include <array>
#include <algorithm>

#include <GUI/NamedColors.h>
#include <Utils/combsort.h>
#include <Utils/Utils.h>
#include <GUI/Color.h>
#include <GUI/View.h>


namespace os
{

struct NamedColorNode
{
    constexpr NamedColorNode(NamedColors colorID, Color color) : NameID(colorID), ColorValue(color) {}

    constexpr bool operator < (const NamedColorNode& rhs) { return NameID < rhs.NameID; }
    constexpr bool operator > (const NamedColorNode& rhs) { return NameID > rhs.NameID; }
    constexpr bool operator == (const NamedColorNode& rhs) { return NameID == rhs.NameID; }

    NamedColors NameID;
    Color       ColorValue;
};

// Generate an array sorted on NamedColorNode::NameID to make it possible to use binary-search when looking up colors.
constexpr auto g_StandardColors = combsort_immutable(std::array<NamedColorNode, 147>
{
    NamedColorNode(NamedColors::aliceblue,               Color::FromRGB32A(240, 248, 255)),  //  #F0F8FF
    NamedColorNode(NamedColors::antiquewhite,            Color::FromRGB32A(250, 235, 215)),  //  #FAEBD7
    NamedColorNode(NamedColors::aqua,                    Color::FromRGB32A(0,   255, 255)),  //  #00FFFF
    NamedColorNode(NamedColors::aquamarine,              Color::FromRGB32A(127, 255, 212)),  //  #7FFFD4
    NamedColorNode(NamedColors::beige,                   Color::FromRGB32A(245, 245, 220)),  //  #F5F5DC
    NamedColorNode(NamedColors::azure,                   Color::FromRGB32A(240, 255, 255)),  //  #F0FFFF
    NamedColorNode(NamedColors::bisque,                  Color::FromRGB32A(255, 228, 196)),  //  #FFE4C4
    NamedColorNode(NamedColors::black,                   Color::FromRGB32A(0,   0,   0)),    //  #000000
    NamedColorNode(NamedColors::blanchedalmond,          Color::FromRGB32A(255, 235, 205)),  //  #FFEBCD
    NamedColorNode(NamedColors::blue,                    Color::FromRGB32A(0,   0,   255)),  //  #0000FF
    NamedColorNode(NamedColors::blueviolet,              Color::FromRGB32A(138, 43,  226)),  //  #8A2BE2
    NamedColorNode(NamedColors::brown,                   Color::FromRGB32A(165, 42,  42)),   //  #A52A2A
    NamedColorNode(NamedColors::burlywood,               Color::FromRGB32A(222, 184, 135)),  //  #DEB887
    NamedColorNode(NamedColors::cadetblue,               Color::FromRGB32A(95,  158, 160)),  //  #5F9EA0
    NamedColorNode(NamedColors::chartreuse,              Color::FromRGB32A(127, 255, 0)),    //  #7FFF00
    NamedColorNode(NamedColors::chocolate,               Color::FromRGB32A(210, 105, 30)),   //  #D2691E
    NamedColorNode(NamedColors::coral,                   Color::FromRGB32A(255, 127, 80)),   //  #FF7F50
    NamedColorNode(NamedColors::cornflowerblue,          Color::FromRGB32A(100, 149, 237)),  //  #6495ED
    NamedColorNode(NamedColors::cornsilk,                Color::FromRGB32A(255, 248, 220)),  //  #FFF8DC
    NamedColorNode(NamedColors::crimson,                 Color::FromRGB32A(220, 20,  60)),   //  #DC143C
    NamedColorNode(NamedColors::cyan,                    Color::FromRGB32A(0,   255, 255)),  //  #00FFFF
    NamedColorNode(NamedColors::darkblue,                Color::FromRGB32A(0,   0,   139)),  //  #00008B
    NamedColorNode(NamedColors::darkcyan,                Color::FromRGB32A(0,   139, 139)),  //  #008B8B
    NamedColorNode(NamedColors::darkgoldenrod,           Color::FromRGB32A(184, 134, 11)),   //  #B8860B
    NamedColorNode(NamedColors::darkgray,                Color::FromRGB32A(169, 169, 169)),  //  #A9A9A9
    NamedColorNode(NamedColors::darkgreen,               Color::FromRGB32A(0,   100, 0)),    //  #006400
    NamedColorNode(NamedColors::darkgrey,                Color::FromRGB32A(169, 169, 169)),  //  #A9A9A9
    NamedColorNode(NamedColors::darkkhaki,               Color::FromRGB32A(189, 183, 107)),  //  #BDB76B
    NamedColorNode(NamedColors::darkmagenta,             Color::FromRGB32A(139, 0,   139)),  //  #8B008B
    NamedColorNode(NamedColors::darkolivegreen,          Color::FromRGB32A(85,  107, 47)),   //  #556B2F
    NamedColorNode(NamedColors::darkorange,              Color::FromRGB32A(255, 140, 0)),    //  #FF8C00
    NamedColorNode(NamedColors::darkorchid,              Color::FromRGB32A(153, 50,  204)),  //  #9932CC
    NamedColorNode(NamedColors::darkred,                 Color::FromRGB32A(139, 0,   0)),    //  #8B0000
    NamedColorNode(NamedColors::darksalmon,              Color::FromRGB32A(233, 150, 122)),  //  #E9967A
    NamedColorNode(NamedColors::darkseagreen,            Color::FromRGB32A(143, 188, 143)),  //  #8FBC8F
    NamedColorNode(NamedColors::darkslateblue,           Color::FromRGB32A(72,  61,  139)),  //  #483D8B
    NamedColorNode(NamedColors::darkslategray,           Color::FromRGB32A(47,  79,  79)),   //  #2F4F4F
    NamedColorNode(NamedColors::darkslategrey,           Color::FromRGB32A(47,  79,  79)),   //  #2F4F4F
    NamedColorNode(NamedColors::darkturquoise,           Color::FromRGB32A(0,   206, 209)),  //  #00CED1
    NamedColorNode(NamedColors::darkviolet,              Color::FromRGB32A(148, 0,   211)),  //  #9400D3
    NamedColorNode(NamedColors::deeppink,                Color::FromRGB32A(255, 20,  147)),  //  #FF1493
    NamedColorNode(NamedColors::deepskyblue,             Color::FromRGB32A(0,   191, 255)),  //  #00BFFF
    NamedColorNode(NamedColors::dimgray,                 Color::FromRGB32A(105, 105, 105)),  //  #696969
    NamedColorNode(NamedColors::dimgrey,                 Color::FromRGB32A(105, 105, 105)),  //  #696969
    NamedColorNode(NamedColors::dodgerblue,              Color::FromRGB32A(30,  144, 255)),  //  #1E90FF
    NamedColorNode(NamedColors::firebrick,               Color::FromRGB32A(178, 34,  34)),   //  #B22222
    NamedColorNode(NamedColors::floralwhite,             Color::FromRGB32A(255, 250, 240)),  //  #FFFAF0
    NamedColorNode(NamedColors::forestgreen,             Color::FromRGB32A(34,  139, 34)),   //  #228B22
    NamedColorNode(NamedColors::fuchsia,                 Color::FromRGB32A(255, 0,   255)),  //  #FF00FF
    NamedColorNode(NamedColors::gainsboro,               Color::FromRGB32A(220, 220, 220)),  //  #DCDCDC
    NamedColorNode(NamedColors::ghostwhite,              Color::FromRGB32A(248, 248, 255)),  //  #F8F8FF
    NamedColorNode(NamedColors::gold,                    Color::FromRGB32A(255, 215, 0)),    //  #FFD700
    NamedColorNode(NamedColors::goldenrod,               Color::FromRGB32A(218, 165, 32)),   //  #DAA520
    NamedColorNode(NamedColors::gray,                    Color::FromRGB32A(128, 128, 128)),  //  #808080
    NamedColorNode(NamedColors::green,                   Color::FromRGB32A(0,   128, 0)),    //  #008000
    NamedColorNode(NamedColors::greenyellow,             Color::FromRGB32A(173, 255, 47)),   //  #ADFF2F
    NamedColorNode(NamedColors::grey,                    Color::FromRGB32A(128, 128, 128)),  //  #808080
    NamedColorNode(NamedColors::honeydew,                Color::FromRGB32A(240, 255, 240)),  //  #F0FFF0
    NamedColorNode(NamedColors::hotpink,                 Color::FromRGB32A(255, 105, 180)),  //  #FF69B4
    NamedColorNode(NamedColors::indianred,               Color::FromRGB32A(205, 92,  92)),   //  #CD5C5C
    NamedColorNode(NamedColors::indigo,                  Color::FromRGB32A(75,  0,   130)),  //  #4B0082
    NamedColorNode(NamedColors::ivory,                   Color::FromRGB32A(255, 255, 240)),  //  #FFFFF0
    NamedColorNode(NamedColors::khaki,                   Color::FromRGB32A(240, 230, 140)),  //  #F0E68C
    NamedColorNode(NamedColors::lavender,                Color::FromRGB32A(230, 230, 250)),  //  #E6E6FA
    NamedColorNode(NamedColors::lavenderblush,           Color::FromRGB32A(255, 240, 245)),  //  #FFF0F5
    NamedColorNode(NamedColors::lawngreen,               Color::FromRGB32A(124, 252, 0)),    //  #7CFC00
    NamedColorNode(NamedColors::lemonchiffon,            Color::FromRGB32A(255, 250, 205)),  //  #FFFACD
    NamedColorNode(NamedColors::lightblue,               Color::FromRGB32A(173, 216, 230)),  //  #ADD8E6
    NamedColorNode(NamedColors::lightcoral,              Color::FromRGB32A(240, 128, 128)),  //  #F08080
    NamedColorNode(NamedColors::lightcyan,               Color::FromRGB32A(224, 255, 255)),  //  #E0FFFF
    NamedColorNode(NamedColors::lightgoldenrodyellow,    Color::FromRGB32A(250, 250, 210)),  //  #FAFAD2
    NamedColorNode(NamedColors::lightgray,               Color::FromRGB32A(211, 211, 211)),  //  #D3D3D3
    NamedColorNode(NamedColors::lightgreen,              Color::FromRGB32A(144, 238, 144)),  //  #90EE90
    NamedColorNode(NamedColors::lightgrey,               Color::FromRGB32A(211, 211, 211)),  //  #D3D3D3
    NamedColorNode(NamedColors::lightpink,               Color::FromRGB32A(255, 182, 193)),  //  #FFB6C1
    NamedColorNode(NamedColors::lightsalmon,             Color::FromRGB32A(255, 160, 122)),  //  #FFA07A
    NamedColorNode(NamedColors::lightseagreen,           Color::FromRGB32A(32,  178, 170)),  //  #20B2AA
    NamedColorNode(NamedColors::lightskyblue,            Color::FromRGB32A(135, 206, 250)),  //  #87CEFA
    NamedColorNode(NamedColors::lightslategray,          Color::FromRGB32A(119, 136, 153)),  //  #778899
    NamedColorNode(NamedColors::lightslategrey,          Color::FromRGB32A(119, 136, 153)),  //  #778899
    NamedColorNode(NamedColors::lightsteelblue,          Color::FromRGB32A(176, 196, 222)),  //  #B0C4DE
    NamedColorNode(NamedColors::lightyellow,             Color::FromRGB32A(255, 255, 224)),  //  #FFFFE0
    NamedColorNode(NamedColors::lime,                    Color::FromRGB32A(0,   255, 0)),    //  #00FF00
    NamedColorNode(NamedColors::limegreen,               Color::FromRGB32A(50,  205, 50)),   //  #32CD32
    NamedColorNode(NamedColors::linen,                   Color::FromRGB32A(250, 240, 230)),  //  #FAF0E6
    NamedColorNode(NamedColors::magenta,                 Color::FromRGB32A(255, 0,   255)),  //  #FF00FF
    NamedColorNode(NamedColors::maroon,                  Color::FromRGB32A(128, 0,   0)),    //  #800000
    NamedColorNode(NamedColors::mediumaquamarine,        Color::FromRGB32A(102, 205, 170)),  //  #66CDAA
    NamedColorNode(NamedColors::mediumblue,              Color::FromRGB32A(0,   0,   205)),  //  #0000CD
    NamedColorNode(NamedColors::mediumorchid,            Color::FromRGB32A(186, 85,  211)),  //  #BA55D3
    NamedColorNode(NamedColors::mediumpurple,            Color::FromRGB32A(147, 112, 219)),  //  #9370DB
    NamedColorNode(NamedColors::mediumseagreen,          Color::FromRGB32A(60,  179, 113)),  //  #3CB371
    NamedColorNode(NamedColors::mediumslateblue,         Color::FromRGB32A(123, 104, 238)),  //  #7B68EE
    NamedColorNode(NamedColors::mediumspringgreen,       Color::FromRGB32A(0,   250, 154)),  //  #00FA9A
    NamedColorNode(NamedColors::mediumturquoise,         Color::FromRGB32A(72,  209, 204)),  //  #48D1CC
    NamedColorNode(NamedColors::mediumvioletred,         Color::FromRGB32A(199, 21,  133)),  //  #C71585
    NamedColorNode(NamedColors::midnightblue,            Color::FromRGB32A(25,  25,  112)),  //  #191970
    NamedColorNode(NamedColors::mintcream,               Color::FromRGB32A(245, 255, 250)),  //  #F5FFFA
    NamedColorNode(NamedColors::mistyrose,               Color::FromRGB32A(255, 228, 225)),  //  #FFE4E1
    NamedColorNode(NamedColors::moccasin,                Color::FromRGB32A(255, 228, 181)),  //  #FFE4B5
    NamedColorNode(NamedColors::navajowhite,             Color::FromRGB32A(255, 222, 173)),  //  #FFDEAD
    NamedColorNode(NamedColors::navy,                    Color::FromRGB32A(0,   0,   128)),  //  #000080
    NamedColorNode(NamedColors::oldlace,                 Color::FromRGB32A(253, 245, 230)),  //  #FDF5E6
    NamedColorNode(NamedColors::olive,                   Color::FromRGB32A(128, 128, 0)),    //  #808000
    NamedColorNode(NamedColors::olivedrab,               Color::FromRGB32A(107, 142, 35)),   //  #6B8E23
    NamedColorNode(NamedColors::orange,                  Color::FromRGB32A(255, 165, 0)),    //  #FFA500
    NamedColorNode(NamedColors::orangered,               Color::FromRGB32A(255, 69,  0)),    //  #FF4500
    NamedColorNode(NamedColors::orchid,                  Color::FromRGB32A(218, 112, 214)),  //  #DA70D6
    NamedColorNode(NamedColors::palegoldenrod,           Color::FromRGB32A(238, 232, 170)),  //  #EEE8AA
    NamedColorNode(NamedColors::palegreen,               Color::FromRGB32A(152, 251, 152)),  //  #98FB98
    NamedColorNode(NamedColors::paleturquoise,           Color::FromRGB32A(175, 238, 238)),  //  #AFEEEE
    NamedColorNode(NamedColors::palevioletred,           Color::FromRGB32A(219, 112, 147)),  //  #DB7093
    NamedColorNode(NamedColors::papayawhip,              Color::FromRGB32A(255, 239, 213)),  //  #FFEFD5
    NamedColorNode(NamedColors::peachpuff,               Color::FromRGB32A(255, 218, 185)),  //  #FFDAB9
    NamedColorNode(NamedColors::peru,                    Color::FromRGB32A(205, 133, 63)),   //  #CD853F
    NamedColorNode(NamedColors::pink,                    Color::FromRGB32A(255, 192, 203)),  //  #FFC0CB
    NamedColorNode(NamedColors::plum,                    Color::FromRGB32A(221, 160, 221)),  //  #DDA0DD
    NamedColorNode(NamedColors::powderblue,              Color::FromRGB32A(176, 224, 230)),  //  #B0E0E6
    NamedColorNode(NamedColors::purple,                  Color::FromRGB32A(128, 0,   128)),  //  #800080
    NamedColorNode(NamedColors::red,                     Color::FromRGB32A(255, 0,   0)),    //  #FF0000
    NamedColorNode(NamedColors::rosybrown,               Color::FromRGB32A(188, 143, 143)),  //  #BC8F8F
    NamedColorNode(NamedColors::royalblue,               Color::FromRGB32A(65,  105, 225)),  //  #4169E1
    NamedColorNode(NamedColors::saddlebrown,             Color::FromRGB32A(139, 69,  19)),   //  #8B4513
    NamedColorNode(NamedColors::salmon,                  Color::FromRGB32A(250, 128, 114)),  //  #FA8072
    NamedColorNode(NamedColors::sandybrown,              Color::FromRGB32A(244, 164, 96)),   //  #F4A460
    NamedColorNode(NamedColors::seagreen,                Color::FromRGB32A(46,  139, 87)),   //  #2E8B57
    NamedColorNode(NamedColors::seashell,                Color::FromRGB32A(255, 245, 238)),  //  #FFF5EE
    NamedColorNode(NamedColors::sienna,                  Color::FromRGB32A(160, 82,  45)),   //  #A0522D
    NamedColorNode(NamedColors::silver,                  Color::FromRGB32A(192, 192, 192)),  //  #C0C0C0
    NamedColorNode(NamedColors::skyblue,                 Color::FromRGB32A(135, 206, 235)),  //  #87CEEB
    NamedColorNode(NamedColors::slateblue,               Color::FromRGB32A(106, 90,  205)),  //  #6A5ACD
    NamedColorNode(NamedColors::slategray,               Color::FromRGB32A(112, 128, 144)),  //  #708090
    NamedColorNode(NamedColors::slategrey,               Color::FromRGB32A(112, 128, 144)),  //  #708090
    NamedColorNode(NamedColors::snow,                    Color::FromRGB32A(255, 250, 250)),  //  #FFFAFA
    NamedColorNode(NamedColors::springgreen,             Color::FromRGB32A(0,   255, 127)),  //  #00FF7F
    NamedColorNode(NamedColors::steelblue,               Color::FromRGB32A(70,  130, 180)),  //  #4682B4
    NamedColorNode(NamedColors::tan,                     Color::FromRGB32A(210, 180, 140)),  //  #D2B48C
    NamedColorNode(NamedColors::teal,                    Color::FromRGB32A(0,   128, 128)),  //  #008080
    NamedColorNode(NamedColors::thistle,                 Color::FromRGB32A(216, 191, 216)),  //  #D8BFD8
    NamedColorNode(NamedColors::tomato,                  Color::FromRGB32A(255, 99,  71)),   //  #FF6347
    NamedColorNode(NamedColors::turquoise,               Color::FromRGB32A(64,  224, 208)),  //  #40E0D0
    NamedColorNode(NamedColors::violet,                  Color::FromRGB32A(238, 130, 238)),  //  #EE82EE
    NamedColorNode(NamedColors::wheat,                   Color::FromRGB32A(245, 222, 179)),  //  #F5DEB3
    NamedColorNode(NamedColors::white,                   Color::FromRGB32A(255, 255, 255)),  //  #FFFFFF
    NamedColorNode(NamedColors::whitesmoke,              Color::FromRGB32A(245, 245, 245)),  //  #F5F5F5
    NamedColorNode(NamedColors::yellow,                  Color::FromRGB32A(255, 255, 0)),    //  #FFFF00
    NamedColorNode(NamedColors::yellowgreen,             Color::FromRGB32A(154, 205, 50)),   //  #9ACD32
});

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Color Color::FromColorID(NamedColors colorID)
{
    auto i = std::lower_bound(g_StandardColors.begin(), g_StandardColors.end(), colorID, [](const NamedColorNode& lhs, NamedColors rhs) { return lhs.NameID < rhs; });
    if (i != g_StandardColors.end() && i->NameID == colorID) {
        return i->ColorValue;
    }
    return Color(0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Color::Color(NamedColors colorID)
{
    m_Color = FromColorID(colorID).m_Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Color::Color(const String& name)
{
    m_Color = FromColorName(name).m_Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DynamicColor::DynamicColor(StandardColorID colorID) : Color(get_standard_color(colorID))
{
}


} // namespace os

namespace unit_test
{

void TestNamedColors()
{
    for (size_t i = 1; i < os::g_StandardColors.size(); ++i)
    {
        if (os::g_StandardColors[i - 1].NameID == os::g_StandardColors[i].NameID) {
            printf("Hash collision! Both %d and %d have hash 0x%08lx\n", i - 1, i, uint32_t(os::g_StandardColors[i].NameID));
        }
        else if (os::g_StandardColors[i - 1].NameID > os::g_StandardColors[i].NameID) {
            printf("Bad named color table sorting! %d:0x%08lx > %d:0x%08lx\n", i - 1, uint32_t(os::g_StandardColors[i - 1].NameID), i, uint32_t(os::g_StandardColors[i].NameID));
        }
    }
    EXPECT_TRUE(os::Color::FromColorID(os::NamedColors::aliceblue).GetColor32()    == 0xfff0f8ff);
    EXPECT_TRUE(os::Color::FromColorID(os::NamedColors::antiquewhite).GetColor32() == 0xfffaebd7);
    EXPECT_TRUE(os::Color::FromColorID(os::NamedColors::yellow).GetColor32()       == 0xffffff00);
    EXPECT_TRUE(os::Color::FromColorID(os::NamedColors::yellowgreen).GetColor32()  == 0xff9acd32);

    EXPECT_TRUE(os::Color::FromColorName("aliceblue").GetColor32()       == 0xfff0f8ff);
    EXPECT_TRUE(os::Color::FromColorName("YellowGreen").GetColor32()     == 0xff9acd32);
}

} //namespace unit_test
