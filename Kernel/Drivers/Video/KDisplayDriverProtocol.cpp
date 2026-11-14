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
    PScopeExit releaseResources([this]
    {
        m_Brush.Clear();
        m_SelectedBrushResources.clear();
        m_BrushLayerBuffer.clear();
        m_BrushResources.Clear();
        m_BitmapResources.Clear();
        m_ScreenBitmapDriverResourceID = DRIVER_INVALID_RESOURCE_ID;
        m_IsInitialized = false;
        m_IsOpen = false;
    });
    if (m_IsInitialized)
    {
        WaitIdle();
        Close();
    }
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
    if (m_ScreenBitmapDriverResourceID == DRIVER_INVALID_RESOURCE_ID) {
        m_ScreenBitmapDriverResourceID = m_BitmapResources.Add(ptr_new<KDisplayBitmapResource>(*GetScreenBitmap(), true));
    }
    return {*GetScreenBitmap(), m_ScreenBitmapDriverResourceID, GetScreenModeCount()};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleSubmit(const void* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());
    validate_user_read_pointer_trw(buffer, length);
    PScopeFail clearBrush([this]
    {
        m_Brush.Clear();
        m_SelectedBrushResources.clear();
        m_BrushLayerBuffer.clear();
    });
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
    if (accepted) {
        *m_BitmapResources.Get(m_ScreenBitmapDriverResourceID)->GetBitmap(true) = *GetScreenBitmap();
    }
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

PDriverResourceID KDisplayDriver::HandleCreateBitmap(PDisplayBitmap bitmap, bool writable)
{
    if (bitmap.VideoMemory || bitmap.Size.x <= 0 || bitmap.Size.y <= 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    switch (bitmap.ColorSpace)
    {
        case PEColorSpace::MONO1:
        case PEColorSpace::GRAY8:
        case PEColorSpace::CMAP8:
        case PEColorSpace::RGB15:
        case PEColorSpace::RGBA15:
        case PEColorSpace::RGB16:
        case PEColorSpace::RGB24:
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const size_t width = bitmap.Size.x;
    const size_t bitsPerPixel = BitsPerPixel(bitmap.ColorSpace);
    const size_t alignment = (bitsPerPixel == 16 || bitsPerPixel == 32) ? bitsPerPixel / 8 : 1;
    size_t minimumStride;
    if (bitsPerPixel == 1) {
        minimumStride = (width + 7) / 8;
    } else if (__builtin_mul_overflow(width, bitsPerPixel / 8, &minimumStride)) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    size_t length;
    if (bitmap.BytesPerLine < minimumStride || __builtin_mul_overflow(bitmap.BytesPerLine, bitmap.Size.y, &length)
        || uintptr_t(bitmap.Raster) % alignment != 0 || bitmap.BytesPerLine % alignment != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (writable) {
        validate_user_write_pointer_trw(bitmap.Raster, length);
    } else {
        validate_user_read_pointer_trw(bitmap.Raster, length);
    }
    return m_BitmapResources.Add(ptr_new<KDisplayBitmapResource>(bitmap, writable));
}

void KDisplayDriver::HandleDeleteBitmap(PDriverResourceID bitmap)
{
    if (bitmap == m_ScreenBitmapDriverResourceID) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_BitmapResources.Remove(bitmap);
}

PDriverResourceID KDisplayDriver::HandleCreateBrush(PDriverResourceID bitmapID, const PBrushDescription& description)
{
    return m_BrushResources.Add(ptr_new<KDisplayBrushResource>(ResolveBrushBitmap_pl(bitmapID, description), description));
}

void KDisplayDriver::HandleDeleteBrush(PDriverResourceID brush)
{
    m_BrushResources.Remove(brush);
}

void KDisplayDriver::HandleWritePixel(PDriverResourceID bitmapID, PIPoint position, PColor color)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    WritePixel(bitmap, position, color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleDrawLine(
    PDriverResourceID bitmapID,
    PIRect clipRect,
    PIPoint position1,
    PIPoint position2,
    PColor color,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    DrawLine(bitmap, clipRect, position1, position2, color, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillPolygon(
    std::span<const std::byte> payload,
    PDriverResourceID bitmapID,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    FillPolygon(bitmap, clips, points, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangle(
    PDriverResourceID bitmapID,
    PIRect clipRect,
    PIPoint position1,
    PIPoint position2,
    PIPoint position3,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    FillTriangle(bitmap, clipRect, position1, position2, position3, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangleFan(
    std::span<const std::byte> payload,
    PDriverResourceID bitmapID,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    for (const PIRect& clip : clips) {
        FillTriangleFan(bitmap, clip, points, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillTriangleStrip(
    std::span<const std::byte> payload,
    PDriverResourceID bitmapID,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(payload, clipCount);
    const std::span<const PPoint> points = ReadDisplayArray<PPoint>(payload, pointCount);
    ValidateDisplayPayloadEnd(payload);
    for (const PIRect& clip : clips) {
        FillTriangleStrip(bitmap, clip, points, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleFillRect(PDriverResourceID bitmapID, PIRect rect)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    FillRect(bitmap, rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleCopyRect(
    PDriverResourceID destinationID,
    PDriverResourceID sourceID,
    PColor background,
    PColor foreground,
    PIRect sourceRect,
    PIPoint position,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* source = m_BitmapResources.Get(sourceID)->GetBitmap(false);
    PDisplayBitmap* destination = m_BitmapResources.Get(destinationID)->GetBitmap(true);
    CopyRect(destination, source, background, foreground, sourceRect, position, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleScaleRect(
    PDriverResourceID destinationID,
    PDriverResourceID sourceID,
    PColor background,
    PColor foreground,
    PIRect sourceOriginal,
    PIRect destinationOriginal,
    PRect sourceRect,
    PIRect destinationRect,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* source = m_BitmapResources.Get(sourceID)->GetBitmap(false);
    PDisplayBitmap* destination = m_BitmapResources.Get(destinationID)->GetBitmap(true);
    ScaleRect(
        destination,
        source,
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
    PDriverResourceID bitmapID,
    PIRect clipRect,
    PIPoint center,
    int32_t radius,
    PColor color,
    PDrawingMode mode)
{
    kassert(m_Mutex.IsLocked());
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    FillCircle(bitmap, clipRect, center, radius, color, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::HandleWriteString(
    std::span<const std::byte> payload,
    PDriverResourceID bitmapID,
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
        bitmapID,
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
    PDriverResourceID bitmapID,
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
    PDisplayBitmap* bitmap = m_BitmapResources.Get(bitmapID)->GetBitmap(true);
    WriteString(
        bitmap,
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

void KDisplayDriver::HandleSetBrush(std::span<const std::byte> payload, size_t layerCount)
{
    const std::span<const PDisplayBrushBinding> bindings = ReadDisplayArray<PDisplayBrushBinding>(payload, layerCount);
    ValidateDisplayPayloadEnd(payload);
    m_Brush.Clear();
    m_SelectedBrushResources.resize(layerCount);
    m_BrushLayerBuffer.resize(layerCount);
    for (size_t index = 0; index < layerCount; ++index)
    {
        const PDisplayBrushBinding& binding = bindings[index];
        PDisplayBrushLayer& layer = m_BrushLayerBuffer[index];
        layer = {};
        layer.Bounds = PIRect(binding.Origin, binding.Origin);
        layer.ReferenceSize = binding.ReferenceSize;
        if (binding.ReferenceSize.x < 0 || binding.ReferenceSize.y < 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if (binding.DriverResourceID == DRIVER_INVALID_RESOURCE_ID)
        {
            m_SelectedBrushResources[index] = nullptr;
            layer.Description.Color = binding.Color;
        }
        else
        {
            const Ptr<KDisplayBrushResource>& brush = m_BrushResources.Get(binding.DriverResourceID);
            m_SelectedBrushResources[index] = brush;
            RefreshBrushLayer_pl(index);
        }
    }
    m_Brush.SetLayers(m_BrushLayerBuffer);
}

void KDisplayDriver::HandleFillBrushRect(PDriverResourceID bitmap, PIRect bounds)
{
    FillBrushRect(m_BitmapResources.Get(bitmap)->GetBitmap(true), bounds);
}

// Updates are executed between the surrounding drawing commands. Rebuild only
// selected samplers; raster access was validated when the bitmap was registered.
void KDisplayDriver::HandleUpdateBrush(PDriverResourceID brushID, PDriverResourceID bitmapID, const PBrushDescription& description)
{
    const Ptr<KDisplayBrushResource>& brush = m_BrushResources.Get(brushID);
    Ptr<KDisplayBitmapResource> bitmap = ResolveBrushBitmap_pl(bitmapID, description);
    brush->SetProperties(std::move(bitmap), description);
    RefreshSelectedBrushLayers_pl(brush);
}

// A color needs no bitmap lookup or metadata validation. Apply it at the same
// queue position as a full update and refresh every selected use of the brush.
void KDisplayDriver::HandleSetBrushColor(PDriverResourceID brushID, PColor color)
{
    const Ptr<KDisplayBrushResource>& brush = m_BrushResources.Get(brushID);
    brush->SetColor(color);
    RefreshSelectedBrushLayers_pl(brush);
}

Ptr<KDisplayBitmapResource> KDisplayDriver::ResolveBrushBitmap_pl(PDriverResourceID bitmapID, const PBrushDescription& description) const
{
    kassert(m_Mutex.IsLocked());
    Ptr<KDisplayBitmapResource> resource;
    if (bitmapID != DRIVER_INVALID_RESOURCE_ID) {
        resource = m_BitmapResources.Get(bitmapID);
    }
    if (!PBrushUtils::IsValidResource(description, (resource != nullptr) ? resource->GetBitmap(false) : nullptr)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return resource;
}

void KDisplayDriver::RefreshSelectedBrushLayers_pl(const Ptr<KDisplayBrushResource>& brush)
{
    kassert(m_Mutex.IsLocked());
    bool selected = false;
    for (size_t index = 0; index < m_SelectedBrushResources.size(); ++index)
    {
        if (m_SelectedBrushResources[index] == brush)
        {
            RefreshBrushLayer_pl(index);
            selected = true;
        }
    }
    if (selected) {
        m_Brush.SetLayers(m_BrushLayerBuffer);
    }
}

void KDisplayDriver::RefreshBrushLayer_pl(size_t index)
{
    kassert(m_Mutex.IsLocked());
    const Ptr<KDisplayBrushResource>& brush = m_SelectedBrushResources[index];
    PDisplayBrushLayer& layer = m_BrushLayerBuffer[index];
    layer.Description = brush->GetDescription();
    layer.Bitmap = {};
    if (brush->GetBitmap() != nullptr) {
        layer.Bitmap = *brush->GetBitmap()->GetBitmap(false);
    }
    if (layer.Description.Style == PBrushStyle::TexturePattern)
    {
        const PIPoint size = PBrushUtils::TileSize(layer.Description, layer.ReferenceSize);
        layer.Bounds = PBrushUtils::TileBounds(layer.Bounds.TopLeft(), size);
    }
}

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
