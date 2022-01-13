#ifndef _NET_SERVER_H_
#define _NET_SERVER_H_

#include <iostream>
#include <thread>
#include <cstdint>

class net_mgr;
class msg_base;
class net_server
{
private:
	net_server();
	~net_server();

public:
	static net_server* instance();

public:
	void init();
	void startup();
	void loop();
	void shutdown();
	void destory();

public:
	void set_recv_field_ptr(char *recv_field_ptr) { m_recv_field_ptr = recv_field_ptr; }

private:
	void on_msg_receive(msg_base& msg);

private:
	void _sleep(uint32_t ms);

private:
	static net_server* m_instance;

	char *m_recv_field_ptr;
	std::thread *m_thread_ptr;

	bool m_stop;

	net_mgr* m_net_mgr;
};

#endif