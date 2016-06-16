build:
	mpic++ main.cpp -o knapsack -O3

run:
	cat inputlarge | mpirun -n 1 knapsack
	
scalingtest:
	zsh ./scalingtest.sh
	
