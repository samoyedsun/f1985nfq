#include "hello.pb.h"
#include "net_worker.hpp"

using boost::asio::ip::tcp;

#define RPC_Hello 10000

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
