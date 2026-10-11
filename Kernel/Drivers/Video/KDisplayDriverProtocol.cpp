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

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename Command>
static Command ReadDisplayCommand(const uint8_t* data, size_t length)
{
    if (length < sizeof(Command)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return *reinterpret_cast<const Command*>(data);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename Element>
static std::span<const Element> ReadDisplayArray(const uint8_t* data, size_t length, size_t& offset, size_t count)
{
    if (offset > length || count > (length - offset) / sizeof(Element)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const Element* result = reinterpret_cast<const Element*>(data + offset);
    offset += count * sizeof(Element);
    return {result, count};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void ValidateDisplayRecordEnd(size_t offset, size_t length)
{
    if (length - offset >= DISPLAY_COMMAND_ALIGNMENT) {
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

PDisplayBatchResult KDisplayDriver::HandleSubmit(const void* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());
    validate_user_read_pointer_trw(buffer, length);
    // A failed batch must also release all hardware references to its raster storage.
    PScopeExit complete([this] { WaitIdle(); });
    return ExecuteCommands_pl(buffer, length);
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

PDisplayBatchResult KDisplayDriver::ExecuteCommands_pl(const void* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());
    if (uintptr_t(buffer) % DISPLAY_COMMAND_ALIGNMENT != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PDisplayBatchResult result;
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
        switch (header.Code)
        {
            case PDisplayCommandID::SetFgColor:
            {
                PDisplaySetFgColorCommand command = ReadDisplayCommand<PDisplaySetFgColorCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                SetFgColor(command.Color);
                break;
            }
            case PDisplayCommandID::SetBgColor:
            {
                PDisplaySetBgColorCommand command = ReadDisplayCommand<PDisplaySetBgColorCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                SetBgColor(command.Color);
                break;
            }
            case PDisplayCommandID::SetColor:
            {
                PDisplaySetColorCommand command = ReadDisplayCommand<PDisplaySetColorCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                SetColor(command.Index, command.Color);
                break;
            }
            case PDisplayCommandID::PowerLost:
            {
                PDisplayPowerLostCommand command = ReadDisplayCommand<PDisplayPowerLostCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                PowerLost(command.HasPower);
                break;
            }
            case PDisplayCommandID::SetMouseCursorVisible:
            {
                PDisplaySetMouseCursorVisibleCommand command = ReadDisplayCommand<PDisplaySetMouseCursorVisibleCommand>(
                    data,
                    header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                SetMouseCursorVisible(command.Visible);
                break;
            }
            case PDisplayCommandID::SetMousePos:
            {
                PDisplaySetMousePosCommand command = ReadDisplayCommand<PDisplaySetMousePosCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                SetMousePos(command.Position);
                break;
            }
            case PDisplayCommandID::SetMouseCursorBitmap:
            {
                PDisplaySetMouseCursorBitmapCommand command = ReadDisplayCommand<PDisplaySetMouseCursorBitmapCommand>(
                    data,
                    header.Length);
                size_t offset = sizeof(command);
                const std::span<const PMouseCursorPixel> pixels = ReadDisplayArray<PMouseCursorPixel>(
                    data,
                    header.Length,
                    offset,
                    command.PixelCount);
                ValidateDisplayRecordEnd(offset, header.Length);
                const PMouseCursorBitmap cursor
                {
                    command.Width, command.Height, command.HotSpot, command.Color1, command.Color2, pixels
                };
                result.CursorAccepted = SetMouseCursorBitmap(cursor);
                break;
            }
            case PDisplayCommandID::WritePixel:
            {
                PDisplayWritePixelCommand command = ReadDisplayCommand<PDisplayWritePixelCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                WritePixel(&command.Bitmap, command.Position, command.Color);
                break;
            }
            case PDisplayCommandID::DrawLine:
            {
                PDisplayDrawLineCommand command = ReadDisplayCommand<PDisplayDrawLineCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                DrawLine(&command.Bitmap, command.ClipRect, command.Position1, command.Position2, command.Color, command.Mode);
                break;
            }
            case PDisplayCommandID::FillPolygon:
            {
                PDisplayFillPolygonCommand command = ReadDisplayCommand<PDisplayFillPolygonCommand>(data, header.Length);
                size_t offset = sizeof(command);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(data, header.Length, offset, command.ClipCount);
                const std::span<const PPoint> points = ReadDisplayArray<PPoint>(data, header.Length, offset, command.PointCount);
                ValidateDisplayRecordEnd(offset, header.Length);
                FillPolygon(&command.Bitmap, clips, points, command.Mode);
                break;
            }
            case PDisplayCommandID::FillTriangle:
            {
                PDisplayFillTriangleCommand command = ReadDisplayCommand<PDisplayFillTriangleCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                FillTriangle(
                    &command.Bitmap,
                    command.ClipRect,
                    command.Position1,
                    command.Position2,
                    command.Position3,
                    command.Mode);
                break;
            }
            case PDisplayCommandID::FillTriangleFan:
            {
                PDisplayFillTriangleFanCommand command = ReadDisplayCommand<PDisplayFillTriangleFanCommand>(data, header.Length);
                size_t offset = sizeof(command);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(data, header.Length, offset, command.ClipCount);
                const std::span<const PPoint> points = ReadDisplayArray<PPoint>(data, header.Length, offset, command.PointCount);
                ValidateDisplayRecordEnd(offset, header.Length);
                for (const PIRect& clip : clips) {
                    FillTriangleFan(&command.Bitmap, clip, points, command.Mode);
                }
                break;
            }
            case PDisplayCommandID::FillTriangleStrip:
            {
                PDisplayFillTriangleStripCommand command = ReadDisplayCommand<PDisplayFillTriangleStripCommand>(
                    data,
                    header.Length);
                size_t offset = sizeof(command);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                const std::span<const PIRect> clips = ReadDisplayArray<PIRect>(data, header.Length, offset, command.ClipCount);
                const std::span<const PPoint> points = ReadDisplayArray<PPoint>(data, header.Length, offset, command.PointCount);
                ValidateDisplayRecordEnd(offset, header.Length);
                for (const PIRect& clip : clips) {
                    FillTriangleStrip(&command.Bitmap, clip, points, command.Mode);
                }
                break;
            }
            case PDisplayCommandID::FillRect:
            {
                PDisplayFillRectCommand command = ReadDisplayCommand<PDisplayFillRectCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                FillRect(&command.Bitmap, command.Rect);
                break;
            }
            case PDisplayCommandID::CopyRect:
            {
                PDisplayCopyRectCommand command = ReadDisplayCommand<PDisplayCopyRectCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Source.VideoMemory) {
                    validate_user_read_pointer_trw(
                        command.Source.Raster,
                        command.Source.BytesPerLine * size_t(command.Source.Size.y));
                }
                if (!command.Destination.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Destination.Raster,
                        command.Destination.BytesPerLine * size_t(command.Destination.Size.y));
                }
                CopyRect(&command.Destination, &command.Source, command.Background, command.Foreground,
                    command.SourceRect, command.Position, command.Mode);
                break;
            }
            case PDisplayCommandID::ScaleRect:
            {
                PDisplayScaleRectCommand command = ReadDisplayCommand<PDisplayScaleRectCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Source.VideoMemory) {
                    validate_user_read_pointer_trw(
                        command.Source.Raster,
                        command.Source.BytesPerLine * size_t(command.Source.Size.y));
                }
                if (!command.Destination.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Destination.Raster,
                        command.Destination.BytesPerLine * size_t(command.Destination.Size.y));
                }
                // Coordinate conversion in the server can round a clipped edge slightly beyond the source.
                command.SourceRect &= PRect(command.SourceOriginal);
                ScaleRect(
                    &command.Destination,
                    &command.Source,
                    command.Background,
                    command.Foreground,
                    command.SourceOriginal,
                    command.DestinationOriginal,
                    command.SourceRect,
                    command.DestinationRect,
                    command.Mode);
                break;
            }
            case PDisplayCommandID::FillCircle:
            {
                PDisplayFillCircleCommand command = ReadDisplayCommand<PDisplayFillCircleCommand>(data, header.Length);
                ValidateDisplayRecordEnd(sizeof(command), header.Length);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                FillCircle(&command.Bitmap, command.ClipRect, command.Center, command.Radius, command.Color, command.Mode);
                break;
            }
            case PDisplayCommandID::WriteString:
            {
                PDisplayWriteStringCommand command = ReadDisplayCommand<PDisplayWriteStringCommand>(data, header.Length);
                size_t offset = sizeof(command);
                if (!command.Bitmap.VideoMemory) {
                    validate_user_write_pointer_trw(
                        command.Bitmap.Raster,
                        command.Bitmap.BytesPerLine * size_t(command.Bitmap.Size.y));
                }
                const std::span<const char> string = ReadDisplayArray<char>(data, header.Length, offset, command.StringLength);
                ValidateDisplayRecordEnd(offset, header.Length);
                result.TextPosition = WriteString(&command.Bitmap, command.Position, string.data(), string.size(),
                    command.ClipRect, command.Background, command.Foreground, command.FontID);
                break;
            }
            default:
                PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        data += header.Length;
        length -= header.Length;
    }
    return result;
}

} // namespace kernel
