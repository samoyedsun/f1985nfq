#ifndef _DATA_PACKET_H_
#define _DATA_PACKET_H_
#include <iostream>;
#include <cassert>;

class data_packet
{
public:
    data_packet()
        : m_mem_ptr(nullptr)
        , m_end_ptr(nullptr)
        , m_offset(nullptr)
    {
        m_mem_ptr = (char*)malloc(min_mem_size);
        m_end_ptr = m_mem_ptr + min_mem_size;
        m_offset = m_mem_ptr;
    }

    ~data_packet()
    {
        if (m_mem_ptr)
        {
            free(m_mem_ptr);
            m_mem_ptr = nullptr;
            m_end_ptr = nullptr;
            m_offset = nullptr;
        }
    }

    data_packet& operator = (const data_packet&)
    {
        assert(false);
        return *this;
    }

    template <typename T>
    data_packet& operator << (const T& val)
    {
        size_t new_size = (m_offset - m_mem_ptr) + sizeof(T);
        size_t total_size = m_end_ptr - m_mem_ptr;
        if (new_size > total_size)
        {
            if (!resize(new_size))
            {
                return *this;
            }
        }
        *(T*)m_offset = val;
        m_offset = m_mem_ptr + new_size;
        return *this;
    }

    void* write_data(const char* data_ptr, size_t size)
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

    size_t size()
    {
        return m_offset - m_mem_ptr;
    }

    char* get_mem_ptr()
    {
        return m_mem_ptr;
    }

private:
    bool resize(size_t new_size)
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

    static const int32_t min_mem_size = 64;

private:
    char* m_mem_ptr;
    char* m_end_ptr;
    char* m_offset;
};

#endif