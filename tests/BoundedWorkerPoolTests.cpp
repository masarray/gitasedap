#include "TestSupport.h"
#include "runtime/BoundedWorkerPool.h"

#include <atomic>
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

  GS_REQUIRE(firstResult == gsr::BoundedWorkerPool::SubmitResult::Accepted);

  while(!firstStarted.load(std::memory_order_acquire))
    std::this_thread::yield();

  gsr::CancellationSource cancelledSource;

  const auto secondResult = pool.trySubmit(
    [&](const gsr::WorkerTaskContext&) {
      cancelledTaskExecutions.fetch_add(1, std::memory_order_relaxed);
    },
    cancelledSource.token()
  );

  GS_REQUIRE(secondResult == gsr::BoundedWorkerPool::SubmitResult::Accepted);

  const auto overflowResult = pool.trySubmit(
    [](const gsr::WorkerTaskContext&) {}
  );

  GS_REQUIRE(overflowResult == gsr::BoundedWorkerPool::SubmitResult::QueueFull);

  cancelledSource.cancel();

  {
    std::lock_guard lock(gateMutex);
    releaseFirst = true;
  }
  gateCv.notify_all();

  GS_REQUIRE(pool.waitUntilIdleFor(2s));
  GS_REQUIRE(cancelledTaskExecutions.load(std::memory_order_relaxed) == 0);
  GS_REQUIRE(pool.unhandledExceptionCount() == 0);

  GS_REQUIRE(
    pool.trySubmit(
      [](const gsr::WorkerTaskContext&) {
        throw 7;
      }
    )
    == gsr::BoundedWorkerPool::SubmitResult::Accepted
  );

  GS_REQUIRE(pool.waitUntilIdleFor(2s));
  GS_REQUIRE(pool.unhandledExceptionCount() == 1);

  pool.shutdown();

  GS_REQUIRE(
    pool.trySubmit([](const gsr::WorkerTaskContext&) {})
    == gsr::BoundedWorkerPool::SubmitResult::Stopped
  );

  return 0;
}
