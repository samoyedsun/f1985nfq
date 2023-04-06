#include <iostream>
#include <boost/asio.hpp>

#include "hello.pb.h"
#include <windows.h> 
using boost::asio::ip::tcp;

#define MAX_PACKAGE_SIZE 1024 * 1024

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

    bool send(void* data_ptr, int32_t size)
    {
        if (m_status != ENET_STATUS_CONNECTED)
        {
            std::cout << "Have not connected success!" << std::endl;
            return false;
        }
        boost::asio::write(m_socket, boost::asio::buffer(data_ptr, size));
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
        void* data_ptr = malloc(MAX_PACKAGE_SIZE);
        if (!data_ptr)
        {
            return 1;
        }
        char* offset = (char*)data_ptr;

        *(uint16_t*)offset = 10000;
        offset += sizeof(uint16_t);

        *(uint16_t*)offset = data.ByteSize();
        offset += sizeof(uint16_t);

        uint32_t package_size = data.ByteSize() + sizeof(uint16_t) + sizeof(uint16_t);
        if (!data.SerializePartialToArray(offset, MAX_PACKAGE_SIZE - sizeof(uint16_t) - sizeof(uint16_t)))
        {
            std::cout << "serialize fail!" << std::endl;
            return 1;
        }
        net_worker.send(data_ptr, package_size);
    }

    return 0;
}
