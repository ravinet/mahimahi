/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include "poller.hh"
#include "util.hh"

using namespace std;
using namespace PollerShortNames;

Poller::Poller()
    : epoll_fd_( SystemCall( "epoll_create", epoll_create1( 0 ) ) ),
      actions_(),
      events_()
{}

epoll_event Poller::Action::to_epoll_event( const uint32_t index ) const
{
    epoll_event ret;

    ret.data.u32 = index;
    ret.events = (active and when_interested()) ? direction : 0;

    /* don't poll in on fds that have had EOF */
    if ( direction == Direction::In and fd.eof() ) {
        ret.events = 0;
    }

    return ret;
}

void Poller::add_action( Poller::Action action )
{
    events_.push_back( action.to_epoll_event( actions_.size() ) );
    actions_.push_back( action );
    SystemCall( "epoll_ctl", epoll_ctl( epoll_fd_.num(), EPOLL_CTL_ADD, action.fd.num(), &events_.back() ) );
}

Poller::Result Poller::poll( const int & timeout_ms )
{
    assert( events_.size() == actions_.size() );

    /* tell epoll whether we care about each fd */
    for ( unsigned int i = 0; i < actions_.size(); i++ ) {
        const epoll_event new_event = actions_[ i ].to_epoll_event( i );
        if ( new_event.events != events_[ i ].events ) {
            events_[ i ] = new_event;
            SystemCall( "epoll_ctl", epoll_ctl( epoll_fd_.num(), EPOLL_CTL_MOD, actions_[ i ].fd.num(), &events_[ i ] ) );
        }
    }

    /* Quit if no member in events_ has a non-zero direction */
    if ( not accumulate( events_.begin(), events_.end(), false,
                         [] ( bool acc, const epoll_event & x ) { return acc or x.events; } ) ) {
        return Result::Type::Exit;
    }

    epoll_event ready_fd;

    if ( 0 == SystemCall( "epoll_wait", epoll_wait( epoll_fd_.num(), &ready_fd, 1, timeout_ms ) ) ) {
        return Result::Type::Timeout;
    }

    if ( ready_fd.events & (EPOLLERR | EPOLLHUP) ) {
        return Result::Type::Exit;
    }

    const unsigned int action_index = ready_fd.data.u32;

    if ( action_index >= actions_.size() ) {
        throw Exception( "epoll", "returned event index larger than available actions" );
    }

    if ( events_[ action_index ].events & ready_fd.events ) {
            /* we only want to call callback if revents includes
               the event we asked for */
            auto result = actions_[ action_index ].callback();

            switch ( result.result ) {
            case ResultType::Exit:
                return Result( Result::Type::Exit, result.exit_status );
            case ResultType::Cancel:
                actions_[ action_index ].active = false;
            case ResultType::Continue:
                break;
            }
    }

    return Result::Type::Success;
}
