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
Iters2El<Iter> partQSort(Iter begin, Iter end, int64_t dist) {
	--end;

	//move pivot element to the end 
	std::iter_swap(begin + dist / 2, end);
	auto pivot = *end;

	auto left = begin - 1, right = end;
	auto leftEq = left, rightEq = end;

	while (true) {
		//find first element not smaller than pivot
		while (*(++left) < pivot);

		//find last element not bigger than pivot
		while (pivot < *(--right)) {
			//check that all elements are swaped
			if (right == begin) {
				break;
			}
		}

		//check that all elements are swaped
		if (right <= left) {
			break;
		}

		//swap not smaller element on the left with not bigger element on the right
		std::iter_swap(left, right);

		if (*left == pivot) {
			//move to begin an element equal to pivot
			std::iter_swap(++leftEq, left);
		}

		if (*right == pivot) {
			//move to end an element equal to pivot
			std::iter_swap(--rightEq, right);
		}
	}

	//move pivot element from the end
	std::iter_swap(left, end);
	right = left + 1;
	--left;

	//move to middle from begin all elements equal to pivot
	for ( ; begin < leftEq; ++begin, --left) {
		std::iter_swap(begin, left);
	}

	// move to middle from end all elements equal to pivot
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
		qSortTh(first, last, std::distance(first, last));

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
	const int distLimit = 250;	//experimental value

	void qSortTh(Iter first, Iter last, int64_t dist) {
		if (dist > distLimit) {
			auto border = partQSort(first, last, dist);

			auto dist1 = std::distance(first, border.left);
			auto dist2 = std::distance(border.right, last);

			bool flagdist1 = dist1 > distLimit;
			bool flagdist2 = dist2 > distLimit;

			auto func = [this](Iter f, Iter l, int64_t dist) { qSortTh(f, l, dist); };

			if (flagdist1 && flagdist2) {
				th.addTask(func, first, border.left, dist1);
				th.addTask(func, border.right, last, dist2);
			}
			else if (flagdist1) {
				th.addTask(func, first, border.left, dist1);
				insertionSort(border.right, last);
			}
			else if (flagdist2) {
				th.addTask(func, border.right, last, dist2);
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
class QuickSort {
public:
	void operator() (Iter first, Iter last) {
		quickSort2(first, last, std::distance(first, last));

		while (!queueToSort.empty()) {
			auto elToSort = queueToSort.front();
			queueToSort.pop();
			quickSort2(elToSort.left, elToSort.right, distance(elToSort.left, elToSort.right));
		}
	}

private:
	struct Iters2ElDist : public Iters2El<Iter> {
		Iters2ElDist(Iter l, Iter r, int64_t d) : Iters2El<Iter>{ l, r }, dist(d) { }

		int64_t dist;
	};

	std::queue<Iters2ElDist> queueToSort;
	const int distLimit = 100;	//experimental value

	void quickSort2(Iter first, Iter last, int64_t dist) {
		if (dist > distLimit) {
			auto border = partQSort(first, last, dist);

			auto sortFunc = [this](Iter begin, Iter end) {
				auto dist = std::distance(begin, end);
				if (dist > distLimit) {
					queueToSort.emplace(begin, end, dist);
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
};

template<class Func, class Arr>
auto someSort(Func&& sort, Arr arr) {
	std::vector<size_t> arrCopy(arr->size());
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

enum class TypeArr : uint8_t {
	RAND,
	RAND_MOD10,
	INCR,
	DECR 
};

void f(size_t sizeArr, TypeArr typeArr) {
	auto endStr = "ns\n";
	using arrType = std::vector<size_t>;
	auto arr = std::make_shared<arrType>(sizeArr);
	std::srand(unsigned(std::time(0)));
	std::cout << "sizeArr = " << sizeArr << std::endl;
	
	switch (typeArr) {
		size_t i;
	case TypeArr::RAND_MOD10:
		std::srand(unsigned(std::time(0)));
		for (auto&& t : *arr) {
			t = rand() % 10;
		}
		std::cout << "RAND_MOD10" << std::endl;
		break;
	case TypeArr::INCR:
		i = 0;
		for (auto&& t : *arr) {
			t = i++;
		}
		std::cout << "INCR" << std::endl;
		break;
	case TypeArr::DECR:
		i = -1;
		for (auto&& t : *arr) {
			t = i--;
		}
		std::cout << "DECR" << std::endl;
		break;
	default:
		std::srand(unsigned(std::time(0)));
		for (auto&& t : *arr) {
			t = rand();
		}
		std::cout << "RAND" << std::endl;
		break;
	}

	auto time_stdsort = someSort(std::sort<arrType::iterator>, arr);
	std::cout << "std::sort       " << time_stdsort << endStr << std::endl;

	auto time_quickSort = someSort(QuickSort<arrType::iterator>(), arr);
	std::cout << "quickSort       " << time_quickSort << endStr << std::endl;

	auto time_QSortThreadPool = someSort(QSortThreadPool<arrType::iterator>(20), arr);
	std::cout << "QSortThreadPool " << time_QSortThreadPool << endStr << std::endl;

	std::cout << "speedup std::sort  " << double(time_stdsort) / time_QSortThreadPool << std::endl;
	std::cout << "speedup quickSort  " << double(time_quickSort) / time_QSortThreadPool << std::endl;
	std::cout << "\nspeedup stdsort/quickSort  " << double(time_stdsort) / time_quickSort << std::endl;
	std::cout << "______________________________________\n" << std::endl;
}

int main() {
	auto sizeArr = 10'000'000;

	f(sizeArr, TypeArr::RAND);
	f(sizeArr, TypeArr::RAND_MOD10);
	f(sizeArr, TypeArr::INCR);
	f(sizeArr, TypeArr::DECR);

	char c;
	std::cin >> c;
}
