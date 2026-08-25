#include <engine/core/MemUtil.h>

namespace eng {

// -----------------------------------------------------------------------------
//    target -> Vec2&        non-const reference. We modify it in place, and
//                           the caller must see the change.
//    rhs    -> const Vec2&  reference so we do not copy; const because we do
//                           not modify it, and because it lets the caller pass
//                           a temporary.
//
//  For a type this small, `Vec2 rhs` by value would be just as fast; possibly
//  faster, since a reference is itself a pointer that has to be dereferenced.
//
//  Coming from C#: had Vec2 been a class there, both parameters would be
//  references automatically and `rhs` would be modifiable. Had it been a
//  struct, both would be copies and `target` could not be modified at all. C++
//  makes you choose, per parameter, every time.
// -----------------------------------------------------------------------------
void Accumulate(Vec2& target, const Vec2& rhs) {
    target.x += rhs.x;
    target.y += rhs.y;
}

// -----------------------------------------------------------------------------
//  Note what makes this work: `a` and `b` are references, so assigning through
//  them writes to the caller's variables. Without the `&` this function would
//  swap two local copies and do nothing observable; a bug that compiles
//  cleanly, runs happily, and produces no output at all.
//
//  The header asks what would have to change to make this work for Vec2 too.
//  The answer is a TEMPLATE - one function that works for any type, generated
//  per type at compile time. 
// -----------------------------------------------------------------------------
void SwapI32(i32& a, i32& b) {
    const i32 temp = a;
    a = b;
    b = temp;
}

bool ReverseBytes(u8* data, usize count) {
    if (data == nullptr) {
        return false;
    }

    // A count of 0 is not an error - it is a request to reverse nothing, and
    // the correct response is to succeed having done nothing. Deciding this
    // BEFORE writing the loop is why the header asks you to write the test
    // first.
    if (count == 0) {
        return true;
    }

    // Two indices walking toward each other. `back` starts at count - 1, which
    // is why the count == 0 check above has to come first: with count 0 and
    // unsigned arithmetic, count - 1 wraps to an enormous number and the loop
    // reads far off the end.
    //
    // That wrap is the single most likely bug in this function, and it is
    // invisible in a debug build until a sanitizer sees it.
    usize front = 0;
    usize back  = count - 1;

    while (front < back) {
        const u8 temp = data[front];
        data[front]   = data[back];
        data[back]    = temp;
        ++front;
        --back;
    }

    // An odd count leaves the middle byte where it is, which is correct -
    // front and back meet on it and the loop condition `front < back` stops.
    return true;
}

usize CountBytes(const u8* data, usize count, u8 value) {
    // `const u8*` because we only read. That is not decoration: it means a
    // caller holding a const buffer can call this, and it means the compiler
    // rejects an accidental write.
    if (data == nullptr) {
        return 0;
    }

    usize found = 0;
    for (usize i = 0; i < count; ++i) {
        if (data[i] == value) {
            ++found;
        }
    }
    return found;
}

// -----------------------------------------------------------------------------
//  The one worth doing carefully.
//
//  std::memcpy is UNDEFINED BEHAVIOUR if the ranges overlap - not "usually
//  works", not "copies garbage", undefined. It is allowed to assume no overlap
//  and copy in whatever order or width is fastest, which on a modern CPU may
//  be sixteen bytes at a time and possibly backwards. std::memmove is the one
//  that promises correct behaviour on overlap.
//
//  The insight: WHICH DIRECTION YOU COPY IN DECIDES WHETHER YOU ARE CORRECT.
//
//    dst < src   copy FORWARD.  Each byte is read before the write that would
//                clobber it, because writes trail behind reads.
//
//    dst > src   copy BACKWARD. Copying forward here would overwrite bytes of
//                the source that have not been read yet.
//
//    dst == src  nothing to do.
//
//  Getting one direction right does not get you the other, which is why the
//  header asks for both cases as separate tests.
// -----------------------------------------------------------------------------
bool CopyOverlapping(u8* dst, const u8* src, usize count) {
    if (dst == nullptr || src == nullptr) {
        return false;
    }

    if (count == 0 || dst == src) {
        return true;
    }

    if (dst < src) {
        // Forward: writes trail reads.
        for (usize i = 0; i < count; ++i) {
            dst[i] = src[i];
        }
    } else {
        // Backward: start at the far end and walk down.
        //
        // Written as `i` counting down from count to 1 and indexing with
        // `i - 1`, rather than starting at count - 1 and testing `i >= 0`.
        // With an unsigned index, `i >= 0` is ALWAYS TRUE and the loop never
        // ends - it just runs off the front of the buffer. That is a real and
        // very common bug, and the compiler will not warn about it.
        for (usize i = count; i > 0; --i) {
            dst[i - 1] = src[i - 1];
        }
    }

    return true;
}

} // namespace eng
