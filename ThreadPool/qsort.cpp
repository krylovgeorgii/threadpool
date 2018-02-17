#include "threadpool.h"

#include <iostream>
#include <vector>
#include <iterator>

#include <cstdlib>
#include <ctime>
#include <chrono>

#include <algorithm>

template<class Iter>
Iter partQSort(Iter first, Iter last, int dis) {	
	Iter left = first, right = last - 1;
	auto pivot = [dis, first, left, right] {
		auto middle = *(first + dis / 2);

		if (*left <= middle && middle <= *right || *left == *right) {
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
			}
			else {
				--right;
			}
		}
	}

	if (left != first) {
		--left;
	}

	return ++left;
}


class QSortThreadPool {
public:
	QSortThreadPool(size_t num) : th(num) { }

	template<class Iter>
	void operator() (Iter first, Iter last) {
		auto dis = distance(first, last - 1);
		if (dis > 0) {
			auto border = partQSort(first, last, dis);

			bool dis1 = distance(first, border - 1) > 3;
			bool dis2 = distance(border, last - 1) > 3;

			if (dis1 && dis2) {
				th.addTask(quickSort<Iter>, first, border);
				th.addTask(quickSort<Iter>, border, last);
			}
			else if (dis1) {
				th.addTask(quickSort<Iter>, first, border);
				std::sort(border, last);
			}
			else if (dis2) {
				th.addTask(quickSort<Iter>, border, last);
				std::sort(first, border);
			}
			else {
				std::sort(first, border);
				std::sort(border, last);
			}
		}
	}

	threadpool::ThreadPool th;
};

template<class Iter>
void quickSort(Iter first, Iter last) {
	auto dis = distance(first, last - 1);
	if (dis > 0) {
		auto border = partQSort(first, last, dis);

		quickSort(first, border);
		quickSort(border, last);
	}
}

template<class Func, class Arr>
void sortFunc(Func&& sort, Arr&& arr) {
	sort(arr.begin(), arr.end());
}

template<class Func, class Arr>
auto someSort(Func&& sort, Arr arr) {
	std::vector<int> arrCopy(arr->size());
	std::copy(arr->begin(), arr->end(), arrCopy.begin());

	auto startTime = std::chrono::steady_clock::now();
	sortFunc(sort, arrCopy);
	auto finishTime = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();
}

void f(size_t sizeArr) {
	using arrType = std::vector<int>;
	auto arr = std::make_shared<arrType>(sizeArr);
	std::srand(unsigned(std::time(0)));
	for (auto&& t : *arr) {
		t = rand() % 10;
	}


	auto getTime_QSortThreadPool = [arr] {
		std::vector<int> arrCopy(arr->size());
		std::copy(arr->begin(), arr->end(), arrCopy.begin());
		QSortThreadPool qsth(4);

		auto startTime = std::chrono::steady_clock::now();
		sortFunc(qsth, arrCopy);
		
		while (qsth.th.getNumWorkThreads() != 0) {
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10ms);
		}
		auto finishTime = std::chrono::steady_clock::now();

		return std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();
	};

	std::cout << "std::sort " << someSort(std::sort<arrType::iterator>, arr) << std::endl;
	std::cout << "QSortThreadPool " << getTime_QSortThreadPool() << std::endl;
	std::cout << "quickSort " << someSort(quickSort<arrType::iterator>, arr) << std::endl;
}

int main() {
	f(100'000);

	char c;
	std::cin >> c;
}