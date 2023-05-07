#ifndef _NET_WORKER_H_
#define _NET_WORKER_H_

#include <iostream>
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "data_packet.hpp"

using boost::asio::ip::tcp;

class net_worker
{
    class pointer
    {
        enum ENET_STATUS
        {
            ENET_STATUS_UNCONNECT,
            ENET_STATUS_WAITING,
            ENET_STATUS_CONNECTED,
        };
    public:
        pointer(boost::asio::io_context& io_context)
            : m_owner_ptr(nullptr)
            , m_id(0)
            , m_socket(io_context)
            , m_recv_buf_ptr(nullptr)
            , m_recv_size(0)
            , m_status(ENET_STATUS_UNCONNECT)
        {

        }

        ~pointer()
        {
            free(m_recv_buf_ptr);
        }

        void init(int32_t id, net_worker* owner_ptr)
        {
            m_id = id;
            m_owner_ptr = owner_ptr;
            m_recv_buf_ptr = malloc(m_owner_ptr->max_recv_size());
        }

        void start()
        {
            m_status = ENET_STATUS_CONNECTED;
            read_start();
        }

        bool valid()
        {
            return m_status == ENET_STATUS_UNCONNECT;
        }

        void set_waiting()
        {
            m_status = ENET_STATUS_WAITING;
            if (m_socket.is_open())
            {
                m_socket.close();
            }
        }

        void set_unconnect()
        {
            m_status = ENET_STATUS_UNCONNECT;
            if (m_socket.is_open())
            {
                m_socket.close();
            }
        }

        int32_t id()
        {
            return m_id;
        }

        tcp::socket& socket()
        {
            return m_socket;
        }

        bool send(uint16_t msg_id, char* data_ptr, uint16_t size)
        {
            if (m_status != ENET_STATUS_CONNECTED)
            {
                std::cout << "Have not connected success!" << std::endl;
                return false;
            }
            data_packet data_packet;
            data_packet << msg_id;
            data_packet << size;
            data_packet.write_data(data_ptr, size);
            boost::asio::write(m_socket, boost::asio::buffer(data_packet.get_mem_ptr(), data_packet.size()));
            return true;
        }

    private:
        void read_start()
        {
            void* offset_ptr = (void*)((int8_t*)m_recv_buf_ptr + m_recv_size);
            int32_t left_size = m_owner_ptr->max_recv_size() - m_recv_size;
            m_socket.async_read_some(boost::asio::buffer(offset_ptr, left_size),
                [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
                {
                    if (ec || bytes_transferred == 0)
                    {
                        std::cout << "data :" << ec.message() << std::endl;
                        return m_owner_ptr->close(m_id);
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
                m_owner_ptr->close(m_id);
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
                if (!m_owner_ptr->parse_msg(m_id, msg_id, body_ptr, body_size))
                {
                    std::cout << "parse msg error, msg_id:" << msg_id
                        << " size:" << body_size << std::endl;
                    m_owner_ptr->close(m_id);
                    return;
                }
                m_recv_size -= package_size;
                if (m_recv_size > 0)
                {
                    memcpy(m_recv_buf_ptr, offset_ptr, m_recv_size);
                }
            } while (m_recv_size >= 4);
        }

    private:
        net_worker* m_owner_ptr;
        int32_t m_id;
        tcp::socket m_socket;
        void* m_recv_buf_ptr;
        int32_t m_recv_size;
        ENET_STATUS m_status;
    };

    struct outward
    {
        std::string name;
        std::string ip;
        int32_t port;
        int32_t pointer_id;
        boost::asio::deadline_timer timer;

        outward(boost::asio::io_context& context)
            : port(0)
            , pointer_id(0)
            , timer(context, boost::posix_time::milliseconds(0))
        {
        }
    };

    using outward_ptr = std::unique_ptr<outward>;
    using pointer_ptr = std::unique_ptr<pointer>;
    using msg_handler = std::function<bool(int32_t, void*, uint16_t)>;

public:
    net_worker(boost::asio::io_context& context)
        : m_acceptor(context)
        , m_max_recv_size(0)
        , m_reconnect_interval(0)
    {
    }

    void init(boost::asio::io_context& context, int32_t max_recv_size, uint16_t reconnect_interval, uint16_t pointer_num)
    {
        m_max_recv_size = max_recv_size;
        m_reconnect_interval = reconnect_interval;
        for (int32_t id = 1; id <= pointer_num; ++id)
        {
            m_pointer_ptr_umap[id] = std::make_unique<pointer>(context);
            m_pointer_ptr_umap[id]->init(id, this);
        }
    }

    int32_t max_recv_size()
    {
        return m_max_recv_size;
    }

    void push_bullet(boost::asio::io_context& context, const std::string name, const std::string ip, int32_t port)
    {
        int32_t pointer_id = pop_pointer();
        m_outward_ptr_umap[pointer_id] = std::make_unique<outward>(context);
        m_outward_ptr_umap[pointer_id]->name = name;
        m_outward_ptr_umap[pointer_id]->ip = ip;
        m_outward_ptr_umap[pointer_id]->port = port;
        m_outward_ptr_umap[pointer_id]->pointer_id = pointer_id;
    }

    void open(int32_t port)
    {
        tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string("0.0.0.0"), port);
        boost::system::error_code ec;
        m_acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            std::cerr << "socket open error, " << ec.message() << std::endl;
            return;
        }
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
        if (ec)
        {
            std::cerr << "set_option reuse_address error, " << ec.message() << std::endl;
            return;
        }
        m_acceptor.bind(endpoint, ec);
        if (ec)
        {
            std::cerr << "socket bind error, " << ec.message() << std::endl;
            return;
        }
        m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
        if (ec)
        {
            std::cerr << "socket listen error, " << ec.message() << std::endl;
            return;
        }
        start_accept();
    }

    void shoot()
    {
        for (auto iter = m_outward_ptr_umap.begin(); iter != m_outward_ptr_umap.end(); ++iter)
        {
            start_connect(iter->first);
        }
    }

    void close(int32_t pointer_id)
    {
        auto iter = m_outward_ptr_umap.find(pointer_id);
        if (iter != m_outward_ptr_umap.end())
        {
            m_pointer_ptr_umap[pointer_id]->set_waiting();
            std::cout << "closed connection, after 5s reconnect." << std::endl;
            iter->second->timer.expires_from_now(boost::posix_time::seconds(m_reconnect_interval));
            iter->second->timer.async_wait(boost::bind(&net_worker::handle_reconnect, this, pointer_id, boost::asio::placeholders::error));
        }
        else
        {
            m_pointer_ptr_umap[pointer_id]->set_unconnect();
            std::cout << "socket close, pointer_id:" << pointer_id << std::endl;
        }
    }

    bool parse_msg(int32_t pointer_id, uint16_t msg_id, void* data_ptr, uint16_t size)
    {
        auto msg_handler_it = m_msg_handler_umap.find(msg_id);
        if (msg_handler_it == m_msg_handler_umap.end())
        {
            return false;
        }
        return msg_handler_it->second(pointer_id, data_ptr, size);
    }

    void register_msg(uint16_t msg_id, msg_handler msg_handler)
    {
        m_msg_handler_umap.try_emplace(msg_id, msg_handler);
    }

    bool send(int32_t pointer_id, uint16_t msg_id, char* data_ptr, uint16_t size)
    {
        auto pointer_ptr_it = m_pointer_ptr_umap.find(pointer_id);
        if (pointer_ptr_it == m_pointer_ptr_umap.end())
        {
            return false;
        }
        return pointer_ptr_it->second->send(msg_id, data_ptr, size);
    }

private:
    void start_accept()
    {
        int32_t pointer_id = pop_pointer();
        m_acceptor.async_accept(m_pointer_ptr_umap[pointer_id]->socket(),
            [this, pointer_id](const boost::system::error_code& ec)
            {
                if (!ec)
                {
                    std::cout << "Accepted connection from "
                        << m_pointer_ptr_umap[pointer_id]->socket().remote_endpoint().address().to_string() << std::endl;
                    m_pointer_ptr_umap[pointer_id]->start();
                }
                else
                {
                    std::cout << "Accept failed:" << ec.message() << std::endl;
                }
        start_accept();
            });
    }

    void start_connect(int32_t pointer_id)
    {
        std::string& ip = m_outward_ptr_umap[pointer_id]->ip;
        int32_t port = m_outward_ptr_umap[pointer_id]->port;
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
        m_pointer_ptr_umap[pointer_id]->socket().async_connect(endpoint, [this, pointer_id](const boost::system::error_code& ec)
            {
                if (!ec)
                {
                    std::string& name = m_outward_ptr_umap[pointer_id]->name;
                    std::cout << "Connected " << name << " server" << std::endl;
                    m_pointer_ptr_umap[pointer_id]->start();
                    return;
                }
        std::cout << "Connect failed:" << ec.message() << std::endl;
        close(pointer_id);
            });
    }

    void handle_reconnect(int32_t pointer_id, const boost::system::error_code& ec)
    {
        if (ec)
        {
            std::cout << "reconnect error." << ec.message() << std::endl;
            return;
        }
        start_connect(pointer_id);
    }

    int32_t pop_pointer()
    {
        for (auto iter = m_pointer_ptr_umap.begin(); iter != m_pointer_ptr_umap.end(); ++iter)
        {
            if (iter->second->valid())
            {
                iter->second->set_waiting();
                return iter->second->id();
            }
        }
        return 0;
    }

private:
    std::unordered_map<int32_t, pointer_ptr> m_pointer_ptr_umap;
    std::unordered_map<int32_t, outward_ptr> m_outward_ptr_umap;
    tcp::acceptor m_acceptor;
    int32_t m_max_recv_size;
    uint16_t m_reconnect_interval;
    std::unordered_map<int32_t, msg_handler> m_msg_handler_umap;
};

class msg_send_guard
{
public:
    msg_send_guard(int32_t pointer_id, uint16_t msg_id, net_worker& net_worker, google::protobuf::MessageLite& message)
        : m_pointer_id(pointer_id)
        , m_msg_id(msg_id)
        , m_net_worker(net_worker)
        , m_message(message)
    {
    }
    ~msg_send_guard()
    {
        void* ptr = m_data_packet.write_data(nullptr, m_message.ByteSize());
        if (m_message.SerializePartialToArray(ptr, m_message.ByteSize()))
        {
            std::cout << m_message.ByteSize() << std::endl;
            std::cout << m_data_packet.size() << std::endl;
            m_net_worker.send(m_pointer_id, m_msg_id, m_data_packet.get_mem_ptr(), m_data_packet.size());
        }
    }

private:
    int32_t m_pointer_id;
    uint16_t m_msg_id;
    net_worker& m_net_worker;
    google::protobuf::MessageLite& m_message;
    data_packet m_data_packet;
};

#define SEND_GUARD(POINTER_ID, MSG_ID, NET_WORKER, MSG_TYPE) MSG_TYPE msg; \
	msg_send_guard send_guard(POINTER_ID, MSG_ID, NET_WORKER, msg)

#endif
