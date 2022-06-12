// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 05.06.2022 23:00

#pragma once

#include <stdint.h>

enum class USB_LanguageID : uint16_t
{
    UNDEFINED                   = 0x0000,   // Language undefined
    AFRIKAANS                   = 0x0436,   // Afrikaans
    ALBANIAN                    = 0x041c,   // Albanian
    ARABIC_SAUDI_ARABIA         = 0x0401,   // Arabic (Saudi Arabia)
    ARABIC_IRAQ                 = 0x0801,   // Arabic (Iraq)
    ARABIC_EGYPT                = 0x0c01,   // Arabic (Egypt)
    ARABIC_LYBYA                = 0x1001,   // Arabic (Libya)
    ARABIC_ALGERIA              = 0x1401,   // Arabic (Algeria)
    ARABIC_MOROCCO              = 0x1801,   // Arabic (Morocco)
    ARABIC_TUNISIA              = 0x1c01,   // Arabic (Tunisia)
    ARABIC_OMAN                 = 0x2001,   // Arabic (Oman)
    ARABIC_YEMEN                = 0x2401,   // Arabic (Yemen)
    ARABIC_SYRIA                = 0x2801,   // Arabic (Syria)
    ARABIC_JORDAN               = 0x2c01,   // Arabic (Jordan)
    ARABIC_LEBANON              = 0x3001,   // Arabic (Lebanon)
    ARABIC_KUWAIT               = 0x3401,   // Arabic (Kuwait)
    ARABIC_UAE                  = 0x3801,   // Arabic (U.A.E.)
    ARABIC_BAHRAIN              = 0x3c01,   // Arabic (Bahrain)
    ARABIC_QATAR                = 0x4001,   // Arabic (Qatar)
    ARMENIAN                    = 0x042b,   // Armenian
    ASSAMESE                    = 0x044d,   // Assamese
    AZERI_LATIN                 = 0x042c,   // Azeri (Latin)
    AZERI_CYRILLIC              = 0x082c,   // Azeri (Cyrillic)
    BASQUE                      = 0x042d,   // Basque
    BELARUSSIAN                 = 0x0423,   // Belarussian
    BENGALI                     = 0x0445,   // Bengali
    BULGARIAN                   = 0x0402,   // Bulgarian
    BURMESE                     = 0x0455,   // Burmese
    CATALAN                     = 0x0403,   // Catalan
    CHINESE_TAIWAN              = 0x0404,   // Chinese (Taiwan)
    CHINESE_PRC                 = 0x0804,   // Chinese (PRC = People Republic of Chinese)
    CHINESE_HONG_KONG           = 0x0c04,   // Chinese (Hong Kong)
    CHINESE_SINGAPORE           = 0x1004,   // Chinese (Singapore)
    CHINESE_MACAU_SAR           = 0x1404,   // Chinese (Macau SAR)
    CROATIAN                    = 0x041a,   // Croatian
    CZECH                       = 0x0405,   // Czech
    DANISH                      = 0x0406,   // Danish
    DUTCH_NETHERLANDS           = 0x0413,   // Dutch (Netherlands)
    DUTCH_BELGIUM               = 0x0813,   // Dutch (Belgium)
    ENGLISH_UNITED_STATES       = 0x0409,   // English (United States)	
    ENGLISH_UNITED_KINGDOM      = 0x0809,   // English (United Kingdom)	
    ENGLISH_AUSTRALIAN          = 0x0c09,   // English (Australian)	
    ENGLISH_CANADIAN            = 0x1009,   // English (Canadian)	
    ENGLISH_NEW_ZEALAND         = 0x1409,   // English (New Zealand)	
    ENGLISH_IRELAND             = 0x1809,   // English (Ireland)	
    ENGLISH_SOUTH_AFRICA        = 0x1c09,   // English (South Africa)	
    ENGLISH_JAMAICA             = 0x2009,   // English (Jamaica)	
    ENGLISH_CARIBBEAN           = 0x2409,   // English (Caribbean)	
    ENGLISH_BELIZE              = 0x2809,   // English (Belize)	
    ENGLISH_TRINIDAD            = 0x2c09,   // English (Trinidad)	
    ENGLISH_ZIMBABWE            = 0x3009,   // English (Zimbabwe)	
    ENGLISH_PHLIPPINES          = 0x3409,   // English (Philippines)	
    ESTONIAN                    = 0x0425,   // Estonian
    FAEROESE                    = 0x0438,   // Faeroese
    FARSI                       = 0x0429,   // Farsi
    FINNISH                     = 0x040b,   // Finnish
    FRENCH_STANDARD             = 0x040c,   // French (Standard)
    FRENCH_BELGIAN              = 0x080c,   // French (Belgian)
    FRENCH_CANADIAN             = 0x0c0c,   // French (Canadian)
    FRENCH_SWITZERLAND          = 0x100c,   // French (Switzerland)
    FRENCH_LUXEMBOURG           = 0x140c,   // French (Luxembourg)
    FRENCH_MONACO               = 0x180c,   // French (Monaco)
    GEORGIAN                    = 0x0437,   // Georgian
    GERMAN_STANDARD             = 0x0407,   // German (Standard)
    GERMAN_SWITZERLAND          = 0x0807,   // German (Switzerland)
    GERMAN_AUSTRIA              = 0x0c07,   // German (Austria)
    GERMAN_LUXEMBOURG           = 0x1007,   // German (Luxembourg)
    GERMAN_LIECHTENSTEIN        = 0x1407,   // German (Liechtenstein)
    GREEK                       = 0x0408,   // Greek
    GUJARATI                    = 0x0447,   // Gujarati
    HEBREW                      = 0x040d,   // Hebrew
    HINDI                       = 0x0439,   // Hindi
    HUNGARIAN                   = 0x040e,   // Hungarian
    ICELANDIC                   = 0x040f,   // Icelandic
    INDONESIAN                  = 0x0421,   // Indonesian
    ITALIAN_STANDARD            = 0x0410,   // Italian (Standard)
    ITALIAN_SWITZERLAND         = 0x0810,   // Italian (Switzerland)
    JAPANESE                    = 0x0411,   // Japanese
    KANNADA                     = 0x044b,   // Kannada
    KASHMIRI_INDIA              = 0x0860,   // Kashmiri (India)
    KAZAKH                      = 0x043f,   // Kazakh
    KONKANI                     = 0x0457,   // Konkani
    KOREAN_JOHAB                = 0x0412,   // Korean (Johab)
    LATVIAN                     = 0x0426,   // Latvian
    LITHUANIAN                  = 0x0427,   // Lithuanian
    LITHUANIAN_CLASSIC          = 0x0827,   // Lithuanian (Classic)
    MACEDONIAN                  = 0x042f,   // Macedonian
    MALAY_MALAYSIAN             = 0x043e,   // Malay (Malaysian)
    MALAY_BRUNEI_DARUSSALAM     = 0x083e,   // Malay (Brunei Darussalam)
    MALAYALAM                   = 0x044c,   // Malayalam
    MANIPURI                    = 0x0458,   // Manipuri
    MARATHI                     = 0x044e,   // Marathi
    NEPALI_INDIA                = 0x0861,   // Nepali (India)
    NORWEGIAN_BOKMAL            = 0x0414,   // Norwegian (Bokmal)
    NORWEGIAN_NYNORSK           = 0x0814,   // Norwegian (Nynorsk)
    ORIYA                       = 0x0448,   // Oriya
    POLISH                      = 0x0415,   // Polish
    PORTUGUESE_BRAZIL           = 0x0416,   // Portuguese (Brazil)
    PORTUGUESE_STANDARD         = 0x0816,   // Portuguese (Standard)
    PUNJABI                     = 0x0446,   // Punjabi
    ROMANIAN                    = 0x0418,   // Romanian
    RUSSIAN                     = 0x0419,   // Russian
    SANSKRIT                    = 0x044f,   // Sanskrit
    SERBIAN_CYRILLIC            = 0x0c1a,   // Serbian (Cyrillic)
    SERBIAN_LATIN               = 0x081a,   // Serbian (Latin)
    SINDHI                      = 0x0459,   // Sindhi
    SLOVAK                      = 0x041b,   // Slovak
    SLOVENIAN                   = 0x0424,   // Slovenian
    SPANISH_TRADITIONAL_SORT    = 0x040a,   // Spanish (Traditional Sort)
    SPANISH_MEXICAN             = 0x080a,   // Spanish (Mexican)
    SPANISH_MODERN_SORT         = 0x0c0a,   // Spanish (Modern Sort)
    SPANISH_GUATEMALA           = 0x100a,   // Spanish (Guatemala)
    SPANISH_COSTA_RICA          = 0x140a,   // Spanish (Costa Rica)
    SPANISH_PANAMA              = 0x180a,   // Spanish (Panama)
    SPANISH_DOMINICAN_REPUBLIC  = 0x1c0a,   // Spanish (Dominican Republic)
    SPANISH_VENEZUELA           = 0x200a,   // Spanish (Venezuela)
    SPANISH_COLOMBIA            = 0x240a,   // Spanish (Colombia)
    SPANISH_PERU                = 0x280a,   // Spanish (Peru)
    SPANISH_ARGENTINA           = 0x2c0a,   // Spanish (Argentina)
    SPANISH_ECUADOR             = 0x300a,   // Spanish (Ecuador)
    SPANISH_CHILE               = 0x340a,   // Spanish (Chile)
    SPANISH_URUGUAY             = 0x380a,   // Spanish (Uruguay)
    SPANISH_PARAGUAY            = 0x3c0a,   // Spanish (Paraguay)
    SPANISH_BOLIVIA             = 0x400a,   // Spanish (Bolivia)
    SPANISH_EL_SALVADOR         = 0x440a,   // Spanish (El Salvador)
    SPANISH_HONDURAS            = 0x480a,   // Spanish (Honduras)
    SPANISH_NICARAGUA           = 0x4c0a,   // Spanish (Nicaragua)
    SPANISH_PUERTO_RICO         = 0x500a,   // Spanish (Puerto Rico)
    SUTU                        = 0x0430,   // Sutu
    SWAHILI_KENYA               = 0x0441,   // Swahili (Kenya)
    SWEDISH                     = 0x041d,   // Swedish
    SWEDISH_FINLAND             = 0x081d,   // Swedish (Finland)
    TAMIL                       = 0x0449,   // Tamil
    TATAR_TATARSTAN             = 0x0444,   // Tatar (Tatarstan)
    TELUGU                      = 0x044a,   // Telugu
    THAI                        = 0x041e,   // Thai
    TURKISH                     = 0x041f,   // Turkish
    UKRAINIAN                   = 0x0422,   // Ukrainian
    URDU_PAKISTAN               = 0x0420,   // Urdu (Pakistan)
    URDU_INDIA                  = 0x0820,   // Urdu (India)
    UZBEK_LATIN                 = 0x0443,   // Uzbek (Latin)
    UZBEK_CYRILLIC              = 0x0843,   // Uzbek (Cyrillic)
    VIETNAMESE                  = 0x042a,   // Vietnamese

    HID_UDD                     = 0x04ff,   // HID (Usage Data Descriptor)
    HID1                        = 0xf0ff,   // HID (Vendor Defined 1)
    HID2                        = 0xf4ff,   // HID (Vendor Defined 2)
    HID3                        = 0xf8ff,   // HID (Vendor Defined 3)
    HID4                        = 0xfcff    // HID (Vendor Defined 4)
};

