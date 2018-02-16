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
		auto pivot = [dis, first, left, right] {
			auto middle = *(first + dis / 2);
		
			if (*left <= middle && middle <= *right || *left == * right) {
				return middle;
			}

			if (middle <= *left && *left <= *right) {
				return *left;
			}

			return *right;
		}(); 

		while (left < right) {
			while (*left < pivot && left < right) {
				++left;
			}

			while (pivot < *right && left < right) {
				--right;
			}

			if (left < right) {
				std::iter_swap(right, left);

				if (*left < pivot) {
					++left;
				} else {
					--right;
				}
			}
		}

		if (left != first) {
			--left;
		}
		else if (right != last - 1) {
			++right;
		}

		bool dis1 = distance(first, left) > 3;
		bool dis2 = distance(right, last - 1) > 3;

		if (dis1 && dis2) {
			th->addTask(quickSort<Iter>, th, first, left + 1);
			th->addTask(quickSort<Iter>, th, right, last);
		} else if (dis1) {
			th->addTask(quickSort<Iter>, th, first, left + 1);
			quickSort(th, right, last);
		} else if (dis2) {
			th->addTask(quickSort<Iter>, th, right, last);
			quickSort(th, first, left + 1);
		} else {
			quickSort(th, first, left + 1);
			quickSort(th, right, last);
		}
	}
}

void f() {
	threadpool::ThreadPool th(4);
	std::vector<int> arr(51);

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