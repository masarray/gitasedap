#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gitasedap::runtime
{

template <typename State>
class PreparedStateExchange
{
public:
  struct RequestToken
  {
    std::uint64_t generation{0};

    [[nodiscard]] explicit constexpr operator bool() const noexcept
    {
      return generation != 0;
    }
  };

  struct Prepared
  {
    std::uint64_t generation;
    State state;

    template <typename... Args>
    explicit Prepared(RequestToken token, Args&&... args)
    : generation(token.generation)
    , state(std::forward<Args>(args)...)
    {
    }
  };

  using Handle = std::unique_ptr<Prepared>;

  enum class PublishResult
  {
    Published,
    StaleGeneration,
    InvalidHandle
  };

  PreparedStateExchange() = default;

  ~PreparedStateExchange()
  {
    shutdownAfterAudioStopped();
  }

  PreparedStateExchange(const PreparedStateExchange&) = delete;
  PreparedStateExchange& operator=(const PreparedStateExchange&) = delete;
  PreparedStateExchange(PreparedStateExchange&&) = delete;
  PreparedStateExchange& operator=(PreparedStateExchange&&) = delete;

  [[nodiscard]] RequestToken beginRequest()
  {
    std::lock_guard lock(mControlMutex);

    const auto current = mLatestRequested.load(std::memory_order_seq_cst);

    if(current == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("PreparedStateExchange generation exhausted");

    const auto next = current + 1;
    mLatestRequested.store(next, std::memory_order_seq_cst);

    mPending.exchange(nullptr, std::memory_order_seq_cst);
    (void) reclaimLocked();

    return RequestToken{next};
  }

  template <typename... Args>
  [[nodiscard]] static Handle prepare(
    RequestToken token,
    Args&&... args
  )
  {
    if(!token)
      return nullptr;

    return std::make_unique<Prepared>(
      token,
      std::forward<Args>(args)...
    );
  }

  [[nodiscard]] bool isCurrent(RequestToken token) const noexcept
  {
    return token
        && token.generation
          == mLatestRequested.load(std::memory_order_seq_cst);
  }

  [[nodiscard]] PublishResult publish(Handle handle)
  {
    if(!handle)
      return PublishResult::InvalidHandle;

    std::lock_guard lock(mControlMutex);

    if(
      handle->generation
      != mLatestRequested.load(std::memory_order_seq_cst)
    )
    {
      return PublishResult::StaleGeneration;
    }

    auto* raw = handle.get();
    mOwned.emplace_back(std::move(handle));

    mPending.exchange(raw, std::memory_order_seq_cst);
    (void) reclaimLocked();

    return PublishResult::Published;
  }

  [[nodiscard]] const State* acquireForAudioBlock() noexcept
  {
    for(;;)
    {
      auto* candidate = mPending.load(std::memory_order_seq_cst);

      if(candidate == nullptr)
      {
        auto* active = mActive.load(std::memory_order_seq_cst);
        return active ? &active->state : nullptr;
      }

      mHazard.store(candidate, std::memory_order_seq_cst);

      if(candidate != mPending.load(std::memory_order_seq_cst))
      {
        mHazard.store(nullptr, std::memory_order_seq_cst);
        continue;
      }

      auto* expected = candidate;

      if(
        mPending.compare_exchange_strong(
          expected,
          nullptr,
          std::memory_order_seq_cst,
          std::memory_order_seq_cst
        )
      )
      {
        mActive.store(candidate, std::memory_order_seq_cst);
        mActiveGeneration.store(
          candidate->generation,
          std::memory_order_seq_cst
        );
        mHazard.store(nullptr, std::memory_order_seq_cst);
        return &candidate->state;
      }

      mHazard.store(nullptr, std::memory_order_seq_cst);
    }
  }

  [[nodiscard]] std::size_t drainReclaimable()
  {
    std::lock_guard lock(mControlMutex);
    return reclaimLocked();
  }

  [[nodiscard]] std::uint64_t latestRequestedGeneration() const noexcept
  {
    return mLatestRequested.load(std::memory_order_seq_cst);
  }

  [[nodiscard]] std::uint64_t activeGeneration() const noexcept
  {
    return mActiveGeneration.load(std::memory_order_seq_cst);
  }

  [[nodiscard]] std::size_t ownedStateCount() const
  {
    std::lock_guard lock(mControlMutex);
    return mOwned.size();
  }

  void shutdownAfterAudioStopped() noexcept
  {
    std::lock_guard lock(mControlMutex);

    assert(mHazard.load(std::memory_order_seq_cst) == nullptr);

    mPending.store(nullptr, std::memory_order_seq_cst);
    mActive.store(nullptr, std::memory_order_seq_cst);
    mActiveGeneration.store(0, std::memory_order_seq_cst);
    mHazard.store(nullptr, std::memory_order_seq_cst);
    mOwned.clear();
  }

private:
  [[nodiscard]] std::size_t reclaimLocked()
  {
    auto* const pending = mPending.load(std::memory_order_seq_cst);
    auto* const active = mActive.load(std::memory_order_seq_cst);
    auto* const hazard = mHazard.load(std::memory_order_seq_cst);

    const auto before = mOwned.size();

    mOwned.erase(
      std::remove_if(
        mOwned.begin(),
        mOwned.end(),
        [pending, active, hazard](const auto& state) {
          auto* const raw = state.get();
          return raw != pending && raw != active && raw != hazard;
        }
      ),
      mOwned.end()
    );

    return before - mOwned.size();
  }

  mutable std::mutex mControlMutex;
  std::vector<Handle> mOwned;

  std::atomic<Prepared*> mPending{nullptr};
  std::atomic<Prepared*> mActive{nullptr};
  std::atomic<Prepared*> mHazard{nullptr};

  std::atomic<std::uint64_t> mLatestRequested{0};
  std::atomic<std::uint64_t> mActiveGeneration{0};
};

} // namespace gitasedap::runtime
