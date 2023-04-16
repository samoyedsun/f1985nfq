#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <google/protobuf/message_lite.h>
#include "hello.pb.h"
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
        std::cout << "closed connection." << std::endl;
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

#define RPC_Hello 10000
#define SEND_GUARD(MSG_ID, NET_WORKER, MSG_TYPE) MSG_TYPE msg; \
	msg_send_guard send_guard(MSG_ID, NET_WORKER, msg)

class world
{
    using timer_umap_t = std::unordered_map<int32_t, boost::asio::deadline_timer*>;

public:
    world()
        : m_net_worker(m_context)
        , m_timer(m_context, boost::posix_time::milliseconds(1))
    {
        m_net_worker.init(1024 * 1024);
        m_net_worker.set_address("127.0.0.1", 55890);
        m_net_worker.register_msg(RPC_Hello, [this](void* data_ptr, int32_t size)
            {
                Hello data;
                if (!data.ParsePartialFromArray(data_ptr, size))
                {
                    return false;
                }
                //std::cout << "recive " << data.member(0) << " msg abot 10000==" << data.id() << std::endl;

                // process some logic.
                //SEND_GUARD(RPC_Hello, m_net_worker, Hello);
                //msg.set_id(500);
                //msg.add_member(7878);

                return true;
            });
        m_net_worker.start();
        m_timer.async_wait(boost::bind(&world::loop, this, boost::asio::placeholders::error));
    }

    void run()
    {
        m_context.run();
    }

private:
    void loop(const boost::system::error_code& ec)
    {
        if (ec)
        {
            std::cout << "loop failed:" << ec.message() << std::endl;
            return;
        }
        auto begin_tick = boost::asio::chrono::steady_clock::now();
        run_one();
        auto end_tick = boost::asio::chrono::steady_clock::now();
        uint32_t spend_tick = static_cast<uint32_t>((end_tick - begin_tick).count() / 1000 / 1000);
        std::cout << "loop one times. spend_tick:" << spend_tick << std::endl;
        if (spend_tick < TICK_TIME)
        {
            int32_t tick = TICK_TIME - spend_tick;
            m_timer.expires_from_now(boost::posix_time::milliseconds(tick));
            m_timer.async_wait(boost::bind(&world::loop, this, boost::asio::placeholders::error));
        }
        else
        {
            m_timer.expires_from_now(boost::posix_time::milliseconds(1));
            m_timer.async_wait(boost::bind(&world::loop, this, boost::asio::placeholders::error));
        }
    }

    void run_one()
    {
        {
            SEND_GUARD(RPC_Hello, m_net_worker, Hello);
            msg.set_id(100);
            msg.add_member(3434);
        }
    }

    static const int32_t TICK_TIME = 16;

private:
    boost::asio::io_context m_context;
    net_worker m_net_worker;
    boost::asio::deadline_timer m_timer;
};

int main()
{
    world w;
    w.run();
    return 0;
}
