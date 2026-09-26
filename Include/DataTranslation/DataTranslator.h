// This file is part of PadOS.
//
// Copyright (c) 2001-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>

#include <sys/types.h>

#include <Ptr/Ptr.h>
#include <Signals/VFConnector.h>

#include <Math/Rect.h>
#include <GUI/GUIDefines.h>


enum class PDataTranslatorType : uint8_t
{
    Unknown,
    Image
};

enum class PDataTranslatorStatus : uint8_t
{
    Success,
    NotEnoughData,
    UnsupportedType,
    UnknownFormat,
    NoMemory
};


struct PBitmapHeader
{
    size_t      HeaderSize;
    size_t      DataSize;
    uint32_t    Flags;
    PIRect       Bounds;
    int32_t     FrameCount;
    size_t      BytesPerRow;
    PEColorSpace ColorSpace;
};

struct PBitmapFrameHeader
{
    size_t      HeaderSize;
    size_t      DataSize;
    uint32_t    Timestamp;
    PEColorSpace ColorSpace;
    size_t      BytesPerRow;
    PIRect       Frame;
};

class PDataTranslator : public PtrTarget
{
public:
    PDataTranslator();
    virtual ~PDataTranslator();

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

struct PTranslatorInfo
{
    PString             SourceType;
    PDataTranslatorType DestType;
    float               Quality;
};

class PTranslatorNode : public PtrTarget
{
public:
    PTranslatorNode();
    virtual ~PTranslatorNode();

    virtual PDataTranslatorStatus   Identify(const PString& srcType, PDataTranslatorType dstType, const void* data, size_t length) const = 0;
    virtual PTranslatorInfo          GetTranslatorInfo() const = 0;
    virtual Ptr<PDataTranslator>     CreateTranslator() const = 0;
};

class PTranslatorFactory
{
public:
    PTranslatorFactory();
    ~PTranslatorFactory();

    static PTranslatorFactory& Get();
    
    void RegisterTranslator(Ptr<PTranslatorNode> translatorNode) { m_Nodes.push_back(translatorNode); }
    Ptr<PDataTranslator> FindTranslator(const PString& srcType, PDataTranslatorType dstType, const void* data, size_t length, PDataTranslatorStatus& outStatus) const;

    size_t              GetTranslatorCount() const;
    Ptr<PTranslatorNode> GetTranslatorNode(size_t index) const;
    PTranslatorInfo      GetTranslatorInfo(size_t index) const;
    Ptr<PDataTranslator> CreateTranslator(size_t index) const;

private:
    std::vector<Ptr<PTranslatorNode>> m_Nodes;
};

#define PREGISTER_DATA_TRANSLATOR(CLASS) PFactoryAutoRegistrator<CLASS> g_DataTranslatorFactoryRegistrationHelper___##CLASS([]{ Ptr<CLASS> obj = ptr_new<CLASS>();  PTranslatorFactory::Get().RegisterTranslator(obj); });
