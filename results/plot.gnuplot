#!/bin/bash

# result_dir=$1

set terminal pdf
set output "test-sup-ewma.pdf"

set ylabel 'suppression time (ms)' font "Times-Roman,14" offset 1, 0
set y2label 'EWMA' font "Times-Roman,14" offset -1, 0
set xlabel 'sequence number' font "Times-Roman,14"
set grid
# set term png
# set terminal pngcairo
# set output 'test-sup-ewma.png'

resutl_dir= ''
set key autotitle columnhead

#set datafile-seperator comma
set datafile separator '	' # separator 2
set timefmt '%s'

set tics font "Helvetica,14"
set key font ",12"

set logscale x 2

set yrange [10:80];
set y2range [1:6];

set xrange [0.8:600];

set y2tics (1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 6)
# set y2tics (1, 1.5, 2.5, 3.5, 4.5, 6)


set label 1 "phase 2: \n\ do nothing" at 25,59 offset 0,1 font "Times-Roman, 12" center

set label 2 "phase 3 (a): \n\ linear decrease" font "Times-Roman,11" center at 1.8,18 offset 0,1 rotate by -8

set label 3 "phase 1: " at 6.5,35 offset 0,1 font "Times-Roman,12" center rotate by 84
set label 4 "exponential increase" at 8.5,35 offset 0,1 font "Times-Roman,12" center rotate by 82


set label 5 "phase 3 (b): " at 52,38 offset 0,1 font "Times-Roman,12" center rotate by -87
set label 6 "linear decrease" at 40,38 offset 0,1 font "Times-Roman,12" center rotate by -86


set label 7 "operating range" at 150,25 offset 0,1 font "Times-Roman, 12" center
 

# exponential increase
set label "D" at 10,15 point pointtype 9 pointsize 1 lc rgb 'red'
set label "B" at 5,10 point pointtype 9 pointsize 1 lc rgb 'red'

set label "C" offset -0.5, 0.25 at 6,15 point pointtype 3 pointsize 1 lc rgb 'brown'
set label "E" right offset -0.9, 0 at 15,55.5 point pointtype 3 pointsize 1 lc rgb 'brown'
set label "F" left offset 0.5,0 at 28,55.5 point pointtype 6 pointsize 1 lc rgb 'black'

set label "A" left offset -2,0 at 1,14 point pointtype 13 pointsize 1 lc rgb 'grey'

set label "G" left offset 0,-.8 at 36,15 point pointtype 6 pointsize 1 lc rgb 'black'


set arrow from 35,26 to 500,26 heads

plot 'sup-ewma.dat' using 0:4 title 'suppression time' with lp pointsize 0.5, \
     'sup-ewma.dat' using 0:3 title 'EWMA' with lp pointsize 0.5 axes x1y2, \
     1.5 title 'duplicate threshold' dt "--" axes x1y2
      
     