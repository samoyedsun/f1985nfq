#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "boost/version.hpp"
#include "boost/asio.hpp"
#include "boost/array.hpp"

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

	map<string, ip::tcp::socket> conn_map;

	io_service iosrv;	
	ip::tcp::acceptor acceptor(iosrv, ip::tcp::endpoint(ip::tcp::v4(), 19950));

	boost::array<char, 128> buf;
	for (;;)
	{
		ip::tcp::socket socket(iosrv);
		acceptor.accept(socket);
		
		cout << socket.remote_endpoint().address()
			<< ":" << socket.remote_endpoint().port() << endl;

		boost::system::error_code ec;

		socket.read_some(boost::asio::buffer(buf), ec);
		if (ec)
		{
			cout << "on read, error : " << boost::system::system_error(ec).what() << endl;
			break;
		}

		string token = buf.data();
		auto it = conn_map.find(token);
		if (it == conn_map.end())
		{
			conn_map.insert(make_pair(token, move(socket)));
			cout << "recv one connection." << endl;
		}
		else
		{
			cout << "recv two connection." << endl;

			string host = string(socket.remote_endpoint().address().to_string())
				+ ":" + to_string(socket.remote_endpoint().port());
			it->second.write_some(buffer(host), ec);
			if (ec)
			{
				cout << "on write, error : " << boost::system::system_error(ec).what() << endl;
				break;
			}

			host = string(it->second.remote_endpoint().address().to_string())
				+ ":" + to_string(it->second.remote_endpoint().port());
			socket.write_some(buffer(host), ec);
			if (ec)
			{
				cout << "on write, error : " << boost::system::system_error(ec).what() << endl;
				break;
			}
			
			conn_map.erase(it);
			cout << "broadcasting finish." << endl;
		}
	}

	return 0;
}
