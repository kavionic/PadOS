// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <Kernel/Drivers/Video/KDisplayDriver.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <System/ExceptionHandling.h>
#include <GUI/Bitmap.h>
#include <limits>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TElement>
static std::span<const TElement> ReadDisplayArray(std::span<const std::byte>& payload, size_t count)
{
    static_assert(alignof(TElement) <= align_argument_size(1));
    if (count > payload.size() / sizeof(TElement)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const TElement* result = reinterpret_cast<const TElement*>(payload.data());
    payload = payload.subspan(count * sizeof(TElement));
    return {result, count};
}

static void ValidateDisplayPayloadEnd(std::span<const std::byte> payload)
{
    if (payload.size() >= DISPLAY_COMMAND_ALIGNMENT) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

static void ValidateDisplayWriteBitmap(const PDisplayBitmap& bitmap)
{
    if (!bitmap.VideoMemory) {
        validate_user_write_pointer_trw(bitmap.Raster, bitmap.BytesPerLine * size_t(bitmap.Size.y));
    }
}

static void ValidateDisplayReadBitmap(const PDisplayBitmap& bitmap)
{
    if (!bitmap.VideoMemory) {
        validate_user_read_pointer_trw(bitmap.Raster, bitmap.BytesPerLine * size_t(bitmap.Size.y));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KDisplayDriver::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags)
{
    CRITICAL_SCOPE(m_Mutex);
    if (m_IsOpen) {
        PERROR_THROW_CODE(PErrorCode::BUSY);
    }
    Ptr<KFileNode> file = KFilesystemFileOps::OpenFile(volume, inode, openFlags);
    m_IsOpen = true;
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    CRITICAL_SCOPE(m_Mutex);
    if (m_IsInitialized)
    {
        WaitIdle();
        Close();
        m_IsInitialized = false;
    }
    m_IsOpen = false;
    KFilesystemFileOps::CloseFile(volume, file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::DeviceControl(
    Ptr<KFileNode> file,
    int request,
    const void* inData,
    size_t inDataLength,
    void* outData,
    size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    validate_user_read_pointer_trw(inData, inDataLength);
    validate_user_write_pointer_trw(outData, outDataLength);
    if (!m_IsInitialized && request != std::to_underlying(PDisplayRequest::Initialize)) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    if (m_IsInitialized && m_IsFaulted) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    m_DeviceControlDispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayInfo KDisplayDriver::HandleInitialize()
{
    kassert(m_Mutex.IsLocked());
    if (!m_IsInitialized)
    {
        if (!Open()) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        m_IsInitialized = true;
        SetFgColor(PColor(0));
        SetBgColor(PColor(0));
        WaitIdle();
    }
    return {*GetScreenBitmap(), GetScreenModeCount()};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleSubmit(const void* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());
    validate_user_read_pointer_trw(buffer, length);
    // Drain explicitly: WaitIdle() can report EIO and must not throw from a scope-guard destructor.
    try
    {
        ExecuteCommands_pl(buffer, length);
    }
    PERROR_CATCH([this](PErrorCode) { WaitIdle(); throw; });
    WaitIdle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDisplayDriver::HandleGetScreenMode(size_t index, PScreenMode* outMode)
{
    kassert(m_Mutex.IsLocked());
    validate_user_write_pointer_trw(outMode);
    if (uintptr_t(outMode) % alignof(PScreenMode) != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PScreenMode mode;
    const bool found = GetScreenModeDesc(index, mode);
    *outMode = mode;
    return found;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDisplayDriver::HandleSetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate)
{
    kassert(m_Mutex.IsLocked());
    WaitIdle();
    const bool accepted = SetScreenMode(resolution, colorSpace, refreshRate);
    WaitIdle();
    return accepted;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDisplayDriver::HandleSetMouseCursorBitmap(const PMouseCursorBitmap& cursor)
{
    kassert(m_Mutex.IsLocked());
    validate_user_read_pointer_trw(cursor.Raster.data(), cursor.Raster.size_bytes());
    return SetMouseCursorBitmap(cursor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleWritePixel(PDisplayBitmap bitmap, PIPoint position, PColor color)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    WritePixel(&bitmap, position, color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleDrawLine(
    PDisplayBitmap bitmap,
    PIRect clipRect,
    PIPoint position1,
    PIPoint position2,
    PColor color,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    DrawLine(&bitmap, clipRect, position1, position2, color, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillPolygon(
    std::span<const std::byte> payload,
    PDisplayBitmap bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    FillPolygon(&bitmap, clips, points, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangle(
    PDisplayBitmap bitmap,
    PIRect clipRect,
    PIPoint position1,
    PIPoint position2,
    PIPoint position3,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    FillTriangle(&bitmap, clipRect, position1, position2, position3, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangleFan(
    std::span<const std::byte> payload,
    PDisplayBitmap bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    for (const PIRect& clip : clips) {
        FillTriangleFan(&bitmap, clip, points, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangleStrip(
    std::span<const std::byte> payload,
    PDisplayBitmap bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    for (const PIRect& clip : clips) {
        FillTriangleStrip(&bitmap, clip, points, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillRect(PDisplayBitmap bitmap, PIRect rect)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    FillRect(&bitmap, rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleCopyRect(
    PDisplayBitmap destination,
    PDisplayBitmap source,
    PColor background,
    PColor foreground,
    PIRect sourceRect,
    PIPoint position,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayReadBitmap(source);
    ValidateDisplayWriteBitmap(destination);
    CopyRect(&destination, &source, background, foreground, sourceRect, position, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleScaleRect(
    PDisplayBitmap destination,
    PDisplayBitmap source,
    PColor background,
    PColor foreground,
    PIRect sourceOriginal,
    PIRect destinationOriginal,
    PRect sourceRect,
    PIRect destinationRect,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayReadBitmap(source);
    ValidateDisplayWriteBitmap(destination);
    ScaleRect(
        &destination,
        &source,
        background,
        foreground,
        sourceOriginal,
        destinationOriginal,
        sourceRect,
        destinationRect,
        mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillCircle(
    PDisplayBitmap bitmap,
    PIRect clipRect,
    PIPoint center,
    int32_t radius,
    PColor color,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    ValidateDisplayWriteBitmap(bitmap);
    FillCircle(&bitmap, clipRect, center, radius, color, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleWriteString(
    std::span<const std::byte> payload,
    PDisplayBitmap bitmap,
    PIPoint position,
    const PDisplayFontCache* font,
    size_t stringLength,
    size_t clipCount,
    size_t kerningCount,
    PColor background,
    PColor foreground,
    PPoint advance,
    PDrawingMode mode)
{
    HandleWriteStringWidth(
        payload,
        bitmap,
        position,
        font,
        stringLength,
        clipCount,
        kerningCount,
        background,
        foreground,
        advance,
        mode,
        std::numeric_limits<float>::infinity());
}

void KDisplayDriver::HandleWriteStringWidth(
    std::span<const std::byte> payload,
    PDisplayBitmap bitmap,
    PIPoint position,
    const PDisplayFontCache* font,
    size_t stringLength,
    size_t clipCount,
    size_t kerningCount,
    PColor background,
    PColor foreground,
    PPoint advance,
    PDrawingMode mode,
    float width)
{
    kassert(m_Mutex.IsLocked());
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const int32_t> kernings = ReadDisplayArray<int32_t>(payload, kerningCount);
    const std::span<const char> string = ReadDisplayArray<char>(payload, stringLength);
    ValidateDisplayPayloadEnd(payload);
    validate_user_read_pointer_trw(font);
    if (font->GetCharacterBucketCount() > std::numeric_limits<size_t>::max() / sizeof(*font->GetCharacters())) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    validate_user_read_pointer_trw(font->GetCharacters(), font->GetCharacterBucketCount() * sizeof(*font->GetCharacters()));
    const size_t bytesPerPixel = size_t(BitsPerPixel(bitmap.ColorSpace)) / 8;
    if (bitmap.Size.x < 0 || bitmap.Size.y < 0 || bytesPerPixel == 0
        || size_t(bitmap.Size.x) > std::numeric_limits<size_t>::max() / bytesPerPixel
        || bitmap.BytesPerLine < size_t(bitmap.Size.x) * bytesPerPixel
        || (bitmap.Size.y != 0 && bitmap.BytesPerLine > std::numeric_limits<size_t>::max() / size_t(bitmap.Size.y))) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (bitmap.VideoMemory && bitmap.ColorSpace != GetColorSpace()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (!bitmap.VideoMemory && (bitmap.ColorSpace == PEColorSpace::RGB15 || bitmap.ColorSpace == PEColorSpace::RGB16
        || bitmap.ColorSpace == PEColorSpace::RGB32 || bitmap.ColorSpace == PEColorSpace::RGBA32)
        && (uintptr_t(bitmap.Raster) % bytesPerPixel != 0 || bitmap.BytesPerLine % bytesPerPixel != 0)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    ValidateDisplayWriteBitmap(bitmap);
    WriteString(
        &bitmap,
        position,
        std::string_view(string.data(), string.size()),
        *font,
        kernings,
        clips,
        background,
        foreground,
        advance,
        mode,
        width);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::ExecuteCommands_pl(const void* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());
    if (uintptr_t(buffer) % DISPLAY_COMMAND_ALIGNMENT != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    while (length != 0)
    {
        if (length < sizeof(PDisplayCommandHeader)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        const PDisplayCommandHeader header = *reinterpret_cast<const PDisplayCommandHeader*>(data);
        if (header.Length < sizeof(header) || header.Length > length || header.Length % DISPLAY_COMMAND_ALIGNMENT != 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if (std::to_underlying(header.Code) >= std::to_underlying(PDisplayCommandID::Count)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        m_CommandDispatcher.Dispatch(
            std::to_underlying(header.Code),
            data + sizeof(header),
            header.Length - sizeof(header),
            nullptr,
            0);
        data += header.Length;
        length -= header.Length;
    }
}

} // namespace kernel
