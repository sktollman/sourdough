template_file=controller.cc.tmp
output_file=controller.cc


declare -a epsilon_arr=("5" "10")
declare -a delta1_arr=("1")
declare -a delta2_arr=("2")
declare -a r_arr=("1" "1.5" "2" "2.5" "3")
declare -a max_delay_alpha_arr=("0.3" "0.5" "0.7")
declare -a est_delay_alpha_arr=("0.3" "0.5" "0.7")
declare -a ss_thresh_arr=("5")
declare -a d_max_win_in_arr=("0" "0.5" "1" "2")
declare -a d_max_win_dec_arr=("0" "5" "10" "20")
declare -a d_max_win_inc_bdp_arr=("0" "0.5" "1" "2")
declare -a d_max_win_dec_bdp_arr=("0" "5" "10" "20")
declare -a smooth_factor_arr=("5" "10" "15" "20")
declare -a min_win_size_arr=("0" "1" "3" "4" "5")
declare -a max_win_size_arr=("100")
declare -a mult_decrease_arr=("0.3" "0.5" "0.8")
declare -a timeout_mult_arr=("2" "4" "6")
declare -a delay_timeout_mult_arr=("2" "4" "6")
declare -a delivery_rates_cache_sz_arr=("100")

RANDOM=$$$(date +%s)

# Loop loop loop loop loop loop ...
while [ 1 ]
do
    # Get random sample of parameter options.
    epsilon=${epsilon_arr[$RANDOM % ${#epsilon_arr[@]} ]}
    delta1=${delta1_arr[$RANDOM % ${#delta1_arr[@]} ]}
    delta2=${delta2_arr[$RANDOM % ${#delta2_arr[@]} ]}
    r=${r_arr[$RANDOM % ${#r_arr[@]} ]}
    max_delay_alpha=${max_delay_alpha_arr[$RANDOM % ${#max_delay_alpha_arr[@]} ]}
    est_delay_alpha=${est_delay_alpha_arr[$RANDOM % ${#est_delay_alpha_arr[@]} ]}
    ss_thresh=${ss_thresh_arr[$RANDOM % ${#ss_thresh_arr[@]} ]}
    d_max_win_in=${d_max_win_in_arr[$RANDOM % ${#d_max_win_in_arr[@]} ]}
    d_max_win_dec=${d_max_win_dec_arr[$RANDOM % ${#d_max_win_dec_arr[@]} ]}
    d_max_win_inc_bdp=${d_max_win_inc_bdp_arr[$RANDOM % ${#d_max_win_inc_bdp_arr[@]} ]}
    d_max_win_dec_bdp=${d_max_win_dec_bdp_arr[$RANDOM % ${#d_max_win_dec_bdp_arr[@]} ]}
    smooth_factor=${smooth_factor_arr[$RANDOM % ${#smooth_factor_arr[@]} ]}
    min_win_size=${min_win_size_arr[$RANDOM % ${#min_win_size_arr[@]} ]}
    max_win_size=${max_win_size_arr[$RANDOM % ${#max_win_size_arr[@]} ]}
    mult_decrease=${mult_decrease_arr[$RANDOM % ${#mult_decrease_arr[@]} ]}
    timeout_mult=${timeout_mult_arr[$RANDOM % ${#timeout_mult_arr[@]} ]}
    delay_timeout_mult=${delay_timeout_mult_arr[$RANDOM % ${#delay_timeout_mult_arr[@]} ]}
    delivery_rates_cache_sz=${delivery_rates_cache_sz_arr[$RANDOM % ${#delivery_rates_cache_sz_arr[@]} ]}

    # Write to Shell

    python templater.py $template_file $output_file \
      0:EPSILON:"$epsilon" \
      1:DELTA_1:"$delta1" \
      2:DELTA_2:"$delta2" \
      3:R:"$r" \
      4:MAX_DELAY_ALPHA:"$max_delay_alpha" \
      5:EST_DELAY_ALPHA:"$est_delay_alpha" \
      6:SS_THRESH:"$ss_thresh" \
      7:D_MAX_WIN_INC:"$d_max_win_in" \
      8:D_MAX_WIN_DEC:"$d_max_win_dec" \
      9:SMOOTH_FACTOR:"$smooth_factor" \
      a:MIN_WIN_SIZE:"$min_win_size" \
      b:MAX_WIN_SIZE:"$max_win_size" \
      c:MULT_DECREASE:"$mult_decrease" 2>&1 | tee -a out
    make
    ./run-single 2>&1 | tee -a out

done


## NOTE: it works better to choose random parameters.  If you want to
##       do a full search, comment the big loop above and let the code
##       below execute.

for epsilon in "${epsilon_arr[@]}"
do
for delta1 in "${delta1_arr[@]}"
do
for delta2 in "${delta2_arr[@]}"
do
for r in "${r_arr[@]}"
do
for max_delay_alpha in "${max_delay_alpha_arr[@]}"
do
for est_delay_alpha in "${est_delay_alpha_arr[@]}"
do
for ss_thresh in "${ss_thresh_arr[@]}"
do
for d_max_win_in in "${d_max_win_in_arr[@]}"
do
for d_max_win_dec in "${d_max_win_dec_arr[@]}"
do
for d_max_win_inc_bdp in "${d_max_win_inc_bdp_arr[@]}"
do
for d_max_win_dec_bdp in "${d_max_win_dec_bdp_arr[@]}"
do
for smooth_factor in "${smooth_factor_arr[@]}"
do
for min_win_size in "${min_win_size_arr[@]}"
do
for max_win_size in "${max_win_size_arr[@]}"
do
for mult_decrease in "${mult_decrease_arr[@]}"
do
for timeout_mult in "${timeout_mult_arr[@]}"
do
for delay_timeout_mult in "${delay_timeout_mult_arr[@]}"
do
for delivery_rates_cache_sz in "${delivery_rates_cache_sz_arr[@]}"
do

    python templater.py $template_file $output_file \
      0:EPSILON:"$epsilon" \
      1:DELTA_1:"$delta1" \
      2:DELTA_2:"$delta2" \
      3:R:"$r" \
      4:MAX_DELAY_ALPHA:"$max_delay_alpha" \
      5:EST_DELAY_ALPHA:"$est_delay_alpha" \
      6:SS_THRESH:"$ss_thresh" \
      7:D_MAX_WIN_INC:"$d_max_win_in" \
      8:D_MAX_WIN_DEC:"$d_max_win_dec" \
      9:D_MAX_WIN_INC_BDP:"$d_max_win_inc_bdp" \
      a:D_MAX_WIN_DEC_BDP:"$d_max_win_dec_bdp" \
      b:SMOOTH_FACTOR:"$smooth_factor" \
      c:MIN_WIN_SIZE:"$min_win_size" \
      d:MAX_WIN_SIZE:"$max_win_size" \
      e:MULT_DECREASE:"$mult_decrease" \
      f:TIMEOUT_MULT:"$timeout_mult" \
      g:DELAY_TIMEOUT_MULT:"$delay_timeout_mult"\
      h:DELIVERY_RATES_CACHE_SZ:"$delivery_rates_cache_sz" 2>&1 | tee -a out
    make
    ./run-single 2>&1 | tee -a out

done
done
done
done
done
done
done
done
done
done
done
done
done
done
done
done
done
done
