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

template<class Iter>
class QSortThreadPool {
public:
	QSortThreadPool(size_t num) : th(num) { }

	void operator() (Iter first, Iter last) {
		qSortTh(first, last);

		while (th.getNumWorkThreads() != 0) {
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10ms);
		}
	}

	void resize(size_t num) {
		th.resize(num);
	}

private:
	threadpool::ThreadPool th;

	void qSortTh (Iter first, Iter last) {
		auto dis = distance(first, last - 1);
		if (dis > 0) {
			auto border = partQSort(first, last, dis);

			constexpr int distLimit = 10;
			bool dis1 = distance(first, border - 1) > distLimit;
			bool dis2 = distance(border, last - 1) > distLimit;

			auto func = [this](Iter f, Iter l) { qSortTh(f, l); };

			if (dis1 && dis2) {
				th.addTask(func, first, border);
				th.addTask(func, border, last);
			}
			else if (dis1) {
				th.addTask(func, first, border);
				std::sort(border, last);
			}
			else if (dis2) {
				th.addTask(func, border, last);
				std::sort(first, border);
			}
			else {
				std::sort(first, border);
				std::sort(border, last);
			}
		}
	}
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
auto someSort(Func&& sort, Arr arr) {
	std::vector<int> arrCopy(arr->size());
	std::copy(arr->begin(), arr->end(), arrCopy.begin());

	auto startTime = std::chrono::steady_clock::now();
	sort(arrCopy.begin(), arrCopy.end());
	auto finishTime = std::chrono::steady_clock::now();

	std::cout << (validation(arrCopy.begin(), arrCopy.end()) ? "sorted" : "not sorted!!!") << std::endl;

	return std::chrono::duration_cast<std::chrono::nanoseconds>(finishTime - startTime).count();
}

template<class Iter>
bool validation(Iter begin, Iter end) {
	if (begin != end) {
		--end;
	}

	while (end != begin) {
		if (*(end--) < *end) {
			return false;
		}
	}

	return true;
}

void f(size_t sizeArr) {
	using arrType = std::vector<int>;
	auto arr = std::make_shared<arrType>(sizeArr);

	std::srand(unsigned(std::time(0)));
	for (auto&& t : *arr) {
		t = rand() % 10;
	}

	auto time_stdsort = someSort(std::sort<arrType::iterator>, arr);
	std::cout << "std::sort       " << time_stdsort << " nanosec" << std::endl;
	//std::cout << "quickSort " << someSort(quickSort<arrType::iterator>, arr) << std::endl;

	QSortThreadPool<arrType::iterator> qsth(20);
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(30ms);

	auto time_QSortThreadPool = someSort(qsth, arr);
	std::cout << "QSortThreadPool " << time_QSortThreadPool << " nanosec" << std::endl;
	std::cout << "speedup " << double(time_stdsort) / time_QSortThreadPool << std::endl;
}

int main() {
	f(100'000);

	char c;
	std::cin >> c;
}