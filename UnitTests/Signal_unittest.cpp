#include "gtest/gtest.h"
#include <Signals/Signal.h>

namespace {

Signal<void(int a, int b)> g_SignalTestSignal1;

class SignalTestReceiver : public SignalTarget
{
public:
    SignalTestReceiver()
    {
        g_SignalTestSignal1.Connect(this, &SignalTestReceiver::Slot1);
        g_SignalTestSignal1.Connect(this, &SignalTestReceiver::Slot2);
    }

    void Slot1(int a, int b) { m_A += a; m_B += b; }
    void Slot2(int a) { m_A += a; }

    int m_A = 0;
    int m_B = 0;
};

TEST(SignalsTest, SignalTarget)
{
    SignalTestReceiver receiver;

    g_SignalTestSignal1(1, 2);
    g_SignalTestSignal1(10, 30);

    EXPECT_EQ(22, receiver.m_A);
    EXPECT_EQ(32, receiver.m_B);
}



} //namespace
