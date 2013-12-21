/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* run: sudo apt-get install libssl-dev */

#ifndef SECURE_SOCKET_HH
#define SECURE_SOCKET_HH

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "read_write_interface.hh"
#include "socket.hh"

enum SSL_MODE { CLIENT, SERVER };

class SecureSocket : public ReadWriteInterface
{
private:
    Socket underlying_socket;

    SSL_CTX* ctx;

    SSL* ssl_connection;

    SSL_MODE mode;

public:
    SecureSocket( Socket && sock, SSL_MODE type );

    /* copy constructor */
    SecureSocket( const SecureSocket & );

    /* override assignment operator */
    SecureSocket & operator = ( const SecureSocket & );

    void handshake( void );

    void check_server_certificate( void );

    std::string read( void ) override;

    void write( const std::string & message ) override;

    FileDescriptor & fd( void ) override { return underlying_socket.fd(); }

    Socket & sock( void ) { return underlying_socket; }
};

#endif
