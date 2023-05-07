#include "hello.pb.h"
#include "../source/common.hpp"

#define RPC_Hello 10000

int main()
{
    try
    {
        boost::asio::io_context m_context;
        net_worker net_worker(m_context);
        net_worker.init(m_context, 1024 * 1024, 5, 100);
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
        net_worker.open(55890);

        m_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
