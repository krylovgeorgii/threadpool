#include "threadpool.h"

#include <iostream>
#include <vector>
#include <iterator>

#include <cstdlib>
#include <ctime>
#include <chrono>

template<class Iter>
void quickSort(threadpool::ThreadPool * th, const Iter & first, const Iter & last) {
	auto dis = distance(first, last - 1);
	if (dis > 0) {
		Iter left = first, right = last - 1;
		auto pivot = *(first + dis / 2);

		while (left < right) {
			while (*left < pivot && left < right) {
				++left;
			}

			while (pivot < *right && left < right) {
				--right;
			}

			if (left < right) {
				std::iter_swap(right, left);
				++left;
			}
		}

		if (left != first) {
			--left;
		}
		else if (right != last - 1) {
			++right;
		}

		th->addTask(quickSort<Iter>, th, first, left + 1);
		th->addTask(quickSort<Iter>, th, right, last);
	}
}

void f() {
	threadpool::ThreadPool th(4);
	std::vector<int> arr(50);

	std::srand(unsigned(std::time(0)));
	for (auto&& t : arr) {
		t = rand() % 10;
		std::cout << t << " ";
	}
	std::cout << std::endl;

	th.addTask(quickSort<decltype(arr)::iterator>, &th, arr.begin(), arr.end());

	while (th.getNumWorkThreads() != 0) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
	}

	std::cout << std::endl;
	for (auto&& t : arr) {
		std::cout << t << " ";
	}
	std::cout << std::endl;
}

int main() {
	f();

	char c;
	std::cin >> c;
}