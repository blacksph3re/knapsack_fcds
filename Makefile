build:
	mpic++ main.cpp -o knapsack -O3

run:
	cat input | mpirun -n 1 knapsack
	
scalingtest:
	zsh ./scalingtest.sh
	
