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

  /* Max delay of current epoch - max delay of previous epoch, in ms */
  int epoch_max_delay_delta_;

  /* Maps each sequence number sent to the window at the time it was sent */
  std::map<uint64_t, unsigned int> seqno2winsize_;

  /* Maps a window size to an EWMA of experienced delays at that window size */
  std::map<int, double> delay_profile_;

  /* The current window size */
  double window_size_;

  /* The minimum delay in ms experienced so far. */
  int min_delay_;

  /* The estimated delay in ms we should have for the current epoch.
     Verus uses the value of epoch_max_delay_delta_ to tweak this
     value, then it finds the corresponding window size from the delay
     profile and sets it for the next epoch. */
  int est_delay_;

  /* State variables */
  bool in_slow_start_;
  bool in_loss_recovery_;

  /* the timestamp of the start the current epoch */
  uint64_t last_epoch_time_;
  /* the current epoch number */
  uint64_t epoch_no_;

  /* Sets the next est_delay_ given the current value */
  void set_next_delay( uint64_t prev_epoch_max );

  /* Sets the window for the next epoch */
  void set_next_window();

  /* Enters loss recovery state after a delay, timeout, or loss. */
  void enter_loss_recovery();

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

  /* A simple heuristic to smooth outliers in the delay profile */
  void smooth_delay_profile();

  /* Smooths a single point */
  double smooth_point(std::map<int, double> profile, int index, int n);
};

#endif
