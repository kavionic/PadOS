// This file is part of PadOS.
//
// Copyright (C) 2001-2025 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <deque>

#include <sys/types.h>

#include <Ptr/Ptr.h>
#include <Signals/VFConnector.h>

#include <Math/Rect.h>
#include <GUI/GUIDefines.h>

namespace os
{

enum class EDataTranslatorType : uint8_t
{
    Unknown,
    Image
};

enum class EDataTranslatorStatus : uint8_t
{
    Success,
    NotEnoughData,
    UnsupportedType,
    UnknownFormat,
    NoMemory
};


struct BitmapHeader
{
    size_t      HeaderSize;
    size_t      DataSize;
    uint32_t    Flags;
    IRect       Bounds;
    int32_t     FrameCount;
    size_t      BytesPerRow;
    EColorSpace ColorSpace;
};

struct BitmapFrameHeader
{
    size_t      HeaderSize;
    size_t      DataSize;
    uint32_t    Timestamp;
    EColorSpace ColorSpace;
    size_t      BytesPerRow;
    IRect       Frame;
};

class DataTranslator : public PtrTarget
{
public:
    DataTranslator();
    virtual ~DataTranslator();

    virtual status_t AddData(const void* data, size_t length, bool isFinal ) = 0;
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* data, size_t length );
    virtual void     Abort() = 0;
    virtual void     Reset() = 0;

    VFConnector<bool (const void* data, size_t length, bool isFinal)> VFDataReady;

protected:
    void AddProcessedData(void* data, size_t length, bool isFinal);

private:
    std::deque<uint8_t> m_OutBuffer;
};

struct TranslatorInfo
{
    PString             SourceType;
    EDataTranslatorType DestType;
    float               Quality;
};

class TranslatorNode : public PtrTarget
{
public:
    TranslatorNode();
    virtual ~TranslatorNode();

    virtual EDataTranslatorStatus   Identify(const PString& srcType, EDataTranslatorType dstType, const void* data, size_t length) const = 0;
    virtual TranslatorInfo          GetTranslatorInfo() const = 0;
    virtual Ptr<DataTranslator>     CreateTranslator() const = 0;
};

class TranslatorFactory
{
public:
    TranslatorFactory();
    ~TranslatorFactory();

    static TranslatorFactory& Get();
    
    void RegisterTranslator(Ptr<TranslatorNode> translatorNode) { m_Nodes.push_back(translatorNode); }
    Ptr<DataTranslator> FindTranslator(const PString& srcType, EDataTranslatorType dstType, const void* data, size_t length, EDataTranslatorStatus& outStatus) const;

    size_t              GetTranslatorCount() const;
    Ptr<TranslatorNode> GetTranslatorNode(size_t index) const;
    TranslatorInfo      GetTranslatorInfo(size_t index) const;
    Ptr<DataTranslator> CreateTranslator(size_t index) const;

private:
    std::vector<Ptr<TranslatorNode>> m_Nodes;
};

#define REGISTER_DATA_TRANSLATOR(CLASS) FactoryAutoRegistrator<CLASS> g_DataTranslatorFactoryRegistrationHelper___##CLASS([]{ Ptr<CLASS> obj = ptr_new<CLASS>();  TranslatorFactory::Get().RegisterTranslator(obj); });

} // end of namespace
