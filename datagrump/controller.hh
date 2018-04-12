#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <list>
#include <map>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  double max_delay_ewma_a_;
  
  uint64_t epoch_max_delay_; /* Max delay at current epoch in ms */
  unsigned int epoch_duration_; /* How long our epochs take, in ms */ 
  std::list<uint64_t> epoch_packet_delays_;
  int epoch_max_delay_delta_;
  std::map<uint64_t, unsigned int> delay_profile_map_;
  std::map<unsigned int, std::list<unsigned int>> window_size2delays_;
  double the_window_size;

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

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
};

#endif
