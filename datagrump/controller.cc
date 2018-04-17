#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Core Verus parameters */
#define EPSILON         5   // ms, epoch duration
#define DELTA_1         1   // ms
#define DELTA_2         2   // ms
#define R               2   // delay-throughput tradeoff
#define MAX_DELAY_ALPHA 0.3 // alpha for EWMA
#define EST_DELAY_ALPHA 0.3 // alpha for EWMA

/* Additional parameters */
#define SS_THRESH     4  // multiple of min delay to use as the slow start threshold
#define D_MAX_WIN_INC 0.5   // max window size increase per epoch
#define D_MAX_WIN_DEC 30  // max window size decrease per epoch
#define SMOOTH_FACTOR 10  // for smoothing the delay profile
#define MIN_WIN_SIZE  3   // in packets
#define MAX_WIN_SIZE  100 // in packets
#define MULT_DECREASE 0.5 // For the MD in AIMD

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    epoch_max_delay_( 100 ),
    epoch_packet_delays_{},
    epoch_max_delay_delta_( 0 ),
    seqno2winsize_{},
    delay_profile_{},
    window_size_( 1 ),
    min_delay_( 1000 ),
    est_delay_( 50 ),
    in_slow_start_( true ),
    in_loss_recovery_( false ),
    last_epoch_time_( 0 ),
    epoch_no_( 0 )
{}

/* Estimates the delay that the network should have next,
   as specified in Verus. */
void Controller::set_next_delay( uint64_t prev_epoch_max )
{

  if ( prev_epoch_max / (double) min_delay_ > R ) {
    est_delay_ -= DELTA_2;
  } else if ( epoch_max_delay_delta_ > 0) {
    est_delay_ -= DELTA_1;
  } else {
    est_delay_ += DELTA_2;
  }

  est_delay_ = max(est_delay_, min_delay_);
}

void Controller::smooth_delay_profile()
{
  // Heuristically smooth out the delay profile, because there
  // are many outliers.
  //
  // NOTE: Verus interpolates the delay profile using an external library.
  //       this is a heuristic.
  int min_index = -1;
  int max_index = -1;
  std::map<int, double> unsmoothed_delay_profile;
  for ( auto elem : delay_profile_ ) {
    unsmoothed_delay_profile[elem.first] = elem.second;
    if (min_index == -1 || elem.first < min_index) min_index = elem.first;
    if (elem.first > max_index) max_index = elem.first;
  }

  if (delay_profile_[min_index] - min_delay_ > SMOOTH_FACTOR)
    delay_profile_[min_index] = min_delay_ + SMOOTH_FACTOR;
  for (int i = min_index + 1; i <= max_index; i++) {
    double prev_delay = unsmoothed_delay_profile[i-1];
    double curr_delay = unsmoothed_delay_profile[i];

    // If the next element differs from the previous one a lot,
    // smooth out the difference.
    if (prev_delay - curr_delay > SMOOTH_FACTOR) {
      delay_profile_[i] = prev_delay - SMOOTH_FACTOR;
    } else if (curr_delay - prev_delay > SMOOTH_FACTOR) {
      delay_profile_[i] = prev_delay + SMOOTH_FACTOR;
    }
  }
}

/* Set the window size for the next epoch based on est_delay_
   and the delay profile. */
void Controller::set_next_window()
{

  int target_delay = est_delay_;
  double window = MIN_WIN_SIZE;
  double min_diff = 10000;

  smooth_delay_profile();

  // Choose the window size that corresponds to the closest
  // delay to our target delay.
  for (auto elem : delay_profile_)
  {
    if ( elem.second - target_delay < min_diff ) {
      min_diff = fabs(elem.second - target_delay);
      window = elem.first;
    }
  }

  // Make sure that the window size is not changed too abruptly.
  // We care more about overshooting the capacity less than undershooting,
  // so D_MAX_WIN_INC < D_MAX_WIN_DEC.
  if ( window > window_size_ ) {
    if ( fabs(window - window_size_) > D_MAX_WIN_INC )
        window = window_size_ + D_MAX_WIN_INC;
  } else {
    if ( fabs(window - window_size_) > D_MAX_WIN_DEC ) {
        window = window_size_ - D_MAX_WIN_DEC;
    }
  }

  window_size_ = window;
}

/* Get current window size, in datagrams.

   Since this function is called very frequently,
   it serves as the tick function that updates epoch
   information. */
unsigned int Controller::window_size()
{
  uint64_t time = timestamp_ms();

  // If it's time to change epochs...
  if (time - last_epoch_time_ > EPSILON && !in_loss_recovery_ && !in_slow_start_) {

    // ... recalculate the max delay, the max delay delta,
    // and update delay and thus window size we'd like for the coming
    // epoch.

    uint64_t max_delay = 0;
    for ( auto it=epoch_packet_delays_.begin();
          it != epoch_packet_delays_.end(); it++ ) {
      if ( *it > max_delay )
        max_delay = *it;
    }

    uint64_t prev_epoch_max = epoch_max_delay_;
    if ( max_delay != 0 ) {

      // Update our current max delay (EWMA), and store the current delta
      // between time delays.
      epoch_max_delay_ = (uint64_t) (MAX_DELAY_ALPHA * epoch_max_delay_ +
          (1 - MAX_DELAY_ALPHA) * max_delay);
      epoch_max_delay_delta_ = epoch_max_delay_ - prev_epoch_max;
    }

    // Update the next delay estimate and thus the next window.
    set_next_delay( prev_epoch_max );
    set_next_window();

    epoch_packet_delays_.clear();
    last_epoch_time_ = timestamp_ms();
    epoch_no_++;
  }

  if ( debug_ ) {
    cerr << "At time " << time
	 << " window size is " << window_size_ << endl;
  }

  assert(window_size_ <= MAX_WIN_SIZE && window_size_ > 0);

  if (!in_loss_recovery_ && !in_slow_start_) {
    // We allow the window to get smaller in loss recovery
    window_size_ = fmax(MIN_WIN_SIZE, window_size_);
  }

  return window_size_;
}

void Controller::enter_loss_recovery() {
    in_loss_recovery_ = true;
    window_size_ = (int) window_size_ * MULT_DECREASE;
    window_size_ = fmax(0, window_size_);
    est_delay_ = min_delay_;
    epoch_packet_delays_.clear();
    last_epoch_time_ = timestamp_ms();
    epoch_no_ ++;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
            /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{

  seqno2winsize_[sequence_number] = window_size();

  if ( !in_loss_recovery_ && after_timeout ) {
    // Enter loss recovery mode if we had a timeout.
    enter_loss_recovery();
    if ( debug_ ) cerr << "TIMEOUT timeout" << endl;
  }

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << " (timeout = " << after_timeout << ")\n";
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  static uint64_t last_ack_no = 0;
  int observed_delay = timestamp_ack_received - send_timestamp_acked;
  if (!in_loss_recovery_ && sequence_number_acked != 0 && sequence_number_acked - last_ack_no != 1) {
    // Enter loss recovery mode if we missed an ack
    enter_loss_recovery();
    if ( debug_ ) cerr << "TIMEOUT acks" << endl;
  }

  if (!in_loss_recovery_ && observed_delay > 3 * est_delay_) {
    // Enter loss recovery mode if we missed an ack
    enter_loss_recovery();
    if ( debug_ ) cerr << "TIMEOUT delay: " << sequence_number_acked << endl;
  }

  last_ack_no = sequence_number_acked;
  min_delay_ = min(min_delay_, observed_delay);
  epoch_packet_delays_.push_back(observed_delay);

  unsigned int packet_window = seqno2winsize_[sequence_number_acked];

  if ( !in_loss_recovery_ ) {
    if ( delay_profile_.count(packet_window) == 0 ) {
      // Insert window into delay profile
      delay_profile_[packet_window] = observed_delay;
    } else {
      // Update EWMA delay estimate for this window.
      delay_profile_[packet_window] =
          delay_profile_[packet_window] * EST_DELAY_ALPHA +
            (1 - EST_DELAY_ALPHA) * observed_delay;
    }

  } else {

    // In loss recovery we update the window size by one packet
    // every RTT.
    double denom = window_size_;
    if (denom == 0) {
      denom = 1;
    }
    window_size_ += 1 / denom;

    // If we receive an ack from after we halved the window,
    // loss recovery is done.
    if ( packet_window <= window_size() ) {
      in_loss_recovery_ = false;
    }
  }

  if ( in_slow_start_ &&
    (observed_delay  >= SS_THRESH * min_delay_ || window_size_ >= MAX_WIN_SIZE)) {
    in_slow_start_ = false;
  }

  if ( in_slow_start_ ) {
    window_size_ += 1;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return 3 * est_delay_;
}
