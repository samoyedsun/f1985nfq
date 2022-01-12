#include "net_client.h"
#include "net_msg.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <cstdio>
#include <cstring>

using boost::asio::ip::tcp;

net_client::net_client()
{
}


net_client::~net_client()
{
}

void net_client::send_101()
{
	boost::asio::io_service io_service;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query("localhost", "13000");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	auto result = boost::asio::connect(socket, endpoint_iterator);
	std::cout << "connect success!" << std::endl;

	std::cout << "开始发送数据!" << std::endl;
	char *buf = (char *)malloc(12);
	char *offset = buf;

	*((uint16_t *)offset) = 101;
	offset += sizeof(uint16_t);

	*((uint16_t *)offset) = 8;
	offset += sizeof(uint16_t);

	*((uint32_t *)offset) = 9989;
	offset += sizeof(uint32_t);

	*((uint32_t *)offset) = 2312;
	offset += sizeof(uint32_t);

	size_t write_size = offset - buf;
	std::cout << "写入长度:" << write_size << std::endl;
	write_size = socket.write_some(boost::asio::buffer(buf, write_size));
	std::cout << "写入完成:" << write_size << std::endl;

	if (!io_service.stopped())
	{
		io_service.stop();
		std::cout << "关闭链接." << std::endl;
	}
}

void net_client::send_102(char *msg)
{
	uint16_t msg_id = 102;
	uint16_t msg_size = (uint16_t)strlen(msg) + 1;
	size_t total_size = 4 + msg_size;

	boost::asio::io_service io_service;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query("localhost", "13000");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	auto result = boost::asio::connect(socket, endpoint_iterator);
	std::cout << "connect success!" << std::endl;

	std::cout << "开始发送数据!" << std::endl;
	//msg_base *msg_ptr = new msg_base(sid, msg_id, size, msg_ptr);

	char *buf = (char *)malloc(total_size);
	char *offset = buf;

	*((uint16_t *)offset) = msg_id;
	offset += sizeof(msg_id);

	*((uint16_t *)offset) = msg_size;
	offset += sizeof(msg_size);

	memcpy(offset, msg, msg_size);

	std::cout << "写入长度:" << total_size << std::endl;
	total_size = socket.write_some(boost::asio::buffer(buf, total_size));
	std::cout << "写入完成:" << total_size << std::endl;

	if (!io_service.stopped())
	{
		io_service.stop();
		std::cout << "关闭链接." << std::endl;
	}
}