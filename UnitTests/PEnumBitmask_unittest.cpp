#include <gtest/gtest.h>

#include <type_traits>
#include <cstdint>

#include <Utils/EnumBitmask.h>

// Everything is inside a uniquely named namespace to be unity-build safe.
namespace pados_tests_enumbitmask
{
enum class PEB_TE1 : uint32_t { A = 0x01, B = 0x02 };
enum class PEB_TE2 : uint32_t { A = 0x04, B = 0x08 };
enum class PEB_TE3 : uint32_t { A = 0x10, B = 0x20 };

using PEB_TM1 = PEnumBitmask<PEB_TE1>;
using PEB_TM2 = PEnumBitmask<PEB_TE2, PEB_TM1>;
using PEB_TM3 = PEnumBitmask<PEB_TE3, PEB_TM2>;

TEST(PEnumBitmask, Root_EmptyAndFrom)
{
    const PEB_TM1 a;
    EXPECT_TRUE(a.Empty());
    EXPECT_EQ(a.GetBits(), 0u);

    const PEB_TM1 b = PEB_TM1::From();
    EXPECT_TRUE(b.Empty());
    EXPECT_EQ(b.GetBits(), 0u);

    const PEB_TM1 c = PEB_TM1::From(PEB_TE1::A);
    EXPECT_FALSE(c.Empty());
    EXPECT_TRUE(c.Has(PEB_TE1::A));
    EXPECT_FALSE(c.Has(PEB_TE1::B));
    EXPECT_EQ(c.GetBits(), 0x01u);

    const PEB_TM1 d = PEB_TM1::From(PEB_TE1::A, PEB_TE1::B);
    EXPECT_TRUE(d.Has(PEB_TE1::A));
    EXPECT_TRUE(d.Has(PEB_TE1::B));
    EXPECT_EQ(d.GetBits(), 0x03u);
}

TEST(PEnumBitmask, Root_OperatorsAndHelpers)
{
    PEB_TM1 m;
    m |= PEB_TE1::A;
    EXPECT_EQ(m.GetBits(), 0x01u);

    m |= PEB_TE1::B;
    EXPECT_EQ(m.GetBits(), 0x03u);

    const PEB_TM1 a(PEB_TE1::A);
    const PEB_TM1 b(PEB_TE1::B);

    const PEB_TM1 c = a | b;
    EXPECT_EQ(c.GetBits(), 0x03u);

    const PEB_TM1 d = c & PEB_TE1::A;
    EXPECT_EQ(d.GetBits(), 0x01u);

    const PEB_TM1 e = c ^ PEB_TE1::A;
    EXPECT_EQ(e.GetBits(), 0x02u);

    const PEB_TM1 f = ~PEB_TM1(PEB_TE1::A);
    EXPECT_EQ((f.GetBits() & 0x01u), 0u);

    EXPECT_TRUE(PEB_TM1(PEB_TE1::A).IsSubsetOf(PEB_TM1(PEB_TE1::A, PEB_TE1::B)));
    EXPECT_FALSE(PEB_TM1(PEB_TE1::A, PEB_TE1::B).IsSubsetOf(PEB_TM1(PEB_TE1::A)));
}

TEST(PEnumBitmask, Chaining_BaseConversions)
{
    const PEB_TM2 m2(PEB_TE1::A, PEB_TE2::A);
    EXPECT_EQ(m2.GetBits(), (0x01u | 0x04u));
    EXPECT_TRUE(m2.Has(PEB_TE2::A));
    EXPECT_TRUE(m2.Has(PEB_TE1::A));

    PEB_TM1 m1 = m2; // derived-to-base (inheritance)
    EXPECT_EQ(m1.GetBits(), (0x01u | 0x04u));
    EXPECT_TRUE(m1.Has(PEB_TE1::A));
    EXPECT_FALSE(m1.Has(PEB_TE1::B));
}

TEST(PEnumBitmask, Chaining_TransitiveMixedConstruction)
{
    const PEB_TM3 m3(PEB_TE1::A, PEB_TE2::A, PEB_TE3::A, PEB_TE3::B);
    EXPECT_EQ(m3.GetBits(), (0x01u | 0x04u | 0x10u | 0x20u));

    EXPECT_TRUE(m3.Has(PEB_TE3::A));
    EXPECT_TRUE(m3.Has(PEB_TE3::B));
    EXPECT_TRUE(m3.Has(PEB_TE2::A));
    EXPECT_TRUE(m3.Has(PEB_TE1::A));
}

TEST(PEnumBitmask, Chaining_BraceInitMixedConstruction)
{
    const PEB_TM3 m3{ PEB_TE1::B, PEB_TE2::B, PEB_TE3::A };
    EXPECT_EQ(m3.GetBits(), (0x02u | 0x08u | 0x10u));

    EXPECT_TRUE(m3.Has(PEB_TE1::B));
    EXPECT_TRUE(m3.Has(PEB_TE2::B));
    EXPECT_TRUE(m3.Has(PEB_TE3::A));
    EXPECT_FALSE(m3.Has(PEB_TE3::B));
}

TEST(PEnumBitmask, Chaining_DerivedOperatorsReturnDerived)
{
    const PEB_TM3 a(PEB_TE3::A);
    const PEB_TM3 b(PEB_TE3::B);

    static_assert(std::same_as<decltype(a | b), PEB_TM3>);
    static_assert(std::same_as<decltype(a & b), PEB_TM3>);
    static_assert(std::same_as<decltype(a ^ b), PEB_TM3>);
    static_assert(std::same_as<decltype(~a), PEB_TM3>);

    const PEB_TM3 c = a | b;
    EXPECT_EQ(c.GetBits(), (0x10u | 0x20u));
}

TEST(PEnumBitmask, Chaining_OperatorOrEqualsWithBaseEnum_MutatesDerived)
{
    PEB_TM3 m;
    m |= PEB_TE1::A;
    EXPECT_TRUE(m.Has(PEB_TE1::A));
    EXPECT_EQ(m.GetBits(), 0x01u);

    m |= PEB_TE2::B;
    EXPECT_TRUE(m.Has(PEB_TE2::B));
    EXPECT_EQ(m.GetBits(), (0x01u | 0x08u));

    m |= PEB_TE3::A;
    EXPECT_TRUE(m.Has(PEB_TE3::A));
    EXPECT_EQ(m.GetBits(), (0x01u | 0x08u | 0x10u));
}

TEST(PEnumBitmask, Chaining_ConstructFromBaseMasks)
{
    const PEB_TM1 base1(PEB_TE1::B);
    const PEB_TM2 base2(PEB_TE1::A, PEB_TE2::A);

    const PEB_TM3 m3(base1, base2, PEB_TE3::B);
    EXPECT_EQ(m3.GetBits(), (0x02u | 0x01u | 0x04u | 0x20u));
    EXPECT_TRUE(m3.Has(PEB_TE1::A));
    EXPECT_TRUE(m3.Has(PEB_TE1::B));
    EXPECT_TRUE(m3.Has(PEB_TE2::A));
    EXPECT_TRUE(m3.Has(PEB_TE3::B));
}

TEST(PEnumBitmask, Chaining_SubsetChecksAcrossLevels)
{
    const PEB_TM3 all(PEB_TE1::A, PEB_TE1::B, PEB_TE2::A, PEB_TE2::B, PEB_TE3::A, PEB_TE3::B);

    const PEB_TM1 only1(PEB_TE1::A);
    const PEB_TM2 only2(PEB_TE1::A, PEB_TE2::A);
    const PEB_TM3 only3(PEB_TE1::A, PEB_TE2::A, PEB_TE3::A);

    EXPECT_TRUE(only1.IsSubsetOf(PEB_TM1(all)));
    EXPECT_TRUE(PEB_TM1(only2).IsSubsetOf(PEB_TM1(all)));
    EXPECT_TRUE(PEB_TM1(only3).IsSubsetOf(PEB_TM1(all)));

    EXPECT_TRUE(PEB_TM2(only2).IsSubsetOf(PEB_TM2(all)));
    EXPECT_TRUE(PEB_TM2(only3).IsSubsetOf(PEB_TM2(all)));

    EXPECT_TRUE(only3.IsSubsetOf(all));
    EXPECT_FALSE(all.IsSubsetOf(only3));
}

TEST(PEnumBitmask, Chaining_MemberOrEqualsWithMixedEnums_NoAmbiguity)
{
    PEB_TM3 m(PEB_TE3::A);

    m |= PEB_TE1::A;  // base enum via inheritance
    EXPECT_EQ(m.GetBits(), (0x10u | 0x01u));

    m |= PEB_TE2::A;  // base enum via inheritance
    EXPECT_EQ(m.GetBits(), (0x10u | 0x01u | 0x04u));
}

TEST(PEnumBitmask, CompileTime_RejectWrongArgumentTypes)
{
    struct PEB_NotAFlag {};

    static_assert(std::is_constructible_v<PEB_TM1, PEB_TE1>);
    static_assert(!std::is_constructible_v<PEB_TM1, PEB_TE2>);
    static_assert(!std::is_constructible_v<PEB_TM1, PEB_NotAFlag>);

    static_assert(std::is_constructible_v<PEB_TM2, PEB_TE1, PEB_TE2>);
    static_assert(!std::is_constructible_v<PEB_TM2, PEB_TE3>);

    static_assert(std::is_constructible_v<PEB_TM3, PEB_TE1, PEB_TE2, PEB_TE3>);
    static_assert(!std::is_constructible_v<PEB_TM3, PEB_NotAFlag>);

    static_assert(PEnumBitmaskAcceptsArg<PEB_TM1, PEB_TE1>);
    static_assert(!PEnumBitmaskAcceptsArg<PEB_TM1, PEB_TE2>);

    static_assert(PEnumBitmaskAcceptsArg<PEB_TM2, PEB_TE2>);
    static_assert(PEnumBitmaskAcceptsArg<PEB_TM2, PEB_TE1>);

    static_assert(PEnumBitmaskAcceptsArg<PEB_TM3, PEB_TE3>);
    static_assert(PEnumBitmaskAcceptsArg<PEB_TM3, PEB_TE2>);
    static_assert(PEnumBitmaskAcceptsArg<PEB_TM3, PEB_TE1>);
}
} // namespace pados_tests_enumbitmask
