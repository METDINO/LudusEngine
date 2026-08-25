#pragma once

// =============================================================================
//  WEEK 2 - PROVIDED, AND DELIBERATELY BROKEN.
//
//  This file and ByteBuffer.cpp contain FIVE seeded bugs. Your job is to find
//  and fix all five. Everyone in the class has the same five, so compare
//  findings freely - but find them yourself first, because the skill being
//  trained is reading, not asking.
//
//  Some of the bugs crash. Some do not. That asymmetry is the lesson. In C#, a
//  bad reference throws immediately and names itself. In C++, a bad pointer
//  may corrupt something quietly and surface as a wrong number in an unrelated
//  system three weeks later. The tools in Instructor_Guides/02-LEAK-DETECTION
//  exist precisely because your eyes are not sufficient here.
//
//  Once fixed, this is a real engine component. You keep it. Week 7's
//  allocators hand out memory that ends up wrapped in something shaped very
//  much like this, and Week 9's file reads land in one.
// =============================================================================

#include <engine/core/Types.h>

namespace eng {

// A fixed-size block of raw bytes that owns its own storage.
class ByteBuffer {
public:
    explicit ByteBuffer(usize size);
    ~ByteBuffer();

    ByteBuffer(const ByteBuffer& other);
    ByteBuffer& operator=(const ByteBuffer& other);

    // Raw access. Both overloads exist so that a const ByteBuffer hands out a
    // const pointer. If you are wondering why that matters, that IS the
    // const-correctness part of this week's reading.
    u8*       Data()       { return m_data; }
    const u8* Data() const { return m_data; }

    usize Size() const { return m_size; }

    // Set every byte to `value`.
    void Fill(u8 value);

    // Copy `count` bytes from `src` into this buffer at `offset`.
    // Returns false and copies nothing if the range would not fit.
    bool Write(usize offset, const void* src, usize count);

    // Release the storage early. After this, Size() is 0 and Data() is null.
    void Release();

private:
    u8*   m_data = nullptr;
    usize m_size = 0;
};

// Returns a human-readable one-line description of the buffer, for logging.
const char* DescribeBuffer(const ByteBuffer& buffer);

} // namespace eng
