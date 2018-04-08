#include "contest_config.hh"

/* Default Constructor */
ContestConfig::ContestConfig() 
  : mode(ContestConfig::Mode::Vanilla),
    start_window_size(0),
    additive_win_growth(0),
    multiplicative_win_decrease(0),
    rtt_estimate(200),
    rtt_estimate_weight(0.8)
{}
