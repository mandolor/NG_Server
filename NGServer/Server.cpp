/*#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

class session
{
public:
session( boost::asio::io_service& io_service,
boost::asio::ssl::context& context )
: socket_( io_service, context )
{
}

ssl_socket::lowest_layer_type& socket()
{
return socket_.lowest_layer();
}

void start()
{
socket_.async_handshake( boost::asio::ssl::stream_base::server,
boost::bind( &session::handle_handshake, this,
boost::asio::placeholders::error ) );
}

void handle_handshake( const boost::system::error_code& error )
{
if ( !error )
{
socket_.async_read_some( boost::asio::buffer( data_, max_length ),
boost::bind( &session::handle_read, this,
boost::asio::placeholders::error,
boost::asio::placeholders::bytes_transferred ) );
}
else
{
delete this;
}
}

void handle_read( const boost::system::error_code& error,
size_t bytes_transferred )
{
if ( !error )
{
boost::asio::async_write( socket_,
boost::asio::buffer( data_, bytes_transferred ),
boost::bind( &session::handle_write, this,
boost::asio::placeholders::error ) );
}
else
{
delete this;
}
}

void handle_write( const boost::system::error_code& error )
{
if ( !error )
{
socket_.async_read_some( boost::asio::buffer( data_, max_length ),
boost::bind( &session::handle_read, this,
boost::asio::placeholders::error,
boost::asio::placeholders::bytes_transferred ) );
}
else
{
delete this;
}
}

private:
ssl_socket socket_;
enum { max_length = 1024 };
char data_[max_length];
};

class server
{
public:
server( boost::asio::io_service& io_service, unsigned short port )
: io_service_( io_service ),
acceptor_( io_service,
boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), port ) ),
context_( boost::asio::ssl::context::sslv23 )
{
context_.set_options(
boost::asio::ssl::context::default_workarounds
| boost::asio::ssl::context::no_sslv2
| boost::asio::ssl::context::single_dh_use );
context_.set_password_callback( boost::bind( &server::get_password, this ) );
context_.use_certificate_chain_file( "user.crt" );
context_.use_private_key_file( "user.key", boost::asio::ssl::context::pem );
context_.use_tmp_dh_file( "dh2048.pem" );

start_accept();
}

std::string get_password() const
{
return "";
}

void start_accept()
{
session* new_session = new session( io_service_, context_ );
acceptor_.async_accept( new_session->socket(),
boost::bind( &server::handle_accept, this, new_session,
boost::asio::placeholders::error ) );
}

void handle_accept( session* new_session,
const boost::system::error_code& error )
{
if ( !error )
{
new_session->start();
}
else
{
delete new_session;
}

start_accept();
}

private:
boost::asio::io_service& io_service_;
boost::asio::ip::tcp::acceptor acceptor_;
boost::asio::ssl::context context_;
};

int main()
{
boost::asio::io_service service;
boost::asio::ip::tcp::endpoint ep( boost::asio::ip::tcp::v4(), 80 ); // listen on 80
boost::asio::ip::tcp::acceptor acc( service, ep );
socket_ptr sock( new boost::asio::ip::tcp::socket( service ) );

server* ng_server( service, 80 );

ng_server->start_accept();
service.run();

return 0;
}*/

#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif



#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
using namespace boost::asio;
using namespace boost::posix_time;
io_service service;

#define MEM_FN(x)       boost::bind(&self_type::x, shared_from_this())
#define MEM_FN1(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define MEM_FN2(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)


class talk_to_client : public boost::enable_shared_from_this<talk_to_client>, boost::noncopyable {
	typedef talk_to_client self_type;
	talk_to_client() : sock_( service ), started_( false ) {}
public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<talk_to_client> ptr;

	void start() {
		started_ = true;
		do_read();
	}
	static ptr new_() {
		ptr new_( new talk_to_client );
		return new_;
	}
	void stop() {
		if ( !started_ ) return;
		started_ = false;
		sock_.close();
	}
	ip::tcp::socket & sock() { return sock_; }
private:
	void on_read( const error_code & err, size_t bytes ) {
		if ( !err ) {
			std::string msg( read_buffer_, bytes );
			// echo message back, and then stop
			do_write( msg + "\n" );
		}
		stop();
	}

	void on_write( const error_code & err, size_t bytes ) {
		do_read();
	}
	void do_read() {
		async_read( sock_, buffer( read_buffer_ ),
			MEM_FN2( read_complete, _1, _2 ), MEM_FN2( on_read, _1, _2 ) );
	}
	void do_write( const std::string & msg ) {
		std::copy( msg.begin(), msg.end(), write_buffer_ );
		sock_.async_write_some( buffer( write_buffer_, msg.size() ),
			MEM_FN2( on_write, _1, _2 ) );
	}
	size_t read_complete( const boost::system::error_code & err, size_t bytes ) {
		if ( err ) return 0;
		bool found = std::find( read_buffer_, read_buffer_ + bytes, '\n' ) < read_buffer_ + bytes;
		// we read one-by-one until we get to enter, no buffering
		return found ? 0 : 1;
	}
private:
	ip::tcp::socket sock_;
	enum { max_msg = 1024 };
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	bool started_;
};

ip::tcp::acceptor acceptor( service, ip::tcp::endpoint( ip::tcp::v4(), 80 ) );

void handle_accept( talk_to_client::ptr client, const boost::system::error_code & err ) {
	client->start();
	talk_to_client::ptr new_client = talk_to_client::new_();
	acceptor.async_accept( new_client->sock(), boost::bind( handle_accept, new_client, _1 ) );
}


int main( int argc, char* argv[] ) {
	talk_to_client::ptr client = talk_to_client::new_();
	acceptor.async_accept( client->sock(), boost::bind( handle_accept, client, _1 ) );
	service.run();
}

