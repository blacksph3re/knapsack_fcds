#!/bin/sh

for i in 1 2 4 8
do
	echo "Running strong scalingtest for N=$i"
	time (cat inputmedium | mpirun -n $i knapsack)
done
