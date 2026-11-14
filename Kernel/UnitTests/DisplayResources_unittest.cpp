// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstring>
#include <limits>
#include <gtest/gtest.h>
#include <Kernel/Drivers/Video/KDisplayDriver.h>

namespace kernel
{

class DisplayResourceTestDriver : public KDisplayDriver
{
public:
    DisplayResourceTestDriver()
    {
        m_Screen = {PEColorSpace::RGB32, PIPoint(4, 4), 4 * sizeof(uint32_t), reinterpret_cast<uint8_t*>(m_Pixels.data()), false};
    }

    void WaitIdle() override {}
    bool Open() override { return true; }
    void Close() override {}
    void PowerLost(bool hasPower) override {}
    PDisplayBitmap* GetScreenBitmap() override { return &m_Screen; }
    int GetScreenModeCount() override { return 1; }
    bool GetScreenModeDesc(size_t index, PScreenMode& mode) override { return false; }
    bool SetScreenMode(const PIPoint& size, PEColorSpace format, float rate) override { return false; }
    PIPoint GetResolution() override { return m_Screen.Size; }
    int GetBytesPerLine() override { return int(m_Screen.BytesPerLine); }
    PEColorSpace GetColorSpace() override { return m_Screen.ColorSpace; }
    void SetColor(size_t index, PColor color) override {}
    bool SetMouseCursorBitmap(const PMouseCursorBitmap& cursor) override { return false; }
    void SetMouseCursorVisible(bool visible) override {}
    void SetMousePos(PIPoint position) override {}

    template<typename TInvoker, typename... TArgs>
    decltype(auto) Request(TArgs&&... args)
    {
        using Definition = typename TInvoker::Definition;
        PRPCInvoker<false, typename Definition::Signature> invoke(
            [this](const void* input, size_t inputSize, void* output, size_t outputSize)
            {
                DeviceControl(nullptr, Definition::HandlerID, input, inputSize, output, outputSize);
            });
        return invoke(std::forward<TArgs>(args)...);
    }

    template<typename TInvoker, typename... TArgs>
    void Command(std::span<const std::byte> payload, TArgs&&... args)
    {
        const size_t arguments = TInvoker::GetSize(args...);
        const size_t size = sizeof(PDisplayCommandHeader) + arguments + payload.size();
        const size_t length = (size + DISPLAY_COMMAND_ALIGNMENT - 1) & ~(DISPLAY_COMMAND_ALIGNMENT - 1);
        const size_t offset = m_Commands.size();
        m_Commands.resize(offset + length / sizeof(uint64_t));
        PDisplayCommandHeader* header = reinterpret_cast<PDisplayCommandHeader*>(m_Commands.data() + offset);
        std::construct_at(header, PDisplayCommandHeader{PDisplayCommandID(TInvoker::Definition::HandlerID), uint32_t(length)});
        uint8_t* destination = reinterpret_cast<uint8_t*>(header + 1);
        ASSERT_GE(TInvoker::Serializer::WriteArg(destination, arguments, std::forward<TArgs>(args)...), 0);
        if (!payload.empty()) {
            std::memcpy(destination + arguments, payload.data(), payload.size());
        }
    }

    void SelectBrush(std::span<const PDisplayBrushBinding> bindings)
    {
        Command<decltype(PDisplayCommands::SetBrush)>(std::as_bytes(bindings), bindings.size());
    }

    void Submit()
    {
        PScopeExit clear([this] { m_Commands.clear(); });
        Request<decltype(PDisplayDeviceControl::Submit)>(m_Commands.data(), m_Commands.size() * sizeof(uint64_t));
    }

    const std::array<uint32_t, 16>& GetPixels() const noexcept { return m_Pixels; }

private:
    std::array<uint32_t, 16> m_Pixels{};
    PDisplayBitmap m_Screen;
    std::vector<uint64_t> m_Commands;
};

class DisplayResourceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Driver = ptr_new<DisplayResourceTestDriver>();
        m_Info = m_Driver->Request<decltype(PDisplayDeviceControl::Initialize)>();
        m_Source = {PEColorSpace::RGB32, PIPoint(2, 2), 2 * sizeof(uint32_t), reinterpret_cast<uint8_t*>(m_Pixels.data()), false};
        m_Pixels.fill(PColor(40, 80, 120).GetColor32());
    }

    void TearDown() override
    {
        m_Driver->CloseFile(nullptr, nullptr);
    }

    void Fill(PDriverResourceID bitmap)
    {
        m_Driver->Command<decltype(PDisplayCommands::FillRect)>({}, bitmap, PIRect(0, 0, 2, 2));
        m_Driver->Submit();
    }

    Ptr<DisplayResourceTestDriver> m_Driver;
    std::array<uint32_t, 4> m_Pixels;
    PDisplayInfo m_Info;
    PDisplayBitmap m_Source;
};

TEST_F(DisplayResourceTest, CopiesRegisteredPropertiesAndRejectsStaleIDs)
{
    const PDriverResourceID first = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, true);
    m_Source.Raster = nullptr;
    m_Driver->Command<decltype(PDisplayCommands::SetFgColor)>({}, PColor(90, 100, 110));
    Fill(first);
    EXPECT_EQ(m_Pixels[0], PColor(90, 100, 110).GetColor32());
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(first);
    m_Source.Raster = reinterpret_cast<uint8_t*>(m_Pixels.data());
    const PDriverResourceID second = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, true);
    EXPECT_NE(first, second);
    EXPECT_THROW(Fill(first), std::system_error);
    EXPECT_NO_THROW(Fill(second));
}

TEST_F(DisplayResourceTest, ReadOnlySourcesCannotBeUsedAsDestinations)
{
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    m_Driver->Command<decltype(PDisplayCommands::CopyRect)>(
        {}, m_Info.ScreenBitmapDriverResourceID, source, PColor(0), PColor(0), PIRect(0, 0, 2, 2), PIPoint(0, 0), PDrawingMode::Copy);
    m_Driver->Submit();
    EXPECT_EQ(m_Driver->GetPixels()[0], m_Pixels[0]);
    EXPECT_THROW(Fill(source), std::system_error);
    EXPECT_NO_THROW(Fill(m_Info.ScreenBitmapDriverResourceID));
}

TEST_F(DisplayResourceTest, ValidatesRasterAndBrushPropertiesAtCreation)
{
    PDisplayBitmap invalid = m_Source;
    invalid.Raster = nullptr;
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(invalid, false), std::system_error);
    invalid = m_Source;
    invalid.BytesPerLine = 1;
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(invalid, false), std::system_error);
    invalid = m_Source;
    invalid.VideoMemory = true;
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(invalid, false), std::system_error);
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 3, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description), std::system_error);
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    EXPECT_NO_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description));
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description), std::system_error);
    description.Type = PBrushType::Solid;
    description.Scale.x = 0.0f;
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description), std::system_error);
    description.Scale.x = 1.0f;
    description.Style = PBrushStyle(0xff);
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description), std::system_error);
}

TEST_F(DisplayResourceTest, TextureSamplingUsesImageMetadataRegardlessOfBrushType)
{
    m_Pixels = {PColor(10, 20, 30).GetColor32(), PColor(40, 50, 60).GetColor32(),
        PColor(70, 80, 90).GetColor32(), PColor(100, 110, 120).GetColor32()};
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(1, 1, 2, 2);
    description.Image.LayoutSize = PPoint(1, 1);
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    for (PBrushType type : {PBrushType::Solid, PBrushType::Bitmap, PBrushType::Image, PBrushType(0xff)})
    {
        description.Type = type;
        const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description);
        const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
        m_Driver->SelectBrush(std::span(&binding, 1));
        m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
        m_Driver->Submit();
        for (size_t row = 0; row < 4; ++row)
        {
            for (size_t column = 0; column < 4; ++column)
            {
                const size_t sourceIndex = ((row == 0) ? 0 : 2) + ((column == 0) ? 0 : 1);
                EXPECT_EQ(m_Driver->GetPixels()[row * 4 + column], m_Pixels[sourceIndex]);
            }
        }
    }
}

TEST_F(DisplayResourceTest, TextureOriginsBeyondLayoutMaximumPreserveTiling)
{
    m_Pixels = {PColor(10, 20, 30).GetColor32(), PColor(40, 50, 60).GetColor32(),
        PColor(70, 80, 90).GetColor32(), PColor(100, 110, 120).GetColor32()};
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(2, 2);
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(32000001, -64000001), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    for (size_t row = 0; row < 4; ++row)
    {
        for (size_t column = 0; column < 4; ++column) {
            EXPECT_EQ(m_Driver->GetPixels()[row * 4 + column], m_Pixels[((row + 1) % 2) * 2 + (column + 1) % 2]);
        }
    }
}

TEST_F(DisplayResourceTest, NoneAndSolidStylesDoNotRequireTextureProperties)
{
    PBrushDescription description;
    description.Type = PBrushType(0xff);
    description.Color = PColor(30, 60, 90);
    description.Scale = PPoint(std::numeric_limits<float>::quiet_NaN(), 0.0f);
    const PColor background(100, 110, 120);
    for (PBrushStyle style : {PBrushStyle::None, PBrushStyle::SolidPattern})
    {
        description.Style = style;
        const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
        const PDisplayBrushBinding layers[] =
        {
            {DRIVER_INVALID_RESOURCE_ID, background, PIPoint(0, 0), PIPoint(4, 4)},
            {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)}
        };
        m_Driver->SelectBrush(layers);
        m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
        m_Driver->Submit();
        const PColor expected = (style == PBrushStyle::None) ? background : description.Color;
        for (uint32_t pixel : m_Driver->GetPixels()) {
            EXPECT_EQ(pixel, expected.GetColor32());
        }
    }
}

TEST_F(DisplayResourceTest, CompositionPersistsAcrossBatchesAndRetainsItsResources)
{
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Submit();
    // Removing IDs does not destroy resources still referenced by a composition.
    // The owner continues to retain the raster until all such references end.
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(source);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBrush)>(brush);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 2));
    m_Driver->Submit();
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 4));
    m_Driver->Submit();
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, m_Pixels[0]);
    }
    m_Driver->SelectBrush({});
    m_Driver->Submit();
    m_Driver->SelectBrush(std::span(&binding, 1));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
}

TEST_F(DisplayResourceTest, TransparentBrushCompositionMatchesResolvedSampling)
{
    m_Source.ColorSpace = PEColorSpace::RGBA32;
    m_Pixels.fill(PColor(120, 40, 10, 128).GetColor32());
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    description.Image.IsOpaque = false;
    description.Image.OpaqueSlices = 0;
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(source, description);
    const PColor background(20, 60, 100);
    const PDisplayBrushBinding layers[] =
    {
        {DRIVER_INVALID_RESOURCE_ID, background, {}, {}},
        {brush, PColor(0), PIPoint(-1, -1), PIPoint(4, 4)}
    };
    m_Driver->SelectBrush(layers);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    const PColor expected = PBrushUtils::Composite(PColor(120, 40, 10, 128), background);
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, expected.GetColor32());
    }
}

TEST_F(DisplayResourceTest, BrushColorUpdatesAreOrderedWithDrawingAndPersist)
{
    PBrushDescription description;
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, PColor(200, 40, 20));
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, PColor(20, 180, 40));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 2));
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, PColor(40, 20, 160));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 3));
    m_Driver->Submit();
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], PColor(200, 40, 20).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], PColor(20, 180, 40).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], PColor(40, 20, 160).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], PColor(40, 20, 160).GetColor32());
    }
}

TEST_F(DisplayResourceTest, BrushColorChangesRefreshEverySelectedOccurrence)
{
    PBrushDescription description;
    description.Color = PColor(120, 40, 20);
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
    const PColor background(20, 60, 100);
    const PDisplayBrushBinding layers[] =
    {
        {DRIVER_INVALID_RESOURCE_ID, background, {}, {}},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)}
    };
    m_Driver->SelectBrush(layers);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    const PColor translucent(120, 40, 20, 128);
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, translucent);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 2));
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, PColor(120, 40, 20, 0));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 3));
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, description.Color);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const PColor expected = PBrushUtils::Composite(translucent, PBrushUtils::Composite(translucent, background));
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], description.Color.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], expected.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], background.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], description.Color.GetColor32());
    }
}

TEST_F(DisplayResourceTest, BitmapReplacementRefreshesEverySelectedOccurrence)
{
    const PDriverResourceID first = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    std::array<uint32_t, 4> replacement;
    replacement.fill(PColor(160, 90, 30, 128).GetColor32());
    PDisplayBitmap secondBitmap = m_Source;
    secondBitmap.ColorSpace = PEColorSpace::RGBA32;
    secondBitmap.Raster = reinterpret_cast<uint8_t*>(replacement.data());
    const PDriverResourceID second = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(secondBitmap, false);
    PBrushDescription description;
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(first, description);
    const PDisplayBrushBinding layers[] =
    {
        {DRIVER_INVALID_RESOURCE_ID, PColor(10, 20, 30), {}, {}},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)}
    };
    m_Driver->SelectBrush(layers);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    description.Image.IsOpaque = false;
    description.Image.OpaqueSlices = 0;
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, second, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 3));
    m_Driver->Submit();
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(first);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(second);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const PColor sample(160, 90, 30, 128);
    const PColor expected = PBrushUtils::Composite(sample, PBrushUtils::Composite(sample, PColor(10, 20, 30)));
    for (size_t index = 0; index < m_Driver->GetPixels().size(); ++index) {
        EXPECT_EQ(m_Driver->GetPixels()[index], (index < 4) ? m_Pixels[0] : expected.GetColor32());
    }
}

TEST_F(DisplayResourceTest, UnselectedUpdatesAndTypeChangesUseTheSameHandle)
{
    PBrushDescription description;
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(description.Image.SourceRect.Size());
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 2));
    description = PBrushDescription();
    description.Color = PColor(60, 100, 140);
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, brush, description.Color);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 4));
    m_Driver->Submit();
    for (size_t index = 0; index < m_Driver->GetPixels().size(); ++index) {
        EXPECT_EQ(m_Driver->GetPixels()[index], (index < 8) ? m_Pixels[0] : description.Color.GetColor32());
    }
}

TEST_F(DisplayResourceTest, InvalidUpdatesLeaveTheResourcePropertiesIntact)
{
    PBrushDescription original;
    original.Color = PColor(30, 60, 90);
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, original);
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription invalid;
    invalid.Type = PBrushType::Bitmap;
    invalid.Style = PBrushStyle::TexturePattern;
    invalid.Image.SourceRect = PIRect(0, 0, 3, 2);
    invalid.Image.SliceRect = PRect(invalid.Image.SourceRect);
    invalid.Image.LayoutSize = PPoint(invalid.Image.SourceRect.Size());
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, invalid);
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    invalid = original;
    invalid.Style = PBrushStyle(0xff);
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, DRIVER_INVALID_RESOURCE_ID, invalid);
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, original.Color.GetColor32());
    }
}

TEST_F(DisplayResourceTest, BrushColorUpdatesRejectStaleIDs)
{
    PBrushDescription description;
    description.Color = PColor(30, 60, 90);
    const PDriverResourceID stale = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBrush)>(stale);
    const PDriverResourceID current = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(DRIVER_INVALID_RESOURCE_ID, description);
    EXPECT_NE(stale, current);
    m_Driver->Command<decltype(PDisplayCommands::SetBrushColor)>({}, stale, PColor(160, 80, 40));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    const PDisplayBrushBinding binding{current, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, description.Color.GetColor32());
    }
}

TEST_F(DisplayResourceTest, QueuedLayoutScaleAndStyleChangesRefreshTheSelectedBrush)
{
    m_Pixels = {PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32(),
        PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(0, 0, 2, 2);
    description.Image.LayoutSize = PPoint(2, 2);
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(bitmap, description);
    const PColor background(10, 20, 30);
    const PDisplayBrushBinding layers[] =
    {
        {DRIVER_INVALID_RESOURCE_ID, background, {}, {}},
        {brush, PColor(0), PIPoint(1, 0), PIPoint(2, 2)}
    };
    m_Driver->SelectBrush(layers);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    description.Image.LayoutSize.x = 4.0f;
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 2));
    description.Scale.x = 0.5f;
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 3));
    description.Style = PBrushStyle::None;
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Submit();
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const std::array<PColor, 4> stretched = {PColor(200, 100, 40), PColor(0, 0, 0), PColor(50, 25, 10), PColor(150, 75, 30)};
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], m_Pixels[(column + 1) % 2]);
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], stretched[column].GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], m_Pixels[(column + 1) % 2]);
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], background.GetColor32());
    }
}

TEST_F(DisplayResourceTest, RelativeLayoutUpdatesRetainTheSelectedViewsSize)
{
    m_Pixels = {PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32(),
        PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(2, 2);
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(bitmap, description);
    const PDisplayBrushBinding binding{brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)};
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    description.Image.LayoutSize = PPoint(1, 1);
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 2));
    description.Scale = PPoint(2, 3);
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 2, 4, 3));
    description.Image.LayoutSize = PPoint(0.5f, 0.5f);
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Submit();
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const std::array<PColor, 4> stretched = {PColor(0, 0, 0), PColor(50, 25, 10), PColor(150, 75, 30), PColor(200, 100, 40)};
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], m_Pixels[column % 2]);
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], stretched[column].GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], stretched[column].GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], m_Pixels[column % 2]);
    }
}

TEST_F(DisplayResourceTest, SharedRelativeBrushKeepsEachSelectedOwnersReferenceSize)
{
    m_Source.ColorSpace = PEColorSpace::RGBA32;
    m_Pixels = {PColor(0, 0, 0, 128).GetColor32(), PColor(200, 100, 40, 128).GetColor32(),
        PColor(0, 0, 0, 128).GetColor32(), PColor(200, 100, 40, 128).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.SliceRect = PRect(description.Image.SourceRect);
    description.Image.LayoutSize = PPoint(1, 1);
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    description.Image.IsOpaque = false;
    description.Image.OpaqueSlices = 0;
    const PDriverResourceID brush = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBrush)>(bitmap, description);
    const PColor background(20, 40, 60);
    const PDisplayBrushBinding layers[] =
    {
        {DRIVER_INVALID_RESOURCE_ID, background, {}, {}},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(4, 4)},
        {brush, PColor(0), PIPoint(0, 0), PIPoint(2, 4)}
    };
    m_Driver->SelectBrush(layers);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 0, 4, 1));
    description.Scale = PPoint(3, 2);
    m_Driver->Command<decltype(PDisplayCommands::UpdateBrush)>({}, brush, bitmap, description);
    m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, PIRect(0, 1, 4, 2));
    m_Driver->Submit();
    const std::array<PColor, 4> stretched =
    {
        PColor(0, 0, 0, 128), PColor(50, 25, 10, 128), PColor(150, 75, 30, 128), PColor(200, 100, 40, 128)
    };
    for (size_t column = 0; column < 4; ++column)
    {
        const PColor expected = PBrushUtils::Composite(
            PColor::FromRGB32A(m_Pixels[column % 2]),
            PBrushUtils::Composite(stretched[column], background));
        EXPECT_EQ(m_Driver->GetPixels()[column], expected.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], expected.GetColor32());
    }
}

TEST_F(DisplayResourceTest, ClosingTheDeviceRetiresTheRegistrationTable)
{
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, true);
    m_Driver->CloseFile(nullptr, nullptr);
    m_Info = m_Driver->Request<decltype(PDisplayDeviceControl::Initialize)>();
    EXPECT_THROW(Fill(source), std::system_error);
    EXPECT_NO_THROW(Fill(m_Info.ScreenBitmapDriverResourceID));
}

} // namespace kernel
