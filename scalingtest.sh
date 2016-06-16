#!/bin/sh

for i in 1 2 4 8 16
do
	echo "Running strong scalingtest for N=$i"
	cat inputlarge | mpirun -n $i knapsack
done
