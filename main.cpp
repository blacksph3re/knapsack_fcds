#include <mpi.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <vector>

class Item {
public:

	int getIndex() {return i;}
	int getWeight() {return w;}
	int getValue() {return v;}
	int getMaxCount(int knapsackCapacity) {
		return knapsackCapacity/getWeight() + 1;
	}
	float getDensity() {
		return (float)(getValue()) / getWeight();
	}

	Item(int index, int value, int weight)
		: i(index), v(value), w(weight)
		{}

	Item()
		: i(-1), v(-1), w(-1)
		{}
private:
	int i;
	int w;
	int v;
};


std::vector<Item> items;
int capacity;
int itemCount;
int maxValue = 0;

void readInput() {
	scanf("%d %d", &itemCount, &capacity);
	items.reserve(itemCount);
	int maxCount = -1;
	int maxCountIdx = -1;
	for(int i=0; i<itemCount; i++) {
		int v, w;
		scanf("%d %d", &v, &w);
		items.emplace_back(i, v, w);
		int tmp = items[i].getMaxCount(capacity);
		if(tmp > maxCount) {
			maxCount = tmp;
			maxCountIdx = i;
		}
	}

	// Put the greatest item up front
	iter_swap(items.begin(), items.begin() + maxCountIdx);
}

void knapsack(int i, int value, int weight) {
	// Recursion break condidions:
	// Either we checked all the items in the sack or there is no more space in the sack
	if(i >= items.size() || weight == capacity) {
		if(weight <= capacity && value > maxValue) {
			maxValue = value;
		}
		return;
	}

	// If we exceeded the space already, we don't have to check for new maximum
	// Also done in for-loop, one check is redundant...
	if(weight > capacity)
		return;

	for(int n=0; n<items[i].getMaxCount(capacity); n++) {
		int nextWeight = weight + items[i].getWeight()*n;
		// If we exceeded the space already, we don't have to check for new maximum
		if(weight>capacity)
			break;
		knapsack(i+1, value + items[i].getValue()*n, nextWeight);
	}
}

int main() {
	int world_size;
	int rank;
	// Init MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// let only root do the input parsing, then bcast it to all other processes
	if(rank == 0)
		readInput();
	MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&itemCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank != 0)
		items.resize(itemCount);
	MPI_Bcast(&items[0], items.size()*sizeof(Item), MPI_BYTE, 0, MPI_COMM_WORLD);

	// Partition the space on the first element, then just go recursively as in example
	for(int n=rank; n<items[0].getMaxCount(capacity); n+=world_size) {
		knapsack(1, items[0].getValue()*n, items[0].getWeight()*n);
	}

	int globalMaxValue = maxValue;
	MPI_Reduce(&maxValue, &globalMaxValue, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

	if(rank==0)
		std::cout << globalMaxValue << std::endl;

	MPI_Finalize();
	return 0;
}
