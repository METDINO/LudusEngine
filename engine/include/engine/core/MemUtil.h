#pragma once
#include <engine/core/Types.h>

namespace eng {

// --- Minimal 2D vector ------------------------------------------------------
// A placeholder. Week 6 replaces this with the real, fully tested Vec2 that
// carries the whole engine. It lives here now because you need SOMETHING to
// pass by reference this week, and because the difference between passing this
// by value and passing it by reference is exactly the point.
struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

// TODO(week2): add `rhs` into `target`, modifying `target` in place.
//   Signature question you must answer before writing it: which parameter is
//   a reference, which is const, and why? In C# both would be references
//   automatically if Vec2 were a class, and neither would be if it were a
//   struct. In C++ you choose, per parameter, every time.
void Accumulate(Vec2& target, const Vec2& rhs);

// TODO(week2): swap the contents of two objects of the same type.
//   Write it for `i32` first and get it working. Then ask yourself what would
//   have to change to make it work for Vec2 as well, and write that down in a
//   comment. You will get the real answer (templates) in Week 8.
void SwapI32(i32& a, i32& b);

// TODO(week2): reverse `count` bytes starting at `data`, in place.
//   Returns false if `data` is null. A count of 0 is not an error.
bool ReverseBytes(u8* data, usize count);

// TODO(week2): return the number of bytes equal to `value` in the range
//   [data, data + count). Returns 0 for a null pointer.
usize CountBytes(const u8* data, usize count, u8 value);

// TODO(week2): copy `count` bytes from `src` to `dst`, correctly, even when
//   the two ranges OVERLAP. Read what std::memcpy promises about overlap
//   before you start, then read what std::memmove promises. Implement it
//   yourself rather than calling either - the exercise is the pointer
//   arithmetic and the direction of the loop.
//   Returns false if either pointer is null.
bool CopyOverlapping(u8* dst, const u8* src, usize count);

} // namespace eng
