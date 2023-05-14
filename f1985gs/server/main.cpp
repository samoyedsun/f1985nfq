#include "hello.pb.h"
#include "../source/common.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
#include <lua.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif

#define RPC_Hello 10000

static void* l_alloc(void* ud, void* ptr, size_t osize,
    size_t nsize) {
    (void)ud;  (void)osize;  /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

class world
{
    using timer_umap_t = std::unordered_map<int32_t, boost::asio::deadline_timer*>;

public:
    world()
        : m_net_worker(m_context)
        , m_console_reader(m_context)
        , m_timer(m_context, boost::posix_time::milliseconds(1))
        , m_lua_vm(nullptr)
    {
        m_lua_vm = lua_newstate(l_alloc, NULL);
        luaL_openlibs(m_lua_vm);

        m_net_worker.init(m_context);
        m_net_worker.register_msg(RPC_Hello, [this](int32_t pointer_id, void* data_ptr, int32_t size)
            {
                Hello data;
                if (!data.ParsePartialFromArray(data_ptr, size))
                {
                    return false;
                }
                std::cout << "recive " << data.member(0) << " msg abot 10000==" << data.id() << std::endl;

                // process some logic.
                SEND_GUARD(pointer_id, RPC_Hello, m_net_worker, Hello);
                msg.set_id(200);
                msg.add_member(5656);

                return true;
            });
        m_net_worker.open(55890);
        m_console_reader.start();
        m_timer.async_wait(boost::bind(&world::loop, this, boost::asio::placeholders::error));
    }

    void run()
    {
        m_context.run();
        lua_close(m_lua_vm);
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
        if (spend_tick < tick_interval)
        {
            int32_t tick = tick_interval - spend_tick;
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
                    // This number needs to be obtained through an interface that passes in the name
                    SEND_GUARD(1, RPC_Hello, m_net_worker, Hello);
                    msg.set_id(100);
                    msg.add_member(3434);
                }
            }
        }
    }

    static const int32_t tick_interval = 16;

private:
    boost::asio::io_context m_context;
    net_worker m_net_worker;
    console_reader m_console_reader;
    boost::asio::deadline_timer m_timer;
    lua_State* m_lua_vm;
};

int main()
{
    world w;
    w.run();
    return 0;
}
