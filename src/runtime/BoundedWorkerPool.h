#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>

namespace gitasedap::runtime
{

class CancellationToken
{
public:
  CancellationToken() = default;

  [[nodiscard]] bool isCancellationRequested() const noexcept;

private:
  struct State
  {
    std::atomic<bool> cancelled{false};
  };

  explicit CancellationToken(std::shared_ptr<State> state) noexcept
  : mState(std::move(state))
  {
  }

  std::shared_ptr<State> mState;

  friend class CancellationSource;
};

class CancellationSource
{
public:
  CancellationSource();

  [[nodiscard]] CancellationToken token() const noexcept;
  void cancel() noexcept;

private:
  std::shared_ptr<CancellationToken::State> mState;
};

struct WorkerTaskContext
{
  std::stop_token poolStop;
  CancellationToken taskCancellation;

  [[nodiscard]] bool stopRequested() const noexcept
  {
    return poolStop.stop_requested()
        || taskCancellation.isCancellationRequested();
  }
};

class BoundedWorkerPool
{
public:
  using Task = std::function<void(const WorkerTaskContext&)>;

  enum class SubmitResult
  {
    Accepted,
    QueueFull,
    Stopped
  };

  BoundedWorkerPool(std::size_t workerCount, std::size_t queueCapacity);
  ~BoundedWorkerPool();

  BoundedWorkerPool(const BoundedWorkerPool&) = delete;
  BoundedWorkerPool& operator=(const BoundedWorkerPool&) = delete;
  BoundedWorkerPool(BoundedWorkerPool&&) = delete;
  BoundedWorkerPool& operator=(BoundedWorkerPool&&) = delete;

  [[nodiscard]] SubmitResult trySubmit(
    Task task,
    CancellationToken cancellation = {}
  );

  // Control-plane only. Cancels queued work, requests cooperative stop for
  // running tasks, joins every worker, and returns only after shutdown is
  // deterministic.
  void shutdown() noexcept;

  [[nodiscard]] bool waitUntilIdleFor(std::chrono::milliseconds timeout);

  [[nodiscard]] std::size_t workerCount() const noexcept;
  [[nodiscard]] std::size_t queueCapacity() const noexcept;
  [[nodiscard]] std::size_t queuedTaskCount() const noexcept;
  [[nodiscard]] std::size_t activeTaskCount() const noexcept;
  [[nodiscard]] std::size_t unhandledExceptionCount() const noexcept;

private:
  struct QueuedTask
  {
    Task task;
    CancellationToken cancellation;
  };

  void workerLoop(std::stop_token stopToken) noexcept;
  void clearQueueLocked() noexcept;

  const std::size_t mCapacity;

  mutable std::mutex mMutex;
  std::condition_variable mWorkCv;
  std::condition_variable mIdleCv;
  std::vector<std::optional<QueuedTask>> mQueue;
  std::vector<std::jthread> mWorkers;

  std::size_t mHead{0};
  std::size_t mTail{0};
  std::size_t mCount{0};
  std::size_t mActiveTasks{0};
  bool mAccepting{true};
  bool mStopping{false};

  std::atomic<std::size_t> mUnhandledExceptions{0};
};

} // namespace gitasedap::runtime
