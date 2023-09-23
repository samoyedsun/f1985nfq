#include "hello.pb.h"
#include "../source/common.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
using ssl_socket = ssl::stream<tcp::socket>;
using Request = http::request<http::string_body>;
using Response = http::response<http::empty_body>;
using BodyResponse = http::response<http::string_body>;

#ifdef __cplusplus
extern "C"
{
#endif
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#ifdef __cplusplus
}
#endif

#define RPC_Hello 10000
#define LUA_SCRIPT_PATH_DEV "../../../../server/script/?.lua;"
#define LUA_SCRIPT_DEV_LAUNCH "../../../../server/script/main.lua;"
#define LUA_SCRIPT_PATH_PUB "../script/?.lua;"
#define LUA_SCRIPT_PUB_LAUNCH "../script/main.lua"

static void* l_alloc(void* ud, void* ptr, size_t osize,
    size_t nsize) {
    (void)ud;  (void)osize;  /* not used */
    if (nsize == 0) {
        free(ptr);
        return nullptr;
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
        init_script();
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
                if (cmd.name == "refresh")
                {
                    int32_t ret = luaL_dofile(m_lua_vm, LUA_SCRIPT_PUB_LAUNCH);
                    if (ret != 0)
                    {
                        const char* errorMsg = lua_tostring(m_lua_vm, -1);
                        std::cout << "error:" << errorMsg << std::endl;
                        lua_pop(m_lua_vm, 1);
                    }
                }
                if (cmd.name == "reload")
                {
                    init_script();
                }
                if (cmd.name == "testlua1")
                {
                    int count = lua_gettop(m_lua_vm);
                    int32_t type = lua_getglobal(m_lua_vm, "CheckAddBuffManager");
                    count = lua_gettop(m_lua_vm);
                    if (type == LUA_TTABLE)
                    {
                        lua_pushstring(m_lua_vm, "Check");
                        count = lua_gettop(m_lua_vm);
                        type = lua_gettable(m_lua_vm, -2);
                        count = lua_gettop(m_lua_vm);
                        if (type == LUA_TFUNCTION)
                        {
                            lua_pushvalue(m_lua_vm, -2);
                            count = lua_gettop(m_lua_vm);
                            lua_remove(m_lua_vm, 1);
                            count = lua_gettop(m_lua_vm);
                            lua_pushinteger(m_lua_vm, 111);
                            lua_pushinteger(m_lua_vm, 222);
                            int32_t ret = lua_pcall(m_lua_vm, 3, 0, 0);
                            count = lua_gettop(m_lua_vm);
                            count = lua_gettop(m_lua_vm);
                        }
                        else
                        {
                            lua_pop(m_lua_vm, 2);
                        }
                    }
                    else {
                        lua_pop(m_lua_vm, 1);
                    }
                }
                if (cmd.name == "testlua2")
                {
                    int32_t type = lua_getglobal(m_lua_vm, "CheckAddBuffManagerCheck");
                    if (type == LUA_TFUNCTION)
                    {
                        lua_pushstring(m_lua_vm, "sdfsdf111");
                        lua_pushstring(m_lua_vm, "sdfsdf222");
                        lua_call(m_lua_vm, 2, 0);
                    }
                    else
                    {
                        lua_pop(m_lua_vm, 1);
                    }
                }
            }
        }
    }

    void init_script()
    {
        if (m_lua_vm)
        {
            lua_close(m_lua_vm);
            m_lua_vm = nullptr;
        }

        m_lua_vm = lua_newstate(l_alloc, nullptr);
        luaL_openlibs(m_lua_vm);

        // setting package.path
        lua_getglobal(m_lua_vm, "package");
        lua_pushstring(m_lua_vm, LUA_SCRIPT_PATH_PUB);
        lua_getfield(m_lua_vm, -2, "path");
        lua_concat(m_lua_vm, 2);
        lua_setfield(m_lua_vm, -2, "path");
        lua_pop(m_lua_vm, 1);
        int32_t num = lua_gettop(m_lua_vm);
        
        int32_t ret = luaL_dofile(m_lua_vm, LUA_SCRIPT_PUB_LAUNCH);
        if (ret != 0)
        {
            const char* errorMsg = lua_tostring(m_lua_vm, -1);
            std::cout << "error:" << errorMsg << std::endl;
            lua_pop(m_lua_vm, 1);
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
    SetConsoleOutputCP(CP_UTF8);
    world w;
    w.run();
    /* ¶¤¶¤±¨¾¯Ïà¹Ø
    std::string const in_server = "oapi.dingtalk.com";
    boost::asio::io_context io_context;
    tcp::resolver           resolver(io_context);
    ssl::context ctx(ssl::context::sslv23_client);
    //ctx.set_default_verify_paths();
    // Open a socket and connect it to the remote host.
    ssl_socket socket(io_context, ctx);
    boost::asio::connect(socket.lowest_layer(), resolver.resolve(in_server, "https"));
    socket.lowest_layer().set_option(tcp::no_delay(true));
    socket.set_verify_callback(ssl::host_name_verification(in_server));
    {
        socket.set_verify_mode(ssl::verify_none);
        socket.handshake(ssl_socket::client);
        std::cout << "Handshake completed" << std::endl;
    }
    {
        http::request<http::string_body> request;
        request.method(http::verb::post);
        request.target("/robot/send?access_token=33421c5b3bf09748e56d90beff09ae149f988e8d4026b214ab01aa26bd6a81dc");
        request.version(11);
        request.set(http::field::host, in_server);
        request.set(http::field::user_agent, "Boost.Beast");
        request.set(http::field::content_type, "application/json");
        request.body() = "{\"msgtype\": \"markdown\", \"markdown\": {\"title\":\"hello\", \"text\":\"helloworld11\"}}";
        request.prepare_payload();
        http::write(socket, request);
    }
    {
        beast::flat_buffer buf;
        BodyResponse       http_res;
        http::read(socket, buf, http_res);
        std::cout << http_res;
    }
    */
    return 0;
}
