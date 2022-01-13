#include "net_server.h"
#include "net_mgr.h"
#include "net_msg.h"
#include <iostream>
#include <thread>
#include <chrono>

net_server::net_server()
	: m_stop(false)
	, m_recv_field_ptr(NULL)
	, m_thread_ptr(NULL)
{
}

net_server::~net_server()
{
}

net_server* net_server::instance()
{
	if (m_instance == NULL)
	{
		m_instance = new net_server;
		std::cout << "net server create." << std::endl;
	}
	return m_instance;
}

void net_server::init()
{
	m_net_mgr = new net_mgr();

	m_net_mgr->init(2 * 1024 * 1024, 2 * 1024 * 1024);
	m_net_mgr->set_msg_callback(
		std::bind(&net_server::on_msg_receive, this, std::placeholders::_1)
	);

	std::cout << "net server init." << std::endl;
}

void net_server::startup()
{
	m_net_mgr->startup(1, "0.0.0.0", 19851);

	m_thread_ptr = new std::thread(boost::bind(&net_server::loop, this));

	std::cout << "net server startup." << std::endl;
}

void net_server::loop()
{
	while (!m_stop)
	{
		m_net_mgr->process_msg();
		_sleep(100);
	}
	std::cout << "net server exit loop." << std::endl;
}

void net_server::shutdown()
{
	m_stop = true;

	if (m_thread_ptr)
	{
		if (m_thread_ptr->joinable())
		{
			m_thread_ptr->join();
		}
	}

	std::cout << "net server shutdown." << std::endl;
}

void net_server::destory()
{
	delete m_thread_ptr;

	m_net_mgr->shutdown();
	m_net_mgr->destroy();
	delete m_net_mgr;

	delete m_instance;
	std::cout << "net server destory." << std::endl;
}

void net_server::on_msg_receive(msg_base& msg)
{
	std::cout << "==============recv msg==========" << std::endl;
	std::cout << "=========sid==:" << msg.get_sid() << std::endl;
	std::cout << "======msg_id==:" << msg.get_msg_id() << std::endl;
	std::cout << "========size==:" << msg.get_size() << std::endl;
	switch (msg.get_msg_id())
	{
	case 101:
	{
		msg_hello *msg_hello_ptr = (msg_hello *)&msg;
		std::cout << "=========hp:" << msg_hello_ptr->get_hp() << std::endl;
		std::cout << "=========mp:" << msg_hello_ptr->get_mp() << std::endl;
		break;
	}
	case 102:
	{
		msg_world *msg_world_ptr = (msg_world *)&msg;
		memcpy(m_recv_field_ptr, msg_world_ptr->get_content().c_str(), msg.get_size());
		break;
	}
	default:
		std::cout << "msg_id is not found!" << std::endl;
		break;
	}
}

void net_server::_sleep(uint32_t ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

net_server* net_server::m_instance = NULL;