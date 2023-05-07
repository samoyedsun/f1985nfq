#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <google/protobuf/message_lite.h>
#include "hello.pb.h"
#include "console_reader.hpp"
#include "net_worker.hpp"

using boost::asio::ip::tcp;

#define RPC_Hello 10000

class world
{
    using timer_umap_t = std::unordered_map<int32_t, boost::asio::deadline_timer*>;

public:
    world()
        : m_net_worker(m_context)
        , m_console_reader(m_context)
        , m_timer(m_context, boost::posix_time::milliseconds(1))
    {
        m_net_worker.init(1024 * 1024, 5);
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
        m_console_reader.start();
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
        run_once();
        auto end_tick = boost::asio::chrono::steady_clock::now();
        uint32_t spend_tick = static_cast<uint32_t>((end_tick - begin_tick).count() / 1000 / 1000);
        //std::cout << "loop one times. spend_tick:" << spend_tick << std::endl;
        if (spend_tick < TICK_TIME)
        {
            int32_t tick = TICK_TIME - spend_tick;
            m_timer.expires_from_now(boost::posix_time::milliseconds(tick));
        }
        else
        {
            m_timer.expires_from_now(boost::posix_time::milliseconds(1));
        }
        m_timer.async_wait(boost::bind(&world::loop, this, boost::asio::placeholders::error));
    }

    void run_once()
    {
        {
            console_reader::command cmd;
            if (m_console_reader.pop_front(cmd))
            {
                if (cmd.name == "hello")
                {
                    SEND_GUARD(RPC_Hello, m_net_worker, Hello);
                    msg.set_id(100);
                    msg.add_member(3434);
                }
            }
        }
    }

    static const int32_t TICK_TIME = 16;

private:
    boost::asio::io_context m_context;
    net_worker m_net_worker;
    console_reader m_console_reader;
    boost::asio::deadline_timer m_timer;
};

int main(int argc, char *argv[])
{
    world w;
    w.run();
    return 0;
}
