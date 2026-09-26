// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 16.05.2025 22:00

#pragma once

#include <vector>
#include <Storage/SeekableIO.h>


class PMemFile : public PSeekableIO
{
public:
    PMemFile() = default;
    PMemFile(std::vector<uint8_t>&& data);


    virtual ssize_t Read(void* buffer, ssize_t size) override;
    virtual ssize_t Write(const void* buffer, ssize_t size) override;

    virtual ssize_t ReadPos(off64_t position, void* buffer, ssize_t size) const override;
    virtual ssize_t WritePos(off64_t position, const void* buffer, ssize_t size) override;

    virtual off64_t Seek(off64_t position, int mode) override;
private:
    std::vector<uint8_t> m_Buffer;
    ssize_t              m_Position = 0;
};
