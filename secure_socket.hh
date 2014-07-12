/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* run: sudo apt-get install libssl-dev */

#ifndef SECURE_SOCKET_HH
#define SECURE_SOCKET_HH

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "socket.hh"

enum SSL_MODE { CLIENT, SERVER };

class SecureSocket : public Socket
{
private:
    SSL_CTX* ctx;

    SSL* ssl_connection;

    SSL_MODE mode;

public:
    SecureSocket( Socket && sock, SSL_MODE type );

    /* forbid copying or assignment */
    SecureSocket( const SecureSocket & ) = delete;
    SecureSocket & operator=( const SecureSocket & ) = delete;

    void handshake( void );

    void check_server_certificate( void );

    std::string read( void );

    void write( const std::string & message );

    Address original_dest( void ) const { return Socket::original_dest(); }
};

#endif
