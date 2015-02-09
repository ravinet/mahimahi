/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cassert>
#include <vector>
#include <thread>
#include <mutex>

#include "secure_socket.hh"
#include "certificate.hh"
#include "exception.hh"

using namespace std;

/* error category for OpenSSL */
class ssl_error_category : public error_category
{
public:
    const char * name( void ) const noexcept override { return "SSL"; }
    string message( const int ssl_error ) const noexcept override
    {
        return ERR_error_string( ssl_error, nullptr );
    }
};

class ssl_error : public tagged_error
{
public:
    ssl_error( const string & s_attempt,
               const int error_code = ERR_get_error() )
        : tagged_error( ssl_error_category(), s_attempt, error_code )
    {}
};

class OpenSSL
{
private:
    vector<mutex> locks_;

    static void locking_function( int mode, int n, const char *, int )
    {
        if ( mode & CRYPTO_LOCK ) {
            OpenSSL::global_context().locks_.at( n ).lock();
        } else {
            OpenSSL::global_context().locks_.at( n ).unlock();
        }
    }

    static unsigned long id_function( void )
    {
        return pthread_self();
    }

public:
    OpenSSL()
        : locks_( CRYPTO_num_locks() )
    {
        /* SSL initialization: Needs to be done exactly once */
        /* load algorithms/ciphers */
        SSL_library_init();
        OpenSSL_add_all_algorithms();

        /* load error messages */
        SSL_load_error_strings();

        /* set thread-safe callbacks */
        CRYPTO_set_locking_callback( locking_function );
        CRYPTO_set_id_callback( id_function );
    }

    static OpenSSL & global_context( void )
    {
        static OpenSSL os;
        return os;
    }
};

SSLContext::SSLContext( const SSL_MODE type )
    : ctx_()
{
    OpenSSL::global_context();

    ctx_ = SSL_CTX_new( type == CLIENT ? TLSv1_client_method() : TLSv1_server_method() );

    if ( not ctx_ ) {
        throw ssl_error( "SSL_CTL_new" );
    }

    if ( type == SERVER ) {
        if ( not SSL_CTX_use_certificate_ASN1( ctx_, 678, certificate ) ) {
            throw ssl_error( "SSL_CTX_use_certificate_ASN1" );
        }

        if ( not SSL_CTX_use_RSAPrivateKey_ASN1( ctx_, private_key, 1191 ) ) {
            throw ssl_error( "SSL_CTX_use_RSAPrivateKey_ASN1" );
        }

        /* check consistency of private key with loaded certificate */
        if ( not SSL_CTX_check_private_key( ctx_ ) ) {
            throw ssl_error( "SSL_CTX_check_private_key" );
        }
    }
}

SSLContext::~SSLContext()
{
    if ( ctx_ ) {
        SSL_CTX_free( ctx_ );
    }
}

SecureSocket::SecureSocket( Socket && sock, SSL *ssl )
    : Socket( move( sock ) ),
      ssl_( ssl )
{
    if ( not ssl_ ) {
        throw runtime_error( "SecureSocket: constructor must be passed valid SSL structure" );
    }

    if ( not SSL_set_fd( ssl_, num() ) ) {
        throw ssl_error( "SSL_set_fd" );
    }

    /* enable read/write to return only after handshake/renegotiation and successful completion */
    SSL_set_mode( ssl_, SSL_MODE_AUTO_RETRY );
}

SecureSocket::~SecureSocket()
{
    if ( not ssl_ ) { return; }

    /* calling SSL_shutdown seems to raise SIGPIPE */

    SSL_free( ssl_ );
}

SecureSocket::SecureSocket( SecureSocket && other )
    : Socket( move( other ) ),
      ssl_( other.ssl_ )
{
    other.ssl_ = nullptr;
}

SecureSocket SSLContext::new_secure_socket( Socket && sock )
{
    SSL *ssl = SSL_new( ctx_ );
    if ( not ssl ) {
        throw ssl_error( "SSL_new" );
    }

    return SecureSocket( move( sock ), ssl );
}

void SecureSocket::connect( void )
{
    if ( not SSL_connect( ssl_ ) ) {
        throw ssl_error( "SSL_connect" );
    }
}

void SecureSocket::accept( void )
{
    const auto ret = SSL_accept( ssl_ );
    if ( ret == 1 ) {
        return;
    } else {
        throw ssl_error( "SSL_accept" );
    }

    register_read();
}

string SecureSocket::read( void )
{
    /* SSL record max size is 16kB */
    const size_t SSL_max_record_length = 16384;

    char buffer[ SSL_max_record_length ];

    ssize_t bytes_read = SSL_read( ssl_, buffer, SSL_max_record_length );

    /* Make sure that we really are reading from the underlying fd */
    assert( 0 == SSL_pending( ssl_ ) );

    if ( bytes_read == 0 ) {
        int error_return = SSL_get_error( ssl_, bytes_read );
        if ( SSL_ERROR_ZERO_RETURN == error_return ) { /* Clean SSL close */
            set_eof();
        } else if ( SSL_ERROR_SYSCALL == error_return ) { /* Underlying TCP connection close */
            /* Verify error queue is empty so we can conclude it is EOF */
            assert( ERR_get_error() == 0 );
            set_eof();
        }
        register_read();
        return string(); /* EOF */
    } else if ( bytes_read < 0 ) {
        throw ssl_error( "SSL_read" );
    } else {
        /* success */
        register_read();
        return string( buffer, bytes_read );
    }
}

void SecureSocket::write(const string & message )
{
    /* SSL_write returns with success if complete contents of message are written */
    ssize_t bytes_written = SSL_write( ssl_, message.c_str(), message.length() );

    if ( bytes_written < 0 ) {
        throw ssl_error( "SSL_write" );
    }

    register_write();
}
