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
set logscale y
set key outside

set ylabel  "Segundos"
set title   "Tempo - v1 x v2"
set terminal qt 0 title "Tempo - v1 x v2"
plot dir.'dados_Tempo-v1.dat' using 1:2 title "<op1-v1>" with linespoints, \
     '' using 1:3 title "<op2-v1>" with linespoints, \
     dir.'dados_Tempo-v2.dat' using 1:2 title "<op1-v2>" with linespoints, \
     '' using 1:3 title "<op2-v2>" with linespoints