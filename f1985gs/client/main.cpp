#include <iostream>
#include <boost/asio.hpp>
#include <google/protobuf/message_lite.h>
#include "hello.pb.h"
#include <windows.h> 
using boost::asio::ip::tcp;

class data_packet
{
public:
    data_packet()
        : m_mem_ptr(nullptr)
        , m_end_ptr(nullptr)
        , m_offset(nullptr)
    {
        m_mem_ptr = (char*)malloc(MIN_MEM_SIZE);
        m_end_ptr = m_mem_ptr + MIN_MEM_SIZE;
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
        assert(false); // ½ûÖ¹ÒýÓÃ¸³Öµ
    }

    template <typename T>
    data_packet& operator << (const T& val)
    {
        size_t new_size = (m_offset - m_mem_ptr) + sizeof(T);
        size_t total_size = m_end_ptr - m_mem_ptr;
        if (new_size > total_size)
        {
            if (resize(new_size))
            {
                *(T*)m_offset = val;
                m_offset = m_mem_ptr + new_size;
            }
        }
        else
        {
            *(T*)m_offset = val;
            m_offset = m_mem_ptr + new_size;
        }
        return *this;
    }

    void* write_data(const char* data_ptr, size_t size)
    {
        size_t new_size = m_offset - m_mem_ptr + size;
        size_t total_size = m_end_ptr - m_mem_ptr;
        if (new_size > total_size)
        {
            if (resize(new_size))
            {
                if (data_ptr)
                {
                    memcpy(m_offset, data_ptr, size);
                }
                m_offset = m_mem_ptr + new_size;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            if (data_ptr)
            {
                memcpy(m_offset, data_ptr, size);
            }
            m_offset = m_mem_ptr + new_size;
        }
        return m_offset - size;
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

private:
    char* m_mem_ptr;
    char* m_end_ptr;
    char* m_offset;
    static const int32_t MIN_MEM_SIZE = 64;
};

class net_worker
{
    enum ENET_STATUS
    {
        ENET_STATUS_UNCONNECT,
        ENET_STATUS_CONNECTED,

    };
public:
    net_worker()
        : m_socket(m_io_context)
        , m_recv_size(0)
        , m_max_recv_size(0)
        , m_reconnect_interval(0)
        , m_port(0)
        , m_status(ENET_STATUS_UNCONNECT)
    {
    }

    void max_recv_size(int32_t max_recv_size)
    {
        m_max_recv_size = max_recv_size;
    }

    int32_t max_recv_size()
    {
        return m_max_recv_size;
    }

    void set_address(const char* ip, int32_t port)
    {
        m_ip = ip;
        m_port = port;
    }

    void run()
    {
        start_connect();
        m_io_context.run();
    }

    bool send(uint16_t msg_id, google::protobuf::MessageLite& message_lite)
    {
        if (m_status != ENET_STATUS_CONNECTED)
        {
            std::cout << "Have not connected success!" << std::endl;
            return false;
        }
        data_packet data_packet;
        data_packet << (uint16_t)msg_id;
        data_packet << (uint16_t)message_lite.ByteSize();
        void* ptr = data_packet.write_data(nullptr, message_lite.ByteSize());
        if (!message_lite.SerializePartialToArray(ptr, message_lite.ByteSize()))
        {
            std::cout << "serialize fail!" << std::endl;
            return false;
        }
        boost::asio::write(m_socket, boost::asio::buffer(data_packet.get_mem_ptr(), data_packet.size()));
        return true;
    }

private:
    void start_connect()
    {
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_ip), 55890);
        m_socket.async_connect(endpoint, [this](const boost::system::error_code& ec)
            {
                if (ec)
                {
                    std::cout << "Connect failed:" << ec.message() << std::endl;
                    return;
                }
                m_status = ENET_STATUS_CONNECTED;
                std::cout << "Connected!" << std::endl;
            });
    }

    void close()
    {

    }

private:
    boost::asio::io_context m_io_context;
    tcp::socket m_socket;
    int32_t m_recv_size;
    int32_t m_max_recv_size;
    uint16_t m_reconnect_interval;
    uint16_t m_port;
    std::string m_ip;
    ENET_STATUS m_status;
};

int main()
{
    net_worker net_worker;
    net_worker.max_recv_size(1024 * 1024);
    net_worker.set_address("127.0.0.1", 55890);
    net_worker.run();

    while (true)
    {
        Sleep(1000);
        std::cout << "waiting 1s" << std::endl;

        Hello data;
        data.set_id(100);
        data.add_member(3434);
        net_worker.send(10000, data);
    }

    return 0;
}
