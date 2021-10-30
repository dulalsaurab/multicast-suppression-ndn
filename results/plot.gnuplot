#!/bin/bash

result_dir=$1

gnuplot <<- EOF
   set ylabel 'count' font "Times-Roman,9"
   set xlabel 'time (at c1)' font "Times-Roman,9"
   set grid
   set term png
   set terminal pngcairo
   set output 'test.png'

   resutl_dir= ''
   set key autotitle columnhead

   #set datafile-seperator comma
   set datafile separator ',' # separator 2
   set timefmt '%s'

   set multiplot layout 3,1 title "Interest dc/ema over time"

   set tics font "Helvetica,10"
   set key font ",8"

   set yrange [*:*];
   #set xrange [1:150];
   #set xtics 0,10,300
   set ytics 0,100,350


   #set title '@ (c1)'
   set xlabel 'time (at c1)' font "Times-Roman,9"
   plot '$result_dir/2.interest' using 0:1 title 'dc' with lines, solid 1.0 border lt -1, \
        '$result_dir/2.interest' using 0:2 title 'st' with lines,\


   #set title '@ (c2)'
   set xlabel 'time (at c2)' font "Times-Roman,9"
   plot '$result_dir/3.interest' using 0:2 title 'dc' with lines, \
        '$result_dir/3.interest' using 0:3 title 'ema' with lines,\


   set xlabel 'time (at c3)' font "Times-Roman,9"
   plot '$result_dir/4.interest' using 0:2 title 'dc' with lines, \
        '$result_dir/4.interest' using 0:3 title 'ema' with lines,\


   #set output

EOF