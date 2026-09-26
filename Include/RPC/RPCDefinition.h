// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.10.2025 22:30

#pragma once

#include <tuple>

template<int THandlerID, bool TIsConst, typename TReturnType, typename... TArgTypes>
struct PRPCDefinition
{
    static constexpr int    HandlerID   = THandlerID;
    static constexpr bool   IsConst     = TIsConst;

    using ReturnType    = TReturnType;
    using ArgumentTypes = std::tuple<TArgTypes...>;
    using Signature     = ReturnType(TArgTypes...);
};

template<int THandlerID, bool TIsConst, typename TReturnType, typename... TArgTypes>
class PRPCDefinition<THandlerID, TIsConst, TReturnType(TArgTypes...)> : public PRPCDefinition<THandlerID, false, TReturnType, TArgTypes...> {};

template<int THandlerID, bool TIsConst, typename TReturnType, typename... TArgTypes>
class PRPCDefinition<THandlerID, TIsConst, TReturnType(TArgTypes...) const> : public PRPCDefinition<THandlerID, true, TReturnType, TArgTypes...> {};
