#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <sys/stat.h>

#include <Utils/String.h>

namespace {

static std::string FS(int64_t size, int num, PUnitSystem unitSystem)
{
    return PString::format_byte_size(size, num, unitSystem);
}

// Powers we use for boundary tests.
static constexpr int64_t SI_PETA = 1'000'000'000'000'000LL;       // 10^15
static constexpr int64_t SI_EXA = 1'000'000'000'000'000'000LL;   // 10^18

static constexpr int64_t IEC_PETA = (1LL << 50); // 2^50
static constexpr int64_t IEC_EXA = (1LL << 60); // 2^60

} // namespace

TEST(FormatFileSize, UnitSelection_SI_vs_IEC_IntegerMode)
{
    EXPECT_EQ(FS(999, 0, PUnitSystem::SI), "999");
    EXPECT_EQ(FS(1000, 0, PUnitSystem::SI), "1k");

    EXPECT_EQ(FS(1023, 0, PUnitSystem::IEC), "1023");
    EXPECT_EQ(FS(1024, 0, PUnitSystem::IEC), "1K");

    // SI advances at 1000, IEC advances at 1024.
    EXPECT_EQ(FS(1000, 0, PUnitSystem::IEC), "1000");
    EXPECT_EQ(FS(1000, 0, PUnitSystem::SI), "1k");
}

TEST(FormatFileSize, IntegerFastPath_IgnoresDecimalsArgWhenUnitZero)
{
    EXPECT_EQ(FS(0, 2, PUnitSystem::SI), "0");
    EXPECT_EQ(FS(42, 5, PUnitSystem::IEC), "42");
    EXPECT_EQ(FS(-42, 5, PUnitSystem::IEC), "-42");
}

TEST(FormatFileSize, FixedDecimals_Mode_Rounding)
{
    EXPECT_EQ(FS(1500, 1, PUnitSystem::SI), "1.5k");
    EXPECT_EQ(FS(1550, 2, PUnitSystem::SI), "1.55k");
    EXPECT_EQ(FS(1499, 1, PUnitSystem::SI), "1.5k"); // 1.499 -> 1.5
}

TEST(FormatFileSize, FixedDecimals_ClampTo12_IsObservable)
{
    const int64_t v = 1500; // forces unit>0 for SI
    EXPECT_EQ(FS(v, 99, PUnitSystem::SI), FS(v, 12, PUnitSystem::SI));
    EXPECT_EQ(FS(v, 13, PUnitSystem::SI), FS(v, 12, PUnitSystem::SI));
}

TEST(FormatFileSize, SignificantDigits_Mode_Examples)
{
    EXPECT_EQ(FS(1234, -3, PUnitSystem::SI), "1.23k");
    EXPECT_EQ(FS(12340, -3, PUnitSystem::SI), "12.3k");
    EXPECT_EQ(FS(123400, -3, PUnitSystem::SI), "123k");

    // Unit selection advances to 'M' at 1,000,000 for SI.
    EXPECT_EQ(FS(1234000, -3, PUnitSystem::SI), "1.23M");

    // With "values < 1 count as 1 digit" rule, -2 => one decimal when 1<=x<10.
    EXPECT_EQ(FS(9000, -2, PUnitSystem::SI), "9.0k");
    EXPECT_EQ(FS(9500, -2, PUnitSystem::SI), "9.5k");
}

TEST(FormatFileSize, SignificantDigits_ClampTo12_IsObservable)
{
    const int64_t v = 123456789; // SI => 123.456789k (unit>0)
    EXPECT_EQ(FS(v, -999, PUnitSystem::SI), FS(v, -12, PUnitSystem::SI));
    EXPECT_EQ(FS(v, -13, PUnitSystem::SI), FS(v, -12, PUnitSystem::SI));
}

TEST(FormatFileSize, BytesAreAlwaysInteger)
{
    // Your implementation always uses the integer-only path when unit==0 (bytes),
    // regardless of requested decimals/digits.
    EXPECT_EQ(FS(-1, 2, PUnitSystem::IEC), "-1");
    EXPECT_EQ(FS(-1, 3, PUnitSystem::IEC), "-1");
    EXPECT_EQ(FS(1, 2, PUnitSystem::SI), "1");
    EXPECT_EQ(FS(1, -3, PUnitSystem::SI), "1");
}

TEST(FormatFileSize, HandlesInt64Max_IntegerMode_Correct)
{
    const int64_t v = std::numeric_limits<int64_t>::max(); // 9223372036854775807
    EXPECT_EQ(FS(v, 0, PUnitSystem::SI), "9E");
    EXPECT_EQ(FS(v, 0, PUnitSystem::IEC), "7E");
}

TEST(FormatFileSize, HandlesInt64Min_IntegerMode_Correct)
{
    const int64_t v = std::numeric_limits<int64_t>::min(); // -9223372036854775808
    EXPECT_EQ(FS(v, 0, PUnitSystem::SI), "-9E");
    EXPECT_EQ(FS(v, 0, PUnitSystem::IEC), "-8E");
}

// --- Boundary tests around the last selectable scale (P -> E) ---

TEST(FormatFileSize, Boundary_SI_PetaToExa_IntegerMode)
{
    // Just below 1 exa: should still be peta ('p')
    EXPECT_EQ(FS(SI_EXA - 1, 0, PUnitSystem::SI), "999P");

    // Exactly 1 exa: should become 1 exa ('e')
    EXPECT_EQ(FS(SI_EXA, 0, PUnitSystem::SI), "1E");

    // A little above: still exa, integer division
    EXPECT_EQ(FS(SI_EXA + (SI_EXA / 10), 0, PUnitSystem::SI), "1E"); // 1.1e -> 1e in integer mode
}

TEST(FormatFileSize, Boundary_SI_PetaToExa_FixedDecimals)
{
    // Unit selection is decided before rounding, so just below exa stays 'p'
    // and rounds within that unit.
    EXPECT_EQ(FS(SI_EXA - 1, 3, PUnitSystem::SI), "1000.000P");
    EXPECT_EQ(FS(SI_EXA - 1, 6, PUnitSystem::SI), "1000.000000P");

    // Exactly exa is 'e'
    EXPECT_EQ(FS(SI_EXA, 3, PUnitSystem::SI), "1.000E");
}

TEST(FormatFileSize, Boundary_SI_Negative_PetaToExa_IntegerMode)
{
    EXPECT_EQ(FS(-(SI_EXA - 1), 0, PUnitSystem::SI), "-999P");
    EXPECT_EQ(FS(-SI_EXA, 0, PUnitSystem::SI), "-1E");
}

TEST(FormatFileSize, Boundary_IEC_PetaToExa_IntegerMode)
{
    // Just below 2^60 (exbi): should still be peta ('P')
    // At scale 2^50, (2^60 - 1) / 2^50 = 1023 => "1023P"
    EXPECT_EQ(FS(IEC_EXA - 1, 0, PUnitSystem::IEC), "1023P");

    // Exactly 2^60: should become 1E
    EXPECT_EQ(FS(IEC_EXA, 0, PUnitSystem::IEC), "1E");
}

TEST(FormatFileSize, Boundary_IEC_PetaToExa_FixedDecimals)
{
    // Same policy: just below exbi stays 'P' and rounds within that unit.
    EXPECT_EQ(FS(IEC_EXA - 1, 3, PUnitSystem::IEC), "1024.000P");

    // Exactly exbi is 'E'
    EXPECT_EQ(FS(IEC_EXA, 3, PUnitSystem::IEC), "1.000E");
}

TEST(FormatFileSize, Boundary_IEC_Negative_PetaToExa_IntegerMode)
{
    EXPECT_EQ(FS(-(IEC_EXA - 1), 0, PUnitSystem::IEC), "-1023P");
    EXPECT_EQ(FS(-IEC_EXA, 0, PUnitSystem::IEC), "-1E");
}

// Explicitly encode "no unit promotion on rounding" policy.

TEST(FormatFileSize, DoesNotPromoteUnitOnRounding_SI)
{
    EXPECT_EQ(FS(SI_EXA - 1, 0, PUnitSystem::SI), "999P");
    EXPECT_EQ(FS(SI_EXA - 1, 3, PUnitSystem::SI), "1000.000P");
    EXPECT_EQ(FS(SI_EXA, 3, PUnitSystem::SI), "1.000E");
}

TEST(FormatFileSize, DoesNotPromoteUnitOnRounding_IEC)
{
    EXPECT_EQ(FS(IEC_EXA - 1, 0, PUnitSystem::IEC), "1023P");
    EXPECT_EQ(FS(IEC_EXA - 1, 3, PUnitSystem::IEC), "1024.000P");
    EXPECT_EQ(FS(IEC_EXA, 3, PUnitSystem::IEC), "1.000E");
}

TEST(FormatFileSize, Policy_BytesInteger_And_NoUnitPromotion)
{
    // bytes always integer
    EXPECT_EQ(FS(1, 6, PUnitSystem::SI), "1");
    EXPECT_EQ(FS(1, -6, PUnitSystem::SI), "1");
    EXPECT_EQ(FS(-1, 6, PUnitSystem::IEC), "-1");

    // no promotion by rounding
    EXPECT_EQ(FS(SI_EXA - 1, 3, PUnitSystem::SI), "1000.000P");
    EXPECT_EQ(FS(SI_EXA, 3, PUnitSystem::SI), "1.000E");
    EXPECT_EQ(FS(IEC_EXA - 1, 3, PUnitSystem::IEC), "1024.000P");
    EXPECT_EQ(FS(IEC_EXA, 3, PUnitSystem::IEC), "1.000E");
}

TEST(FormatFileSize, Boundaries_SI_AllSteps_IntegerMode)
{
    EXPECT_EQ(FS(999, 0, PUnitSystem::SI), "999");
    EXPECT_EQ(FS(1000, 0, PUnitSystem::SI), "1k");

    EXPECT_EQ(FS(999'999, 0, PUnitSystem::SI), "999k");
    EXPECT_EQ(FS(1'000'000, 0, PUnitSystem::SI), "1M");

    EXPECT_EQ(FS(999'999'999, 0, PUnitSystem::SI), "999M");
    EXPECT_EQ(FS(1'000'000'000, 0, PUnitSystem::SI), "1G");

    EXPECT_EQ(FS(999'999'999'999LL, 0, PUnitSystem::SI), "999G");
    EXPECT_EQ(FS(1'000'000'000'000LL, 0, PUnitSystem::SI), "1T");

    EXPECT_EQ(FS(999'999'999'999'999LL, 0, PUnitSystem::SI), "999T");
    EXPECT_EQ(FS(1'000'000'000'000'000LL, 0, PUnitSystem::SI), "1P");

    EXPECT_EQ(FS(SI_EXA - 1, 0, PUnitSystem::SI), "999P");
    EXPECT_EQ(FS(SI_EXA, 0, PUnitSystem::SI), "1E");
}

TEST(FormatFileSize, Boundaries_IEC_AllSteps_IntegerMode)
{
    EXPECT_EQ(FS(1023, 0, PUnitSystem::IEC), "1023");
    EXPECT_EQ(FS(1024, 0, PUnitSystem::IEC), "1K");

    EXPECT_EQ(FS((1LL << 20) - 1, 0, PUnitSystem::IEC), "1023K");
    EXPECT_EQ(FS((1LL << 20), 0, PUnitSystem::IEC), "1M");

    EXPECT_EQ(FS((1LL << 30) - 1, 0, PUnitSystem::IEC), "1023M");
    EXPECT_EQ(FS((1LL << 30), 0, PUnitSystem::IEC), "1G");

    EXPECT_EQ(FS((1LL << 40) - 1, 0, PUnitSystem::IEC), "1023G");
    EXPECT_EQ(FS((1LL << 40), 0, PUnitSystem::IEC), "1T");

    EXPECT_EQ(FS((1LL << 50) - 1, 0, PUnitSystem::IEC), "1023T");
    EXPECT_EQ(FS((1LL << 50), 0, PUnitSystem::IEC), "1P");

    EXPECT_EQ(FS((1LL << 60) - 1, 0, PUnitSystem::IEC), "1023P");
    EXPECT_EQ(FS((1LL << 60), 0, PUnitSystem::IEC), "1E");
}

TEST(FormatFileSize, Rounding_SI_Kilo_OneDecimal)
{
    // value in k: 1.049k -> 1.0k, 1.050k -> 1.1k
    EXPECT_EQ(FS(1049, 1, PUnitSystem::SI), "1.0k");
    EXPECT_EQ(FS(1050, 1, PUnitSystem::SI), "1.1k");

    // Negative: -1.049k -> -1.0k, -1.050k -> -1.1k
    EXPECT_EQ(FS(-1049, 1, PUnitSystem::SI), "-1.0k");
    EXPECT_EQ(FS(-1050, 1, PUnitSystem::SI), "-1.1k");
}

TEST(FormatFileSize, Rounding_IEC_Kilo_OneDecimal)
{
    // scale=1024. 1074/1024=1.0488.. => 1.0K @ 1 dec
    //              1075/1024=1.0498.. => 1.1K @ 1 dec (round half away from 0)
    EXPECT_EQ(FS(1075, 1, PUnitSystem::IEC), "1.0K");
    EXPECT_EQ(FS(1076, 1, PUnitSystem::IEC), "1.1K");

    EXPECT_EQ(FS(-1075, 1, PUnitSystem::IEC), "-1.0K");
    EXPECT_EQ(FS(-1076, 1, PUnitSystem::IEC), "-1.1K");
}

TEST(FormatFileSize, DigitsMode_TwoDigits_SI)
{
    // -2 digits => keep ~2 digits total for the chosen unit.
    EXPECT_EQ(FS(1100, -2, PUnitSystem::SI), "1.1k");  // 1.1
    EXPECT_EQ(FS(1000, -2, PUnitSystem::SI), "1.0k");  // shows 1 decimal
    EXPECT_EQ(FS(9000, -2, PUnitSystem::SI), "9.0k");
    EXPECT_EQ(FS(9900, -2, PUnitSystem::SI), "9.9k");

    // When integer digits >= digits => no decimals
    EXPECT_EQ(FS(10'000, -2, PUnitSystem::SI), "10k");
    EXPECT_EQ(FS(99'000, -2, PUnitSystem::SI), "99k");
    EXPECT_EQ(FS(100'000, -2, PUnitSystem::SI), "100k"); // don't truncate
}

TEST(FormatFileSize, DigitsMode_ThreeDigits_SI_MovesUnitAsNeeded)
{
    // Unit selection happens first, then digit-mode within that unit.
    EXPECT_EQ(FS(999'500, -3, PUnitSystem::SI), "1000k");
    EXPECT_EQ(FS(1'000'000, -3, PUnitSystem::SI), "1.00M");
}

TEST(FormatFileSize, Clamp_FixedDecimals)
{
    const int64_t v = 1500; // 1.5k
    EXPECT_EQ(FS(v, 12, PUnitSystem::SI), FS(v, 999, PUnitSystem::SI));
}

TEST(FormatFileSize, Clamp_DigitsMode)
{
    const int64_t v = 123456789; // unit>0
    EXPECT_EQ(FS(v, -12, PUnitSystem::SI), FS(v, -999, PUnitSystem::SI));
}

TEST(FormatFileSize, Negative_Boundaries_SI)
{
    EXPECT_EQ(FS(-999, 0, PUnitSystem::SI), "-999");
    EXPECT_EQ(FS(-1000, 0, PUnitSystem::SI), "-1k");

    EXPECT_EQ(FS(-999'999, 0, PUnitSystem::SI), "-999k");
    EXPECT_EQ(FS(-1'000'000, 0, PUnitSystem::SI), "-1M");
}

TEST(FormatFileSize, Negative_Boundaries_IEC)
{
    EXPECT_EQ(FS(-1023, 0, PUnitSystem::IEC), "-1023");
    EXPECT_EQ(FS(-1024, 0, PUnitSystem::IEC), "-1K");

    EXPECT_EQ(FS(-((1LL << 20) - 1), 0, PUnitSystem::IEC), "-1023K");
    EXPECT_EQ(FS(-(1LL << 20), 0, PUnitSystem::IEC), "-1M");
}

TEST(FormatFileSize, Extremes_WithDecimals_DoNotCrash_AndHaveUnit)
{
    const int64_t vMax = std::numeric_limits<int64_t>::max();
    const int64_t vMin = std::numeric_limits<int64_t>::min();

    const std::string s1 = FS(vMax, 3, PUnitSystem::SI);
    const std::string s2 = FS(vMin, 3, PUnitSystem::SI);
    const std::string s3 = FS(vMax, -3, PUnitSystem::IEC);
    const std::string s4 = FS(vMin, -3, PUnitSystem::IEC);

    ASSERT_FALSE(s1.empty());
    ASSERT_FALSE(s2.empty());
    ASSERT_FALSE(s3.empty());
    ASSERT_FALSE(s4.empty());

    // With your capped unit list, extremes should end at the last unit.
    EXPECT_EQ(s1.back(), 'E');
    EXPECT_EQ(s2.back(), 'E');
    EXPECT_EQ(s3.back(), 'E');
    EXPECT_EQ(s4.back(), 'E');
}



namespace pados_tests::pstring_format_file_permissions
{
static PString fmt(mode_t mode)
{
    return PString::format_file_permissions(mode);
}

TEST(PString_FormatFilePermissions, FileTypeCharacter)
{
    EXPECT_EQ(fmt(S_IFREG), "----------");
    EXPECT_EQ(fmt(S_IFDIR), "d---------");
    EXPECT_EQ(fmt(S_IFLNK), "l---------");
    EXPECT_EQ(fmt(S_IFCHR), "c---------");
    EXPECT_EQ(fmt(S_IFBLK), "b---------");
    EXPECT_EQ(fmt(S_IFIFO), "p---------");
    EXPECT_EQ(fmt(S_IFSOCK), "s---------");

    // Unknown type: force something outside known S_IF* values in S_IFMT field.
    const mode_t unknownType = (mode_t)(S_IFMT); // all type bits set -> not a valid S_IF*
    EXPECT_EQ(fmt(unknownType), "?---------");
}

TEST(PString_FormatFilePermissions, NoPermissionsRegularFile)
{
    EXPECT_EQ(fmt(S_IFREG), "----------");
}

TEST(PString_FormatFilePermissions, BasicRwxBits)
{
    EXPECT_EQ(fmt(S_IFREG | S_IRUSR), "-r--------");
    EXPECT_EQ(fmt(S_IFREG | S_IWUSR), "--w-------");
    EXPECT_EQ(fmt(S_IFREG | S_IXUSR), "---x------");

    EXPECT_EQ(fmt(S_IFREG | S_IRGRP), "----r-----");
    EXPECT_EQ(fmt(S_IFREG | S_IWGRP), "-----w----");
    EXPECT_EQ(fmt(S_IFREG | S_IXGRP), "------x---");

    EXPECT_EQ(fmt(S_IFREG | S_IROTH), "-------r--");
    EXPECT_EQ(fmt(S_IFREG | S_IWOTH), "--------w-");
    EXPECT_EQ(fmt(S_IFREG | S_IXOTH), "---------x");

    EXPECT_EQ(fmt(S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR), "-rwx------");
    EXPECT_EQ(fmt(S_IFREG | S_IRGRP | S_IWGRP | S_IXGRP), "----rwx---");
    EXPECT_EQ(fmt(S_IFREG | S_IROTH | S_IWOTH | S_IXOTH), "-------rwx");
    EXPECT_EQ(fmt(S_IFREG | 0777), "-rwxrwxrwx");
}

TEST(PString_FormatFilePermissions, SuidSemantics)
{
    // SUID affects the user-execute position.
    EXPECT_EQ(fmt(S_IFREG | S_ISUID),                               "---S------");
    EXPECT_EQ(fmt(S_IFREG | S_ISUID | S_IXUSR),                     "---s------");
    EXPECT_EQ(fmt(S_IFREG | S_ISUID | S_IRUSR | S_IWUSR),           "-rwS------");
    EXPECT_EQ(fmt(S_IFREG | S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR), "-rws------");
}

TEST(PString_FormatFilePermissions, SgidSemantics)
{
    // SGID affects the group-execute position.
    EXPECT_EQ(fmt(S_IFREG | S_ISGID), "------S---");
    EXPECT_EQ(fmt(S_IFREG | S_ISGID | S_IXGRP), "------s---");
    EXPECT_EQ(fmt(S_IFREG | S_ISGID | S_IRGRP | S_IWGRP), "----rwS---");
    EXPECT_EQ(fmt(S_IFREG | S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP), "----rws---");
}

TEST(PString_FormatFilePermissions, StickySemantics)
{
    // Sticky affects the other-execute position.
    EXPECT_EQ(fmt(S_IFREG | S_ISVTX), "---------T");
    EXPECT_EQ(fmt(S_IFREG | S_ISVTX | S_IXOTH), "---------t");
    EXPECT_EQ(fmt(S_IFREG | S_ISVTX | S_IROTH | S_IWOTH), "-------rwT");
    EXPECT_EQ(fmt(S_IFREG | S_ISVTX | S_IROTH | S_IWOTH | S_IXOTH), "-------rwt");
}

TEST(PString_FormatFilePermissions, CombinedSpecialBits)
{
    EXPECT_EQ(fmt(S_IFREG | S_ISUID | S_ISGID | S_ISVTX), "---S--S--T");
    EXPECT_EQ(fmt(S_IFREG | S_ISUID | S_ISGID | S_ISVTX | 0111), "---s--s--t");
    EXPECT_EQ(fmt(S_IFDIR | S_ISVTX | 0777), "drwxrwxrwt"); // typical /tmp style
}
} // namespace pados_tests::pstring_format_file_permissions




// Unity-build safe: all helpers live inside a uniquely named fixture type.
// No anonymous namespace, no file-scope helper functions.
class PString_ParseFileSize_Tests : public ::testing::Test
{
protected:
    static std::optional<int64_t> Parse(const char* text, PUnitSystem system = PUnitSystem::Auto)
    {
        return PString::parse_byte_size(PString(text), system);
    }

    static int64_t ParseOrFail(const char* text, PUnitSystem system = PUnitSystem::Auto)
    {
        std::optional<int64_t> value = Parse(text, system);
        EXPECT_TRUE(value.has_value()) << "Expected parse success for: \"" << text << "\"";
        return value.value_or(0);
    }

    static void ExpectFail(const char* text, PUnitSystem system = PUnitSystem::Auto)
    {
        std::optional<int64_t> value = Parse(text, system);
        EXPECT_FALSE(value.has_value()) << "Expected parse failure for: \"" << text << "\"";
    }
};

TEST_F(PString_ParseFileSize_Tests, Bytes_NoSuffix_Integer)
{
    EXPECT_EQ(ParseOrFail("0"), 0);
    EXPECT_EQ(ParseOrFail("42"), 42);
    EXPECT_EQ(ParseOrFail("-42"), -42);

    EXPECT_EQ(ParseOrFail("   42"), 42);
    EXPECT_EQ(ParseOrFail("42   "), 42);
    EXPECT_EQ(ParseOrFail(" \t  -42 \t "), -42);
}

TEST_F(PString_ParseFileSize_Tests, Auto_Mode_K_KB_KiB_Semantics)
{
    // Auto: K and KiB are IEC (1024), KB is SI (1000)
    EXPECT_EQ(ParseOrFail("10K", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10KiB", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10KB", PUnitSystem::Auto), 10 * 1000LL);

    // Case-insensitivity for suffix (your implementation upper()s suffix)
    EXPECT_EQ(ParseOrFail("10k", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10kib", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10kb", PUnitSystem::Auto), 10 * 1000LL);
}

TEST_F(PString_ParseFileSize_Tests, Forced_SI_Mode_Forces_1000_Base)
{
    EXPECT_EQ(ParseOrFail("10K", PUnitSystem::SI), 10 * 1000LL);
    EXPECT_EQ(ParseOrFail("10KB", PUnitSystem::SI), 10 * 1000LL);
    EXPECT_EQ(ParseOrFail("10KiB", PUnitSystem::SI), 10 * 1000LL);

    EXPECT_EQ(ParseOrFail("1M", PUnitSystem::SI), 1'000'000LL);
    EXPECT_EQ(ParseOrFail("1MB", PUnitSystem::SI), 1'000'000LL);
    EXPECT_EQ(ParseOrFail("1MiB", PUnitSystem::SI), 1'000'000LL);
}

TEST_F(PString_ParseFileSize_Tests, Forced_IEC_Mode_Forces_1024_Base)
{
    EXPECT_EQ(ParseOrFail("10K", PUnitSystem::IEC), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10KB", PUnitSystem::IEC), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10KiB", PUnitSystem::IEC), 10 * 1024LL);

    EXPECT_EQ(ParseOrFail("1M", PUnitSystem::IEC), (1LL << 20));
    EXPECT_EQ(ParseOrFail("1MB", PUnitSystem::IEC), (1LL << 20));
    EXPECT_EQ(ParseOrFail("1MiB", PUnitSystem::IEC), (1LL << 20));
}

TEST_F(PString_ParseFileSize_Tests, Whitespace_Between_Number_And_Suffix_Is_Allowed)
{
    EXPECT_EQ(ParseOrFail("10 K", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("10   KB", PUnitSystem::Auto), 10 * 1000LL);
    EXPECT_EQ(ParseOrFail("10\tKiB", PUnitSystem::Auto), 10 * 1024LL);
    EXPECT_EQ(ParseOrFail("  10   KB  ", PUnitSystem::Auto), 10 * 1000LL);
}

TEST_F(PString_ParseFileSize_Tests, Fractional_Rounding_Auto)
{
    // Auto + K => IEC: 1.2 * 1024 = 1228.8 => round => 1229
    EXPECT_EQ(ParseOrFail("1.2K", PUnitSystem::Auto), 1229);

    // Auto + KB => SI: 1.2 * 1000 = 1200 exactly
    EXPECT_EQ(ParseOrFail("1.2KB", PUnitSystem::Auto), 1200);

    // 0.5K => 512 exactly
    EXPECT_EQ(ParseOrFail("0.5K", PUnitSystem::Auto), 512);

    // Negative rounding: -1.2K => -1228.8 => -1229 (half away from zero)
    EXPECT_EQ(ParseOrFail("-1.2K", PUnitSystem::Auto), -1229);
    EXPECT_EQ(ParseOrFail("-0.5K", PUnitSystem::Auto), -512);
}

TEST_F(PString_ParseFileSize_Tests, Fractional_Forced_SI_And_IEC)
{
    // Forced IEC: KB treated as 1024
    EXPECT_EQ(ParseOrFail("1.2KB", PUnitSystem::IEC), 1229);

    // Forced SI: K treated as 1000
    EXPECT_EQ(ParseOrFail("1.2K", PUnitSystem::SI), 1200);
}

TEST_F(PString_ParseFileSize_Tests, ScientificNotation_IsRejected)
{
    // You decided not to support exponent notation.
    ExpectFail("1e3");
    ExpectFail("1e3K");
    ExpectFail("9.223372036854776e18");
}

TEST_F(PString_ParseFileSize_Tests, Invalid_Strings_Fail)
{
    ExpectFail("");
    ExpectFail("   ");
    ExpectFail("K");
    ExpectFail("KB");
    ExpectFail("KiB");
    ExpectFail("1..2K");
    ExpectFail("1.2.3K");
    ExpectFail("1-2K");     // invalid numeric
    ExpectFail("1.2Ki");    // missing 'B'
    ExpectFail("1XB");      // unknown prefix
    ExpectFail("1 K B");    // space inside suffix (doesn't match KB)
}

TEST_F(PString_ParseFileSize_Tests, Handles_Int64Min_IntegerPath)
{
    const int64_t v = std::numeric_limits<int64_t>::min();
    EXPECT_EQ(ParseOrFail("-9223372036854775808"), v);
}

TEST_F(PString_ParseFileSize_Tests, Overflow_Fails)
{
    // One more than INT64_MAX
    ExpectFail("9223372036854775808");

    // Clearly too big after scaling
    ExpectFail("9223372036854775807K"); // Auto: K => *1024 => overflow
    ExpectFail("9223372036854775807KB"); // Auto: KB => *1000 => overflow
}

TEST_F(PString_ParseFileSize_Tests, ExaSuffix_E_IsParsedAsSuffix_NotExponent)
{
    // Ensure 'E' is treated as suffix (since scientific notation is not supported).
    EXPECT_EQ(ParseOrFail("1E", PUnitSystem::IEC), (1LL << 60));              // 1 * 1024^6
    EXPECT_EQ(ParseOrFail("1EB", PUnitSystem::Auto), 1'000'000'000'000'000'000LL); // 1 * 1000^6
}
