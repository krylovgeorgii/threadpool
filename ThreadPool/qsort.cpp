#include "threadpool.h"

#include <iostream>
#include <vector>
#include <iterator>

#include <cstdlib>
#include <ctime>
#include <chrono>

#include <algorithm>

template<class Iter> 
void insertionSort(Iter begin, Iter end) {
	for (auto it = begin; it != end; ++it) {
		std::rotate(std::upper_bound(begin, it, *it), it, it + 1);
	}
}

template<class Iter>
struct Iters2El {
	Iter left;
	Iter right;
};

template<class Iter> 
Iters2El<Iter> partQSort(Iter begin, Iter end, int64_t dis) {
	--end;

	auto pivot = *end;

	auto left = begin - 1, right = end;
	auto leftEq = left, rightEq = end;

	while (true) {
		while (*(++left) < pivot);

		while (pivot < *(--right)) {
			if (right == begin) {
				break;
			}
		}

		if (right <= left) {
			break;
		}

		std::iter_swap(left, right);

		if (*left == pivot) {
			std::iter_swap(++leftEq, left);
		}

		if (*right == pivot) {
			std::iter_swap(--rightEq, right);
		}
	}

	std::iter_swap(left, end);
	right = left + 1;
	--left;

	for (; begin < leftEq; ++begin, --left) {
		std::iter_swap(begin, left);
	}

	for (--end; end > rightEq; --end, ++right) {
		std::iter_swap(right, end);
	}

	++left;
	return Iters2El<Iter> { left, right };
}

template<class Iter>
class QSortThreadPool {
public:
	QSortThreadPool(size_t num) : th(num) { }

	void operator() (Iter first, Iter last) {
		qSortTh(first, last, std::distance(first, last) - 1);

		do {
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10ms);
		} while (th.getNumWorkThreads() != 0);
	}

	void resize(size_t num) {
		th.resize(num);
	}

private:
	threadpool::ThreadPool th;

	
	void qSortTh(Iter first, Iter last, int64_t dis) {
		constexpr auto distLimit = 500;

		if (dis > distLimit) {
			auto border = partQSort(first, last, dis);

			auto dis1 = std::distance(first, border.left);
			auto dis2 = std::distance(border.right, last);

			bool flagDis1 = dis1 > distLimit;
			bool flagDis2 = dis2 > distLimit;

			auto func = [this](Iter f, Iter l, int64_t dis) { qSortTh(f, l, dis - 1); };

			if (flagDis1 && flagDis2) {
				th.addTask(func, first, border.left, dis1);
				th.addTask(func, border.right, last, dis2);
			}
			else if (flagDis1) {
				th.addTask(func, first, border.left, dis1);
				insertionSort(border.right, last);
			}
			else if (flagDis2) {
				th.addTask(func, border.right, last, dis2);
				insertionSort(first, border.left);
			}
			else {
				insertionSort(first, border.left);
				insertionSort(border.right, last);
			}
		}
		else {
			insertionSort(first, last);
		}
	}
};

template<class Iter>
void quickSort(Iter first, Iter last) {
	quickSort2(first, last, std::distance(first, last) - 1);
}

template<class Iter> inline
void quickSort2(Iter first, Iter last, int64_t dis) {
	constexpr int distLimit = 500;

	if (dis > distLimit) {
		auto border = partQSort(first, last, dis);

		auto sortFunc = [distLimit](Iter begin, Iter end) {
			auto dis = std::distance(begin, end);
			if (dis > distLimit) {
				quickSort2(begin, end, dis - 1);
			}
			else {
				insertionSort(begin, end);
			}
		};

		sortFunc(first, border.left);
		sortFunc(border.right, last);
	}
	else {
		insertionSort(first, last);
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
	auto endStr = "ns\n";
	using arrType = std::vector<int>;
	auto arr = std::make_shared<arrType>(sizeArr);

	std::srand(unsigned(std::time(0)));
	for (auto&& t : *arr) {
		t = rand() % 1000;
	}

	auto time_stdsort = someSort(std::sort<arrType::iterator>, arr);
	std::cout << "std::sort       " << time_stdsort << endStr << std::endl;

	auto time_quickSort = someSort(quickSort<arrType::iterator>, arr);
	std::cout << "quickSort       " << time_quickSort << endStr << std::endl;

	QSortThreadPool<arrType::iterator> qsth(10);
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(30ms);

	auto time_QSortThreadPool = someSort(qsth, arr);
	std::cout << "QSortThreadPool " << time_QSortThreadPool << endStr << std::endl;
	qsth.resize(0);

	std::cout << "speedup std::sort  " << double(time_stdsort) / time_QSortThreadPool << std::endl;
	std::cout << "speedup quickSort  " << double(time_quickSort) / time_QSortThreadPool << std::endl;
	std::cout << "\nspeedup stdsort/quickSort  " << double(time_stdsort) / time_quickSort << std::endl;
}

int main() {
	f(1'000'000);

	char c;
	std::cin >> c;
}