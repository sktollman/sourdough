#include <iostream>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, ContestConfig config)
  : debug_( debug ), config_ ( config )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  unsigned int time = timestamp_ms();
  if ( debug_ ) {
    cerr << "At time " << time
	 << " window size is " << config_.window_size << endl;
  }

  return (int) config_.window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  if ( config_.mode == ContestConfig::Mode::Vanilla ) {
    /* Default: take no action */
  } else if ( config_.mode == ContestConfig::Mode::SimpleAIMD ) {
    if ( after_timeout ) {
      config_.window_size = (double) (config_.window_size * config_.multiplicative_win_decrease);
      config_.window_size = fmax(0, config_.window_size);
      config_.window_size = fmin(500, config_.window_size);
      if ( debug_ ) {
        cerr << " --> Cut window size due to timeout\n" << endl;
      }
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
  static uint64_t last_seq_acked = 0;

  if ( config_.mode == ContestConfig::Mode::Vanilla ) {
    /* Default: take no action */
  } else if ( config_.mode == ContestConfig::Mode::SimpleAIMD ) {

    /* If we miss one of the consecutive ACKs after the zeroth one, we consider
       it a loss */
    if (sequence_number_acked - last_seq_acked != 1 && sequence_number_acked != 0) {
      config_.window_size = (double) ((int) config_.window_size * config_.multiplicative_win_decrease);
      config_.window_size = fmax(0, config_.window_size);
      config_.window_size = fmin(500, config_.window_size);
      if ( debug_ ) {
        cerr << " --> Cut window due to missed ACK.\n" << endl;
      }
    } else {

      /* Update cwnd */
      double denom = (int) config_.window_size;
      if (denom == 0)
        denom = 1;
      config_.window_size += config_.additive_win_growth / denom;
    }

    last_seq_acked = sequence_number_acked;

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
  return 30; /* timeout of one second */
}
