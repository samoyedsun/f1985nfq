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

int main(int argc, char *argv[])
{
	version();

	if (argc < 2)
    {
        return 0;
    }
    char *server_address = argv[1];

	try
	{
		io_service io_srv;
		ip::tcp::socket socket(io_srv);

		ip::tcp::endpoint endpoint(ip::address::from_string(server_address), 19950);
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

		string self_address = socket.local_endpoint().address().to_string();
		size_t self_port = socket.local_endpoint().port();

		cout << "对方地址:" << remote_host << endl;
		cout << "自己地址:" << self_address << ":" << self_port << endl;

		self_address = "0.0.0.0";

		socket.shutdown(ip::tcp::socket::shutdown_both, ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
		socket.close(ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}

        {
            try
            {
                io_service io_cli;
                ip::tcp::endpoint cli_endpoint(ip::address::from_string(remote_address.c_str()), stoi(remote_port));
                ip::tcp::socket cli_socket(io_cli);
                cout << "connnect to " << remote_host <<  endl;
			    cli_socket.connect(cli_endpoint);
			    cout << "connnect success " <<  endl;
            }
        	catch (std::exception& e)
            {
                cout << "打洞异常:" << e.what() << endl;
            }
        }

		ip::tcp::acceptor acceptor(io_srv);
        ip::tcp::endpoint svc_endpoint(ip::address_v4::from_string(self_address.c_str()), self_port);
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
				
				// 回一条消息
                p_data_socket->write_some(buffer("Hello P2P"), ec);
                if (ec)
                {
                    throw boost::system::system_error(ec);
                }
                cout << "send some data" << endl;

			}
		});

		cout << "begin listen" << endl;
        io_srv.run();
		cout << "executor finish" << endl;
	}
	catch (std::exception& e)
	{
		cout << "异常:" << e.what() << endl;
	}

	return 0;
}
