// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.05.2025 20:30

#pragma once

#include <vector>
#include <stdint.h>

#include <Utils/String.h>


class PBase64Codec
{
public:
    static PString Encode(const uint8_t* data, size_t length);
    static std::vector<uint8_t> Decode(const void* data, const size_t length);

    static uint8_t EncodeBit00_05(const uint8_t* srcData) { return g_Base64EncodeAlphabet[srcData[0] >> 2]; }
    static uint8_t EncodeBit06_07(const uint8_t* srcData) { return g_Base64EncodeAlphabet[(srcData[0] & 0x03) << 4]; }
    static uint8_t EncodeBit06_11(const uint8_t* srcData) { return g_Base64EncodeAlphabet[((srcData[0] & 0x03) << 4) | (srcData[1] >> 4)]; }
    static uint8_t EncodeBit12_15(const uint8_t* srcData) { return g_Base64EncodeAlphabet[(srcData[1] & 0x0f) << 2]; }
    static uint8_t EncodeBit12_17(const uint8_t* srcData) { return g_Base64EncodeAlphabet[((srcData[1] & 0x0f) << 2) | (srcData[2] >> 6)]; }
    static uint8_t EncodeBit18_23(const uint8_t* srcData) { return g_Base64EncodeAlphabet[srcData[2] & 0x3f]; }

    static uint32_t DecodeCharacter(uint8_t character) { return g_Base64DecodeAlphabet[(character & 0x7f)]; }

private:
    static const uint8_t g_Base64EncodeAlphabet[64];
    static const uint8_t g_Base64DecodeAlphabet[128];

};
