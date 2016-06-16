#include <mpi.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <chrono>

const bool PRINT_TIMES = false;

long int to_millis(auto input) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(input).count();
}

std::chrono::system_clock::time_point myclock() {
	return std::chrono::system_clock::now();
}

class Item {
public:

	int getIndex() const {return i;}
	int getWeight() const {return w;}
	int getValue() const {return v;}
	int getMaxCount(int knapsackCapacity) const {
		return knapsackCapacity/getWeight() + 1;
	}
	float getDensity() const {return density;}
	
	Item(int index, int value, int weight)
		: i(index), v(value), w(weight), density((float)(value)/weight)
		{}

	Item()
		: i(-1), v(-1), w(-1), density(0)
		{}
		
	bool operator < (const Item& rhs) const {return getDensity() < rhs.getDensity();}
private:
	int i;
	int w;
	int v;
	float density;
};

bool compare(const Item& lhs, const Item& rhs) {
	return lhs.getDensity() < rhs.getDensity();
}



void readInput(std::vector<Item>& items, int& capacity) {
	int itemCount = 0;
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
	
	// Sort by density
	std::sort(items.begin(), items.end());
}

int knapsack(int i, int capacity, const std::vector<Item>& items) {
	if(i >= items.size())
		return 0;
	
	int v, w, r;
	v = w = r = 0;
	
	while(capacity - w >= 0) {
		r = std::max(r, v + knapsack(i+1, capacity-w, items));
		w += items[i].getWeight();
		v += items[i].getValue();
	}
	return r;
}

int main() {
	auto begin = myclock();
	
	int world_size;
	int rank;
	// Init MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// let only root do the input parsing, then bcast it to all other processes
	std::vector<Item> items;
	int capacity;
	
	if(rank == 0)
		readInput(items, capacity);
	int itemCount = items.size();
	MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&itemCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank != 0)
		items.resize(itemCount);
	MPI_Bcast(&items[0], items.size()*sizeof(Item), MPI_BYTE, 0, MPI_COMM_WORLD);
	
	auto startcalc = myclock();

	// Partition the space on the first element, then just go recursively as in example
	int maxValue = 0;
	for(int n=rank; n<items[0].getMaxCount(capacity); n+=world_size) {
		maxValue = std::max(maxValue, 
							items[0].getValue()*n + knapsack(1, capacity - items[0].getWeight()*n, items));
	}

	auto endcalc = myclock();
	
	int globalMaxValue = maxValue;
	MPI_Reduce(&maxValue, &globalMaxValue, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
	
	if(rank==0)
		std::cout << globalMaxValue << std::endl;
	
	auto end = myclock();
	
	if(PRINT_TIMES)
		std::cout << "Rank " << rank << ": " << std::endl
				  << "  " << to_millis(startcalc-begin) << "ms init+map" << std::endl
				  << "  " << to_millis(endcalc-startcalc) << "ms calc" << std::endl
				  << "  " << to_millis(end-endcalc) << "ms reduce" << std::endl;

	MPI_Finalize();
	return 0;
}
