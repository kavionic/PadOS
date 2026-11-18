// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.03.2018 19:43:54

#pragma once

#include <assert.h>
#include <atomic>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <Ptr/Ptr.h>

template<typename TNodeType, typename TForwardPointer = TNodeType*> class PIntrusiveListNode;
template<typename TNodeType, auto NodeMember = nullptr, typename TForwardPointer = TNodeType*> class PIntrusiveList;

////////////////////////////////////////////////////////////////////////////////
/// PIntrusiveListNode / PIntrusiveList
///
/// Intrusive doubly-linked list where the link node can be a member of the
/// elements, or the base class. This allows an object to participate in
/// multiple independent intrusive lists simultaneously (one per member node).
///
/// Member-based usage:
///   struct Foo {
///       PIntrusiveListNode<Foo> m_ListNode;
///   };
///   using FooList = PIntrusiveList<Foo, &Foo::m_ListNode>;
///
/// Inheritance-based usage (NodeMember defaults to nullptr):
///   class Bar : public PIntrusiveListNode<Bar> { ... };
///   using BarList = PIntrusiveList<Bar>;
///
/// Owning usage:
///   class Item : public PtrTarget,
///                public PIntrusiveListNode<Item, Ptr<Item>> { ... };
///   using ItemList = PIntrusiveList<Item, nullptr, Ptr<Item>>;
///
/// TForwardPointer selects raw TNodeType* or owning Ptr<TNodeType> links.
/// Raw lists keep Node* links, including their debugger-visible representation.
/// Owning lists retain the first element and each successor through Ptr<T>.
/// Backward links and the tail remain non-owning. Destroying an owning list
/// unlinks and releases its elements iteratively. Raw lists do not own or
/// unlink their elements on destruction.
////////////////////////////////////////////////////////////////////////////////

template<typename TNodeType, typename TForwardPointer>
class PIntrusiveListNode
{
public:
    using ForwardPointer = std::conditional_t<std::is_pointer_v<TForwardPointer>, PIntrusiveListNode*, TForwardPointer>;

    bool IsListMember() const noexcept { return m_List != nullptr; }
    bool IsListMember(const void* list) const noexcept { return m_List == list; }

    template<auto NodeMember = nullptr>
    PIntrusiveList<TNodeType, NodeMember, TForwardPointer>* GetList() const
    {
        return static_cast<PIntrusiveList<TNodeType, NodeMember, TForwardPointer>*>(m_List);
    }

private:
    template<typename TOwner, auto Member, typename TPointer>
    friend class PIntrusiveList;

    PIntrusiveListNode* m_Prev = nullptr;
    ForwardPointer     m_Next = nullptr;
    void*              m_List = nullptr;
};

template<typename TNodeType, auto NodeMember, typename TForwardPointer>
class PIntrusiveList
{
public:
    using Node = PIntrusiveListNode<TNodeType, TForwardPointer>;
    using ForwardPointer = typename Node::ForwardPointer;

    static_assert(std::is_same_v<TForwardPointer, TNodeType*> || std::is_same_v<TForwardPointer, Ptr<TNodeType>>);

    constexpr PIntrusiveList() noexcept = default;
    PIntrusiveList(const PIntrusiveList&) requires std::is_pointer_v<TForwardPointer> = default;
    PIntrusiveList& operator=(const PIntrusiveList&) requires std::is_pointer_v<TForwardPointer> = default;

    ~PIntrusiveList() requires std::is_pointer_v<TForwardPointer> = default;
    ~PIntrusiveList() requires (!std::is_pointer_v<TForwardPointer>) { Clear(); }

    // Field offsets for halted-target debugger metadata of raw lists.
    static constexpr size_t GetFirstNodeOffset() noexcept
    {
        static_assert(std::is_pointer_v<TForwardPointer>);
        return offsetof(PIntrusiveList, m_First);
    }

    static constexpr size_t GetNextNodeOffset() noexcept
    {
        static_assert(std::is_pointer_v<TForwardPointer>);
        return offsetof(Node, m_Next);
    }

    void Append(TForwardPointer owner) noexcept
    {
        Node* node = OwnerToNode(std::to_address(owner));

        assert(!node->IsListMember());

        node->m_List = this;
        node->m_Prev = m_Last;
        node->m_Next = nullptr;
        // Keep initialization before publishing the forward link to a halted-target debugger.
        std::atomic_signal_fence(std::memory_order_seq_cst);
        if (m_Last != nullptr) {
            m_Last->m_Next = MakeForwardPointer(node);
        }
        m_Last = node;
        if (m_First == nullptr) {
            m_First = MakeForwardPointer(node);
        }
        m_NodeCount++;
    }

    // Insert before next, or prepend when next is nullptr. Use Append() for the tail.
    void Insert(TForwardPointer next, TForwardPointer owner) noexcept
    {
        Node* node = OwnerToNode(std::to_address(owner));

        assert(!node->IsListMember());

        node->m_List = this;
        if (next == nullptr)
        {
            node->m_Next = std::move(m_First);
            node->m_Prev = nullptr;
            if (node->m_Next != nullptr) {
                ForwardToNode(node->m_Next)->m_Prev = node;
            }
            // Publish only after the node is initialized.
            std::atomic_signal_fence(std::memory_order_seq_cst);
            m_First = MakeForwardPointer(node);
            if (m_Last == nullptr) {
                m_Last = node;
            }
        }
        else
        {
            Node* nextNode = OwnerToNode(std::to_address(next));
            assert(nextNode->IsListMember(this));

            node->m_Next = MakeForwardPointer(nextNode);
            node->m_Prev = nextNode->m_Prev;
            nextNode->m_Prev = node;

            std::atomic_signal_fence(std::memory_order_seq_cst);
            if (node->m_Prev != nullptr) {
                node->m_Prev->m_Next = MakeForwardPointer(node);
            } else {
                m_First = MakeForwardPointer(node);
            }
        }
        m_NodeCount++;
    }

    void Remove(TForwardPointer owner) noexcept
    {
        // The argument retains an owned element until all links and the count are updated.
        Node* node = OwnerToNode(std::to_address(owner));
        assert(node->IsListMember(this));

        Node* next = ForwardToNode(node->m_Next);
        node->m_List = nullptr;

        if (node->m_Prev != nullptr) {
            node->m_Prev->m_Next = std::move(node->m_Next);
        } else {
            m_First = std::move(node->m_Next);
        }
        // Remove forward reachability before clearing links or releasing the owner.
        std::atomic_signal_fence(std::memory_order_seq_cst);
        if (next != nullptr) {
            next->m_Prev = node->m_Prev;
        } else {
            m_Last = node->m_Prev;
        }
        node->m_Prev = nullptr;
        node->m_Next = nullptr;
        m_NodeCount--;
    }

    void Clear() noexcept
    {
        while (!IsEmpty()) {
            Remove(GetFirst());
        }
    }

    TForwardPointer GetFirst() const noexcept { return MakeOwnerPointer(ForwardToNode(m_First)); }
    TForwardPointer GetLast() const noexcept { return MakeOwnerPointer(m_Last); }

    TForwardPointer GetNext(const TForwardPointer& owner) const noexcept
    {
        const Node* node = OwnerToNode(std::to_address(owner));
        assert(node->IsListMember(this));
        return MakeOwnerPointer(ForwardToNode(node->m_Next));
    }

    TForwardPointer GetPrevious(const TForwardPointer& owner) const noexcept
    {
        const Node* node = OwnerToNode(std::to_address(owner));
        assert(node->IsListMember(this));
        return MakeOwnerPointer(node->m_Prev);
    }

    bool IsEmpty() const noexcept { return m_First == nullptr; }
    size_t GetCount() const noexcept { return m_NodeCount; }

    bool empty() const noexcept { return IsEmpty(); }
    size_t size() const noexcept { return GetCount(); }
    TForwardPointer front() const noexcept { return GetFirst(); }
    TForwardPointer back() const noexcept { return GetLast(); }

    class Iterator
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = TForwardPointer;
        using difference_type = ptrdiff_t;
        using pointer = TNodeType*;
        using reference = std::conditional_t<std::is_pointer_v<TForwardPointer>, TForwardPointer, const TForwardPointer&>;

        Iterator() noexcept = default;
        Iterator(const PIntrusiveList* list, Node* node) noexcept : m_List(list), m_Cur(node) {}

        TNodeType* operator->() const noexcept { return NodeToOwner(m_Cur); }
        reference operator*() const noexcept
        {
            if constexpr (std::is_pointer_v<TForwardPointer>) {
                return NodeToOwner(m_Cur);
            } else {
                return (m_Cur->m_Prev != nullptr) ? m_Cur->m_Prev->m_Next : m_List->m_First;
            }
        }

        Iterator& operator++() noexcept { m_Cur = ForwardToNode(m_Cur->m_Next); return *this; }
        Iterator operator++(int) noexcept { Iterator previous = *this; ++*this; return previous; }
        Iterator& operator--() noexcept { m_Cur = (m_Cur != nullptr) ? m_Cur->m_Prev : m_List->m_Last; return *this; }
        Iterator operator--(int) noexcept { Iterator previous = *this; --*this; return previous; }

        bool operator==(const Iterator& other) const noexcept { return m_List == other.m_List && m_Cur == other.m_Cur; }
        bool operator!=(const Iterator& other) const noexcept { return !(*this == other); }

    private:
        const PIntrusiveList* m_List = nullptr;
        Node* m_Cur = nullptr;
    };

    using iterator = Iterator;
    using const_iterator = Iterator;
    using reverse_iterator = std::reverse_iterator<Iterator>;
    using const_reverse_iterator = reverse_iterator;

    Iterator begin() const noexcept { return { this, ForwardToNode(m_First) }; }
    Iterator end() const noexcept { return { this, nullptr }; }
    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

    Iterator GetIterator(const TForwardPointer& owner) const noexcept
    {
        Node* node = OwnerToNode(std::to_address(owner));
        return (node != nullptr && node->IsListMember(this)) ? Iterator(this, node) : end();
    }

    reverse_iterator GetReverseIterator(const TForwardPointer& owner) const noexcept
    {
        Iterator iterator = GetIterator(owner);
        return (iterator != end()) ? reverse_iterator(++iterator) : rend();
    }

private:
    static constexpr ptrdiff_t GetNodeOffset() noexcept
    {
        static_assert(NodeMember != nullptr);
        constexpr char* buf = nullptr;
        return reinterpret_cast<char*>(&(reinterpret_cast<TNodeType*>(buf)->*NodeMember)) - buf;
    }

    static TNodeType* NodeToOwner(Node* node) noexcept
    {
        if constexpr (NodeMember == nullptr) {
            return static_cast<TNodeType*>(node);
        } else {
            return (node != nullptr) ? reinterpret_cast<TNodeType*>(reinterpret_cast<char*>(node) - GetNodeOffset()) : nullptr;
        }
    }

    static Node* OwnerToNode(TNodeType* owner) noexcept
    {
        if constexpr (NodeMember == nullptr) {
            return static_cast<Node*>(owner);
        } else {
            return (owner != nullptr) ? &(owner->*NodeMember) : nullptr;
        }
    }

    static TForwardPointer MakeOwnerPointer(Node* node) noexcept
    {
        if constexpr (std::is_pointer_v<TForwardPointer>) {
            return NodeToOwner(node);
        } else {
            return ptr_tmp_cast(NodeToOwner(node));
        }
    }

    static ForwardPointer MakeForwardPointer(Node* node) noexcept
    {
        if constexpr (std::is_pointer_v<TForwardPointer>) {
            return node;
        } else {
            return MakeOwnerPointer(node);
        }
    }

    static Node* ForwardToNode(const ForwardPointer& pointer) noexcept
    {
        if constexpr (std::is_pointer_v<TForwardPointer>) {
            return pointer;
        } else {
            return OwnerToNode(std::to_address(pointer));
        }
    }

    ForwardPointer m_First = nullptr;
    Node* m_Last = nullptr;
    size_t m_NodeCount = 0;
};
