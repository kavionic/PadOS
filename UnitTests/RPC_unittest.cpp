// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <limits>
#include <DeviceControl/Display.h>
#include <RPC/RPCDispatcher.h>

namespace pados_tests_rpc
{

using SumDefinition = PRPCDefinition<0, false, int32_t(int32_t, int32_t)>;
using PayloadDefinition = PRPCDefinition<1, false, void(bool, uint32_t)>;
using StringDefinition = PRPCDefinition<2, false, void(const PString&)>;
using EmptyDefinition = PRPCDefinition<3, true, size_t()>;

struct RPCBuffer
{
    alignas(std::max_align_t) std::array<std::byte, 256> Data;
    size_t Allocations = 0;
    size_t Length = 0;
    int CommandID = -1;
};

class RPCAllocator
{
public:
    RPCAllocator(RPCBuffer& buffer, int commandID = 0) : m_Buffer(buffer), m_CommandID(commandID) {}

    void* operator()(size_t size) const
    {
        if (size > m_Buffer.Data.size()) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        ++m_Buffer.Allocations;
        m_Buffer.Length = size;
        m_Buffer.CommandID = m_CommandID;
        return m_Buffer.Data.data();
    }

private:
    RPCBuffer& m_Buffer;
    int m_CommandID;
};

struct RPCReceiver
{
    size_t Calls = 0;
    std::span<const std::byte> Payload;
    uint32_t Value = 0;
    bool Enabled = false;
    PString Text;

    int32_t Sum(int32_t lhs, int32_t rhs)
    {
        ++Calls;
        return lhs + rhs;
    }

    void Receive(std::span<const std::byte> payload, bool enabled, uint32_t value)
    {
        ++Calls;
        Payload = payload;
        Enabled = enabled;
        Value = value;
    }

    void ReceiveString(std::span<const std::byte> payload, const PString& text)
    {
        ++Calls;
        Payload = payload;
        Text = text;
    }

    size_t PayloadSize(std::span<const std::byte> payload) const { return payload.size(); }
    size_t Constant() const { return 42; }
};

struct PolygonReceiver
{
    std::span<const std::byte> Payload;
    size_t ClipCount = 0;
    size_t PointCount = 0;

    void Receive(std::span<const std::byte> payload, PDisplayBitmap, size_t clipCount, size_t pointCount, PDrawingMode)
    {
        Payload = payload;
        ClipCount = clipCount;
        PointCount = pointCount;
    }
};

struct CursorReceiver
{
    PMouseCursorBitmap Cursor;
    bool Accepted = true;

    bool SetMouseCursorBitmap(const PMouseCursorBitmap& cursor)
    {
        Cursor = cursor;
        return Accepted;
    }
};

static_assert(PArgumentSerializer<void, bool, uint32_t>::GetFixedSize() == 8);
static_assert(!PArgumentSerializer<void, const PString&>::HAS_FIXED_SIZE);
static_assert(sizeof(PRPCInvoker<false, void()>) == sizeof(PRPCInvoker<false, void()>::CallbackSignature));

TEST(RPC, ImmediateAndSuppliedBuffers)
{
    RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<SumDefinition>(&receiver, &RPCReceiver::Sum);
    PRPCInvoker<false, SumDefinition::Signature> invoke(
        [&dispatcher](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            dispatcher.Dispatch(SumDefinition::HandlerID, input, inputSize, output, outputSize);
        });
    EXPECT_EQ(invoke(17, 25), 42);
    alignas(std::max_align_t) std::byte buffer[invoke.GetFixedSize()];
    EXPECT_EQ(invoke(buffer, 20, 22), 42);
    EXPECT_EQ(invoke(buffer, sizeof(buffer), 40, 2), 42);
    EXPECT_EQ(receiver.Calls, 3u);
    EXPECT_THROW(invoke(buffer, sizeof(buffer) - 1, 20, 22), std::system_error);
    EXPECT_EQ(receiver.Calls, 3u);
}

TEST(RPC, MissingHandlersReturnNOSYS)
{
    const RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    const auto expectMissingHandler = [&dispatcher](int request)
    {
        try
        {
            dispatcher.Dispatch(request, nullptr, 0, nullptr, 0);
            FAIL() << "Expected NOSYS for request " << request;
        }
        catch (const std::system_error& error)
        {
            EXPECT_EQ(error.code().value(), std::to_underlying(PErrorCode::NOSYS));
        }
    };

    expectMissingHandler(0);
    expectMissingHandler(-1);
    expectMissingHandler(std::numeric_limits<int>::min());
    expectMissingHandler(std::numeric_limits<int>::max());

    dispatcher.AddHandler<EmptyDefinition>(&receiver, &RPCReceiver::Constant);
    for (int request = 0; request < EmptyDefinition::HandlerID; ++request) {
        expectMissingHandler(request);
    }
    expectMissingHandler(EmptyDefinition::HandlerID + 1);
    expectMissingHandler(-1);
}

TEST(RPC, RegistrationSupportsGapsGrowthAndReplacement)
{
    RPCReceiver firstReceiver;
    RPCReceiver secondReceiver;
    PRPCDispatcher dispatcher;
    int request = 0;
    PRPCInvoker<false, SumDefinition::Signature> invoke(
        [&dispatcher, &request](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            dispatcher.Dispatch(request, input, inputSize, output, outputSize);
        });

    EXPECT_THROW(dispatcher.AddHandler(-1, &firstReceiver, &RPCReceiver::Sum), std::system_error);
    dispatcher.AddHandler(0, &firstReceiver, &RPCReceiver::Sum);
    EXPECT_EQ(invoke(20, 22), 42);

    dispatcher.AddHandler(7, &secondReceiver, &RPCReceiver::Sum);
    EXPECT_EQ(invoke(10, 32), 42);

    dispatcher.AddHandler(3, &firstReceiver, &RPCReceiver::Sum);
    request = 3;
    EXPECT_EQ(invoke(30, 12), 42);

    request = 7;
    EXPECT_EQ(invoke(40, 2), 42);
    dispatcher.AddHandler(7, &firstReceiver, &RPCReceiver::Sum);
    EXPECT_EQ(invoke(41, 1), 42);
    EXPECT_EQ(firstReceiver.Calls, 4u);
    EXPECT_EQ(secondReceiver.Calls, 1u);
}

TEST(RPC, CustomAllocatorCanTransmitAndReturn)
{
    RPCBuffer buffer;
    RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<SumDefinition>(&receiver, &RPCReceiver::Sum);
    PRPCInvoker<false, SumDefinition::Signature, RPCAllocator> invoke(
        [&dispatcher, &buffer](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            EXPECT_EQ(input, buffer.Data.data());
            dispatcher.Dispatch(SumDefinition::HandlerID, input, inputSize, output, outputSize);
        },
        RPCAllocator(buffer));
    EXPECT_EQ(invoke(11, 31), 42);
    EXPECT_EQ(buffer.Allocations, 1u);
    EXPECT_EQ(invoke(buffer.Data.data(), 12, 30), 42);
    EXPECT_EQ(buffer.Allocations, 1u);
}

TEST(RPC, SuppliedBufferPreservesCallerFilledPayload)
{
    RPCBuffer buffer;
    RPCAllocator allocator(buffer, PayloadDefinition::HandlerID);
    PRPCInvoker<false, PayloadDefinition::Signature, RPCAllocator, PRPCNoTransmission> invoke(PRPCNoTransmission(), allocator);
    void* storage = allocator(invoke.GetFixedSize() + 3);
    std::byte* payload = static_cast<std::byte*>(storage) + invoke.GetFixedSize();
    payload[0] = std::byte{0x41};
    payload[1] = std::byte{0x00};
    payload[2] = std::byte{0xff};
    invoke(storage, true, uint32_t(123));
    EXPECT_EQ(buffer.Allocations, 1u);
    EXPECT_EQ(buffer.CommandID, PayloadDefinition::HandlerID);

    RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<PayloadDefinition>(&receiver, &RPCReceiver::Receive);
    dispatcher.Dispatch(buffer.CommandID, storage, buffer.Length, nullptr, 0);
    EXPECT_EQ(receiver.Payload.data(), payload);
    ASSERT_EQ(receiver.Payload.size(), 3u);
    EXPECT_EQ(receiver.Payload[0], std::byte{0x41});
    EXPECT_EQ(receiver.Payload[1], std::byte{0x00});
    EXPECT_EQ(receiver.Payload[2], std::byte{0xff});
    EXPECT_TRUE(receiver.Enabled);
    EXPECT_EQ(receiver.Value, 123u);

    invoke(false, uint32_t(456));
    EXPECT_EQ(buffer.Allocations, 2u);
    dispatcher.Dispatch(buffer.CommandID, buffer.Data.data(), buffer.Length, nullptr, 0);
    EXPECT_TRUE(receiver.Payload.empty());
    EXPECT_FALSE(receiver.Enabled);
    EXPECT_EQ(receiver.Value, 456u);
}

TEST(RPC, TransmittedPayloadFollowsDynamicArguments)
{
    RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<StringDefinition>(&receiver, &RPCReceiver::ReceiveString);
    PRPCInvoker<false, StringDefinition::Signature> invoke(
        [&dispatcher](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            dispatcher.Dispatch(StringDefinition::HandlerID, input, inputSize, output, outputSize);
        });
    alignas(std::max_align_t) std::byte buffer[128];
    const PString text("abcde");
    const size_t argumentSize = invoke.GetSize(text);
    buffer[argumentSize] = std::byte{0x7a};
    invoke(buffer, argumentSize + 1, text);
    EXPECT_EQ(receiver.Text, text);
    EXPECT_EQ(receiver.Payload.data(), buffer + argumentSize);
    ASSERT_EQ(receiver.Payload.size(), 1u);
    EXPECT_EQ(receiver.Payload[0], std::byte{0x7a});
}

TEST(RPC, RejectsTruncatedArgumentsIncludingAlignment)
{
    RPCBuffer buffer;
    PRPCInvoker<false, PayloadDefinition::Signature, RPCAllocator, PRPCNoTransmission> invoke(
        PRPCNoTransmission(),
        RPCAllocator(buffer, PayloadDefinition::HandlerID));
    invoke(true, uint32_t(123));
    RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<PayloadDefinition>(&receiver, &RPCReceiver::Receive);
    for (size_t length = 0; length < invoke.GetFixedSize(); ++length) {
        EXPECT_THROW(
            dispatcher.Dispatch(PayloadDefinition::HandlerID, buffer.Data.data(), length, nullptr, 0),
            std::system_error);
    }
    EXPECT_EQ(receiver.Calls, 0u);

    using BoolDeserializer = PArgumentDeserializer<void, bool>;
    EXPECT_THROW(
        BoolDeserializer::Invoke<true>(buffer.Data.data(), 1, [](std::span<const std::byte>, bool) {}),
        std::system_error);
    using BoolSerializer = PArgumentSerializer<void, bool>;
    EXPECT_EQ(BoolSerializer::WriteArg(buffer.Data.data(), 1, true), -1);
}

TEST(RPC, ConstCallbacksAndPayloadOnlyCalls)
{
    const RPCReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler<EmptyDefinition>(&receiver, &RPCReceiver::PayloadSize);
    const PRPCInvoker<true, EmptyDefinition::Signature> invoke(
        [&dispatcher](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            dispatcher.Dispatch(EmptyDefinition::HandlerID, input, inputSize, output, outputSize);
        });
    EXPECT_EQ(invoke(), 0u);
    EXPECT_EQ(invoke(nullptr, size_t(0)), 0u);
    std::byte payload[3] = {};
    EXPECT_EQ(invoke(payload, sizeof(payload)), 3u);
    dispatcher.AddHandler<EmptyDefinition>(&receiver, &RPCReceiver::Constant);
    EXPECT_EQ(invoke(), 42u);
}

TEST(RPC, CursorDeviceControlReturnsAcceptanceWithoutCopyingRaster)
{
    const std::array<PMouseCursorPixel, 4> pixels
    {
        PMouseCursorPixel::Transparent,
        PMouseCursorPixel::Color1,
        PMouseCursorPixel::Color2,
        PMouseCursorPixel::Invert
    };
    const PMouseCursorBitmap cursor{2, 2, PIPoint(1, 1), PColor(0xff000000), PColor(0xffffffff), pixels};
    CursorReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler(&PDisplayDeviceControl::SetMouseCursorBitmap, &receiver, &CursorReceiver::SetMouseCursorBitmap);

    using CursorInvoker = decltype(PDisplayDeviceControl::SetMouseCursorBitmap);
    using CursorDefinition = CursorInvoker::Definition;
    PRPCInvoker<CursorDefinition::IsConst, CursorDefinition::Signature> invoke(
        [&dispatcher](const void* input, size_t inputSize, void* output, size_t outputSize)
        {
            dispatcher.Dispatch(CursorDefinition::HandlerID, input, inputSize, output, outputSize);
        });
    EXPECT_TRUE(invoke(cursor));
    EXPECT_EQ(receiver.Cursor.Width, cursor.Width);
    EXPECT_EQ(receiver.Cursor.Height, cursor.Height);
    EXPECT_EQ(receiver.Cursor.Raster.data(), pixels.data());
    EXPECT_EQ(receiver.Cursor.Raster.size(), pixels.size());

    receiver.Accepted = false;
    EXPECT_FALSE(invoke(cursor));
}

TEST(RPC, GeometryPayloadIsAlignedAndReservedInPlace)
{
    RPCBuffer buffer;
    using PolygonInvoker = decltype(PDisplayCommands::FillPolygon);
    using PolygonDefinition = PolygonInvoker::Definition;
    RPCAllocator allocator(buffer, PolygonDefinition::HandlerID);
    PRPCInvoker<PolygonDefinition::IsConst, PolygonDefinition::Signature, RPCAllocator, PRPCNoTransmission> invoke(
        PRPCNoTransmission(),
        allocator);
    const size_t payloadSize = sizeof(PIRect) + 3 * sizeof(PPoint);
    void* storage = allocator(invoke.GetFixedSize() + payloadSize);
    std::byte* payload = static_cast<std::byte*>(storage) + invoke.GetFixedSize();
    EXPECT_EQ(uintptr_t(payload) % alignof(PIRect), 0u);
    EXPECT_EQ(uintptr_t(payload + sizeof(PIRect)) % alignof(PPoint), 0u);
    std::memset(payload, 0x5a, payloadSize);
    invoke(storage, PDisplayBitmap(), size_t(1), size_t(3), PDrawingMode());
    EXPECT_EQ(buffer.Allocations, 1u);
    EXPECT_EQ(buffer.Length, invoke.GetFixedSize() + payloadSize);
    for (size_t index = 0; index < payloadSize; ++index) {
        EXPECT_EQ(payload[index], std::byte{0x5a});
    }
    PolygonReceiver receiver;
    PRPCDispatcher dispatcher;
    dispatcher.AddHandler(&PDisplayCommands::FillPolygon, &receiver, &PolygonReceiver::Receive);
    dispatcher.Dispatch(buffer.CommandID, storage, buffer.Length, nullptr, 0);
    EXPECT_EQ(receiver.Payload.data(), payload);
    EXPECT_EQ(receiver.Payload.size(), payloadSize);
    EXPECT_EQ(receiver.ClipCount, 1u);
    EXPECT_EQ(receiver.PointCount, 3u);
}

} // namespace pados_tests_rpc
