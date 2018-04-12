#include <iostream>
#include <cassert>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), 
    max_delay_ewma_a_ ( 0.6 ),
    epoch_max_delay_ ( 100 ),
    epoch_duration_ ( 5 ),
    epoch_packet_delays_{},
    epoch_max_delay_delta_ ( 0 ),
    delay_profile_map_{},
    delay2winsize_{},
    the_window_size ( 1 ),
    in_slow_start_ ( true ),
    min_delay_ ( 100 ),
    slow_start_thresh_ ( 2 ),
    in_loss_recovery_ ( false ),
    r_ ( 1.2 ),
    curr_delay_estimate_ ( 50 ),
    mult_decrease_factor_ ( 0.5 )
{}

void Controller::set_next_delay()
{
  unsigned int delta_1 = 1; // ms
  unsigned int delta_2 = 2; // ms

  if ( epoch_max_delay_ / (double) min_delay_ > r_ ) {
    curr_delay_estimate_ -= delta_2;
  } else if ( epoch_max_delay_delta_ > 0 ) {
    curr_delay_estimate_ -= delta_1;
    if ( curr_delay_estimate_ < min_delay_ ) {
      curr_delay_estimate_ = min_delay_;
    }
  } else {
    curr_delay_estimate_ += delta_2;
  }
}

void Controller::set_next_window()
{
  // Based on curr_delay_estimate_, choose the window. 
  // Works on a heuristic for now.
  map<int, list<int>>::iterator low, prev;
  int target_delay = curr_delay_estimate_;
  low = delay2winsize_.lower_bound(target_delay);
  int found_delay;

  if (low == delay2winsize_.end()) {
    found_delay = delay2winsize_.rbegin()->first; // Get last key in map.
  } else if (low == delay2winsize_.begin()) {
    found_delay = low->first;
  } else {
    prev = std::prev(low);
    if (( target_delay - prev->first) < (low->first - target_delay))
      found_delay = prev->first;
    else
      found_delay = low->first;
  }

  list<int> windows = delay2winsize_[found_delay];
  if ( windows.size() == 0 ) {
    return;
  }
  int avg_window = 0;
  for (auto window : windows) {
    avg_window += window;
  }


  cerr << "SETTING: " << found_delay << ", " << target_delay << endl; 
  the_window_size = (int) ((double) avg_window / windows.size());
}

/* Get current window size, in datagrams.
   Also works as a 'tick' function, called almost
   every millisecond.  */
unsigned int Controller::window_size()
{
  static uint64_t last_epoch_time = 0;
  uint64_t time = timestamp_ms();

  if (time - last_epoch_time > epoch_duration_ && !in_slow_start_) {

    // On every epoch change, we recalculate the delay max, the delta, and update the window
    // size. 
    last_epoch_time = timestamp_ms();
    uint64_t max_delay = 0;
    for (auto it=epoch_packet_delays_.begin(); 
          it != epoch_packet_delays_.end(); it++) {
      if ( debug_ ) {
        cerr << ' ' << *it << endl;
      }
      if ( *it > max_delay )
        max_delay = *it;
    }

    if ( max_delay != 0 ) {

      // Update our current max delay, and store the current delta
      // between time delays. 
      uint64_t prev_epoch_max = epoch_max_delay_;
      epoch_max_delay_ = (uint64_t) (max_delay_ewma_a_ * epoch_max_delay_ + 
          (1 - max_delay_ewma_a_) * max_delay);
      epoch_max_delay_delta_ = epoch_max_delay_ - prev_epoch_max;
    }
    
    // If we are not in loss recovery mode, update window
    if ( !in_loss_recovery_ ) {
      set_next_delay();
      set_next_window();
    }
    epoch_packet_delays_.clear();
  } 
  


  if ( 1 ) {
    cerr << "At time " << time
	 << " window size is " << the_window_size << endl;
  }

  return (int) the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{

  unsigned int window_size_ = window_size();
  delay_profile_map_[sequence_number] = window_size_;

  if ( 0 ) {
    for (auto elem : delay_profile_map_) {
      cerr << elem.first << " " << elem.second << "\n" << endl;
    }
  }
  
  if ( after_timeout ) {
    in_loss_recovery_ = true;
    the_window_size = (int) (the_window_size * mult_decrease_factor_);
    cerr << "LOSS timeout" << endl;
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
  if ( sequence_number_acked != 0 && sequence_number_acked - last_ack_no != 1) {
    in_loss_recovery_ = true;
    the_window_size = (int) (the_window_size * mult_decrease_factor_);
    cerr << "LOSS ack" << endl;
  }
  last_ack_no = sequence_number_acked;
  
  // TODO: need to add a check for loss here and switch to loss mode

  // Add current delay to delay list
  int delay = timestamp_ack_received - send_timestamp_acked;
  if ( delay < min_delay_ ) {
    min_delay_ = delay;
  }

  epoch_packet_delays_.push_back(delay);

  unsigned int packet_window = delay_profile_map_[sequence_number_acked];
  unsigned int current_window = window_size();

  if ( !in_loss_recovery_ ) {
    if (delay2winsize_.count(delay) == 0) {
      delay2winsize_[delay] = {};
    }

    delay2winsize_[delay].push_back((int) packet_window);
    cerr << "PUSHED BACK\n" << endl;
  } else {
    
    the_window_size += 1 / the_window_size;

    // If we receive an ack from after we halved the window,
    // loss recovery is done.
    if ( packet_window <= current_window ) {
      in_loss_recovery_ = false;
    }
  }

  if ( 0 ) {
    for (auto elem : delay2winsize_) {
      cerr << elem.first << endl;
      for (auto elem_inner : elem.second) {
        cerr << " --> " << elem_inner << endl;
      }
    }
  }

  if ( in_slow_start_ && delay  >= slow_start_thresh_ * min_delay_ ) {
    in_slow_start_ = false;
  }

  if ( in_slow_start_ ) {
    the_window_size += 1;
  }

  if ( 0 ) {
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
  return 1000; /* timeout of one second */
}
