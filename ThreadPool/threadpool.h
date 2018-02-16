#pragma once

#include <queue>
#include <vector>

#include <memory>
#include <functional>

#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace threadpool {
	class ThreadPool {
	public:
		ThreadPool(size_t);

		template<class Func, class... Args>
		auto addTask(Func&& func, Args&&... args)->std::future<decltype(func(args...))>;

		void resize(size_t);
		size_t getNumThreads() const;
		size_t getNumWorkThreads();

		~ThreadPool();

	private:
		enum class statusTh : uint8_t {
			WORK,
			READY_TO_DELL,
			DELETED
		};

		std::vector<std::thread> threads;
		std::vector<statusTh> threadFinished;
		std::queue<std::function<void()>> tasksQueue;

		std::mutex mutexTask;
		std::mutex mutexAddThread;
		std::condition_variable condVar;
		std::condition_variable condFinish;
		std::atomic<size_t> numThreads = 0;
		size_t numWorkThreads = 0;
		size_t delSomeThreads = 0;

		void addThread();
	};

	template<class Func, class... Args>
	auto ThreadPool::addTask(Func&& func, Args&&... args)->std::future<decltype(func(args...))> {
		auto newTask = std::make_shared<std::packaged_task<decltype(func(args...))()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

		auto fut = newTask->get_future();

		mutexTask.lock();
		tasksQueue.emplace([newTask]() { (*newTask)(); });
		mutexTask.unlock();

		condVar.notify_one();
		return fut;
	}
};
