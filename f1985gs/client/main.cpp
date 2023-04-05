#include <iostream>
#include <boost/asio.hpp>

#include "hello.pb.h"

using boost::asio::ip::tcp;

#define MAX_PACKAGE_SIZE 1024 * 1024

int main()
{
    try
    {
        boost::asio::io_context io_context;

        // 创建一个TCP endpoint并连接到远程服务器
        tcp::socket socket(io_context);
        tcp::resolver resolver(io_context);
        boost::asio::connect(socket, resolver.resolve("127.0.0.1", "55890"));

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
        boost::asio::write(socket, boost::asio::buffer(data_ptr, package_size));

        free(data_ptr);
        system("pause");
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
