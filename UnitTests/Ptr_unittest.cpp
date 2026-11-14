// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <type_traits>
#include <gtest/gtest.h>
#include <Ptr/WeakPtr.h>

namespace pointer_tests
{

class ComparisonTarget : public PtrTarget
{
};

class UnrelatedTarget : public PtrTarget
{
};

// Reject invalid conversions during overload resolution, while retaining
// implicit derived-to-base conversions and addition of const qualification.
static_assert(std::is_convertible_v<Ptr<ComparisonTarget>, Ptr<PtrTarget>>);
static_assert(std::is_convertible_v<Ptr<ComparisonTarget>, Ptr<const ComparisonTarget>>);
static_assert(std::is_convertible_v<Ptr<ComparisonTarget>, Ptr<const PtrTarget>>);
static_assert(!std::is_convertible_v<Ptr<PtrTarget>, Ptr<ComparisonTarget>>);
static_assert(!std::is_convertible_v<Ptr<const ComparisonTarget>, Ptr<ComparisonTarget>>);
static_assert(!std::is_convertible_v<Ptr<ComparisonTarget>, Ptr<UnrelatedTarget>>);

TEST(PtrTest, ComparesRawPointersInBothDirectionsWithoutTakingOwnership)
{
    const Ptr<PtrTarget> pointer = ptr_new<ComparisonTarget>();
    ComparisonTarget* rawPointer = static_cast<ComparisonTarget*>(&*pointer);
    const ComparisonTarget* constPointer = rawPointer;
    PtrTarget* nullPointer = nullptr;

    EXPECT_EQ(pointer, rawPointer);
    EXPECT_EQ(rawPointer, pointer);
    EXPECT_EQ(pointer, constPointer);
    EXPECT_EQ(constPointer, pointer);
    EXPECT_NE(pointer, nullPointer);
    EXPECT_NE(nullPointer, pointer);
    EXPECT_NE(pointer, nullptr);
    EXPECT_NE(nullptr, pointer);
    EXPECT_FALSE(pointer < rawPointer);
    EXPECT_FALSE(rawPointer < pointer);
    EXPECT_EQ(rawPointer->GetPtrCount(), 1);

    const Ptr<PtrTarget> empty;
    EXPECT_EQ(empty, nullptr);
    EXPECT_EQ(nullptr, empty);
    EXPECT_EQ(empty, nullPointer);
    EXPECT_EQ(nullPointer, empty);
    EXPECT_NE(empty, rawPointer);
    EXPECT_NE(rawPointer, empty);
}

TEST(WeakPtrTest, RawPointerComparisonsTrackExpirationInBothDirections)
{
    Ptr<ComparisonTarget> owner = ptr_new<ComparisonTarget>();
    const WeakPtr<PtrTarget> pointer(owner);
    ComparisonTarget* rawPointer = &*owner;
    const ComparisonTarget* constPointer = rawPointer;
    PtrTarget* nullPointer = nullptr;

    EXPECT_EQ(pointer, rawPointer);
    EXPECT_EQ(rawPointer, pointer);
    EXPECT_EQ(pointer, constPointer);
    EXPECT_EQ(constPointer, pointer);
    EXPECT_NE(pointer, nullPointer);
    EXPECT_NE(nullPointer, pointer);
    EXPECT_NE(pointer, nullptr);
    EXPECT_NE(nullptr, pointer);
    EXPECT_FALSE(pointer < rawPointer);
    EXPECT_FALSE(rawPointer < pointer);
    EXPECT_EQ(owner->GetPtrCount(), 1);

    owner.Reset();
    EXPECT_EQ(pointer, nullptr);
    EXPECT_EQ(nullptr, pointer);
    EXPECT_EQ(pointer, nullPointer);
    EXPECT_EQ(nullPointer, pointer);

    const WeakPtr<PtrTarget> empty;
    EXPECT_EQ(empty, nullptr);
    EXPECT_EQ(nullptr, empty);
}

}
