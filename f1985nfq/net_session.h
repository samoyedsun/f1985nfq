class net_session
{
public:
	typedef std::mutex wlock_type;
	typedef std::lock_guard<wlock_type> wguard_type;
	typedef boost::asio::ip::tcp::socket wsocket_type;

public:
	net_session(boost::asio::io_service& io_service, net_mgr& net_mgr, uint16_t index);
	~net_session();

	bool init();

	wsocket_type& get_socket();

	void connected();

	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

private:
	bool _parse_msg();

private:
	uint32_t			m_session_id;
	net_mgr&			m_net_mgr;
	wsocket_type		m_socket;

	char*				m_recv_buf_ptr;		// 接收消息
	uint32_t			m_recv_size;
};