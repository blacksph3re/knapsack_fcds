build:
	mpic++ main2.cpp -o knapsack

run:
	cat input | mpirun -n 4 knapsack