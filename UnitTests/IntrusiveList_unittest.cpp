// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <Utils/IntrusiveList.h>

namespace intrusive_list_tests
{

struct RawItem
{
    size_t Value = 0;
    PIntrusiveListNode<RawItem> Link{};
};

using RawList = PIntrusiveList<RawItem, &RawItem::Link>;

static_assert(sizeof(PIntrusiveListNode<RawItem>) == 3 * sizeof(void*));
static_assert(sizeof(RawList) == 2 * sizeof(void*) + sizeof(size_t));
static_assert(std::is_trivially_destructible_v<RawList>);
static_assert(std::bidirectional_iterator<RawList::iterator>);
static_assert(RawList::GetFirstNodeOffset() == 0);
static_assert(RawList::GetNextNodeOffset() == sizeof(void*));

TEST(IntrusiveList, RawMemberNodesSupportStableBidirectionalIteration)
{
    RawItem first{1};
    RawItem middle{2};
    RawItem last{3};
    RawList list;
    list.Append(&first);
    list.Append(&last);
    const auto savedLast = list.GetIterator(&last);
    list.Insert(&last, &middle);

    EXPECT_EQ((middle.Link.GetList<&RawItem::Link>()), &list);
    EXPECT_EQ(list.GetCount(), 3u);
    EXPECT_EQ(std::vector<RawItem*>(list.begin(), list.end()), (std::vector<RawItem*>{&first, &middle, &last}));
    EXPECT_EQ(std::vector<RawItem*>(list.rbegin(), list.rend()), (std::vector<RawItem*>{&last, &middle, &first}));
    EXPECT_EQ(*std::prev(list.end()), &last);
    EXPECT_EQ(*std::prev(savedLast), &middle);
    EXPECT_EQ(list.GetNext(&first), &middle);
    EXPECT_EQ(list.GetPrevious(&last), &middle);
    EXPECT_EQ(*list.GetReverseIterator(&middle), &middle);

    list.Remove(&middle);
    EXPECT_EQ(*savedLast, &last);
    EXPECT_EQ(*std::prev(savedLast), &first);
    EXPECT_FALSE(middle.Link.IsListMember());
    EXPECT_EQ(list.GetIterator(&middle), list.end());
    EXPECT_EQ(list.GetReverseIterator(&middle), list.rend());

    list.Insert(nullptr, &middle);
    EXPECT_EQ(list.GetFirst(), &middle);
    list.Remove(&last);
    EXPECT_EQ(list.GetLast(), &first);
    list.Clear();
    EXPECT_TRUE(list.IsEmpty());
    EXPECT_EQ(list.GetCount(), 0u);
    EXPECT_FALSE(first.Link.IsListMember());
    EXPECT_FALSE(middle.Link.IsListMember());
    EXPECT_FALSE(last.Link.IsListMember());
}

class OwnedItem : public PtrTarget, public PIntrusiveListNode<OwnedItem, Ptr<OwnedItem>>
{
public:
    explicit OwnedItem(size_t& destructionCount) : m_DestructionCount(destructionCount) {}

    virtual ~OwnedItem() override
    {
        EXPECT_FALSE(IsListMember());
        EXPECT_FALSE(m_OtherLink.IsListMember());
        ++m_DestructionCount;
    }

    PIntrusiveListNode<OwnedItem, Ptr<OwnedItem>> m_OtherLink;

private:
    size_t& m_DestructionCount;
};

using OwnedList = PIntrusiveList<OwnedItem, nullptr, Ptr<OwnedItem>>;
using OtherOwnedList = PIntrusiveList<OwnedItem, &OwnedItem::m_OtherLink, Ptr<OwnedItem>>;

static_assert(std::bidirectional_iterator<OwnedList::iterator>);
static_assert(!std::is_copy_constructible_v<OwnedList>);

TEST(IntrusiveList, OwningLinksRetainElementsAndReleaseRemovedElements)
{
    size_t destructionCount = 0;
    OwnedList list;
    Ptr<OwnedItem> first = ptr_new<OwnedItem>(destructionCount);
    Ptr<OwnedItem> middle = ptr_new<OwnedItem>(destructionCount);
    Ptr<OwnedItem> last = ptr_new<OwnedItem>(destructionCount);
    list.Append(first);
    list.Append(last);
    list.Insert(last, middle);
    EXPECT_EQ(first->GetPtrCount(), 2);
    EXPECT_EQ(middle->GetPtrCount(), 2);
    EXPECT_EQ(last->GetPtrCount(), 2);
    EXPECT_EQ(*std::prev(list.end()), last);
    EXPECT_EQ(list.GetNext(first), middle);
    EXPECT_EQ(list.GetPrevious(last), middle);

    middle = nullptr;
    EXPECT_EQ(destructionCount, 0u);
    list.Remove(list.GetNext(first));
    EXPECT_EQ(destructionCount, 1u);
    EXPECT_EQ(list.GetNext(first), last);
    EXPECT_EQ(list.GetPrevious(last), first);

    first = nullptr;
    list.Remove(list.GetFirst());
    EXPECT_EQ(destructionCount, 2u);
    EXPECT_EQ(list.GetFirst(), last);
    EXPECT_EQ(list.GetPrevious(last), nullptr);

    last = nullptr;
    list.Remove(list.GetLast());
    EXPECT_EQ(destructionCount, 3u);
    EXPECT_TRUE(list.IsEmpty());
    EXPECT_EQ(list.GetCount(), 0u);
}

TEST(IntrusiveList, OwningMemberAndBaseListsRetainIndependently)
{
    size_t destructionCount = 0;
    OwnedList firstList;
    OtherOwnedList secondList;
    Ptr<OwnedItem> item = ptr_new<OwnedItem>(destructionCount);
    firstList.Append(item);
    secondList.Insert(nullptr, item);
    EXPECT_EQ(item->GetList(), &firstList);
    EXPECT_EQ((item->m_OtherLink.GetList<&OwnedItem::m_OtherLink>()), &secondList);
    EXPECT_EQ(item->GetPtrCount(), 3);
    item = nullptr;

    firstList.Clear();
    EXPECT_EQ(destructionCount, 0u);
    EXPECT_FALSE(secondList.GetFirst()->IsListMember());
    secondList.Clear();
    EXPECT_EQ(destructionCount, 1u);
}

TEST(IntrusiveList, OwningListDestructionUnlinksEveryElementIncludingSurvivors)
{
    size_t destructionCount = 0;
    Ptr<OwnedItem> survivor = ptr_new<OwnedItem>(destructionCount);
    {
        OwnedList list;
        for (size_t index = 0; index < 128; ++index) {
            list.Append(ptr_new<OwnedItem>(destructionCount));
        }
        list.Insert(list.GetFirst(), survivor);
        EXPECT_EQ(destructionCount, 0u);
    }
    EXPECT_EQ(destructionCount, 128u);
    EXPECT_FALSE(survivor->IsListMember());
    EXPECT_EQ(survivor->GetPtrCount(), 1);
    survivor = nullptr;
    EXPECT_EQ(destructionCount, 129u);
}

}
