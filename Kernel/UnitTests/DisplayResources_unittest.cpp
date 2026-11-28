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
        m_Image.SourceRect = PIRect(0, 0, 2, 2);
        m_Image.SliceRect = PRect(m_Image.SourceRect);
        m_Image.LayoutSize = PPoint(m_Image.SourceRect.Size());
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

    void QueueBrushFill(const PIRect& bounds)
    {
        m_Driver->Command<decltype(PDisplayCommands::FillBrushRect)>({}, m_Info.ScreenBitmapDriverResourceID, bounds);
    }

    static PDisplayBrushBinding MakeImageBinding(PDriverResourceID image, PIPoint origin = PIPoint(0, 0), PIPoint referenceSize = PIPoint(4, 4))
    {
        PDisplayBrushBinding binding;
        binding.Data.ImageID = image;
        binding.Data.Type = PBrushType::Image;
        binding.Data.Style = PBrushStyle::TexturePattern;
        binding.Origin = origin;
        binding.ReferenceSize = referenceSize;
        return binding;
    }

    static PDisplayBrushBinding MakeSolidBinding(PColor color)
    {
        PDisplayBrushBinding binding;
        binding.Data.Color = color;
        return binding;
    }

    Ptr<DisplayResourceTestDriver> m_Driver;
    std::array<uint32_t, 4> m_Pixels;
    PDisplayInfo m_Info;
    PDisplayBitmap m_Source;
    PImageDescription m_Image;
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

TEST_F(DisplayResourceTest, ValidatesRasterAndImagePropertiesAtCreation)
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
    PImageDescription image = m_Image;
    image.SourceRect.right = 3;
    image.SliceRect = PRect(image.SourceRect);
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, image), std::system_error);
    EXPECT_NO_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, m_Image));
    EXPECT_THROW(m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(DRIVER_INVALID_RESOURCE_ID, m_Image), std::system_error);
}

TEST_F(DisplayResourceTest, TextureSamplingUsesImageMetadataRegardlessOfBrushType)
{
    m_Pixels = {PColor(10, 20, 30).GetColor32(), PColor(40, 50, 60).GetColor32(),
        PColor(70, 80, 90).GetColor32(), PColor(100, 110, 120).GetColor32()};
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    m_Image.SliceRect = PRect(1, 1, 2, 2);
    m_Image.LayoutSize = PPoint(1, 1);
    m_Image.LayoutMode = PImageLayoutMode::ViewRelative;
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, m_Image);
    for (PBrushType type : {PBrushType::Solid, PBrushType::Bitmap, PBrushType::Image, PBrushType(0xff)})
    {
        PDisplayBrushBinding binding = MakeImageBinding(image);
        binding.Data.Type = type;
        m_Driver->SelectBrush(std::span(&binding, 1));
        QueueBrushFill(PIRect(0, 0, 4, 4));
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
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, m_Image);
    const PDisplayBrushBinding binding = MakeImageBinding(image, PIPoint(32000001, -64000001));
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 0, 4, 4));
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
    const PColor background(100, 110, 120);
    for (PBrushStyle style : {PBrushStyle::None, PBrushStyle::SolidPattern})
    {
        PDisplayBrushBinding binding = MakeSolidBinding(PColor(30, 60, 90));
        binding.Data.Type = PBrushType(0xff);
        binding.Data.Style = style;
        binding.Data.Scale = PPoint(std::numeric_limits<float>::quiet_NaN(), 0.0f);
        binding.Data.ImageID = std::numeric_limits<PImageID>::max();
        const std::array layers = {MakeSolidBinding(background), binding};
        m_Driver->SelectBrush(layers);
        QueueBrushFill(PIRect(0, 0, 4, 4));
        m_Driver->Submit();
        const PColor expected = (style == PBrushStyle::None) ? background : binding.Data.Color;
        for (uint32_t pixel : m_Driver->GetPixels()) {
            EXPECT_EQ(pixel, expected.GetColor32());
        }
    }
}

TEST_F(DisplayResourceTest, CompositionPersistsAcrossBatchesAndRetainsItsResources)
{
    const PDriverResourceID source = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, m_Image);
    const PDisplayBrushBinding binding = MakeImageBinding(image);
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Submit();
    // Removing IDs does not destroy resources still referenced by a composition.
    // The owner continues to retain the raster until all such references end.
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(source);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteImage)>(image);
    QueueBrushFill(PIRect(0, 0, 4, 2));
    m_Driver->Submit();
    QueueBrushFill(PIRect(0, 2, 4, 4));
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
    m_Image.IsOpaque = false;
    m_Image.OpaqueSlices = 0;
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(source, m_Image);
    const PColor background(20, 60, 100);
    const std::array layers = {MakeSolidBinding(background), MakeImageBinding(image, PIPoint(-1, -1))};
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    const PColor expected = PBrushUtils::Composite(PColor(120, 40, 10, 128), background);
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, expected.GetColor32());
    }
}

TEST_F(DisplayResourceTest, BrushValueSelectionsAreOrderedWithDrawingAndPersist)
{
    PDisplayBrushBinding binding = MakeSolidBinding(PColor(200, 40, 20));
    m_Driver->SelectBrush(std::span(&binding, 1));
    binding.Data.Color = PColor(20, 180, 40);
    QueueBrushFill(PIRect(0, 0, 4, 1));
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 1, 4, 2));
    binding.Data.Color = PColor(40, 20, 160);
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 2, 4, 3));
    m_Driver->Submit();
    binding.Data.Color = PColor(255, 255, 255);
    QueueBrushFill(PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], PColor(200, 40, 20).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], PColor(20, 180, 40).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], PColor(40, 20, 160).GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], PColor(40, 20, 160).GetColor32());
    }
}

TEST_F(DisplayResourceTest, EachSelectedOccurrenceRetainsItsOwnBrushValue)
{
    const PColor background(20, 60, 100);
    const PColor initial(120, 40, 20);
    const PColor translucent(120, 40, 20, 128);
    std::array layers = {MakeSolidBinding(background), MakeSolidBinding(initial), MakeSolidBinding(initial)};
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 0, 4, 1));
    layers[1].Data.Color = translucent;
    layers[2].Data.Color = translucent;
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 1, 4, 2));
    layers[1].Data.Color = PColor(120, 40, 20, 0);
    layers[2].Data.Color = PColor(120, 40, 20, 0);
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 2, 4, 3));
    layers[1].Data.Color = initial;
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const PColor expected = PBrushUtils::Composite(translucent, PBrushUtils::Composite(translucent, background));
    for (size_t column = 0; column < 4; ++column)
    {
        EXPECT_EQ(m_Driver->GetPixels()[column], initial.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[4 + column], expected.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[8 + column], background.GetColor32());
        EXPECT_EQ(m_Driver->GetPixels()[12 + column], initial.GetColor32());
    }
}

TEST_F(DisplayResourceTest, ImageSelectionsRetainOldAndNewRastersThroughSubmission)
{
    const PDriverResourceID firstBitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID firstImage = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(firstBitmap, m_Image);
    std::array<uint32_t, 4> translucent;
    translucent.fill(PColor(160, 90, 30, 128).GetColor32());
    PDisplayBitmap secondSource = m_Source;
    secondSource.Raster = reinterpret_cast<uint8_t*>(translucent.data());
    secondSource.ColorSpace = PEColorSpace::RGBA32;
    const PDriverResourceID secondBitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(secondSource, false);
    PImageDescription secondDescription = m_Image;
    secondDescription.IsOpaque = false;
    secondDescription.OpaqueSlices = 0;
    const PDriverResourceID secondImage = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(secondBitmap, secondDescription);
    std::array layers = {MakeSolidBinding(PColor(10, 20, 30)), MakeImageBinding(firstImage), MakeImageBinding(firstImage)};
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 0, 4, 1));
    layers[1].Data.ImageID = secondImage;
    layers[2].Data.ImageID = secondImage;
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 1, 4, 3));
    m_Driver->Submit();
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(firstBitmap);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteBitmap)>(secondBitmap);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteImage)>(firstImage);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteImage)>(secondImage);
    QueueBrushFill(PIRect(0, 3, 4, 4));
    m_Driver->Submit();
    const PColor sample(160, 90, 30, 128);
    const PColor expected = PBrushUtils::Composite(sample, PBrushUtils::Composite(sample, PColor(10, 20, 30)));
    for (size_t index = 0; index < m_Driver->GetPixels().size(); ++index) {
        EXPECT_EQ(m_Driver->GetPixels()[index], (index < 4) ? m_Pixels[0] : expected.GetColor32());
    }
    m_Driver->SelectBrush({});
    m_Driver->Submit();
}

TEST_F(DisplayResourceTest, SelectingAColorReplacesTheTexture)
{
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    PDisplayBrushBinding binding = MakeImageBinding(image);
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 0, 4, 2));
    binding.Data.SetColor(PColor(60, 100, 140));
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 2, 4, 4));
    m_Driver->Submit();
    for (size_t index = 0; index < m_Driver->GetPixels().size(); ++index) {
        EXPECT_EQ(m_Driver->GetPixels()[index], (index < 8) ? m_Pixels[0] : binding.Data.Color.GetColor32());
    }
}

TEST_F(DisplayResourceTest, InvalidSelectionsDoNotChangeRegisteredImageDefinitions)
{
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    PDisplayBrushBinding binding = MakeImageBinding(image);
    binding.Data.Scale.x = 0.0f;
    m_Driver->SelectBrush(std::span(&binding, 1));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    binding.Data.Scale.x = std::numeric_limits<float>::quiet_NaN();
    m_Driver->SelectBrush(std::span(&binding, 1));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    binding.Data.Scale.x = 1.0f;
    binding.Data.Style = PBrushStyle(0xff);
    m_Driver->SelectBrush(std::span(&binding, 1));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    binding.Data.Style = PBrushStyle::TexturePattern;
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, m_Pixels[0]);
    }
}

TEST_F(DisplayResourceTest, ImageSelectionsRejectStaleIDs)
{
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID stale = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    m_Driver->Request<decltype(PDisplayDeviceControl::DeleteImage)>(stale);
    const PDriverResourceID current = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    EXPECT_NE(stale, current);
    PDisplayBrushBinding binding = MakeImageBinding(stale);
    m_Driver->SelectBrush(std::span(&binding, 1));
    EXPECT_THROW(m_Driver->Submit(), std::system_error);
    binding.Data.ImageID = current;
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 0, 4, 4));
    m_Driver->Submit();
    for (uint32_t pixel : m_Driver->GetPixels()) {
        EXPECT_EQ(pixel, m_Pixels[0]);
    }
}

TEST_F(DisplayResourceTest, QueuedImageScaleAndStyleSelectionsPreserveSnapshots)
{
    m_Pixels = {PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32(),
        PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID original = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    m_Image.LayoutSize.x = 4.0f;
    const PDriverResourceID stretchedImage = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    const PColor background(10, 20, 30);
    std::array layers = {MakeSolidBinding(background), MakeImageBinding(original, PIPoint(1, 0), PIPoint(2, 2))};
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 0, 4, 1));
    layers[1].Data.ImageID = stretchedImage;
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 1, 4, 2));
    layers[1].Data.Scale.x = 0.5f;
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 2, 4, 3));
    layers[1].Data.Style = PBrushStyle::None;
    m_Driver->SelectBrush(layers);
    m_Driver->Submit();
    QueueBrushFill(PIRect(0, 3, 4, 4));
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

TEST_F(DisplayResourceTest, RelativeImagesRetainTheSelectedViewsSize)
{
    m_Pixels = {PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32(),
        PColor(0, 0, 0).GetColor32(), PColor(200, 100, 40).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    const PDriverResourceID original = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    m_Image.LayoutSize = PPoint(1, 1);
    m_Image.LayoutMode = PImageLayoutMode::ViewRelative;
    const PDriverResourceID relative = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    m_Image.LayoutSize = PPoint(0.5f, 0.5f);
    const PDriverResourceID smaller = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    PDisplayBrushBinding binding = MakeImageBinding(original);
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 0, 4, 1));
    binding.Data.ImageID = relative;
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 1, 4, 2));
    binding.Data.Scale = PPoint(2, 3);
    m_Driver->SelectBrush(std::span(&binding, 1));
    QueueBrushFill(PIRect(0, 2, 4, 3));
    binding.Data.ImageID = smaller;
    m_Driver->SelectBrush(std::span(&binding, 1));
    m_Driver->Submit();
    QueueBrushFill(PIRect(0, 3, 4, 4));
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

TEST_F(DisplayResourceTest, SharedRelativeImagesKeepEachSelectedOwnersReferenceSize)
{
    m_Source.ColorSpace = PEColorSpace::RGBA32;
    m_Pixels = {PColor(0, 0, 0, 128).GetColor32(), PColor(200, 100, 40, 128).GetColor32(),
        PColor(0, 0, 0, 128).GetColor32(), PColor(200, 100, 40, 128).GetColor32()};
    const PDriverResourceID bitmap = m_Driver->Request<decltype(PDisplayDeviceControl::CreateBitmap)>(m_Source, false);
    m_Image.LayoutSize = PPoint(1, 1);
    m_Image.LayoutMode = PImageLayoutMode::ViewRelative;
    m_Image.IsOpaque = false;
    m_Image.OpaqueSlices = 0;
    const PDriverResourceID image = m_Driver->Request<decltype(PDisplayDeviceControl::CreateImage)>(bitmap, m_Image);
    const PColor background(20, 40, 60);
    std::array layers = {MakeSolidBinding(background), MakeImageBinding(image), MakeImageBinding(image, PIPoint(0, 0), PIPoint(2, 4))};
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 0, 4, 1));
    layers[1].Data.Scale = PPoint(3, 2);
    layers[2].Data.Scale = PPoint(3, 2);
    m_Driver->SelectBrush(layers);
    QueueBrushFill(PIRect(0, 1, 4, 2));
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

TEST_F(DisplayResourceTest, ImagePreparationSurvivesImageAndSizeSwitching)
{
    Ptr<KDisplayBitmapResource> bitmap = ptr_new<KDisplayBitmapResource>(m_Source, false);
    Ptr<KDisplayImageResource> first = ptr_new<KDisplayImageResource>(bitmap, m_Image);
    Ptr<KDisplayImageResource> second = ptr_new<KDisplayImageResource>(bitmap, m_Image);
    PBrushData data;
    data.Style = PBrushStyle::TexturePattern;
    Ptr<const KDisplayBrushSampler> original = first->GetSampler(data, PIPoint(4, 4));
    Ptr<const KDisplayBrushSampler> otherImage = second->GetSampler(data, PIPoint(4, 4));
    EXPECT_NE(original, otherImage);
    EXPECT_EQ(original, first->GetSampler(data, PIPoint(8, 8)));
    data.Scale = PPoint(2, 2);
    Ptr<const KDisplayBrushSampler> scaled = first->GetSampler(data, PIPoint(4, 4));
    EXPECT_NE(original, scaled);
    data.Scale = PPoint(1, 1);
    EXPECT_EQ(original, first->GetSampler(data, PIPoint(4, 4)));
}

TEST_F(DisplayResourceTest, EvictingImagePreparationDoesNotInvalidateActiveSamplers)
{
    Ptr<KDisplayBitmapResource> bitmap = ptr_new<KDisplayBitmapResource>(m_Source, false);
    m_Image.LayoutMode = PImageLayoutMode::ViewRelative;
    m_Image.LayoutSize = PPoint(1, 1);
    Ptr<KDisplayImageResource> image = ptr_new<KDisplayImageResource>(bitmap, m_Image);
    PBrushData data;
    data.Style = PBrushStyle::TexturePattern;
    Ptr<const KDisplayBrushSampler> original = image->GetSampler(data, PIPoint(4, 4));
    image->GetSampler(data, PIPoint(6, 6));
    image->GetSampler(data, PIPoint(8, 8));
    Ptr<const KDisplayBrushSampler> replacement = image->GetSampler(data, PIPoint(4, 4));
    EXPECT_NE(original, replacement);
    std::array<PColor, 9> expected;
    std::array<PColor, 9> actual;
    original->SampleSpan(-2, 3, expected);
    replacement->SampleSpan(-2, 3, actual);
    EXPECT_EQ(actual, expected);
}

TEST_F(DisplayResourceTest, SharedPreparationUsesEachLayersOrigin)
{
    m_Pixels = {PColor(10, 20, 30).GetColor32(), PColor(40, 50, 60).GetColor32(),
        PColor(70, 80, 90).GetColor32(), PColor(100, 110, 120).GetColor32()};
    Ptr<KDisplayBitmapResource> bitmap = ptr_new<KDisplayBitmapResource>(m_Source, false);
    Ptr<KDisplayImageResource> image = ptr_new<KDisplayImageResource>(bitmap, m_Image);
    PBrushData data;
    data.Style = PBrushStyle::TexturePattern;
    KDisplayBrushLayer layer;
    layer.Style = PBrushStyle::TexturePattern;
    layer.Sampler = image->GetSampler(data, PIPoint(4, 4));
    KDisplayBrush brush;
    brush.SetLayers(std::span(&layer, 1));
    std::array<PColor, 9> expected;
    brush.RenderSpan(-2, 3, expected);
    layer.Origin = PIPoint(15000000, -15000000);
    brush.SetLayers(std::span(&layer, 1));
    std::array<PColor, 9> moved;
    brush.RenderSpan(-2 + layer.Origin.x, 3 + layer.Origin.y, moved);
    EXPECT_EQ(moved, expected);
    EXPECT_EQ(layer.Sampler, image->GetSampler(data, PIPoint(4, 4)));
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
