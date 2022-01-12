#include "net_mgr.h"
#include "net_session.h"

#include <vector>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#define HEADER_SIZE 4

net_session::net_session(boost::asio::io_service& io_service,
	net_mgr& net_mgr, uint16_t index)
	: m_session_id(0X00010000 | index),
	m_socket(io_service),
	m_net_mgr(net_mgr)
{
}

net_session::~net_session()
{
}


bool net_session::init()
{
	m_recv_buf_ptr = new char[m_net_mgr._get_recv_buf_size()];
	if (!m_recv_buf_ptr)
	{
		std::cout << "alloc m_recv_buf_ptr memory error!" << std::endl;
		return false;
	}
	return true;
}

net_session::wsocket_type& net_session::get_socket()
{
	return m_socket;
}

void net_session::connected()
{
	// 投递读请求
	m_socket.async_read_some(boost::asio::buffer(m_recv_buf_ptr, m_net_mgr._get_recv_buf_size()),
		boost::bind(&net_session::handle_read, this, boost::asio::placeholders::error
			, boost::asio::placeholders::bytes_transferred)
	);
}

void net_session::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (error || 0 == bytes_transferred)
	{
		std::cout << "读取错误关闭连接:" << error.message() << std::endl;
		boost::system::error_code ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close(ec);
		return;
	}

	m_recv_size += static_cast<uint32_t>(bytes_transferred);
	std::cout << "读出长度:" << m_recv_size << std::endl;

	//解析消息
	if (!_parse_msg())
	{
		//_close();
		return;
	}

	m_socket.async_read_some(
		boost::asio::buffer(m_recv_buf_ptr + m_recv_size,
			m_net_mgr._get_recv_buf_size() - m_recv_size),
		boost::bind(&net_session::handle_read, this, boost::asio::placeholders::error
			, boost::asio::placeholders::bytes_transferred)
	);
}

bool net_session::_parse_msg()
{
	const char *offset = m_recv_buf_ptr;

	// 这里应该只管解析，对错由业务层处理，具体可以参考下skynet
	// msg_id找不到说明有非法请求，报消息号找不到
	// size过大，这个怎么判定？？？？这个没法判定。。。
	while (m_recv_size)
	{
		if (m_recv_size < HEADER_SIZE)
		{
			std::cout << "不够一个消息头" << std::endl;
			memmove(m_recv_buf_ptr, offset, m_recv_size);
			break;
		}
		uint16_t msg_id = ((uint16_t *)offset)[0];
		offset += sizeof(msg_id);
		uint16_t size = ((uint16_t *)offset)[0];
		offset += sizeof(size);
		if (m_recv_size - 4 < size)
		{
			std::cout << "不够一个消息体" << std::endl;
			memmove(m_recv_buf_ptr, offset, m_recv_size);
			break;
		}
		m_recv_size -= 4;

		msg_base *msg_base_ptr = m_net_mgr._create_msg(m_session_id, msg_id, size, offset);
		if (!msg_base_ptr)
		{
			std::cout << "内存不够分配内存失败" << std::endl;
			break;
		}
		m_recv_size -= size;
		m_net_mgr._push_msg(msg_base_ptr);
	}
	return true;
}