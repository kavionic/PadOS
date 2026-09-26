// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.06.2020 19:47

#pragma once

#include <stdint.h>
#include <stddef.h>


enum class PHashAlgorithm
{
    CRC8,
//    CRC16,
    CRC32
};

template<PHashAlgorithm ALGORITHM>
class PHashCalculator
{

};

template<>
class PHashCalculator<PHashAlgorithm::CRC8>
{
public:
    void    Start() { m_CRC = 0; }
    void    AddData(const void* data, size_t length);
    uint8_t Finalize() { return m_CRC; }

private:
    uint8_t m_CRC = 0;

};

template<>
class PHashCalculator<PHashAlgorithm::CRC32>
{
public:
    void Start() { m_CRC = 0xffffffff; }
    void AddData(const void* data, size_t length);
    uint32_t Finalize() { return ~m_CRC; }

private:
    uint32_t m_CRC = 0xffffffff;

};
