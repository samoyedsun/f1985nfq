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

        // waiting peer punch a hole
		int delay_count = 0;
		while (delay_count ++ < 3)
		{
			sleep(1);	
			cout << "after closed socket, delay count : " << delay_count << endl;
		}

		boost::thread thrd([&]()
		{
			io_service io_cli;
			ip::tcp::endpoint cli_endpoint(ip::address::from_string(remote_address.c_str()), stoi(remote_port));
			ip::tcp::socket cli_socket(io_cli);
			cout << "connnect to " << remote_host <<  endl;
			cli_socket.connect(cli_endpoint);
			cout << "connnect success " <<  endl;

			cout << "press any key to continue!" << endl;
			char a;
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
