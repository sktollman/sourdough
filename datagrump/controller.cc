#include <iostream>
#include <cassert>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), 
    max_delay_ewma_a_ ( 0.5 ),
    delay_ewma_a_ ( 0.5 ),
    epoch_max_delay_ ( 100 ),
    epoch_duration_ ( 5 ),
    epoch_packet_delays_{},
    epoch_max_delay_delta_ ( 0 ),
    delay_profile_map_{},
    delay2winsize_{},
    winsize2delay_{},
    the_window_size ( 1 ),
    in_slow_start_ ( true ),
    min_delay_ ( 1000 ),
    slow_start_thresh_ ( 10 ),
    in_loss_recovery_ ( false ),
    r_ ( 2 ),
    curr_delay_estimate_ ( 50 ),
    mult_decrease_factor_ ( 0.5 )
{}

void Controller::set_next_delay( uint64_t prev_epoch_max )
{
  unsigned int delta_1 = 1; // ms
  unsigned int delta_2 = 2; // ms

  if ( prev_epoch_max / (double) min_delay_ > r_ ) {
    curr_delay_estimate_ -= delta_2;
  } else if ( epoch_max_delay_delta_ > 0) {
    curr_delay_estimate_ -= delta_1;
  } else {
    curr_delay_estimate_ += delta_2;
  }

  if ( curr_delay_estimate_ < min_delay_ ) {
      curr_delay_estimate_ = min_delay_;
  }

  cerr << "DELAY EST: " << curr_delay_estimate_ << endl;
}

void Controller::set_next_window()
{
  // Based on curr_delay_estimate_, choose the window. 
  // Works on a heuristic for now.
 // map<int, int>::iterator low, prev;
  int target_delay = curr_delay_estimate_;
  //cerr << " curr delay_estimate: " << curr_delay_estimate_ << endl;
/*  low = delay2winsize_.lower_bound(target_delay);
  int found_delay;*/


/*  if (low == delay2winsize_.end()) {
    found_delay = delay2winsize_.rbegin()->first; // Get last key in map.
  } else if (low == delay2winsize_.begin()) {
    found_delay = low->first;
  } else {
    prev = std::prev(low);
    if (( target_delay - prev->first) < (low->first - target_delay))
      found_delay = prev->first;
    else
      found_delay = low->first;
  }*/

  double window = 0;
  double min_diff = 10000;

  // Heuristically smooth out map.
  double map_delay_prev = 40;
  int cutoff = 10;
  for(auto elem : winsize2delay_)
  {
    if (fabs(elem.second - map_delay_prev) > cutoff) {
      if (elem.second > map_delay_prev)
        winsize2delay_[elem.first] = map_delay_prev + cutoff;
      else
        winsize2delay_[elem.first] = map_delay_prev - cutoff;
    }
    map_delay_prev = elem.second;
  }

  // Choose closest window size
  for(auto elem : winsize2delay_)
  {
    if (elem.second - target_delay < min_diff) {
      min_diff = fabs(elem.second - target_delay);
      window = elem.first;
    }

    //cerr << " " << elem.first << ", " << elem.second << endl;
  }

  int dmax_window_dec = 10;   
  int dmax_window_inc = 2; 
  if (window > the_window_size) {
    if (fabs(window - the_window_size) > dmax_window_inc)
        window = the_window_size + dmax_window_inc;
  } else {
    if (fabs(window - the_window_size) > dmax_window_dec)
        window = the_window_size - dmax_window_dec;
  }

  cerr << "Window: " << window << endl;
  the_window_size = window;
}


/* Get current window size, in datagrams.
   Also works as a 'tick' function, called almost
   every millisecond.  */
unsigned int Controller::window_size()
{
  static uint64_t last_epoch_time = 0;
  uint64_t time = timestamp_ms();

  if (time - last_epoch_time > epoch_duration_) {

    // On every epoch change, we recalculate the delay max, the delta, and update the window
    // size. 

    uint64_t max_delay = 0;
    for (auto it=epoch_packet_delays_.begin(); 
          it != epoch_packet_delays_.end(); it++) {
      if ( debug_ ) {
        cerr << ' ' << *it << endl;
      }
      if ( *it > max_delay )
        max_delay = *it;
    }

    uint64_t prev_epoch_max = epoch_max_delay_;
    if ( max_delay != 0 ) {

      // Update our current max delay, and store the current delta
      // between time delays. 
      epoch_max_delay_ = (uint64_t) (max_delay_ewma_a_ * epoch_max_delay_ + 
          (1 - max_delay_ewma_a_) * max_delay);
      epoch_max_delay_delta_ = epoch_max_delay_ - prev_epoch_max;
    }
    
    // If we are not in loss recovery mode, update window
    if ( !in_loss_recovery_ && !in_slow_start_) {
      set_next_delay( prev_epoch_max );
      set_next_window();
      //cerr << curr_delay_estimate_ << " " << the_window_size << endl;
    }

    epoch_packet_delays_.clear();
    last_epoch_time = timestamp_ms();
  } 
  


  if ( 0 ) {
    cerr << "At time " << time
	 << " window size is " << the_window_size << endl;
  }

/*  if (time > 130000) {
    for(auto elem : winsize2delay_)
    {
      cout << elem.first << " " << elem.second << endl;
    }
    exit(0);
  }
*/

  if (the_window_size < 3)
    the_window_size = 3;
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

  delay_profile_map_[sequence_number] = the_window_size;

  if ( 0 ) {
    for (auto elem : delay_profile_map_) {
      cerr << elem.first << " " << elem.second << "\n" << endl;
    }
  }
  
  if ( after_timeout ) {
    in_loss_recovery_ = true;
    the_window_size = the_window_size * mult_decrease_factor_;
    if (the_window_size < 1)
      the_window_size = 1;
    curr_delay_estimate_ = min_delay_;
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
    the_window_size = the_window_size * mult_decrease_factor_;
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
  //cerr << "(" << packet_window << ", " << delay << ")" << endl;

  if ( !in_loss_recovery_ ) {
    if (winsize2delay_.count(packet_window) == 0) {
      winsize2delay_[packet_window] = (double) delay;
      delay2winsize_[delay] = packet_window;
    } else {
      double curr_delay = winsize2delay_[packet_window];

      // Update EWMA entry.
      winsize2delay_[packet_window] = curr_delay * delay_ewma_a_ + 
        (1 - delay_ewma_a_) * delay; 

      delay2winsize_[(int) winsize2delay_[packet_window]] = packet_window;
    }

  } else {
    
    the_window_size += 1 / the_window_size;

    // If we receive an ack from after we halved the window,
    // loss recovery is done.
    if ( packet_window <= window_size() ) {
      in_loss_recovery_ = false;
    }
  }

  if ( in_slow_start_ && delay  >= slow_start_thresh_ * min_delay_ ) {
    in_slow_start_ = false;
    cerr << "SLOW START DONE" << endl;
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
