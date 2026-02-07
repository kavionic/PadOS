// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.02.2026 23:00

#include <cstdint>
#include <type_traits>
#include <utility>

template<class TEnumType>
concept PBitmaskEnum = std::is_enum_v<TEnumType>;

template<PBitmaskEnum TEnumType, class TBaseMask = void>
class PEnumBitmask;


template<class T> struct PIsEnumBitmask : std::false_type {};

template<class E, class B> struct PIsEnumBitmask<PEnumBitmask<E, B>> : std::true_type {};

template<class T>
inline constexpr bool PIsEnumBitmask_t = PIsEnumBitmask<std::remove_cvref_t<T>>::value;

template<class T>
concept PEnumBitmaskType = PIsEnumBitmask_t<T>;

template<class TMask, class TArg>
consteval bool PEnumBitmaskAcceptsArgImpl();

// A mask accepts:
//  - its own enum_type
//  - itself (the mask type)
//  - anything its base mask accepts (recursively)
template<class TMask, class TArg>
concept PEnumBitmaskAcceptsArg = PEnumBitmaskAcceptsArgImpl<TMask, TArg>();

template<class TMask, class TArg>
consteval bool PEnumBitmaskAcceptsArgImpl()
{
    if constexpr (std::same_as<std::remove_cvref_t<TArg>, TMask>) {
        return true;
    } else if constexpr (std::same_as<std::remove_cvref_t<TArg>, typename TMask::enum_type>) {
        return true;
    } else if constexpr (PEnumBitmaskType<TMask> && !std::same_as<typename TMask::base_mask_type, void>) {
        return PEnumBitmaskAcceptsArgImpl<typename TMask::base_mask_type, TArg>();
    } else {
        return false;
    }
}

/**
 * @brief Type-safe, composable bitmask wrapper for enum-based flags.
 *
 * PEnumBitmask provides a strongly-typed alternative to raw integer bitmasks.
 * It is designed for APIs that take sets of flags while enforcing at compile
 * time which flags are valid for a given context.
 *
 * ## Basic usage
 *
 * Flags are defined as enum values whose underlying values represent bitmasks:
 *
 *   enum class PViewFlag : uint32_t
 *   {
 *       Visible = 0x01,
 *       Enabled = 0x02
 *   };
 *
 *   using PViewFlags = PEnumBitmask<PViewFlag>;
 *
 *   PViewFlags flags(PViewFlag::Visible, PViewFlag::Enabled);
 *   flags |= PViewFlag::Enabled;
 *
 * ## Hierarchical flag sets
 *
 * PEnumBitmask supports hierarchical flag sets through inheritance.
 * A derived flag set may accept its own flags *and* all flags accepted
 * by its base flag set:
 *
 *   enum class PTextViewFlag : uint32_t
 *   {
 *       Multiline = 0x10,
 *       Password  = 0x20
 *   };
 *
 *   using PTextViewFlags = PEnumBitmask<PTextViewFlag, PViewFlags>;
 *
 *   PTextViewFlags flags(
 *       PViewFlag::Visible,
 *       PTextViewFlag::Multiline
 *   );
 *
 * This allows higher-level APIs to add new flags without breaking or
 * duplicating lower-level ones.
 *
 * ## Transitive construction
 *
 * Construction is transitive across the inheritance chain. A derived
 * PEnumBitmask may be constructed from:
 *
 *   - its own enum values
 *   - any base enum values
 *   - any base PEnumBitmask instances
 *
 * Example:
 *
 *   PTextViewFlags flags(
 *       PViewFlag::Visible,
 *       PTextViewFlag::Multiline
 *   );
 *
 * Invalid flag types are rejected at compile time.
 *
 * ## Type safety guarantees
 *
 * - Only enum values explicitly associated with a PEnumBitmask (or its bases)
 *   are accepted.
 * - Accidental mixing of unrelated flag enums is a compile-time error.
 * - All bitwise operations preserve strong typing.
 *
 * ## Operators and behavior
 *
 * - Supports |, &, ^, ~ and their assignment variants.
 * - Member operators (e.g. operator|=) are preferred and unambiguous.
 * - Non-member operators may resolve to base types in some expressions;
 *   this is intentional and documented.
 *
 * ## Performance
 *
 * PEnumBitmask is a thin wrapper around an integral bitmask value.
 * All operations are constexpr-friendly and compile down to simple
 * integer operations with no runtime overhead.
 *
 * ## Design notes
 *
 * - Enum values are expected to represent bit *masks*, not bit positions.
 * - Flag interpretation is delegated to the nearest base mask in the
 *   inheritance chain, ensuring consistent semantics.
 * - Passing flags by value is intentional; all supported types are
 *   trivially copyable and word-sized.
 * 
 * \author Kurt Skauen
 *****************************************************************************/

// Root (no base) — owns storage
template<PBitmaskEnum TEnumType>
class PEnumBitmask<TEnumType, void>
{
public:
    using enum_type = TEnumType;
    using underlying_type = std::underlying_type_t<TEnumType>;
    using base_mask_type = void;

    constexpr PEnumBitmask() = default;

    template<class... TFlagType>
        requires ((std::same_as<std::remove_cvref_t<TFlagType>, enum_type>) && ...)
    constexpr PEnumBitmask(TFlagType... flags)
        : m_Bits((underlying_type{ 0 } | ... | std::to_underlying(flags)))
    {
    }

    template<class... TFlagType>
        requires ((std::same_as<std::remove_cvref_t<TFlagType>, enum_type>) && ...)
    static constexpr PEnumBitmask From(TFlagType... flags)
    {
        return PEnumBitmask(flags...);
    }

    // Queries
    constexpr bool Empty() const { return m_Bits == 0; }
    constexpr underlying_type GetBits() const { return m_Bits; }

    constexpr bool Has(enum_type flag) const
    {
        return (m_Bits & std::to_underlying(flag)) != 0;
    }

    constexpr bool HasAll(PEnumBitmask other) const
    {
        return (m_Bits & other.m_Bits) == other.m_Bits;
    }

    constexpr bool HasAny(PEnumBitmask other) const
    {
        return (m_Bits & other.m_Bits) != 0;
    }

    // Modifiers
    constexpr void SetFlag(enum_type flag) { m_Bits |= std::to_underlying(flag); }
    constexpr void ClearFlag(enum_type flag) { m_Bits &= ~std::to_underlying(flag); }
    constexpr void ToggleFlag(enum_type flag) { m_Bits ^= std::to_underlying(flag); }

    // Operators with flags
    constexpr PEnumBitmask& operator|=(enum_type flag) { m_Bits |= std::to_underlying(flag); return *this; }
    constexpr PEnumBitmask& operator&=(enum_type flag) { m_Bits &= std::to_underlying(flag); return *this; }
    constexpr PEnumBitmask& operator^=(enum_type flag) { m_Bits ^= std::to_underlying(flag); return *this; }

    // Operators with masks
    constexpr PEnumBitmask& operator|=(PEnumBitmask rhs) { m_Bits |= rhs.m_Bits; return *this; }
    constexpr PEnumBitmask& operator&=(PEnumBitmask rhs) { m_Bits &= rhs.m_Bits; return *this; }
    constexpr PEnumBitmask& operator^=(PEnumBitmask rhs) { m_Bits ^= rhs.m_Bits; return *this; }

    friend constexpr PEnumBitmask operator|(PEnumBitmask a, PEnumBitmask b) { return a |= b; }
    friend constexpr PEnumBitmask operator&(PEnumBitmask a, PEnumBitmask b) { return a &= b; }
    friend constexpr PEnumBitmask operator^(PEnumBitmask a, PEnumBitmask b) { return a ^= b; }

    friend constexpr PEnumBitmask operator|(PEnumBitmask a, enum_type b) { return a |= b; }
    friend constexpr PEnumBitmask operator&(PEnumBitmask a, enum_type b) { return a &= b; }
    friend constexpr PEnumBitmask operator^(PEnumBitmask a, enum_type b) { return a ^= b; }

    friend constexpr PEnumBitmask operator|(enum_type a, PEnumBitmask b) { return b |= a; }
    friend constexpr PEnumBitmask operator&(enum_type a, PEnumBitmask b) { return PEnumBitmask{ a } &= b; }
    friend constexpr PEnumBitmask operator^(enum_type a, PEnumBitmask b) { return b ^= a; }

    friend constexpr PEnumBitmask operator~(PEnumBitmask a)
    {
        a.m_Bits = ~a.m_Bits;
        return a;
    }

    constexpr bool IsSubsetOf(PEnumBitmask allowed) const
    {
        return (m_Bits & ~allowed.m_Bits) == 0;
    }

protected:
    // For derived-mask conversions (copy bits)
    constexpr explicit PEnumBitmask(underlying_type bits) : m_Bits(bits) {}

    underlying_type m_Bits = 0;
};


// With-base — inherits from TBaseMask
template<PBitmaskEnum TEnumType, class TBaseMask>
class PEnumBitmask : public TBaseMask
{
public:
    using enum_type = TEnumType;
    using base_mask_type = TBaseMask;
    using underlying_type = typename TBaseMask::underlying_type;

    constexpr PEnumBitmask() = default;

    // Mixed construction:
    //  - accepts this level enum_type
    //  - accepts anything the base mask accepts (transitively, via PEnumBitmaskAcceptsArg)
    template<class... TFlagType>
        requires ((std::same_as<std::remove_cvref_t<TFlagType>, enum_type> ||
    PEnumBitmaskAcceptsArg<base_mask_type, TFlagType>) && ...)
        constexpr PEnumBitmask(TFlagType... flags)
    {
        (AddOne(flags), ...);
    }

    template<class... TFlagType>
        requires ((std::same_as<std::remove_cvref_t<TFlagType>, enum_type> ||
    PEnumBitmaskAcceptsArg<base_mask_type, TFlagType>) && ...)
        static constexpr PEnumBitmask From(TFlagType... flags)
    {
        return PEnumBitmask(flags...);
    }

    // Queries for this enum_type
    constexpr bool Has(enum_type flag) const
    {
        return (this->m_Bits & std::to_underlying(flag)) != 0;
    }

    // Modifiers for this enum_type
    constexpr void SetFlag(enum_type flag) { this->m_Bits |= std::to_underlying(flag); }
    constexpr void ClearFlag(enum_type flag) { this->m_Bits &= ~std::to_underlying(flag); }
    constexpr void ToggleFlag(enum_type flag) { this->m_Bits ^= std::to_underlying(flag); }

    // Operators with this enum_type
    constexpr PEnumBitmask& operator|=(enum_type flag) { SetFlag(flag); return *this; }
    constexpr PEnumBitmask& operator&=(enum_type flag) { this->m_Bits &= std::to_underlying(flag); return *this; }
    constexpr PEnumBitmask& operator^=(enum_type flag) { this->m_Bits ^= std::to_underlying(flag); return *this; }

    // Operators with same derived-mask type (keeps return types "derived")
    constexpr PEnumBitmask& operator|=(PEnumBitmask rhs) { this->m_Bits |= rhs.m_Bits; return *this; }
    constexpr PEnumBitmask& operator&=(PEnumBitmask rhs) { this->m_Bits &= rhs.m_Bits; return *this; }
    constexpr PEnumBitmask& operator^=(PEnumBitmask rhs) { this->m_Bits ^= rhs.m_Bits; return *this; }

    friend constexpr PEnumBitmask operator|(PEnumBitmask a, PEnumBitmask b) { return a |= b; }
    friend constexpr PEnumBitmask operator&(PEnumBitmask a, PEnumBitmask b) { return a &= b; }
    friend constexpr PEnumBitmask operator^(PEnumBitmask a, PEnumBitmask b) { return a ^= b; }

    friend constexpr PEnumBitmask operator|(PEnumBitmask a, enum_type b) { return a |= b; }
    friend constexpr PEnumBitmask operator&(PEnumBitmask a, enum_type b) { return a &= b; }
    friend constexpr PEnumBitmask operator^(PEnumBitmask a, enum_type b) { return a ^= b; }

    friend constexpr PEnumBitmask operator|(enum_type a, PEnumBitmask b) { return b |= a; }
    friend constexpr PEnumBitmask operator&(enum_type a, PEnumBitmask b) { return PEnumBitmask{ a } &= b; }
    friend constexpr PEnumBitmask operator^(enum_type a, PEnumBitmask b) { return b ^= a; }

    friend constexpr PEnumBitmask operator~(PEnumBitmask a)
    {
        a.m_Bits = ~a.m_Bits;
        return a;
    }

    using base_mask_type::Has;
    using base_mask_type::SetFlag;
    using base_mask_type::ClearFlag;
    using base_mask_type::ToggleFlag;
private:
    // Add derived enum
    constexpr void AddOne(enum_type flag)
    {
        this->m_Bits |= std::to_underlying(flag);
    }

    // Add base mask (or any base-derived mask, due to inheritance conversions)
    constexpr void AddOne(base_mask_type mask)
    {
        this->m_Bits |= mask.GetBits();
    }

    // Add anything else accepted by the base (including deeper base enums)
    template<class T>
        requires (PEnumBitmaskAcceptsArg<base_mask_type, T> &&
        !std::same_as<std::remove_cvref_t<T>, base_mask_type>)
    constexpr void AddOne(T flags)
    {
        this->m_Bits |= base_mask_type(flags).GetBits();
    }
};
