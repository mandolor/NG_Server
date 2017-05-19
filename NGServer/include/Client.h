//
//  Client.h
//  2BytesNetworkLib
//
//  Created by Jeki Ulko on 2/20/17.
//  Copyright © 2017 2Bytes. All rights reserved.
//

#ifndef Client_h
#define Client_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>

const int max_length = 1024;

class Client
{
public:
    static void create_session(std::string ip_address,
                               std::string port,
                               const char* data,
                               boost::function<void(char*)>);

private:
    Client(boost::asio::io_service& io_service,
           boost::asio::ssl::context& context,
           boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

    void handle_connect(const boost::system::error_code& error);
    void handle_handshake(const boost::system::error_code& error);

    void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;
    boost::function<void(char*)> m_receive_callback;
    
    char m_request[max_length];
    char m_response[max_length];
};

#endif /* Client_h */
