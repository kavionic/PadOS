// This file is part of PadOS.
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
// SPDX-License-Identifier: GPL-3.0-or-later

// Standalone host test: c++ -std=c++23 -I../Include USBCDCQueue_test.cpp -o cdc_queue_test
#include <Kernel/USB/ClassDrivers/USBCDCQueue.h>
#include <array>
#include <deque>
#include <iostream>
#include <random>

static void TestTransmitOwnership()
{
    alignas(32) std::array<uint8_t, 2048> storage{};
    kernel::USBCDCQueue queue(storage.data(), 1024, 2);
    std::array<uint8_t, 2048> source{};
    for (size_t i = 0; i < source.size(); ++i) {
        source[i] = static_cast<uint8_t>(i);
    }
    for (size_t length : {size_t(1), size_t(63), size_t(64), size_t(65), size_t(512), size_t(1024)})
    {
        assert(queue.Write(source.data(), length) == length);
        size_t transferLength = 0;
        uint8_t* active = queue.BeginTransmit(transferLength);
        assert(active != nullptr && transferLength == length);
        assert(reinterpret_cast<uintptr_t>(active) % 32 == 0);
        assert(queue.GetWriteSpace() == 1024);
        assert(queue.Write(source.data() + 5, 1019) == 1019);
        assert(std::memcmp(active, source.data(), length) == 0);
        assert(queue.Write(source.data(), 6) == 5);
        assert(queue.GetWriteSpace() == 0);
        assert(queue.BeginTransmit(transferLength) == nullptr);
        queue.CompleteTransmit();
        assert(queue.GetWriteSpace() == 1024);
        active = queue.BeginTransmit(transferLength);
        assert(active != nullptr && transferLength == 1024);
        assert(std::memcmp(active, source.data() + 5, 1019) == 0);
        queue.CancelTransmit(); // Submission failure must retain the entire queued payload.
        assert(queue.GetLength() == 1024);
        assert(queue.BeginTransmit(transferLength) == active);
        queue.CompleteTransmit();
        assert(queue.GetLength() == 0 && queue.GetWriteSpace() == 2048);
    }
}

static void TestReceiveOwnership()
{
    alignas(32) std::array<uint8_t, 1024> storage{};
    kernel::USBCDCQueue queue(storage.data(), 64, 16);
    std::array<uint8_t, 1024> output{};
    for (size_t cycle = 0; cycle < 100; ++cycle)
    {
        uint8_t* active = queue.BeginReceive();
        assert(active != nullptr);
        assert(queue.BeginReceive() == nullptr);
        queue.CompleteReceive(0);
        assert(queue.GetLength() == 0);
        for (size_t i = 0; i < 16; ++i)
        {
            active = queue.BeginReceive();
            assert(active != nullptr && reinterpret_cast<uintptr_t>(active) % 32 == 0);
            active[0] = static_cast<uint8_t>(i);
            queue.CompleteReceive(1);
        }
        assert(queue.GetLength() == 16 && queue.BeginReceive() == nullptr);
        assert(queue.Read(output.data(), 1) == 1 && output[0] == 0);
        active = queue.BeginReceive();
        assert(active != nullptr);
        assert(queue.Read(output.data(), output.size()) == 15);
        for (size_t i = 0; i < 15; ++i) {
            assert(output[i] == i + 1);
        }
        // Draining while DMA owns the tail must not change which block gets published.
        for (size_t i = 0; i < 64; ++i) {
            active[i] = static_cast<uint8_t>(i + 10);
        }
        queue.CompleteReceive(64);
        assert(queue.Read(output.data(), 17) == 17);
        assert(queue.GetLength() == 47);
        assert(queue.Read(output.data() + 17, 100) == 47);
        for (size_t i = 0; i < 64; ++i) {
            assert(output[i] == i + 10);
        }
    }
}

static void TestRandomizedStreams()
{
    alignas(32) std::array<uint8_t, 2048> transmitStorage{};
    alignas(32) std::array<uint8_t, 1024> receiveStorage{};
    kernel::USBCDCQueue transmit(transmitStorage.data(), 1024, 2);
    kernel::USBCDCQueue receive(receiveStorage.data(), 64, 16);
    std::deque<uint8_t> expectedTransmit;
    std::deque<uint8_t> expectedReceive;
    std::mt19937 random(81273);
    std::array<uint8_t, 2300> buffer{};
    uint8_t* transmitActive = nullptr;
    size_t transmitLength = 0;
    uint8_t* receiveActive = nullptr;
    for (size_t iteration = 0; iteration < 100000; ++iteration)
    {
        switch (random() % 6)
        {
            case 0:
            {
                const size_t length = random() % buffer.size();
                for (size_t i = 0; i < length; ++i) {
                    buffer[i] = static_cast<uint8_t>(random());
                }
                const size_t available = transmit.GetWriteSpace();
                const size_t written = transmit.Write(buffer.data(), length);
                assert(written == std::min(available, length));
                expectedTransmit.insert(expectedTransmit.end(), buffer.begin(), buffer.begin() + written);
                break;
            }
            case 1:
                if (transmitActive == nullptr) {
                    transmitActive = transmit.BeginTransmit(transmitLength);
                }
                break;
            case 2:
                if (transmitActive != nullptr)
                {
                    for (size_t i = 0; i < transmitLength; ++i)
                    {
                        assert(transmitActive[i] == expectedTransmit.front());
                        expectedTransmit.pop_front();
                    }
                    transmit.CompleteTransmit();
                    transmitActive = nullptr;
                }
                break;
            case 3:
                if (receiveActive == nullptr) {
                    receiveActive = receive.BeginReceive();
                }
                break;
            case 4:
                if (receiveActive != nullptr)
                {
                    const size_t length = random() % 65;
                    for (size_t i = 0; i < length; ++i)
                    {
                        receiveActive[i] = static_cast<uint8_t>(random());
                        expectedReceive.push_back(receiveActive[i]);
                    }
                    receive.CompleteReceive(length);
                    receiveActive = nullptr;
                }
                break;
            case 5:
            {
                const size_t length = receive.Read(buffer.data(), random() % 200);
                for (size_t i = 0; i < length; ++i)
                {
                    assert(buffer[i] == expectedReceive.front());
                    expectedReceive.pop_front();
                }
                break;
            }
        }
        assert(transmit.GetLength() == expectedTransmit.size());
        assert(receive.GetLength() == expectedReceive.size());
    }
}

int main()
{
    TestTransmitOwnership();
    TestReceiveOwnership();
    TestRandomizedStreams();
    std::cout << "CDC queue ownership and 100000 randomized operations passed.\n";
}
