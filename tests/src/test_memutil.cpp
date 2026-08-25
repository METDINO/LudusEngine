#include <doctest/doctest.h>
#include <engine/core/MemUtil.h>

#include <array>

using namespace eng;

// =============================================================================
//  Accumulate
// =============================================================================

TEST_CASE("Accumulate adds into the target in place") {
    Vec2 target{1.0f, 2.0f};
    const Vec2 rhs{10.0f, 20.0f};

    Accumulate(target, rhs);

    CHECK(target.x == doctest::Approx(11.0f));
    CHECK(target.y == doctest::Approx(22.0f));
    CHECK(rhs.x    == doctest::Approx(10.0f));   // source unchanged
}

TEST_CASE("Accumulate with a zero vector changes nothing") {
    Vec2 target{3.0f, -4.0f};
    Accumulate(target, Vec2{0.0f, 0.0f});
    CHECK(target.x == doctest::Approx(3.0f));
    CHECK(target.y == doctest::Approx(-4.0f));
}

TEST_CASE("Accumulate is cumulative across calls") {
    // Catches an implementation that ASSIGNS instead of adding - which passes
    // the first test above if the target happens to start at zero.
    Vec2 target{0.0f, 0.0f};
    const Vec2 step{1.0f, 2.0f};

    for (int i = 0; i < 5; ++i) {
        Accumulate(target, step);
    }

    CHECK(target.x == doctest::Approx(5.0f));
    CHECK(target.y == doctest::Approx(10.0f));
}

// =============================================================================
//  SwapI32
// =============================================================================

TEST_CASE("SwapI32 exchanges two values") {
    i32 a = 7;
    i32 b = -3;
    SwapI32(a, b);
    CHECK(a == -3);
    CHECK(b == 7);
}

TEST_CASE("SwapI32 of a variable with itself leaves it intact") {
    // The classic self-swap. A naive implementation still works here, but the
    // case is worth having: the equivalent self-assignment in ByteBuffer's
    // operator= is a real bug, and the habit of testing aliasing transfers.
    i32 a = 42;
    SwapI32(a, a);
    CHECK(a == 42);
}

// =============================================================================
//  ReverseBytes
// =============================================================================

TEST_CASE("ReverseBytes reverses an even-length range") {
    std::array<u8, 4> data{1, 2, 3, 4};
    REQUIRE(ReverseBytes(data.data(), data.size()));
    CHECK(data[0] == 4);
    CHECK(data[1] == 3);
    CHECK(data[2] == 2);
    CHECK(data[3] == 1);
}

TEST_CASE("ReverseBytes leaves the middle byte of an odd-length range in place") {
    std::array<u8, 5> data{1, 2, 3, 4, 5};
    REQUIRE(ReverseBytes(data.data(), data.size()));
    CHECK(data[0] == 5);
    CHECK(data[1] == 4);
    CHECK(data[2] == 3);      // the pivot
    CHECK(data[3] == 2);
    CHECK(data[4] == 1);
}

TEST_CASE("ReverseBytes of a single byte succeeds and changes nothing") {
    u8 one = 9;
    CHECK(ReverseBytes(&one, 1));
    CHECK(one == 9);
}

TEST_CASE("ReverseBytes with a count of zero succeeds without touching memory") {
    // THE CASE THAT CATCHES THE REAL BUG. With count 0 and unsigned
    // arithmetic, `count - 1` wraps to a colossal number. An implementation
    // that computes `back = count - 1` before checking for zero walks off the
    // buffer immediately.
    //
    // The guard bytes below try to notice. The sanitizer definitely will.
    std::array<u8, 3> data{0xAA, 0xBB, 0xCC};
    CHECK(ReverseBytes(data.data(), 0));
    CHECK(data[0] == 0xAA);
    CHECK(data[1] == 0xBB);
    CHECK(data[2] == 0xCC);
}

TEST_CASE("ReverseBytes rejects a null pointer") {
    CHECK_FALSE(ReverseBytes(nullptr, 4));
    CHECK_FALSE(ReverseBytes(nullptr, 0));
}

TEST_CASE("ReverseBytes twice returns the original") {
    // A round-trip property test. Cheap to write, and it catches a whole class
    // of indexing mistakes without you having to predict which one.
    std::array<u8, 7> data{1, 2, 3, 4, 5, 6, 7};
    const auto original = data;

    REQUIRE(ReverseBytes(data.data(), data.size()));
    REQUIRE(ReverseBytes(data.data(), data.size()));

    CHECK(data == original);
}

// =============================================================================
//  CountBytes
// =============================================================================

TEST_CASE("CountBytes counts every match") {
    const std::array<u8, 6> data{1, 7, 7, 3, 7, 9};
    CHECK(CountBytes(data.data(), data.size(), 7) == 3);
}

TEST_CASE("CountBytes returns zero when the value is absent") {
    const std::array<u8, 3> data{1, 2, 3};
    CHECK(CountBytes(data.data(), data.size(), 99) == 0);
}

TEST_CASE("CountBytes respects the count rather than the buffer size") {
    // Only the first two bytes are in range, so the third 7 must not be seen.
    // Catches an implementation that ignores `count` and runs to some other
    // terminator.
    const std::array<u8, 3> data{7, 7, 7};
    CHECK(CountBytes(data.data(), 2, 7) == 2);
}

TEST_CASE("CountBytes with a count of zero returns zero") {
    const std::array<u8, 3> data{7, 7, 7};
    CHECK(CountBytes(data.data(), 0, 7) == 0);
}

TEST_CASE("CountBytes returns zero for a null pointer") {
    CHECK(CountBytes(nullptr, 10, 0) == 0);
}

// =============================================================================
//  CopyOverlapping - the one worth testing properly
// =============================================================================

TEST_CASE("CopyOverlapping handles non-overlapping ranges") {
    const std::array<u8, 4> src{1, 2, 3, 4};
    std::array<u8, 4>       dst{0, 0, 0, 0};

    REQUIRE(CopyOverlapping(dst.data(), src.data(), src.size()));
    CHECK(dst[0] == 1);
    CHECK(dst[1] == 2);
    CHECK(dst[2] == 3);
    CHECK(dst[3] == 4);
}

TEST_CASE("CopyOverlapping shifts LEFT correctly (dst < src)") {
    // Requires a FORWARD copy. Writes trail reads, so nothing is clobbered
    // before it is read.
    std::array<u8, 6> data{1, 2, 3, 4, 5, 6};

    REQUIRE(CopyOverlapping(data.data(), data.data() + 2, 4));

    CHECK(data[0] == 3);
    CHECK(data[1] == 4);
    CHECK(data[2] == 5);
    CHECK(data[3] == 6);
    // Bytes 4 and 5 are outside the destination range and must be untouched.
    CHECK(data[4] == 5);
    CHECK(data[5] == 6);
}

TEST_CASE("CopyOverlapping shifts RIGHT correctly (dst > src)") {
    // Requires a BACKWARD copy. A forward loop here overwrites source bytes
    // before reading them, producing 1,2,1,1,1,1 - the classic smear.
    //
    // This is the case that catches an implementation which got the other
    // direction right and assumed it was done.
    std::array<u8, 6> data{1, 2, 3, 4, 5, 6};

    REQUIRE(CopyOverlapping(data.data() + 2, data.data(), 4));

    CHECK(data[0] == 1);
    CHECK(data[1] == 2);
    CHECK(data[2] == 1);
    CHECK(data[3] == 2);
    CHECK(data[4] == 3);
    CHECK(data[5] == 4);
}

TEST_CASE("CopyOverlapping with fully overlapping identical pointers is a no-op") {
    std::array<u8, 3> data{1, 2, 3};
    REQUIRE(CopyOverlapping(data.data(), data.data(), 3));
    CHECK(data[0] == 1);
    CHECK(data[1] == 2);
    CHECK(data[2] == 3);
}

TEST_CASE("CopyOverlapping with a count of zero succeeds and copies nothing") {
    const std::array<u8, 2> src{9, 9};
    std::array<u8, 2>       dst{1, 2};

    CHECK(CopyOverlapping(dst.data(), src.data(), 0));
    CHECK(dst[0] == 1);
    CHECK(dst[1] == 2);
}

TEST_CASE("CopyOverlapping rejects null pointers") {
    const std::array<u8, 2> src{1, 2};
    std::array<u8, 2>       dst{0, 0};

    CHECK_FALSE(CopyOverlapping(nullptr, src.data(), 2));
    CHECK_FALSE(CopyOverlapping(dst.data(), nullptr, 2));
    CHECK_FALSE(CopyOverlapping(nullptr, nullptr, 0));
}
