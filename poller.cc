/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "poller.hh"
#include "util.hh"

using namespace std;

void Poller::add_action( Poller::Action action )
{
    actions_.push_back( action );
    pollfds_.push_back( { action.fd.num(), 0, 0 } );
}

Poller::Result Poller::poll( const int & timeout_ms )
{
    assert( pollfds_.size() == actions_.size() );

    /* tell poll whether we care about each fd */
    for ( unsigned int i = 0; i < actions_.size(); i++ ) {
        assert( pollfds_.at( i ).fd == actions_.at( i ).fd.num() );
        pollfds_.at( i ).events = actions_.at( i ).when_interested ? actions_.at( i ).direction : 0;
    }

    const int poll_return = ::poll( &pollfds_[ 0 ], pollfds_.size(), timeout_ms );

    if ( poll_return < 0 ) {
        throw Exception( "poll" );
    } else if ( poll_return == 0 ) {
        return Result::Type::Timeout;
    }

    for ( unsigned int i = 0; i < pollfds_.size(); i++ ) {
        if ( pollfds_[ i ].revents & (POLLERR | POLLHUP | POLLNVAL) ) {
            throw Exception( "poll fd error" );
        }

        if ( pollfds_[ i ].revents & pollfds_[ i ].events ) {
            /* we only want to call callback if revents includes
               the event we asked for */
            auto result = actions_.at( i ).callback();
            if ( result.result == Action::Result::Type::Exit ) {
                return Result( Result::Type::Exit, result.exit_status );
            }
        }
    }

    return Result::Type::Success;
}
