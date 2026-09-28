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

// Control-plane -> realtime immutable-state handoff.
//
// The control plane owns every heap allocation. The audio thread only performs
// a bounded number of atomic pointer operations once per block. Reclamation is
// hazard-protected and happens only when drainReclaimable(), beginRequest(),
// publish(), or shutdownAfterAudioStopped() runs on a non-realtime thread.
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

    // A not-yet-consumed state from an older request is no longer useful.
    // Removing it here provides true latest-request coalescing.
    mPending.exchange(nullptr, std::memory_order_seq_cst);
    reclaimLocked();

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

    // Replacing pending is safe: the audio thread hazard-protects a candidate
    // before it dereferences it. The displaced node remains owned until a
    // control-plane reclamation pass proves it is neither pending, active,
    // nor hazard-protected.
    mPending.exchange(raw, std::memory_order_seq_cst);
    reclaimLocked();

    return PublishResult::Published;
  }

  // Realtime safe. Call exactly once at an audio block boundary. The returned
  // pointer stays valid until at least the next call from the same audio thread.
  //
  // No allocation, destruction, lock, wait, I/O, logging, or unbounded loop is
  // performed. The retry loop only contends with structural publication, which
  // is a low-rate control-plane event.
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

      // Standard hazard-pointer pattern:
      // 1. publish candidate as hazard,
      // 2. verify source still points to it,
      // 3. claim it with CAS,
      // 4. only then dereference/publish active.
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

  // Control-plane only. Returns the number of immutable states destroyed by
  // this pass. Destructors therefore never run on the audio callback.
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

  // Must be called only after the host has stopped the audio callback.
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
