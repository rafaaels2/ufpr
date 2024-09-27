#!/bin/bash
# set -x
# trap read debug

if $(echo "performance" > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor); then
   echo "CPU setada para performance"
else
   echo "Erro tentar mudar CPU para performance, por favor rode: sudo chmod a+rw /sys/devices/system/cpu/cpufreq/policy3/scaling_governor"
   exit
fi

echo "Rodando make clean e make..."

make clean -C ./v1 > /dev/null
make -C ./v1 > /dev/null

make clean -C ./v2 > /dev/null
make -C ./v2 > /dev/null

rm -f ./saida/*.txt > /dev/null
rm -f ./saida/*.dat > /dev/null
rm -f ./saida/done-v1 > /dev/null
rm -f ./saida/done-v2 > /dev/null

#pega core mais isolado da CPU
CORES=$(likwid-topology | grep Cores | cut -c 19-)
let CORES--

#prepara arquivos de plot
for O in L3 Tempo L2CACHE FLOPS_DP-AVX FLOPS_DP
do
   echo "# Marcador "v1" $O"  >> ./saida/dados_$O-v1.dat
   echo "# n op1 op2" >> ./saida/dados_$O-v1.dat

   echo "# Marcador "v2" $O"  >> ./saida/dados_$O-v2.dat
   echo "# n op1 op2" >> ./saida/dados_$O-v2.dat
done

#Loop de testes
for V in v1 v2
do
   for O in L3 L2CACHE FLOPS_DP
   do   
      echo "Executando teste de $O - $V"
      for N in 32 64 128 256 512 1000 2000 4000 8000
      do
         printf "\tExecutando teste para N = $N\n"    
         FILE=${O}_${N}_${V}

         #vai para o diretorio da versao
         pushd ./$V > /dev/null

         likwid-perfctr -C $CORES -g $O -m ./cgSolver -i 150 -p 1 -o ../saida/result_$N-$V.txt -n $N -k 7 > ../saida/$FILE.txt

         # volta ao diretorio raiz
         popd > /dev/null

         case $O in

         L3)
               RES=$(cat ./saida/$FILE.txt | grep "L3 bandwidth" | sed 's/[^0-9.]*//g' | sed 'N;s/\n/ /')

               echo "$N $RES" >> ./saida/dados_$O-$V.dat
               ;;

         L2CACHE)
               RES="$(cat ./saida/$FILE.txt | grep "miss ratio" | sed 's/[^0-9.]*//g' | cut -c 2- | sed 'N;s/\n/ /')"
               
               echo "$N $RES" >> ./saida/dados_$O-$V.dat
               ;;

         FLOPS_DP)
               #separa saida avx e sse
               AVX="$(cat ./saida/$FILE.txt | grep "MFLOP/s" | sed 's/[^0-9.]*//g' | sed -n 'n;p' | sed 'N;s/\n/ /')"
               SSE="$(cat ./saida/$FILE.txt | grep "MFLOP/s" | sed 's/[^0-9.]*//g' | sed -n 'p;n' | sed 'N;s/\n/ /')"

               echo "$N $AVX" >> ./saida/dados_$O-AVX-$V.dat

               echo "$N $SSE" >> ./saida/dados_$O-$V.dat
               ;;

         esac
      done
      touch ./saida/done-$V
   done
done

echo "powersave" > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor
echo "CPU setada pra powersave"
