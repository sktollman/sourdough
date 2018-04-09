#ifndef CONTEST_CONFIG_HH
#define CONTEST_CONFIG_HH

/* Congestion Controller config interface */

class ContestConfig
{
  /* Simple wrapper around some config values */
  public:

    /* Default constructor */
    ContestConfig();

    /* What controller mode to run */
    enum Mode: unsigned char
    {
      Vanilla = 0, /* Starter code + choose fixed window size */
      SimpleAIMD = 1, /* For Part B */
      DelayTriggered = 2 /* For Part C */
     /* Add more here ... */
    };

    /* Mode */
    Mode mode;

    /* Vanilla configuration */
    int window_size;

    /* Simple AIMD configuration */
    int additive_win_growth;
    double multiplicative_win_decrease;
    unsigned int rtt_estimate;
    double rtt_estimate_weight;
};

#endif
