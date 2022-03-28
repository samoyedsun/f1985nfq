#include <iostream>

#include "boost/version.hpp"
#include "boost/asio.hpp"
#include "boost/array.hpp"
#include "boost/thread/thread.hpp"

using namespace std;
using namespace boost::asio;

void version()
{
	cout << "BOOST VERSION : "
		<< BOOST_VERSION / 100000 << "."
		<< BOOST_VERSION / 100 % 1000 << "."
		<< BOOST_VERSION % 100 << endl;
	return;
}

int main()
{
	version();
	
	try
	{
		io_service io_srv;
		ip::tcp::socket socket(io_srv);

		ip::tcp::endpoint endpoint(ip::address::from_string("127.0.0.1"), 19950);
		socket.connect(endpoint);
		
		size_t len;
		boost::array<char, 128> buf;
		boost::system::error_code ec;

		socket.write_some(buffer("778899"), ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}

		len = socket.read_some(boost::asio::buffer(buf), ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
		
		string remote_host = string(buf.data(), len);
		string::size_type pos = remote_host.find(":", 0);
		string remote_address = remote_host.substr(0, pos);
		string remote_port = remote_host.substr(pos + 1, remote_host.size() - 1);

		cout << "对方地址:" << remote_host << endl;
		cout << "自己地址:"
			<< socket.local_endpoint().address()
			<< ":"
			<< socket.local_endpoint().port()
		<< endl;
		size_t self_port = socket.local_endpoint().port();

		socket.close();

		ip::tcp::acceptor acceptor(io_srv);
                ip::tcp::endpoint svc_endpoint(ip::address_v4::from_string("0.0.0.0"), self_port);
        	acceptor.open(svc_endpoint.protocol());
               	acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
               	acceptor.bind(svc_endpoint);
               	acceptor.listen();

		std::shared_ptr<ip::tcp::socket> p_data_socket = std::make_shared<ip::tcp::socket>(io_srv);
		acceptor.async_accept(*p_data_socket, [p_data_socket](boost::system::error_code ec)
		{
			if (ec)
			{
				cout << "accept error:" << ec.message() << endl;
			}
			else
			{
				cout << "accept success!" << endl;

				boost::array<char, 128> buf;
				size_t len = p_data_socket->read_some(boost::asio::buffer(buf), ec);
				if (ec)
				{
					throw boost::system::system_error(ec);
				}
				cout << "===========read len:" << len << endl;
				cout << "===========read str:" << string(buf.data(), len) << endl;
				
				// 接下来继续读新的数据
			}
		});

		char a;
		cin >> a;
		cout << a;

		boost::thread thrd([&]()
		{
			io_service io_cli;
			ip::tcp::endpoint cli_endpoint(ip::address::from_string(remote_address.c_str()), stoi(remote_port));
			ip::tcp::socket cli_socket(io_cli);
			cout << "connnect to " << remote_host <<  endl;
			cli_socket.connect(cli_endpoint);
			cout << "connnect success " <<  endl;

			cin >> a;
			cout << a;

			cli_socket.write_some(buffer("Hello P2P"), ec);
			if (ec)
			{
				throw boost::system::system_error(ec);
			}
			cout << "send some data" << endl;
			
			io_cli.run();
		});

		io_srv.run();
        	thrd.join();

		cout << "executor finish" << endl;
	}
	catch (std::exception& e)
	{
		cout << "异常:" << e.what() << endl;
	}

	return 0;
}
