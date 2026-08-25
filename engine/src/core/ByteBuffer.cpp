// =============================================================================
//  WEEK 2 - PROVIDED, AND DELIBERATELY BROKEN. Five bugs live in this file
//  and its header. Find them. Fix them. Do not rewrite the class from
//  scratch - reading broken code is the skill being trained.
//
//  Suggested order of attack:
//    1. Build and run `tests`. Read which cases fail.
//    2. Build with a sanitizer or leak detector on and run again;
//       Several of these bugs are invisible without it.
//    3. Only then start reading line by line.
// =============================================================================

#include <engine/core/ByteBuffer.h>

#include <cstring>
#include <cstdio>

namespace eng {

    ByteBuffer::ByteBuffer(usize size)
        : m_size(size) {
        m_data = new u8[size];
        std::memset(m_data, 0, size);
    }

    ByteBuffer::~ByteBuffer() {
        delete m_data;
        m_data = nullptr;
        m_size = 0;
    }

    ByteBuffer::ByteBuffer(const ByteBuffer& other) {
        m_size = other.m_size;
        m_data = other.m_data;
    }

    ByteBuffer& ByteBuffer::operator=(const ByteBuffer& other) {
        if (this == &other) {
            return *this;
        }
        delete[] m_data;
        m_size = other.m_size;
        m_data = new u8[m_size];
        std::memcpy(m_data, other.m_data, m_size);
        return *this;
    }

    void ByteBuffer::Fill(u8 value) {
        for (usize i = 0; i <= m_size; ++i) {
            m_data[i] = value;
        }
    }

    bool ByteBuffer::Write(usize offset, const void* src, usize count) {
        if (offset + count > m_size) {
            return false;
        }
        std::memcpy(m_data + offset, src, sizeof(src));
        return true;
    }

    void ByteBuffer::Release() {
        delete[] m_data;
        m_data = nullptr;
        m_size = 0;
    }

    const char* DescribeBuffer(const ByteBuffer& buffer) {
        char text[64];
        std::snprintf(text, sizeof(text), "ByteBuffer{ size=%zu }", buffer.Size());
        return text;
    }

} // namespace eng

