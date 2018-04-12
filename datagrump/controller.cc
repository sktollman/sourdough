#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), 
    max_delay_ewma_a_ ( 0.9 ),
    epoch_max_delay_ ( 100 ),
    epoch_duration_ ( 10 ),
    epoch_packet_delays_{},
    epoch_max_delay_delta_ ( 0 ),
    delay_profile_map_{},
    window_size2delays_{},
    the_window_size ( 50 )
{}

/* Get current window size, in datagrams.
   Also works as a 'tick' function, called almost
   every millisecond.  */
unsigned int Controller::window_size()
{
  static uint64_t last_epoch_time = 0;
  uint64_t time = timestamp_ms();

  if (time - last_epoch_time > epoch_duration_) {
    cout << "-----------------------------EPOCH change------------------------------" << endl;

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
      cerr << epoch_max_delay_delta_ << endl;
    }
    
    
    epoch_packet_delays_.clear();
  } 
  


  if ( 0 ) {
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
  //static uint64_t last_ack_no = 0; 
  //last_ack_no = sequence_number_acked;

  // Add current delay to delay list
  // TODO: any reason to keep this if we are just taking the max?
  unsigned int delay = timestamp_ack_received - send_timestamp_acked;
  epoch_packet_delays_.push_back(delay);
  unsigned int packet_window = delay_profile_map_[sequence_number_acked];
  if (window_size2delays_.count(packet_window) == 0) {
    window_size2delays_[packet_window] = {};
  }

  window_size2delays_[packet_window].push_back(delay);

  if ( 1 ) {
    for (auto elem : window_size2delays_) {
      cerr << elem.first << endl;
      for (auto elem_inner : elem.second) {
        cerr << " --> " << elem_inner << endl;
      }
    }
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
