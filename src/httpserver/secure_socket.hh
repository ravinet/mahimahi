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
    friend class SSLContext;

private:
    SSL *ssl_;

    SecureSocket( Socket && sock, SSL *ssl );

public:
    ~SecureSocket();

    void connect( void );
    void accept( void );

    /* forbid copying or assignment */
    SecureSocket( const SecureSocket & ) = delete;
    SecureSocket & operator=( const SecureSocket & ) = delete;

    /* allow moving */
    SecureSocket( SecureSocket && other );

    std::string read( void );
    void write( const std::string & message );
};

class SSLContext
{
private:
    SSL_CTX *ctx_;

public:
    SSLContext( const SSL_MODE type );
    ~SSLContext();

    SecureSocket new_secure_socket( Socket && sock );

    /* forbid copying or assignment */
    SSLContext( const SSLContext & ) = delete;
    SSLContext & operator=( const SSLContext & ) = delete;
};

#endif
