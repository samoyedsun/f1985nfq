#ifndef _NET_CLIENT_H_
#define _NET_CLIENT_H_

class net_client
{
public:
	net_client();
	~net_client();

public:
	static void send_101();
	static void send_102(char *msg);
};

#endif