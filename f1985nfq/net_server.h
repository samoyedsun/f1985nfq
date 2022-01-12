#include <cstdint>

class net_mgr;
class net_server
{
private:
	net_server();
	~net_server();

public:
	static net_server* instance();

public:
	void init();
	void loop();
	void destory();
	void stop();

private:
	void _sleep(uint32_t ms);

private:
	static net_server* m_instance;

	bool m_stop;

	net_mgr* m_net_mgr;
};
