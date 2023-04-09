#include "data_packet.h"
#include <iostream>;
#include <cassert>;

#define MIN_MEM_SIZE 64

data_packet::data_packet()
    : m_mem_ptr(nullptr)
    , m_end_ptr(nullptr)
    , m_offset(nullptr)
{
    m_mem_ptr = (char*)malloc(MIN_MEM_SIZE);
    m_end_ptr = m_mem_ptr + MIN_MEM_SIZE;
    m_offset = m_mem_ptr;
}

data_packet::~data_packet()
{
    if (m_mem_ptr)
    {
        free(m_mem_ptr);
        m_mem_ptr = nullptr;
        m_end_ptr = nullptr;
        m_offset = nullptr;
    }
}

data_packet& data_packet::operator = (const data_packet&)
{
    assert(false);
    return *this;
}

void* data_packet::write_data(const char* data_ptr, size_t size)
{
    size_t new_size = m_offset - m_mem_ptr + size;
    size_t total_size = m_end_ptr - m_mem_ptr;
    if (new_size > total_size)
    {
        if (!resize(new_size))
        {
            return nullptr;
        }
    }
    char* offset = m_offset;
    if (data_ptr)
    {
        memcpy(offset, data_ptr, size);
    }
    m_offset = m_mem_ptr + new_size;
    return offset;
}

size_t data_packet::size()
{
    return m_offset - m_mem_ptr;
}

char* data_packet::get_mem_ptr()
{
    return m_mem_ptr;
}

bool data_packet::resize(size_t new_size)
{
    size_t size = m_offset - m_mem_ptr;
    size_t realloc_size = (m_end_ptr - m_mem_ptr) * 2;
    if (realloc_size > new_size)
    {
        new_size = realloc_size;
    }
    char* mem_ptr = (char*)realloc(m_mem_ptr, new_size);
    if (!mem_ptr)
    {
        return false;
    }
    m_mem_ptr = mem_ptr;
    m_end_ptr = mem_ptr + new_size;
    m_offset = mem_ptr + size;
    return true;
}
