#!/usr/bin/gnuplot -c
## set encoding iso_8859_15
set encoding utf
set terminal qt persist
set grid
set style data point
set style function line
set style line 1 lc 3 pt 7 ps 0.3
set boxwidth 1
set xtics
set xrange ["0":]
set xlabel  "N (tamanho da matriz)"
dir = system("dirname ".ARG0)."/"
set logscale x
set key outside

set ylabel  "M/Bytes"
set title   "L3 Bandwidth - v1 x v2"
set terminal qt 0 title "L3 Bandwidth - v1 x v2"
plot dir.'dados_L3-v1.dat' using 1:2 title "<op1-v1>" with linespoints, \
     '' using 1:3 title "<op2-v1>" with linespoints, \
     dir.'dados_L3-v2.dat' using 1:2 title "<op1-v2>" with linespoints, \
     '' using 1:3 title "<op2-v2>" with linespoints