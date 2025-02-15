#include <chrono>

#include "abstract_dualpi2_packet_queue.hh"
#include "timestamp.hh"

using namespace std;

#define DQ_COUNT_INVALID   (uint32_t)-1

AbstractDualPI2PacketQueue::AbstractDualPI2PacketQueue( const string & args )
  : DroppingPacketQueue(args),
    recur_count_ (0)
{
  
}

uint32_t scale_prob( double prob )
{
    if ( prob < 0.0 || prob > 1.0 )
        throw runtime_error ("Probability out of range! Provided value: " + std::to_string(prob));

    return static_cast<uint32_t> ( prob * MAX_PROB ) ;
}

