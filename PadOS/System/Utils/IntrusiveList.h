// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.03.2018 19:43:54

#pragma once


template<typename T>
struct IntrusiveList
{
    void Append(T* node)
    {
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
    }
    void Insert(T* next, T* node)
    {
        node->m_List = this;
        if (next == nullptr)
        {
            node->m_Next = m_First;
            node->m_Prev = nullptr;
            m_First = node;
            if (m_Last == nullptr) {
                m_Last = node;
            }
        }
        else
        {
            node->m_Next = next;
            node->m_Prev = next->m_Prev;
            next->m_Prev = node;
            if (node->m_Prev != nullptr) {
                node->m_Prev->m_Next = node;
            } else {
                m_First = node;
            }
        }
    }
    void Remove(T* node)
    {
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
    }
    T* m_First = nullptr;
    T* m_Last = nullptr;
};

template<typename T>
class IntrusiveListNode
{
public:
    T* GetNext() { return m_Next; }
    T* GetPrev() { return m_Prev; }
    IntrusiveList<T>* GetList() { return m_List; }
        
    bool RemoveFromList()
    {
        if (m_List != nullptr) {
            m_List->Remove(static_cast<T*>(this));
            return true;
        } else {
            return false;
        }
    }        
protected:
    friend class IntrusiveList<T>;
    
    T*                m_Prev = nullptr;
    T*                m_Next = nullptr;
    IntrusiveList<T>* m_List = nullptr;    
};
