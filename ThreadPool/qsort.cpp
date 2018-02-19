#include "threadpool.h"

#include <iostream>
#include <vector>
#include <iterator>

#include <cstdlib>
#include <ctime>
#include <chrono>

#include <algorithm>

template<class Iter>
Iter partQSort(Iter left, Iter right, int64_t dis) {
	--right;

	auto pivot = [dis, left, right] {
		auto middle = *(left + dis / 2);

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

	return left;
}

template<class Iter>
class QSortThreadPool {
public:
	QSortThreadPool(size_t num) : th(num) { }

	void operator() (Iter first, Iter last) {
		qSortTh(first, last, std::distance(first, last));

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

	void qSortTh (Iter first, Iter last, int64_t dis) {
		constexpr auto distLimit = 10;

		if (dis > distLimit) {
			auto border = partQSort(first, last, dis);

			auto dis1 = std::distance(first, border);
			auto dis2 = std::distance(border, last);

			bool flagDis1 = dis1 > distLimit;
			bool flagDis2 = dis2 > distLimit;

			auto func = [this](Iter f, Iter l, int64_t dis) { qSortTh(f, l, dis); };

			if (flagDis1 && flagDis2) {
				th.addTask(func, first, border, dis1);
				th.addTask(func, border, last, dis2);
			}
			else if (flagDis1) {
				th.addTask(func, first, border, dis1);
				std::sort(border, last);
			}
			else if (flagDis2) {
				th.addTask(func, border, last, dis2);
				std::sort(first, border);
			}
			else {
				std::sort(first, border);
				std::sort(border, last);
			}
		} else {
			std::sort(first, last);
		}
	}
};

template<class Iter>
void quickSort(Iter first, Iter last) {
	quickSort2(first, last, std::distance(first, last));
}

template<class Iter>
void quickSort2(Iter first, Iter last, int64_t dis) {
	constexpr int distLimit = 1;

	if (dis > distLimit) {
		auto border = partQSort(first, last, dis);

		auto sortFunc = [distLimit](Iter begin, Iter end) {
			auto dis = std::distance(begin, end);
			if (dis > distLimit) {
				quickSort2(begin, end, dis);
			} else  {
				std::sort(begin, end);
			}
		};

		sortFunc(first, border);
		sortFunc(border, last);		
	} else {
		std::sort(first, last);
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
	auto list = { 1, 2, 3 };
	//auto arr = std::make_shared<arrType>(list); 
	auto arr = std::make_shared<arrType>(sizeArr);

	std::srand(unsigned(std::time(0)));
	for (auto&& t : *arr) {
		t = rand() % 10;
	}

	auto endStr = "ns\n";

	auto time_stdsort = someSort(std::sort<arrType::iterator>, arr);
	std::cout << "std::sort       " << time_stdsort << endStr << std::endl;

	auto time_quickSort = someSort(quickSort<arrType::iterator>, arr);
	std::cout << "quickSort       " << time_quickSort << endStr << std::endl;

	QSortThreadPool<arrType::iterator> qsth(20);
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(30ms);

	auto time_QSortThreadPool = someSort(qsth, arr);
	std::cout << "QSortThreadPool " << time_QSortThreadPool << endStr << std::endl;

	std::cout << "speedup std::sort  " << double(time_stdsort) / time_QSortThreadPool << std::endl;
	std::cout << "speedup quickSort  " << double(time_quickSort) / time_QSortThreadPool << std::endl;
}

int main() {
	f(100);

	char c;
	std::cin >> c;
}