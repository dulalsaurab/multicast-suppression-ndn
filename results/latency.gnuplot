#!/bin/bash

# result_dir=$1
set terminal pdf
set output "latency.pdf"

set grid
# set term png
# set terminal pngcairo
# set output 'latency.png'

resutl_dir= ''
set key autotitle columnhead

# set multiplot layout 4,1 title "Goodput and Total transfer time"

#set datafile-seperator comma
# set datafile separator "," # separator 2
# set timefmt '%s'

set multiplot layout 2,1

set logscale y 2
# set logscale x 10

set tics font "Helvetica,10"
set key font ",8"

set xrange [0:658]
set yrange [0.05:2]
# set ytics (0, 0.2, 0.4, 0.8, 1.6, 2)

set ylabel 'delay stretch' font "Times-Roman,9" offset 3.3,0
set xlabel 'sequence number' font "Times-Roman,9" offset 0,0.5

set key horizontal font "Helvetica, 8" width 2 at graph 0.5, graph 0.2 center maxrows 1

plot  'stretch.dat' using 1:3 with linespoints pointinterval 30 pointsize 0.5 title '2c', \
      'stretch.dat' using 1:4 with linespoints pointinterval 30 pointsize 0.5 title '3c', \
      'stretch.dat' using 1:5 with linespoints pointinterval 30 pointsize 0.5 title '4c', \
      'stretch.dat' using 1:6 with linespoints pointinterval 30 pointsize 0.5 title '5c', \
      'stretch.dat' using 1:7 with linespoints pointinterval 30 pointsize 0.5 title '6c', \
      'stretch.dat' using 1:8 with linespoints pointinterval 30 pointsize 0.5 title '7c', \

