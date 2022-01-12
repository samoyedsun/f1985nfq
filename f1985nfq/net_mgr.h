#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class msg_base;
class net_mgr
{
	friend class net_session;

public:
	//连接回调
	typedef std::function<void(uint32_t session_id, uint32_t para, const char* buf)> wconnect_cb;
	//断开连接回调
	typedef std::function<void(uint32_t session_id)> wdisconnect_cb;
	//消息回调1 隐藏网络消息结构
	typedef std::function<void(uint32_t session_id, msg_base* msg_ptr)> wmsg_cb;

public:
	typedef std::vector<net_session*> wsessions;
	typedef std::vector<std::thread> wthreads;

	typedef boost::asio::io_service wservice;
	typedef boost::asio::io_service::work wwork;
	typedef boost::asio::ip::tcp::acceptor wacceptor;
	typedef boost::asio::ip::tcp::socket wsocket;
	typedef boost::asio::ip::tcp::endpoint wendpoint;
	typedef boost::asio::deadline_timer wtimer;
	typedef boost::system::error_code werror;

public:
	net_mgr();
	~net_mgr();

public:
	bool init(uint32_t send_buf_size, uint32_t recv_buf_size);

	// 启动服务器
	bool startup(uint32_t thread_num, const std::string& ip, uint16_t port);

	// 关闭监听
	void shut_accept();

	// 关闭服务器
	void shutdown();

	//销毁
	void destroy();

	void process_msg();

private:
	// 工作线程
	void _worker_thread();

private:
	//server
	bool _start_listen(const std::string& ip, uint16_t port, uint32_t thread_num);

	void _post_accept();

	void _handle_accept(const werror& error, net_session* session_ptr);

	void *_create_accept_msg(const wsocket& socket);

	uint32_t _get_recv_buf_size();

	msg_base *_create_msg(uint32_t sid, uint16_t msg_id, uint16_t size, const char *msg_ptr);

	void _push_msg(msg_base *msg_ptr);


private:
	//config
	uint32_t		m_max_received_msg_size;
	uint32_t		m_msg_post_placeholder;
	uint32_t		m_send_buf_size;
	uint32_t		m_recv_buf_size;


	//callback
	wconnect_cb		m_connect_callback;
	wdisconnect_cb	m_disconnect_callback;
	wmsg_cb			m_msg_callback;

	wservice		m_io_service;		// io服务
	wwork			m_work;

	wthreads		m_threads;			// 线程池
	wacceptor*		m_acceptor_ptr;		// 监听socket

	std::queue<msg_base *> m_msgs;
};