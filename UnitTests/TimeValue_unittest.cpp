
#include <gtest/gtest.h>
#include <System/TimeValue.h>

TEST(TimeValue, TimeValueOperators)
{
    TimeValMillis millis = TimeValMillis::FromNative(1000);
    TimeValMicros micros = TimeValMicros::FromNative(1000000);
    TimeValNanos  nanos = TimeValNanos::FromNative(1000000000);

    EXPECT_EQ(millis.AsNative(), 1000);
    EXPECT_EQ(micros.AsNative(), 1000000);
    EXPECT_EQ(nanos.AsNative(), 1000000000);
    EXPECT_TRUE(micros == nanos);
    EXPECT_FALSE(micros != nanos);

    EXPECT_TRUE(micros == millis);
    EXPECT_TRUE(!(micros != millis));

    TimeValMicros sum1 = micros + nanos;
    TimeValMicros sum2 = nanos + micros;
    TimeValNanos sum3 = micros + nanos;
    TimeValNanos sum4 = nanos + micros;
    TimeValNanos sum5 = micros + millis;
    TimeValNanos sum6 = nanos + millis;

    EXPECT_EQ(sum1, sum2);
    EXPECT_EQ(sum1, sum3);
    EXPECT_EQ(sum1, sum4);
    EXPECT_EQ(sum1, sum5);
    EXPECT_EQ(sum1, sum6);
    EXPECT_EQ(sum2, sum1);
    EXPECT_EQ(sum2, sum3);
    EXPECT_EQ(sum2, sum4);
    EXPECT_EQ(sum2, sum5);
    EXPECT_EQ(sum2, sum6);
    EXPECT_EQ(sum3, sum1);
    EXPECT_EQ(sum3, sum2);
    EXPECT_EQ(sum3, sum4);
    EXPECT_EQ(sum3, sum5);
    EXPECT_EQ(sum3, sum6);
    EXPECT_EQ(sum4, sum1);
    EXPECT_EQ(sum4, sum2);
    EXPECT_EQ(sum4, sum3);
    EXPECT_EQ(sum4, sum5);
    EXPECT_EQ(sum4, sum6);


    EXPECT_GT(sum1, millis);
    EXPECT_GT(sum3, millis);
    EXPECT_GT(sum1, micros);
    EXPECT_GT(sum3, micros);
    EXPECT_GT(sum1, nanos);
    EXPECT_GT(sum3, nanos);

    EXPECT_GE(sum1, millis);
    EXPECT_GE(sum3, millis);
    EXPECT_GE(sum1, micros);
    EXPECT_GE(sum3, micros);
    EXPECT_GE(sum1, nanos);
    EXPECT_GE(sum3, nanos);

    EXPECT_LT(millis, sum1);
    EXPECT_LT(millis, sum3);
    EXPECT_LT(micros, sum1);
    EXPECT_LT(micros, sum3);
    EXPECT_LT(nanos, sum1);
    EXPECT_LT(nanos, sum3);
    EXPECT_LE(millis, sum1);
    EXPECT_LE(millis, sum3);
    EXPECT_LE(micros, sum1);
    EXPECT_LE(micros, sum3);
    EXPECT_LE(nanos, sum1);
    EXPECT_LE(nanos, sum3);

    EXPECT_LE(nanos, micros);
    EXPECT_GE(nanos, micros);
    EXPECT_LE(micros, nanos);
    EXPECT_GE(micros, nanos);
    EXPECT_LE(micros, millis);
    EXPECT_GE(micros, millis);

    EXPECT_NEAR(sum1.AsSeconds(), 2.0, 0.0000001);
    EXPECT_NEAR(sum3.AsSeconds(), 2.0, 0.0000001);

    EXPECT_EQ(millis.AsMilliseconds(), 1000LL);
    EXPECT_EQ(millis.AsMicroseconds(), 1000000LL);
    EXPECT_EQ(millis.AsNanoseconds(), 1000000000LL);

    EXPECT_EQ(micros.AsMilliseconds(), 1000LL);
    EXPECT_EQ(micros.AsMicroseconds(), 1000000LL);
    EXPECT_EQ(micros.AsNanoseconds(), 1000000000LL);

    EXPECT_EQ(nanos.AsMilliseconds(), 1000LL);
    EXPECT_EQ(nanos.AsMicroseconds(), 1000000LL);
    EXPECT_EQ(nanos.AsNanoseconds(), 1000000000LL);

    millis += 1.0;
    micros += 1.0;
    nanos += 1.0;

    EXPECT_EQ(millis.AsMilliseconds(), 2000LL);
    EXPECT_EQ(millis.AsMicroseconds(), 2000000LL);
    EXPECT_EQ(millis.AsNanoseconds(), 2000000000LL);

    EXPECT_EQ(micros.AsMilliseconds(), 2000LL);
    EXPECT_EQ(micros.AsMicroseconds(), 2000000LL);
    EXPECT_EQ(micros.AsNanoseconds(), 2000000000LL);

    EXPECT_EQ(nanos.AsMilliseconds(), 2000LL);
    EXPECT_EQ(nanos.AsMicroseconds(), 2000000LL);
    EXPECT_EQ(nanos.AsNanoseconds(), 2000000000LL);

    EXPECT_EQ(TimeValMillis::FromSeconds(1LL).AsNative(), 1000);
    EXPECT_EQ(TimeValMillis::FromMilliseconds(1000LL).AsNative(), 1000);
    EXPECT_EQ(TimeValMillis::FromMicroseconds(1000000LL).AsNative(), 1000);
    EXPECT_EQ(TimeValMillis::FromNanoseconds(1000000000LL).AsNative(), 1000);

    EXPECT_EQ(TimeValMicros::FromSeconds(1LL).AsNative(), 1000000);
    EXPECT_EQ(TimeValMicros::FromMilliseconds(1000LL).AsNative(), 1000000);
    EXPECT_EQ(TimeValMicros::FromMicroseconds(1000000LL).AsNative(), 1000000);
    EXPECT_EQ(TimeValMicros::FromNanoseconds(1000000000LL).AsNative(), 1000000);

    EXPECT_EQ(TimeValNanos::FromSeconds(1LL).AsNative(), 1000000000);
    EXPECT_EQ(TimeValNanos::FromMilliseconds(1000LL).AsNative(), 1000000000);
    EXPECT_EQ(TimeValNanos::FromMicroseconds(1000000LL).AsNative(), 1000000000);
    EXPECT_EQ(TimeValNanos::FromNanoseconds(1000000000LL).AsNative(), 1000000000);
}
