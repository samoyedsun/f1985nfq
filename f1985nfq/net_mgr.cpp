#include "net_mgr.h"
#include "net_session.h"
#include "net_msg.h"

#include <iostream>

net_mgr::net_mgr()
	: m_work(m_io_service)
{
}


net_mgr::~net_mgr()
{
}

bool net_mgr::init(uint32_t send_buf_size, uint32_t recv_buf_size)
{
	m_send_buf_size = send_buf_size;
	m_recv_buf_size = recv_buf_size;

	m_acceptor_ptr = new wacceptor(m_io_service);
	if (!m_acceptor_ptr)
	{
		std::cout << "new wacceptor failed" << std::endl;
		return false;
	}
	return true;
}

// 启动服务器
bool net_mgr::startup(uint32_t thread_num, const std::string& ip, uint16_t port)
{
	if (!_start_listen(ip, port, thread_num))
		return false;

	for (uint32_t i = 0; i < thread_num; ++i)
		m_threads.push_back(std::thread(boost::bind(&net_mgr::_worker_thread, this)));

	return true;
}

void net_mgr::shut_accept()
{
}

// 关闭服务器
void net_mgr::shutdown()
{
	if (!m_io_service.stopped())
	{
		// 关闭工作线程
		m_io_service.stop();
	}

	for (std::thread& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void net_mgr::destroy()
{
	m_threads.clear();
}

void net_mgr::process_msg()
{
	while (m_msgs.size() > 0)
	{
		msg_base *msg_ptr = m_msgs.front();
		m_msg_callback(*msg_ptr);
		m_msgs.pop();
	}
}

void net_mgr::_worker_thread()
{
	for (;;)
	{
		try
		{
			m_io_service.run();
			std::cout << "net mgr thread exit." << std::endl;
			break;
		}
		catch (boost::system::system_error& e)
		{
			std::cout << "work thread exception, code = " << e.code().message() << std::endl;
		}
	}
}

bool net_mgr::_start_listen(const std::string& ip, uint16_t port, uint32_t thread_num)
{
	wendpoint endpoint(boost::asio::ip::address::from_string(ip), port);

	werror ec;
	m_acceptor_ptr->open(endpoint.protocol(), ec);
	if (ec)
	{
		std::cout << "open socket error " << ec.message() << std::endl;
		return false;
	}

	{
		m_acceptor_ptr->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
		if (ec)
		{
			std::cout << "set_option reuse_address error, " << ec.message() << std::endl;
			return false;
		}
	}

	m_acceptor_ptr->bind(endpoint, ec);
	if (ec)
	{
		std::cout << "bind error, " << ec.message() << std::endl;
		return false;
	}

	m_acceptor_ptr->listen(boost::asio::socket_base::max_connections, ec);
	if (ec)
	{
		std::cout << "listen error, " << ec.message() << std::endl;
		return false;
	}

	for (uint32_t i = 0; i < thread_num; ++i)
		_post_accept();

	std::cout << "listen on ip=" << ip << " port=" << port << std::endl;
	return true;
}

void net_mgr::_post_accept()
{
	net_session* session_ptr = new net_session(m_io_service, *this, 100);
	session_ptr->init();
	m_acceptor_ptr->async_accept(session_ptr->get_socket(),
		boost::bind(&net_mgr::_handle_accept, this, boost::asio::placeholders::error, session_ptr)
	);
}

void net_mgr::_handle_accept(const net_mgr::werror& error, net_session* session_ptr)
{
	void * msg_ptr = NULL;
	if (!error)
	{
		std::cout << "来了个新连接!!!" << std::endl;
		//msg_ptr = _create_accept_msg(session_ptr->get_socket());
		_post_accept();
		session_ptr->connected();
	}
	else
	{
		std::cout << "哈哈，报错了!!!" << error << std::endl;
		//session_ptr->set_status_init();
		if (session_ptr->get_socket().is_open())
		{
			werror ec;
			session_ptr->get_socket().close(ec);
		}
		//_return_session(session_ptr);
		_post_accept();
	}
}

void* net_mgr::_create_accept_msg(const wsocket& socket)
{
	werror ec;
	wendpoint remote_endpoint = socket.remote_endpoint(ec);

	std::string remote_address = remote_endpoint.address().to_string(ec);
	//const uint32_t msg_size = static_cast<uint32_t>(sizeof(wmsg_connect) + remote_address.length());

	uint16_t remote_port = remote_endpoint.port();
	std::cout << "new connection from: " << remote_address << ":" << remote_port << std::endl;
	return NULL;
}

uint32_t net_mgr::_get_recv_buf_size()
{
	return m_recv_buf_size;
}

msg_base *net_mgr::_create_msg(uint32_t sid, uint16_t msg_id, uint16_t size, const char *msg_ptr)
{
	// TODO:
	//		检查sys_id是否合法
	//		检查size是否过大
	//		检查内存分配是否失败

	return new msg_base(sid, msg_id, size, msg_ptr);;
}

void net_mgr::_push_msg(msg_base *msg_ptr)
{
	m_msgs.push(msg_ptr);
}