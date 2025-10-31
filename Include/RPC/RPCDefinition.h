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
// Created: 19.10.2025 22:30

#pragma once

#include <tuple>

template<int THandlerID, typename TReturnType, typename... TArgTypes>
struct PRPCDefinition
{
    static constexpr int HandlerID = THandlerID;

    using ReturnType = TReturnType;
    using ArgumentTypes = std::tuple<TArgTypes...>;
    using Signature = ReturnType(TArgTypes...);
};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PRPCDefinition<THandlerID, TReturnType(TArgTypes...)> : public PRPCDefinition<THandlerID, TReturnType, TArgTypes...> {};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PRPCDefinition<THandlerID, TReturnType(TArgTypes...) const> : public PRPCDefinition<THandlerID, TReturnType, TArgTypes...> {};
