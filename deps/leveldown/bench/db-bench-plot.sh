#!/bin/sh

gnuplot <<EOF
  reset
  set terminal pngcairo truecolor enhanced font "Ubuntu Mono,13" size 1920, 1080
  set output "/tmp/5mbench.png"
  set datafile separator ','

  set logscale y
  set nologscale y2
  unset log y2
  set autoscale y
  set autoscale y2
  set ytics nomirror
  set y2tics
  set tics out

  set xlabel "Minutes" tc rgb "#777777"
  set ylabel "Milliseconds per write" tc rgb "#777777"
  set y2label "Throughput MB/s" tc rgb "#777777"

  set title "Node.js LevelDB (LevelDOWN): 100,000,000 random writes, 64M write buffer, HDD RAID1" tc rgb "#777777"
  set key left tc rgb "#777777"
  set border lc rgb "#777777"

  set style line 1 lt 7 ps 1.2 lc rgb "#55019FD7"
  set style line 2 lt 7 ps 0.1 lc rgb "#55019FD7"
  set style line 3 lt 1 lw 2   lc rgb "#55019FD7"

  set style line 4 lt 7 ps 1.2 lc rgb "#559ECC3C"
  set style line 5 lt 7 ps 0.1 lc rgb "#559ECC3C"
  set style line 6 lt 1 lw 2   lc rgb "#559ECC3C"

  set style line 7 lt 7 ps 1.2 lc rgb "#55CC3C3C"
  set style line 8 lt 7 ps 0.1 lc rgb "#55CC3C3C"
  set style line 9 lt 1 lw 2   lc rgb "#55CC3C3C"

  set style line 10 lt 7 ps 1.2 lc rgb "#553C3C3C"
  set style line 11 lt 7 ps 0.1 lc rgb "#553C3C3C"
  set style line 12 lt 1 lw 2   lc rgb "#553C3C3C"

  plot \
      1/0 with points title "Google LevelDB" ls 1 \
    , 1/0 with points title "Hyper LevelDB"  ls 4 \
    , 1/0 with points title "Basho LevelDB"  ls 7 \
    , 1/0 with points title "LMDB"  ls 10 \
    , "5m_google.csv" using (\$1/1000/60):(\$4/1000000) notitle         ls 2 axes x1y1 \
    , "5m_hyper.csv"  using (\$1/1000/60):(\$4/1000000) notitle         ls 5 axes x1y1 \
    , "5m_basho.csv"  using (\$1/1000/60):(\$4/1000000) notitle         ls 8 axes x1y1 \
    , "5m_lmdb.csv"   using (\$1/1000/60):(\$4/1000000) notitle         ls 11 axes x1y1 \
    , "5m_google.csv" using (\$1/1000/60):(\$5)         w lines notitle ls 3 axes x1y2 \
    , "5m_hyper.csv"  using (\$1/1000/60):(\$5)         w lines notitle ls 6 axes x1y2 \
    , "5m_basho.csv"  using (\$1/1000/60):(\$5)         w lines notitle ls 9 axes x1y2 \
    , "5m_lmdb.csv"   using (\$1/1000/60):(\$5)         w lines notitle ls 12 axes x1y2 \

EOF
