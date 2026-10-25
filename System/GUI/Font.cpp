// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <string_view>

#include <GUI/Font.h>
#include <GUI/FontMetrics.h>
#include <ApplicationServer/Protocol.h>
#include <System/ExceptionHandling.h>
#include <Threads/Looper.h>
#include <Threads/Mutex.h>
#include <Utils/UTF8Utils.h>

// FreeType stays on the appserver looper. Client fonts use an IPC channel
// for font creation and measurement.
class PFont::FreeTypeState : public PtrTarget
{
public:
    explicit FreeTypeState(const PFontDescription& description)
        : m_Description(description)
        , m_Mutex("font_metrics", PEMutexRecursionMode::PEMutexRecursionMode_RaiseError)
        , m_ReplyPort("font_reply", 1)
        , m_ServerPort(p_get_appserver_port())
    {
        if (m_Mutex.GetHandle() == INVALID_HANDLE || m_ReplyPort.GetHandle() == INVALID_HANDLE) {
            PERROR_THROW_CODE(PErrorCode::NOMEM);
        }
        const PLooper* looper = PLooper::GetCurrentLooper();
        if (looper != nullptr && looper->GetPortID() == m_ServerPort) {
            PERROR_THROW_CODE(PErrorCode::DEADLK);
        }
        if (ASCreateFont::Sender::Serializer::GetSize(m_ReplyPort.GetHandle(), description) > PAPPSERVER_MSG_BUFFER_SIZE) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        if (!p_post_to_remotesignal<ASCreateFont>(
            m_ServerPort,
            INVALID_HANDLE,
            TimeValNanos::infinit,
            m_ReplyPort.GetHandle(),
            description)) {
            PERROR_THROW_CODE(PErrorCode(get_last_error()));
        }
        const MsgCreateFontReply reply = ReceiveReply<MsgCreateFontReply>(PAppserverProtocol::CREATE_FONT_REPLY);
        PERROR_ERRORCODE_THROW_ON_FAIL(reply.Error);
        m_Handle = reply.FontHandle;
        m_Height = reply.Height;
    }

    ~FreeTypeState()
    {
        p_post_to_remotesignal<ASDeleteFont>(m_ServerPort, INVALID_HANDLE, TimeValNanos::infinit, m_Handle);
    }

    const PFontDescription& GetDescription() const { return m_Description; }
    PFontHeight GetHeight() const { return m_Height; }

    PFontTextMetrics Measure(std::string_view text, float width, bool includeLast)
    {
        if (text.empty()) {
            return {};
        }
        CRITICAL_SCOPE(m_Mutex);
        const bool cacheable = text.size() <= MAX_CACHED_TEXT_BYTES;
        const size_t hash = cacheable ? std::hash<std::string_view>()(text) : 0;
        if (cacheable)
        {
            for (const Measurement& entry : m_Measurements)
            {
                if (entry.Valid && entry.Hash == hash && entry.Width == width && entry.IncludeLast == includeLast
                    && entry.Text == text) {
                    return entry.Metrics;
                }
            }
        }
        const PFontTextMetrics result = Measure_pl(text, width, includeLast);
        if (cacheable)
        {
            Measurement& entry = m_Measurements[m_NextMeasurement];
            entry.Valid = false;
            entry.Text.assign(text.data(), text.size());
            entry.Hash = hash;
            entry.Width = width;
            entry.IncludeLast = includeLast;
            entry.Metrics = result;
            entry.Valid = true;
            m_NextMeasurement = (m_NextMeasurement + 1) % m_Measurements.size();
        }
        return result;
    }

private:
    static constexpr size_t MEASUREMENT_CACHE_SIZE = 16;
    static constexpr size_t MAX_CACHED_TEXT_BYTES = 256;

    struct Measurement
    {
        PString Text;
        PFontTextMetrics Metrics;
        size_t Hash = 0;
        float Width = 0.0f;
        bool IncludeLast = false;
        bool Valid = false;
    };

    template<typename Reply>
    Reply ReceiveReply(int32_t expectedCode)
    {
        for (;;)
        {
            Reply reply{};
            int32_t code = 0;
            const ssize_t length = m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply));
            if (length >= 0)
            {
                if (code != expectedCode || size_t(length) != sizeof(reply)) {
                    PERROR_THROW_CODE(PErrorCode::INVAL);
                }
                return reply;
            }
            if (get_last_error() != EINTR) {
                PERROR_THROW_CODE(PErrorCode(get_last_error()));
            }
        }
    }

    PFontTextMetrics Measure_pl(std::string_view text, float width, bool includeLast)
    {
        assert(m_Mutex.IsLocked());
        PFontTextMetrics result;
        uint32_t previousGlyph = 0;
        while (result.ByteCount < text.size())
        {
            const size_t remaining = text.size() - result.ByteCount;
            size_t chunkLength = std::min(remaining, FONT_MEASUREMENT_CHUNK_BYTES);
            if (chunkLength < remaining)
            {
                while (chunkLength != 0 && is_utf8_continuation_byte(uint8_t(text[result.ByteCount + chunkLength]))) {
                    --chunkLength;
                }
                if (chunkLength == 0) {
                    chunkLength = std::min(remaining, FONT_MEASUREMENT_CHUNK_BYTES);
                }
            }
            const PString chunk(text.data() + result.ByteCount, chunkLength);
            if (!p_post_to_remotesignal<ASMeasureFont>(
                m_ServerPort,
                INVALID_HANDLE,
                TimeValNanos::infinit,
                m_ReplyPort.GetHandle(),
                m_Handle,
                chunk,
                width - result.Width,
                includeLast,
                previousGlyph)) {
                PERROR_THROW_CODE(PErrorCode(get_last_error()));
            }
            const MsgMeasureFontReply reply = ReceiveReply<MsgMeasureFontReply>(PAppserverProtocol::MEASURE_FONT_REPLY);
            PERROR_ERRORCODE_THROW_ON_FAIL(reply.Error);
            result.Advance += reply.Metrics.Advance;
            result.Width += reply.Metrics.Width;
            result.ByteCount += reply.Metrics.ByteCount;
            previousGlyph = reply.PreviousGlyph;
            if (reply.Metrics.ByteCount < chunkLength || result.Width > width) {
                break;
            }
        }
        return result;
    }

    PFontDescription m_Description;
    PMutex m_Mutex;
    PMessagePort m_ReplyPort;
    std::array<Measurement, MEASUREMENT_CACHE_SIZE> m_Measurements;
    PFontHeight m_Height{};
    size_t m_NextMeasurement = 0;
    port_id m_ServerPort;
    uint32_t m_Handle = 0;
};

PFont::PFont(PFontID font) : PFont(GetDefaultDescription(font))
{
}

PFont::PFont(const PFontDescription& description)
    : m_FreeType(ptr_new<FreeTypeState>(description))
    , m_Font(PFontID::e_FontCount)
{
}

PFont::~PFont() = default;

void PFont::Set(PFontID font)
{
    Set(GetDefaultDescription(font));
}

void PFont::Set(const PFontDescription& description)
{
    m_FreeType = ptr_new<FreeTypeState>(description);
    m_Font = PFontID::e_FontCount;
}

const PFontDescription& PFont::GetDescription() const
{
    if (m_FreeType == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return m_FreeType->GetDescription();
}

PFontHeight PFont::GetHeight() const
{
    if (m_FreeType != nullptr) {
        return m_FreeType->GetHeight();
    }
    PFontHeight height;
    height.ascender = 0.0f;
    height.descender = PFontMetrics::GetFontHeight(m_Font);
    height.line_gap = std::ceil(height.descender / 10.0f);
    return height;
}

int PFont::GetStringLength(const char* string, float width, bool includeLast) const
{
    const size_t length = std::strlen(string);
    if (length > size_t(std::numeric_limits<int>::max())) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return GetStringLength(string, int(length), width, includeLast);
}

int PFont::GetStringLength(const char* string, int length, float width, bool includeLast) const
{
    if (length < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_FreeType != nullptr) {
        return int(m_FreeType->Measure(std::string_view(string, size_t(length)), width, includeLast).ByteCount);
    }
    return int(PFontMetrics::GetStringLength(m_Font, string, size_t(length), width, includeLast));
}

int PFont::GetStringLength(const std::string& string, float width, bool includeLast) const
{
    if (string.size() > size_t(std::numeric_limits<int>::max())) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return GetStringLength(string.data(), int(string.size()), width, includeLast);
}

void PFont::GetStringLengths(
    const char** stringArray,
    const int* lengthArray,
    int stringCount,
    float width,
    int* maxLengthArray,
    bool includeLast) const
{
    for (int i = 0; i < stringCount; ++i) {
        maxLengthArray[i] = GetStringLength(stringArray[i], lengthArray[i], width, includeLast);
    }
}

float PFont::GetStringWidth(const char* string, size_t length) const
{
    if (m_FreeType != nullptr) {
        return m_FreeType->Measure(std::string_view(string, length), std::numeric_limits<float>::infinity(), false).Width;
    }
    return PFontMetrics::GetStringWidth(m_Font, string, length);
}

PFontDescription PFont::GetDefaultDescription(PFontID font)
{
    PFontDescription description;
    // Match the bitmap font heights: 23, 30, and 96 pixels at FONT_DPI.
    switch (font)
    {
        case PFontID::e_FontSmall:
        case PFontID::e_FontNormal:
            description.PointSize = 14.0f;
            break;
        case PFontID::e_FontLarge:
            description.PointSize = 19.0f;
            break;
        case PFontID::e_Font7Seg:
            description.PointSize = 61.0f;
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return description;
}
