#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, ContestConfig config)
  : debug_( debug ), config_ ( config ), increase_ack_ (config.window_size),
   curr_seqno_(0), increased_ (false)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  static unsigned int prev_timestamp = 0;
  unsigned int time = timestamp_ms();

  if ( debug_ ) {
    cerr << "At time " << time
	 << " window size is " << config_.window_size << endl;
  }

  if ( config_.mode == ContestConfig::Mode::SimpleAIMD ) {
    if ( time - prev_timestamp >= config_.rtt_estimate ) {
      prev_timestamp = time;
      config_.window_size = config_.window_size + config_.additive_win_growth;
    }
  }
  return config_.window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  if ( config_.mode == ContestConfig::Mode::Vanilla ||
    config_.mode == ContestConfig::Mode::DelayTriggered ) {
    /* Default: take no action */
  } else if ( config_.mode == ContestConfig::Mode::SimpleAIMD ) {
    if ( after_timeout ) {
      config_.window_size = (int) config_.window_size * config_.multiplicative_win_decrease;
      if ( debug_ ) {
        cerr << "Halfed window size after timeout" << endl;
      }
    }
  }

  curr_seqno_ = sequence_number;

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
  static uint64_t last_seq_acked = 0;
  static int duplicate_acks = 0;

  if ( config_.mode == ContestConfig::Mode::Vanilla ) {
    /* Default: take no action */
  } else if ( config_.mode == ContestConfig::Mode::SimpleAIMD ) {

    /* Update duplicate acks */
    if ( sequence_number_acked >= last_seq_acked ) {
      last_seq_acked = sequence_number_acked;
    } else {
      duplicate_acks++;
      if ( duplicate_acks == 3 ) {
        config_.window_size = (int) config_.window_size * config_.multiplicative_win_decrease;
        duplicate_acks = 0;
        cerr << "Halve window size after duplicate acks" << endl;
      }
    }

    /* Update RTT estimate */
    uint64_t round_trip_sample = timestamp_ack_received - send_timestamp_acked;
    config_.rtt_estimate = (int) (config_.rtt_estimate_weight * config_.rtt_estimate +
     (1 - config_.rtt_estimate_weight) * round_trip_sample);
    if ( debug_ ) {
      cerr << " --> RTT estimate: " << config_.rtt_estimate << endl;
    }
  } else if ( config_.mode == ContestConfig::Mode::DelayTriggered ) {
    /* Decrease the window size if the threshold has been crossed, increase if now */
    //cerr << "delay: " << (timestamp_ack_received - send_timestamp_acked) << endl;
    if (timestamp_ack_received - send_timestamp_acked >= config_.rtt_estimate) {
      if (increased_) {
        config_.window_size = (int) (config_.window_size * config_.multiplicative_win_decrease);
        if (config_.window_size == 0) {
          config_.window_size = 1;
        }
        if (debug_) {
          cerr << "Halve window size if delay crosses threshold" << endl;
        }
        increased_ = false;
      }
      if (sequence_number_acked == increase_ack_) {
          increase_ack_ ++;
      }
    } else if (sequence_number_acked == increase_ack_ ){
      config_.window_size = config_.window_size + config_.additive_win_growth;
      increase_ack_ = curr_seqno_;
      if (debug_) {
        cerr << "Incrementing window size" << endl;
      }
      increased_ = true;
    }
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
  return 1000; /* timeout of one second */
}
