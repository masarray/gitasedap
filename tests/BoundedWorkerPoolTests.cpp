#include "runtime/BoundedWorkerPool.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace gsr = gitasedap::runtime;

int main()
{
  using namespace std::chrono_literals;

  gsr::BoundedWorkerPool pool(1, 1);

  std::mutex gateMutex;
  std::condition_variable gateCv;
  bool releaseFirst = false;

  std::atomic<bool> firstStarted{false};
  std::atomic<int> cancelledTaskExecutions{0};

  const auto firstResult = pool.trySubmit(
    [&](const gsr::WorkerTaskContext& context) {
      firstStarted.store(true, std::memory_order_release);

      std::unique_lock lock(gateMutex);
      gateCv.wait(
        lock,
        [&] {
          return releaseFirst || context.stopRequested();
        }
      );
    }
  );

  assert(firstResult == gsr::BoundedWorkerPool::SubmitResult::Accepted);

  while(!firstStarted.load(std::memory_order_acquire))
    std::this_thread::yield();

  gsr::CancellationSource cancelledSource;

  const auto secondResult = pool.trySubmit(
    [&](const gsr::WorkerTaskContext&) {
      cancelledTaskExecutions.fetch_add(1, std::memory_order_relaxed);
    },
    cancelledSource.token()
  );

  assert(secondResult == gsr::BoundedWorkerPool::SubmitResult::Accepted);

  const auto overflowResult = pool.trySubmit(
    [](const gsr::WorkerTaskContext&) {}
  );

  assert(overflowResult == gsr::BoundedWorkerPool::SubmitResult::QueueFull);

  cancelledSource.cancel();

  {
    std::lock_guard lock(gateMutex);
    releaseFirst = true;
  }
  gateCv.notify_all();

  assert(pool.waitUntilIdleFor(2s));
  assert(cancelledTaskExecutions.load(std::memory_order_relaxed) == 0);
  assert(pool.unhandledExceptionCount() == 0);

  // Exceptions do not kill worker threads or escape into the host/control
  // thread. They are surfaced through a bounded diagnostic counter.
  assert(
    pool.trySubmit(
      [](const gsr::WorkerTaskContext&) {
        throw 7;
      }
    )
    == gsr::BoundedWorkerPool::SubmitResult::Accepted
  );

  assert(pool.waitUntilIdleFor(2s));
  assert(pool.unhandledExceptionCount() == 1);

  pool.shutdown();

  assert(
    pool.trySubmit([](const gsr::WorkerTaskContext&) {})
    == gsr::BoundedWorkerPool::SubmitResult::Stopped
  );

  return 0;
}
