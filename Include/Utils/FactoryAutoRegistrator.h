// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.05.2025 16:00

#pragma once

#include <functional>

class PFactoryAutoRegistratorBase
{
public:
    PFactoryAutoRegistratorBase();

    static void InvokeAll();

    virtual void Invoke() = 0;
private:
    static PFactoryAutoRegistratorBase* s_FirstRegistrator;

    PFactoryAutoRegistratorBase* m_NextRegistrator = nullptr;
};

template<typename T>
class PFactoryAutoRegistrator : public PFactoryAutoRegistratorBase
{
public:
    template<typename TCallback>
    PFactoryAutoRegistrator(TCallback&& callback) : m_Callback(std::move(callback)) {}

    virtual void Invoke() override { if (m_Callback) m_Callback(); m_Callback = {}; }

private:
    std::function<void()> m_Callback;
};
