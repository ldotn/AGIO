#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

class WorkerPool
{
public:
	WorkerPool(uint32_t InWorkersCount) : Pool(InWorkersCount)
	{
		Alive_ = true;
		WorkersCount = InWorkersCount;
		WorkerLocks = new CV[WorkersCount];

		for (int id = 0; id < WorkersCount; id++)
		{
			Pool[id] = std::thread([id, this]() // worker code
			{
				WorkerLocks[id].Wait();

				while (Alive_)
				{
					while (true)
					{
						int widx = CurrentIdx.fetch_add(1);
						if (widx >= CurrentJobSize)
						{
							// Only signal when the last worker finished
							if (ActiveWorkersCount.fetch_sub(1) == 1) MainLock.Signal();
							break;
						}

						WorkFunction(widx, id);
					}

					WorkerLocks[id].Wait();
				}

				// Closing the thread. If this is the last worker to close, signal.
				if (ActiveWorkersCount.fetch_sub(1) == 1) MainLock.Signal();
			});
		}
	}

	~WorkerPool()
	{
		Alive_ = false;

		// Wake up all workers so that they terminate correctly
		ActiveWorkersCount = WorkersCount;
		for (int id = 0; id < WorkersCount; id++)
			WorkerLocks[id].Signal();

		MainLock.Wait();

		delete[] WorkerLocks;

		for (int id = 0; id < WorkersCount; id++)
			Pool[id].join();
	}

	void Dispatch(uint32_t JobSize, std::function<void(int, int)> Job)
	{
		CurrentJobSize = JobSize;
		WorkFunction = Job;

		CurrentIdx = 0;
		ActiveWorkersCount = WorkersCount;
		for (int id = 0; id < WorkersCount; id++)
			WorkerLocks[id].Signal();

		MainLock.Wait();
	}

	struct CV
	{
		std::mutex m;
		std::condition_variable cv;

		void Wait()
		{
			std::unique_lock<std::mutex> wlock(m);
			cv.wait(wlock);
		}

		void Signal()
		{
			cv.notify_one();
		}

		void SignalAll()
		{
			cv.notify_all();
		}
	};
private:
	std::vector<std::thread> Pool;

	uint32_t WorkersCount;
	std::atomic<int> CurrentIdx;
	std::atomic<int> ActiveWorkersCount;
	CV* WorkerLocks;
	CV MainLock;
	bool Alive_;

	uint32_t CurrentJobSize;
	std::function<void(int, int)> WorkFunction;
};
