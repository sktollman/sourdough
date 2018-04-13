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
  double delay_ewma_a_;

  uint64_t epoch_max_delay_; /* Max delay at current epoch in ms */
  unsigned int epoch_duration_; /* How long our epochs take, in ms */ 
  std::list<uint64_t> epoch_packet_delays_;
  int epoch_max_delay_delta_;
  std::map<uint64_t, unsigned int> delay_profile_map_;
  std::map<int, int> delay2winsize_;
  std::map<int, double> winsize2delay_;
  double the_window_size;
  bool in_slow_start_;
  int min_delay_;
  int slow_start_thresh_;
  bool in_loss_recovery_;
  double r_;
  int curr_delay_estimate_;
  double mult_decrease_factor_;

  void set_next_delay( uint64_t prev_epoch_max );
  void set_next_window();

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
