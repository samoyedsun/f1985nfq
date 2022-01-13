#include "net_client.h"
#include "net_msg.h"

#include <iostream>
#include <string>
#include <thread>
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

net_client* net_client::instance()
{
	if (m_instance == NULL)
	{
		m_instance = new net_client;
		std::cout << "net client create." << std::endl;
	}
	return m_instance;
}

void net_client::destory()
{
	if (m_instance != NULL)
	{
		delete m_instance;
		std::cout << "net client destory." << std::endl;
	}
}

void net_client::send_101()
{
	std::thread([this]()
	{
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(m_remote_host.c_str(), "19851");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		tcp::socket socket(io_service);

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

		try
		{
			auto result = boost::asio::connect(socket, endpoint_iterator);
			std::cout << "connect success!" << std::endl;
			std::cout << "写入长度:" << write_size << std::endl;
			write_size = socket.write_some(boost::asio::buffer(buf, write_size));
			std::cout << "写入完成:" << write_size << std::endl;
		}
			catch (boost::system::system_error& e)
		{
			std::cout << "work thread exception, code = " << e.code().message() << std::endl;
		}

		if (!io_service.stopped())
		{
			io_service.stop();
			std::cout << "关闭链接." << std::endl;
		}
	}).detach();
}

void net_client::send_102(char *msg)
{
	std::thread([this, msg]()
	{
		uint16_t msg_id = 102;
		uint16_t msg_size = (uint16_t)strlen(msg) + 1;
		size_t total_size = 4 + msg_size;

		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(m_remote_host.c_str(), "19851");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		tcp::socket socket(io_service);

		std::cout << "开始发送数据!" << std::endl;

		char *buf = (char *)malloc(total_size);
		char *offset = buf;

		*((uint16_t *)offset) = msg_id;
		offset += sizeof(msg_id);

		*((uint16_t *)offset) = msg_size;
		offset += sizeof(msg_size);

		memcpy(offset, msg, msg_size);

		try
		{
			std::cout << "写入长度:" << total_size << std::endl;
			std::cout << "connecting to remote!" << std::endl;
			auto result = boost::asio::connect(socket, endpoint_iterator);
			std::cout << "connect success!" << std::endl;
			total_size = socket.write_some(boost::asio::buffer(buf, total_size));
			std::cout << "写入完成:" << total_size << std::endl;
		}
		catch (boost::system::system_error& e)
		{
			std::cout << "work thread exception, code = " << e.code().message() << std::endl;
		}

		if (!io_service.stopped())
		{
			io_service.stop();
			std::cout << "关闭链接." << std::endl;
		}
	}).detach();
}

net_client* net_client::m_instance = NULL;