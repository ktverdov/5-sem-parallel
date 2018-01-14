#!/bin/bash
for a in 2 5 7
do
for b in 2 3 4 
do
var1=$(($a * $b))
mpiexec -n $var1 ./run 10 $a $b 550 30 0.4 0.3 0.15 0.15
cat stats.txt
printf "\n"
done
done