#ifndef _DATA_PACKET_H_
#define _DATA_PACKET_H_

class data_packet
{
public:
    data_packet();

    ~data_packet();

    data_packet& operator = (const data_packet&);

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

    void* write_data(const char* data_ptr, size_t size);

    size_t size();

    char* get_mem_ptr();

private:
    bool resize(size_t new_size);

private:
    char* m_mem_ptr;
    char* m_end_ptr;
    char* m_offset;
};

#endif