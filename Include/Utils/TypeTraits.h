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
// Created: 05.05.2025 20:00

#pragma once

///////////////////////////////////////////////////////////////////////////////
/// Type trait for extracting return type and argument types
/// from a callable type
/// 
/// The Signature template argument can be a function pointer
/// a lambda or a functor.
/// 
/// get_callable_argument_types::return_type
///     - The type deduced for the return value.
/// 
/// get_callable_argument_types::argument_types
///     - std::tuple<> of deduced argument types.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

// Primary template. Fallback if nothing matches.
template <typename Signature, typename = void>
struct get_callable_argument_types
{
    using return_type    = void;
    using argument_types = std::tuple<>;
};

// Specialization for function pointers.
template <typename Signature>
struct get_callable_argument_types<Signature, std::enable_if_t<std::is_pointer_v<Signature>&& std::is_function_v<std::remove_pointer_t<Signature>>>>
{
private:
    template <typename Ret, typename... Arguments>
    static Ret deduce_return_type(Ret(*)(Arguments...));

    template <typename Ret, typename... Arguments>
    static std::tuple<Arguments...> deduce_argument_types(Ret(*)(Arguments...));

public:
    using return_type    = decltype(deduce_return_type(static_cast<Signature>(nullptr)));
    using argument_types = decltype(deduce_argument_types(static_cast<Signature>(nullptr)));
};

// Specialization for lambdas and functors.
template <typename Signature>
struct get_callable_argument_types<Signature, std::void_t<decltype(&std::decay_t<Signature>::operator())>>
{
private:
    using FunctionOperatorType = decltype(&std::decay_t<Signature>::operator());

    // Handle const function operator:
    template <typename Ret, typename Class, typename... Arguments>
    static Ret deduce_return_type(Ret(Class::*)(Arguments...) const);

    template <typename Ret, typename Class, typename... Arguments>
    static std::tuple<Arguments...> deduce_argument_types(Ret(Class::*)(Arguments...) const);

    // Handle non-const function operator:
    template <typename Ret, typename Class, typename... Arguments>
    static Ret deduce_return_type(Ret(Class::*)(Arguments...));

    template <typename Ret, typename Class, typename... Arguments>
    static std::tuple<Arguments...> deduce_argument_types(Ret(Class::*)(Arguments...));

public:
    using return_type    = decltype(deduce_return_type(FunctionOperatorType{}));
    using argument_types = decltype(deduce_argument_types(FunctionOperatorType{}));
};

// Helper to get the return type.
template <typename Signature>
using callable_return_type_t = typename get_callable_argument_types<std::decay_t<Signature>>::return_type;

// Helper to get the arguments type tuple.
template <typename Signature>
using callable_argument_types_t = typename get_callable_argument_types<std::decay_t<Signature>>::argument_types;

// Helper to get the type of a specific argument.
template <typename Signature, int ARG_INDEX>
using callable_argument_type_t = typename std::tuple_element_t<ARG_INDEX, typename get_callable_argument_types<std::decay_t<Signature>>::argument_types>;
