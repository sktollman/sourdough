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

  /* Max delay at current epoch in ms */
  uint64_t epoch_max_delay_;

  /* List of packet delays for the current epoch */
  std::list<uint64_t> epoch_packet_delays_;

  std::list<double> rtts_;
  std::list<double> delivery_rates_; // In packets/ms

  /* Max delay of current epoch - max delay of previous epoch, in ms */
  int epoch_max_delay_delta_;

  /* Maps each sequence number sent to the window at the time it was sent */
  std::map<uint64_t, unsigned int> seqno2winsize_;

  /* Maps a window size to an EWMA of experienced delays at that window size */
  std::map<int, double> delay_profile_;

  /* The current window size */
  double the_window_size;

  /* The minimum delay in ms experienced so far. */
  int min_delay_;

  /* The estimated delay in ms we should have for the current epoch. 
     Verus uses the value of epoch_max_delay_delta_ to tweak this
     value, then it finds the corresponding window size from the delay
     profile and sets it for the next epoch. */
  int est_delay_;

  int epoch_throughput_;
  double epoch_allowed_;
  int epoch_sent_;
  int inflight_;
  int curr_delay_obs_;

  double rtt_prop_;
  double btlbw_;

  /* State variables */
  bool in_slow_start_;
  bool in_loss_recovery_;

  /* Sets the next est_delay_ given the current value */
  void set_next_delay( uint64_t prev_epoch_max );

  /* Sets the window for the next epoch */
  void set_next_window( uint64_t curr_epoch );
  void set_next_window_2( uint64_t prev_epoch_max );

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
