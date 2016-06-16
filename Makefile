build:
	mpic++ main.cpp -o knapsack

run:
	cat input | mpirun -n 1 knapsack
	
scalingtest:
	zsh ./scalingtest.sh
	
