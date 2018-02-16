#include "threadpool.h"

namespace threadpool {
	ThreadPool::ThreadPool(size_t num) {
		resize(num);
	};

	size_t ThreadPool::getNumThreads() const {
		return numThreads;
	};

	size_t ThreadPool::getNumWorkThreads() {
		std::lock_guard<std::mutex> lock(mutexAddThread);
		std::lock_guard<std::mutex> lock2(mutexTask);
		return numWorkThreads;
	};

	ThreadPool::~ThreadPool() {
		resize(0);
	};

	void ThreadPool::addThread() {
		threadFinished.push_back(statusTh::WORK);
		auto lastFlag = threadFinished.size() - 1;
		mutexAddThread.lock();
		++numWorkThreads;
		mutexAddThread.unlock();

		threads.emplace_back([this, lastFlag] {
			while (true) {
				std::unique_lock<std::mutex> lock(mutexTask);
				--numWorkThreads;
				condVar.wait(lock, [this] { return !tasksQueue.empty() || delSomeThreads > 0; });

				if (delSomeThreads > 0 && (tasksQueue.empty() || (numThreads > 1 && !tasksQueue.empty()))) {
					--delSomeThreads;
					--numThreads;
					threadFinished[lastFlag] = statusTh::READY_TO_DELL;

					condFinish.notify_one();
					return;
				}

				++numWorkThreads;
				std::function<void()> task = std::move(tasksQueue.front());
				tasksQueue.pop();
				lock.unlock();

				task();
			}
		});
	}

	void ThreadPool::resize(size_t num) {
		if (numThreads == num) {
			return;
		}

		if (num > numThreads) {
			for (; num > numThreads; ++numThreads) {
				addThread();
			}
		}
		else {
			mutexTask.lock();
			delSomeThreads = numThreads - num;
			auto notDeletedTh = delSomeThreads;
			mutexTask.unlock();

			condVar.notify_all();

			while (notDeletedTh > 0) {
				std::unique_lock<std::mutex> lock(mutexTask);
				if (notDeletedTh == delSomeThreads) {
					condFinish.wait(lock, [this, notDeletedTh] { return notDeletedTh != delSomeThreads; });
				}

				for (size_t i = 0; i < threadFinished.size(); ++i) {
					if (threadFinished[i] == statusTh::READY_TO_DELL) {
						threadFinished[i] = statusTh::DELETED;
						--notDeletedTh;

						if (threads[i].joinable()) {
							threads[i].join();
						}
						break;
					}
				}
			}
		}
	}
};