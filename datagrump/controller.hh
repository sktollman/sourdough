#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>

/* Congestion controller interface */

class Controller
{

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* What controller mode to run */
  enum Mode: unsigned char
  {
    Vanilla = 0, /* Starter code + choose fixed window size */
    SimpleAIMD = 1 /* For Part B */
    /* Add more here ... */

  };

  /* Default constructor */
  Controller( const uint window_size, const bool debug, Mode mode );

  /* Get current window size, in datagrams */
  unsigned int window_size();

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp,
			  const bool after_timeout );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms();

private:
  bool debug_; /* Enables debugging output */
  uint window_size_; /* fixed window size */
  Mode mode_; /* What congestion control mode to run */


};

#endif
