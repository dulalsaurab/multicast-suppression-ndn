#!/bin/bash

# result_dir=$1

# set terminal pdf
# set output "st.pdf"

set ylabel 'count' font "Times-Roman,9"
set xlabel 'time (at c1)' font "Times-Roman,9"
set grid
set term png
set terminal pngcairo
set output 'latency.png'

resutl_dir= ''
set key autotitle columnhead

#set datafile-seperator comma
set datafile separator "," # separator 2
# set timefmt '%s'

# set multiplot layout 3,1 title "Interest dc/ema over time"

set tics font "Helvetica,10"
set key font ",8"

# set yrange [0:1];
# set xrange [1:200];
# #set xtics 0,10,300
# set ytics 0,100,350

# `paste -d "," w_s_1p3c w_o_s_1p3c | awk -F "," '{print $1 "," $5/$10}' > f.dat`


plot 'check00' using 2:5 with lines title 'w/ sup'
     # 'w_o_s_1p3c' using 1:5 with lines title 'w/o sup', \
     # 'w_s_1p3c' with linespoints linestyle 1, \

# #set title '@ (c1)'
# set xlabel 'time (at c1)' font "Times-Roman,9"
# plot '/tmp/minindn/2.interest' using 0:2 title 'dc' with lines, \
#      '/tmp/minindn/2.interest' using 0:3 title 'st' with lines,\


# #set title '@ (c2)'
# set xlabel 'time (at c2)' font "Times-Roman,9"
# plot '/tmp/minindn/3.interest' using 0:2 title 'dc' with lines, \
#      '/tmp/minindn/3.interest' using 0:3 title 'ema' with lines,\


# set xlabel 'time (at c3)' font "Times-Roman,9"
# plot '/tmp/minindn/4.interest' using 0:2 title 'dc' with lines, \
#      '/tmp/minindn/4.interest' using 0:3 title 'ema' with lines,\

   #set output