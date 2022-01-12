#include "net_server.h"
#include "net_mgr.h"
#include <iostream>
#include <thread>
#include <chrono>

net_server::net_server()
	: m_stop(false)
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
	}
	return m_instance;
}

void net_server::init()
{
	m_net_mgr = new net_mgr();

	m_net_mgr->init(2 * 1024 * 1024, 2 * 1024 * 1024);

	m_net_mgr->startup(1, "0.0.0.0", 13000);

	std::cout << "starup!!!" << std::endl;
}

void net_server::loop()
{
	uint8_t count = 0;

	while (!m_stop)
	{
		_sleep(1000);

		std::cout << "Loop " << (int)count++ << " times!" << std::endl;
	}
}

void net_server::destory()
{
	m_net_mgr->shutdown();
	m_net_mgr->destroy();

	delete m_net_mgr;
	std::cout << "destory!!!" << std::endl;
}

void net_server::stop()
{
	m_stop = true;
}

void net_server::_sleep(uint32_t ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

net_server* net_server::m_instance = NULL;