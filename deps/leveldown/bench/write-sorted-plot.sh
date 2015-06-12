#!/bin/sh

gnuplot <<EOF
  reset
  set terminal png size 1920, 1080
  set output "write_sorted_times.png"
  set datafile separator ','

  #set yrange [0:0.6]

  set xlabel "Seconds"
  set ylabel "Milliseconds per write"

  set title "1.3G / 10,000,000 writes"
  set key below
  set grid

  plot "write_sorted_times_g.csv" using (\$1/1000):(\$2/1000000) title "Google LevelDB" lc rgb "red" lt 7 ps 0.3, \
       "write_sorted_times_h.csv" using (\$1/1000):(\$2/1000000) title "HyperDex LevelDB" lc rgb "green" lt 7 ps 0.3, \
       "write_sorted_times_b.csv" using (\$1/1000):(\$2/1000000) title "Basho LevelDB" lc rgb "blue" lt 7 ps 0.3

EOF