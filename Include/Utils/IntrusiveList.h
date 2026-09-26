// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.03.2018 19:43:54

#pragma once

#include <assert.h>
#include <cstddef>

template<typename TNodeType> class PIntrusiveListNode;
template<typename TNodeType, PIntrusiveListNode<TNodeType> TNodeType::* NodeMember = nullptr> struct PIntrusiveList;

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
/// The list stores Node* internally (no owner pointer in each node). The
/// reverse-mapping (member case) is done via a compile-time constant offset
/// with no per-element overhead beyond a pointer subtraction.
////////////////////////////////////////////////////////////////////////////////

template<typename TNodeType>
class PIntrusiveListNode
{
public:
    bool IsListMember() const noexcept { return m_List != nullptr; }
    bool IsListMember(const void* list) const noexcept { return m_List == list; }

    template<PIntrusiveListNode<TNodeType> TNodeType::* NodeMember = nullptr>
    PIntrusiveList<TNodeType, NodeMember>* GetList() const { return static_cast<PIntrusiveList<TNodeType, NodeMember>*>(m_List); }

private:
    template<typename O, PIntrusiveListNode<O> O::* M>
    friend struct PIntrusiveList;

    PIntrusiveListNode*   m_Prev = nullptr;
    PIntrusiveListNode*   m_Next = nullptr;
    void*                 m_List = nullptr;
};

template<typename TNodeType, PIntrusiveListNode<TNodeType> TNodeType::* NodeMember>
class PIntrusiveList
{
public:
    using Node = PIntrusiveListNode<TNodeType>;

    void Append(TNodeType* owner) noexcept
    {
        Node* node = OwnerToNode(owner);

        assert(!node->IsListMember());

        node->m_List = this;
        node->m_Prev = m_Last;
        node->m_Next = nullptr;
        if (m_Last != nullptr) {
            m_Last->m_Next = node;
        }
        m_Last = node;
        if (m_First == nullptr) {
            m_First = node;
        }
        m_NodeCount++;
    }

    void Insert(TNodeType* next, TNodeType* owner) noexcept
    {
        Node* node = OwnerToNode(owner);

        assert(!node->IsListMember());

        node->m_List = this;
        if (next == nullptr)
        {
            node->m_Next = m_First;
            node->m_Prev = nullptr;
            if (m_First != nullptr) {
                m_First->m_Prev = node;
            }
            m_First = node;
            if (m_Last == nullptr) {
                m_Last = node;
            }
        }
        else
        {
            Node* nextNode = OwnerToNode(next);
            
            node->m_Next = nextNode;
            node->m_Prev = nextNode->m_Prev;
            
            nextNode->m_Prev = node;
            
            if (node->m_Prev != nullptr) {
                node->m_Prev->m_Next = node;
            } else {
                m_First = node;
            }
        }
        m_NodeCount++;
    }

    void Remove(TNodeType* owner) noexcept
    {
        Node* node = OwnerToNode(owner);
        
        assert(node->IsListMember(this));
        
        node->m_List = nullptr;
        
        if (node->m_Prev != nullptr) {
            node->m_Prev->m_Next = node->m_Next;
        } else {
            m_First = node->m_Next;
        }
        if (node->m_Next != nullptr) {
            node->m_Next->m_Prev = node->m_Prev;
        } else {
            m_Last = node->m_Prev;
        }
        node->m_Prev = nullptr;
        node->m_Next = nullptr;

        m_NodeCount--;
    }

    TNodeType* GetFirst() const noexcept { return NodeToOwner(m_First); }
    TNodeType* GetLast()  const noexcept { return NodeToOwner(m_Last); }

    bool   IsEmpty()  const noexcept { return m_First == nullptr; }

    size_t GetCount() const noexcept { return m_NodeCount; }

    class Iterator
    {
    public:
        Iterator(Node* obj) : m_Cur(obj) {}
        TNodeType* operator->() const noexcept  { return NodeToOwner(m_Cur); }
        TNodeType* operator*() const noexcept   { return NodeToOwner(m_Cur); }
        Iterator& operator++() noexcept         { m_Cur = m_Cur->m_Next; return *this; }

        bool      operator!=(const Iterator& other) const noexcept { return m_Cur != other.m_Cur; }
    private:
        Node* m_Cur;
    };

    Iterator begin() const noexcept { return { m_First }; }
    Iterator end()   const noexcept { return { nullptr }; }

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

    static const TNodeType* NodeToOwner(const Node* node) noexcept
    {
        if constexpr (NodeMember == nullptr) {
            return static_cast<const TNodeType*>(node);
        } else {
            return (node != nullptr) ? reinterpret_cast<const TNodeType*>(reinterpret_cast<const char*>(node) - GetNodeOffset()) : nullptr;
        }
    }

    static Node* OwnerToNode(TNodeType* owner) noexcept
    {
        if constexpr (NodeMember == nullptr) {
            return static_cast<Node*>(owner);
        } else {
            return owner ? &(owner->*NodeMember) : nullptr;
        }
    }

    Node*   m_First = nullptr;
    Node*   m_Last  = nullptr;
    size_t  m_NodeCount = 0;
};
