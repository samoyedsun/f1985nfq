#include <iostream>
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "hello.pb.h"
#include "data_packet.h"

using boost::asio::ip::tcp;

class net_worker
{
    class pointer
    {
    public:
        pointer(boost::asio::io_context& io_context)
            : m_owner_ptr(nullptr)
            , m_id(0)
            , m_socket(io_context)
            , m_recv_buf_ptr(nullptr)
            , m_recv_size(0)
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

        int32_t id()
        {
            return m_id;
        }

        tcp::socket& socket()
        {
            return m_socket;
        }

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
    };

    using pointer_ptr = std::unique_ptr<pointer>;
    using msg_handler = std::function<bool(int32_t, void*, uint16_t)>;

public:
    net_worker(int32_t port)
        : m_acceptor(m_io_context, tcp::endpoint(tcp::v4(), port))
        , m_max_recv_size(0)
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

    void run()
    {
        start_accept();
        m_io_context.run();
    }

    void close(int32_t pointer_id)
    {
        m_pointer_ptr_umap.erase(pointer_id);
        std::cout << "socket close, pointer_id:" << pointer_id << std::endl;
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

    bool send(int32_t pointer_id, uint16_t msg_id, char* data_ptr, int32_t size)
    {
        auto pointer_ptr_it = m_pointer_ptr_umap.find(pointer_id);
        if (pointer_ptr_it == m_pointer_ptr_umap.end())
        {
            return false;
        }
        data_packet data_packet;
        data_packet << (uint16_t)msg_id;
        data_packet << (uint16_t)size;
        data_packet.write_data(data_ptr, size);
        boost::asio::write(pointer_ptr_it->second->socket(), boost::asio::buffer(data_packet.get_mem_ptr(), data_packet.size()));
        return true;
    }

private:
    void start_accept()
    {
        static int32_t id = 0;
        int32_t pointer_id = ++id;
        m_pointer_ptr_umap[pointer_id] = std::make_unique<pointer>(m_io_context);
        m_pointer_ptr_umap[pointer_id]->init(pointer_id, this);
        m_acceptor.async_accept(m_pointer_ptr_umap[pointer_id]->socket(),
            [this, pointer_id](const boost::system::error_code& error)
            {
                if (!error)
                {
                    std::cout << "Accepted connection from "
                        << m_pointer_ptr_umap[id]->socket().remote_endpoint().address().to_string() << std::endl;
                    m_pointer_ptr_umap[id]->read_start();
                }
        start_accept();
            });
    }

private:
    boost::asio::io_context m_io_context;
    std::unordered_map<int32_t, pointer_ptr> m_pointer_ptr_umap;
    tcp::acceptor m_acceptor;
    int32_t m_max_recv_size;
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

#define RPC_Hello 10000
#define SEND_GUARD(POINTER_ID, MSG_ID, NET_WORKER, MSG_TYPE) MSG_TYPE msg; \
	msg_send_guard send_guard(POINTER_ID, MSG_ID, NET_WORKER, msg)


int main()
{
    try
    {
        net_worker net_worker(55890);
        net_worker.max_recv_size(1024 * 1024);
        net_worker.register_msg(RPC_Hello, [&net_worker](int32_t pointer_id, void* data_ptr, int32_t size)
            {
                Hello data;
                if (!data.ParsePartialFromArray(data_ptr, size))
                {
                    return false;
                }
                std::cout << "recive " << data.member(0) << " msg abot 10000==" << data.id() << std::endl;

                // process some logic.
                SEND_GUARD(pointer_id, RPC_Hello, net_worker, Hello);
                msg.set_id(200);
                msg.add_member(5656);

                return true;
            });


        net_worker.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
