#include <mpi.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <chrono>

const bool PRINT_TIMES = true;
const bool PRINT_DEBUG = false;

long int to_millis(auto input) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(input).count();
}

std::chrono::system_clock::time_point myclock() {
	return std::chrono::system_clock::now();
}

class Item {
public:

	int getWeight() const {return w;}
	int getValue() const {return v;}
	int getMaxCount() const {return maxCount;}
	float getDensity() const {return density;}
	
	Item(int value, int weight, int capacity)
		: v(value), w(weight), density((float)(value)/weight), maxCount(capacity/weight + 1)
		{}

	Item()
		: w(-1), v(-1), density(0), maxCount(0)
		{}
		
	bool operator < (const Item& rhs) const {return getDensity() < rhs.getDensity();}
private:
	int w;
	int v;
	float density;
	int maxCount;
};

bool compare(const Item& lhs, const Item& rhs) {
	return lhs.getDensity() > rhs.getDensity();
}



void readInput(std::vector<Item>& items, int& capacity) {
	int itemCount = 0;
	scanf("%d %d", &itemCount, &capacity);
	items.reserve(itemCount);
	for(int i=0; i<itemCount; i++) {
		int v, w;
		scanf("%d %d", &v, &w);
		items.emplace_back(v, w, capacity);
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
	
	if(rank == 0) {
		readInput(items, capacity);
		if(PRINT_DEBUG) {
			std::cout << "Processing " << items.size() << " items, knapsack capacity: " << capacity << std::endl;
			std::cout << "Using " << world_size << " processes" << std::endl;
		}
	}
	int itemCount = items.size();
	MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&itemCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank != 0)
		items.resize(itemCount);
	MPI_Bcast(&items[0], items.size()*sizeof(Item), MPI_BYTE, 0, MPI_COMM_WORLD);
	
	auto startcalc = myclock();
	
	
	int splitdepth = 10;
	
	// Partition based on splitdepth dimensions
	std::vector<int> n;
	n.resize(splitdepth, 0);

	// Increments the n vector by step, leaving a valid vector
	auto increment = [&](int step){
		int carry=step;
		for(int i=0; i<splitdepth; i++) {
			if(carry == 0)
				break;
			
			n[i] += carry;
			carry = n[i]/items[i].getMaxCount();
			n[i] = n[i]%items[i].getMaxCount();
		}
		return carry==0;
	};
	
	/*for(int i=0; i<100; i++) {
		bool tmp = increment(1);
		std::cout << n[0] << " " << n[1] << " " << n[2] << " - " << tmp << std::endl;
	}*/
	int x = 0;

	int maxValue = 0;
	if(increment(rank)) {
		do {
			int curValue = 0;
			int curWeight = 0;
			for(int i=0; i<splitdepth; i++) {
				curValue += n[i]*items[i].getValue();
				curWeight += n[i]*items[i].getWeight();
			}
			x++;
			if(capacity - curWeight < 0)
				continue;
			maxValue = std::max(maxValue, 
								curValue + knapsack(splitdepth, capacity - curWeight, items));
			
		} while(increment(world_size));
	}
	else if(PRINT_DEBUG) {
		std::cout << "Fatal error, less items in first " << splitdepth << " dimensions than threads" << std::endl;
	}

	if(PRINT_DEBUG)
		std::cout << "Rank " << rank << " load " << x << std::endl;
	
	auto endcalc = myclock();
	
	int globalMaxValue = maxValue;
	MPI_Reduce(&maxValue, &globalMaxValue, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
	
	if(rank==0)
		std::cout << globalMaxValue << std::endl;
	MPI_Finalize();
	
	auto end = myclock();
	
	if(PRINT_TIMES)
		std::cout << "Rank " << rank << ": "
				  << "  " << to_millis(startcalc-begin) << "ms init+map"
				  << ",  " << to_millis(endcalc-startcalc) << "ms calc"
				  << ",  " << to_millis(end-endcalc) << "ms reduce" 
				  << ",  " << to_millis(end-begin) << "ms total" << std::endl;

	
	return 0;
}
