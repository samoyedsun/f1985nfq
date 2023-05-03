#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>
#include <map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "data_packet.h"

using boost::asio::ip::tcp;

class net_worker
{
    enum ENET_STATUS
    {
        ENET_STATUS_UNCONNECT,
        ENET_STATUS_CONNECTED,
    };
    using msg_handler = std::function<bool(void*, uint16_t)>;
public:
    net_worker(boost::asio::io_context& context)
        : m_socket(context)
        , m_recv_buf_ptr(nullptr)
        , m_recv_size(0)
        , m_max_recv_size(0)
        , m_reconnect_interval(0)
        , m_port(0)
        , m_status(ENET_STATUS_UNCONNECT)
        , m_timer(context, boost::posix_time::milliseconds(5))
    {
    }

    ~net_worker()
    {
        free(m_recv_buf_ptr);
    }

    void init(int32_t max_recv_size)
    {
        m_max_recv_size = max_recv_size;
        m_recv_buf_ptr = malloc(m_max_recv_size);
    }

    void set_address(const char* ip, int32_t port)
    {
        m_ip = ip;
        m_port = port;
    }

    void start()
    {
        start_connect();
    }

    bool send(uint16_t msg_id, char *data_ptr, uint16_t size)
    {
        if (m_status != ENET_STATUS_CONNECTED)
        {
            std::cout << "Have not connected success!" << std::endl;
            return false;
        }
        data_packet data_packet;
        data_packet << (uint16_t)msg_id;
        data_packet << (uint16_t)size;
        data_packet.write_data(data_ptr, size);
        boost::asio::write(m_socket, boost::asio::buffer(data_packet.get_mem_ptr(), data_packet.size()));
        return true;
    }

    void register_msg(uint16_t msg_id, msg_handler msg_handler)
    {
        m_msg_handler_umap.try_emplace(msg_id, msg_handler);
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
                    close();
                    return;
                }
                m_status = ENET_STATUS_CONNECTED;
                std::cout << "Connected!" << std::endl;
                read_start();
            });
    }

    void read_start()
    {
        void* offset_ptr = (void*)((int8_t*)m_recv_buf_ptr + m_recv_size);
        int32_t left_size = m_max_recv_size - m_recv_size;
        m_socket.async_read_some(boost::asio::buffer(offset_ptr, left_size),
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
            {
                if (ec || bytes_transferred == 0)
                {
                    std::cout << "data :" << ec.message() << std::endl;
                    return close();
                }
                m_recv_size += static_cast<int32_t>(bytes_transferred);
                parse_data(bytes_transferred);
                read_start();
            });
    }

    void parse_data(size_t bytes_transferred)
    {
        if (m_recv_size < 4)
        {
            std::cout << "data package too short, recv_size:" << m_recv_size
                << ", bytes_transferred:" << bytes_transferred << std::endl;
            close();
        }
        do
        {
            void* offset_ptr = m_recv_buf_ptr;
            uint16_t msg_id = *(uint16_t*)offset_ptr;
            offset_ptr = (void*)((int8_t*)offset_ptr + 2);
            uint16_t body_size = *(uint16_t*)offset_ptr;
            offset_ptr = (void*)((int8_t*)offset_ptr + 2);
            void* body_ptr = offset_ptr;
            offset_ptr = (void*)((int8_t*)offset_ptr + body_size);
            uint32_t package_size = body_size + sizeof(msg_id) + sizeof(body_size);
            if (package_size > m_recv_size)
            {
                std::cout << "not enough to read, package_size:" << package_size
                    << ", m_recv_size:" << m_recv_size << std::endl;
                break;
            }
            if (!parse_msg(msg_id, body_ptr, body_size))
            {
                std::cout << "parse msg error, msg_id:" << msg_id
                    << " size:" << body_size << std::endl;
                close();
                return;
            }
            m_recv_size -= package_size;
            if (m_recv_size > 0)
            {
                memcpy(m_recv_buf_ptr, offset_ptr, m_recv_size);
            }
        } while (m_recv_size >= 4);
    }

    bool parse_msg(uint16_t msg_id, void* data_ptr, uint16_t size)
    {
        auto msg_handler_it = m_msg_handler_umap.find(msg_id);
        if (msg_handler_it == m_msg_handler_umap.end())
        {
            return false;
        }
        return msg_handler_it->second(data_ptr, size);
    }

    void close()
    {
        m_status = ENET_STATUS_UNCONNECT;
        if (m_socket.is_open())
        {
            m_socket.close();
        }
        std::cout << "closed connection, after 5s reconnect." << std::endl;

        m_timer.expires_from_now(boost::posix_time::seconds(5));
        m_timer.async_wait(boost::bind(&net_worker::handle_reconnect, this, boost::asio::placeholders::error));
    }

    void handle_reconnect(const boost::system::error_code& ec)
    {
        if (ec)
        {
            std::cout << "reconnect error." << ec.message() << std::endl;
            return;
        }
        start_connect();
    }


private:
    tcp::socket m_socket;
    void* m_recv_buf_ptr;
    int32_t m_recv_size;
    int32_t m_max_recv_size;
    uint16_t m_reconnect_interval;
    uint16_t m_port;
    std::string m_ip;
    ENET_STATUS m_status;
    std::unordered_map<int32_t, msg_handler> m_msg_handler_umap;
    boost::asio::deadline_timer m_timer;
};

class msg_send_guard
{
public:
    msg_send_guard(uint16_t msg_id, net_worker& net_worker, google::protobuf::MessageLite& message_lite)
        : m_msg_id(msg_id)
        , m_net_worker(net_worker)
        , m_message_lite(message_lite)
    {
    }
    ~msg_send_guard()
    {
        void* ptr = m_data_packet.write_data(nullptr, m_message_lite.ByteSize());
        if (m_message_lite.SerializePartialToArray(ptr, m_message_lite.ByteSize()))
        {
            //std::cout << m_message_lite.ByteSize() << std::endl;
            //std::cout << m_data_packet.size() << std::endl;
            m_net_worker.send(m_msg_id, m_data_packet.get_mem_ptr(), m_data_packet.size());
        }
    }

private:
    uint16_t m_msg_id;
    net_worker& m_net_worker;
    google::protobuf::MessageLite& m_message_lite;
    data_packet m_data_packet;
};


#endif
