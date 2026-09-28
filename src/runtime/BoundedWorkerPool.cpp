#include "runtime/BoundedWorkerPool.h"

#include <stdexcept>
#include <utility>

namespace gitasedap::runtime
{

bool CancellationToken::isCancellationRequested() const noexcept
{
  return mState && mState->cancelled.load(std::memory_order_acquire);
}

CancellationSource::CancellationSource()
: mState(std::make_shared<CancellationToken::State>())
{
}

CancellationToken CancellationSource::token() const noexcept
{
  return CancellationToken{mState};
}

void CancellationSource::cancel() noexcept
{
  mState->cancelled.store(true, std::memory_order_release);
}

BoundedWorkerPool::BoundedWorkerPool(
  std::size_t workerCountValue,
  std::size_t queueCapacityValue
)
: mCapacity(queueCapacityValue)
, mQueue(queueCapacityValue)
{
  if(workerCountValue == 0)
    throw std::invalid_argument("BoundedWorkerPool requires at least one worker");

  if(queueCapacityValue == 0)
    throw std::invalid_argument("BoundedWorkerPool requires a non-zero queue");

  mWorkers.reserve(workerCountValue);

  for(std::size_t i = 0; i < workerCountValue; ++i)
  {
    mWorkers.emplace_back(
      [this](std::stop_token stopToken) {
        workerLoop(stopToken);
      }
    );
  }
}

BoundedWorkerPool::~BoundedWorkerPool()
{
  shutdown();
}

BoundedWorkerPool::SubmitResult BoundedWorkerPool::trySubmit(
  Task task,
  CancellationToken cancellation
)
{
  if(!task)
    return SubmitResult::Stopped;

  {
    std::lock_guard lock(mMutex);

    if(!mAccepting || mStopping)
      return SubmitResult::Stopped;

    if(mCount == mCapacity)
      return SubmitResult::QueueFull;

    mQueue[mTail].emplace(
      QueuedTask{std::move(task), std::move(cancellation)}
    );

    mTail = (mTail + 1) % mCapacity;
    ++mCount;
  }

  mWorkCv.notify_one();
  return SubmitResult::Accepted;
}

void BoundedWorkerPool::shutdown() noexcept
{
  {
    std::lock_guard lock(mMutex);

    if(mStopping)
      return;

    mAccepting = false;
    mStopping = true;
    clearQueueLocked();
  }

  for(auto& worker : mWorkers)
    worker.request_stop();

  mWorkCv.notify_all();
  mIdleCv.notify_all();

  // Destroying std::jthread joins. Clearing here makes the join point explicit
  // and deterministic instead of deferring it to member destruction.
  mWorkers.clear();
}

bool BoundedWorkerPool::waitUntilIdleFor(std::chrono::milliseconds timeout)
{
  std::unique_lock lock(mMutex);

  return mIdleCv.wait_for(
    lock,
    timeout,
    [this] {
      return mCount == 0 && mActiveTasks == 0;
    }
  );
}

std::size_t BoundedWorkerPool::workerCount() const noexcept
{
  std::lock_guard lock(mMutex);
  return mWorkers.size();
}

std::size_t BoundedWorkerPool::queueCapacity() const noexcept
{
  return mCapacity;
}

std::size_t BoundedWorkerPool::queuedTaskCount() const noexcept
{
  std::lock_guard lock(mMutex);
  return mCount;
}

std::size_t BoundedWorkerPool::activeTaskCount() const noexcept
{
  std::lock_guard lock(mMutex);
  return mActiveTasks;
}

std::size_t BoundedWorkerPool::unhandledExceptionCount() const noexcept
{
  return mUnhandledExceptions.load(std::memory_order_relaxed);
}

void BoundedWorkerPool::workerLoop(std::stop_token stopToken) noexcept
{
  while(true)
  {
    std::optional<QueuedTask> queued;

    {
      std::unique_lock lock(mMutex);
      mWorkCv.wait(
        lock,
        [this, stopToken] {
          return mStopping || stopToken.stop_requested() || mCount > 0;
        }
      );

      if((mStopping || stopToken.stop_requested()) && mCount == 0)
        return;

      if(mCount == 0)
        continue;

      queued = std::move(mQueue[mHead]);
      mQueue[mHead].reset();
      mHead = (mHead + 1) % mCapacity;
      --mCount;
      ++mActiveTasks;
    }

    if(queued.has_value())
    {
      WorkerTaskContext context{
        stopToken,
        queued->cancellation
      };

      if(!context.stopRequested())
      {
        try
        {
          queued->task(context);
        }
        catch(...)
        {
          // Keep the worker alive. The counter is intentionally lock-free and
          // logging-free; the control plane decides how to surface task errors.
          mUnhandledExceptions.fetch_add(1, std::memory_order_relaxed);
        }
      }
    }

    {
      std::lock_guard lock(mMutex);
      --mActiveTasks;

      if(mCount == 0 && mActiveTasks == 0)
        mIdleCv.notify_all();
    }
  }
}

void BoundedWorkerPool::clearQueueLocked() noexcept
{
  for(auto& slot : mQueue)
    slot.reset();

  mHead = 0;
  mTail = 0;
  mCount = 0;

  if(mActiveTasks == 0)
    mIdleCv.notify_all();
}

} // namespace gitasedap::runtime
