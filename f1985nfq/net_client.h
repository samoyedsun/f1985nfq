#ifndef _NET_CLIENT_H_
#define _NET_CLIENT_H_
#include <iostream>
#include <string>

class net_client
{
private:
	net_client();
	~net_client();

public:
	static net_client* instance();

public:
	void destory();

public:
	void send_101();
	void send_102(char *msg);

public:
	void set_remote_host(std::string &rhost) { m_remote_host = rhost; };

private:
	static net_client* m_instance;

	std::string m_remote_host;
};

#endif