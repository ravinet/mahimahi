/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* run: sudo apt-get install libssl-dev */

#ifndef SECURE_SOCKET_HH
#define SECURE_SOCKET_HH

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "read_write_interface.hh"
#include "socket.hh"

enum SSL_MODE { CLIENT, SERVER };

class Secure_Socket : public ReadWriteInterface
{
private:
    Socket underlying_socket;

    SSL_CTX* ctx;

    SSL* ssl_connection;

    std::string certificate;

    SSL_MODE mode;

public:
    Secure_Socket( Socket && sock, SSL_MODE type, const std::string & cert );

    /* copy constructor */
    Secure_Socket( const Secure_Socket & );

    /* override assignment operator */
    Secure_Socket & operator = ( const Secure_Socket & );

    /* initialize context object */
    void handshake( void );

    void check_server_certificate( void );//const std::string expected_host );

    std::string read( void ) override;

    void write( const std::string & message ) override;

    FileDescriptor & fd( void ) override { return underlying_socket.fd(); }

    Socket & sock( void ) { return underlying_socket; }
};

#endif
