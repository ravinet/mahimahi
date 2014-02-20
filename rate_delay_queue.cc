#include "rate_delay_queue.hh"

using namespace std;

void RateDelayQueue::write_packets( FileDescriptor & fd ) {
    /* Move packets from delay_queue_ into cell_queue_ */
    string next_packet = delay_queue_.get_next();
    while ( not next_packet.empty() ) {
        cell_queue_.read_packet( next_packet );
        next_packet = delay_queue_.get_next();
    }

    /* Write out packets from cell_queue_ into fd */
    cell_queue_.write_packets( fd );
}
