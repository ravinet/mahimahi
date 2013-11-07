/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <csignal>

#include "signalfd.hh"

using namespace std;

/* add given signals to the mask */
SignalMask::SignalMask( const initializer_list< int > signals )
    : mask_()
{
    SystemCall( "sigemptyset", sigemptyset( &mask_ ) );

    for ( const auto signal : signals ) {
        SystemCall( "sigaddset", sigaddset( &mask_, signal ) );
    }
}

/* block these signals from interrupting our process */
/* (because we'll use a signalfd instead to read them */
void SignalMask::block( void ) const
{
    SystemCall( "sigprocmask", sigprocmask( SIG_BLOCK, &mask(), NULL ) );
}

SignalFD::SignalFD( const SignalMask & signals )
    : fd_( SystemCall( "signalfd", signalfd( -1, &signals.mask(), 0 ) ) )
{
}

/* read one signal */
signalfd_siginfo SignalFD::read_signal( void )
{
    signalfd_siginfo delivered_signal;

    string delivered_signal_str = fd_.read( sizeof( signalfd_siginfo ) );

    if ( delivered_signal_str.size() != sizeof( signalfd_siginfo ) ) {
        throw Exception( "signalfd read size mismatch" );
    }

    memcpy( &delivered_signal, delivered_signal_str.data(), sizeof( signalfd_siginfo ) );

    return delivered_signal;
}
