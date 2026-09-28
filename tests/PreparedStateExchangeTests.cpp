#include "runtime/PreparedStateExchange.h"

#include <atomic>
#include <cassert>

namespace gsr = gitasedap::runtime;

namespace
{

struct TrackedState
{
  int value{0};
  std::atomic<int>* destructionCount{nullptr};

  TrackedState(int initialValue, std::atomic<int>* counter)
  : value(initialValue)
  , destructionCount(counter)
  {
  }

  TrackedState(const TrackedState&) = delete;
  TrackedState& operator=(const TrackedState&) = delete;
  TrackedState(TrackedState&&) = delete;
  TrackedState& operator=(TrackedState&&) = delete;

  ~TrackedState()
  {
    if(destructionCount)
      destructionCount->fetch_add(1, std::memory_order_relaxed);
  }
};

} // namespace

int main()
{
  using Exchange = gsr::PreparedStateExchange<TrackedState>;

  std::atomic<int> destroyed{0};
  Exchange exchange;

  const auto request1 = exchange.beginRequest();
  assert(exchange.isCurrent(request1));

  auto state1 = Exchange::prepare(request1, 11, &destroyed);
  assert(
    exchange.publish(std::move(state1))
    == Exchange::PublishResult::Published
  );

  const auto* active1 = exchange.acquireForAudioBlock();
  assert(active1 != nullptr);
  assert(active1->value == 11);
  assert(exchange.activeGeneration() == request1.generation);
  assert(destroyed.load(std::memory_order_relaxed) == 0);

  const auto request2 = exchange.beginRequest();
  auto state2 = Exchange::prepare(request2, 22, &destroyed);
  assert(
    exchange.publish(std::move(state2))
    == Exchange::PublishResult::Published
  );

  const auto* active2 = exchange.acquireForAudioBlock();
  assert(active2 != nullptr);
  assert(active2->value == 22);

  // Switching active state on the audio side must never run the old state's
  // destructor. Reclamation is explicitly control-plane work.
  assert(destroyed.load(std::memory_order_relaxed) == 0);
  assert(exchange.drainReclaimable() == 1);
  assert(destroyed.load(std::memory_order_relaxed) == 1);

  const auto staleRequest = exchange.beginRequest();
  auto stale = Exchange::prepare(staleRequest, 33, &destroyed);

  const auto newestRequest = exchange.beginRequest();
  assert(!exchange.isCurrent(staleRequest));
  assert(exchange.isCurrent(newestRequest));

  assert(
    exchange.publish(std::move(stale))
    == Exchange::PublishResult::StaleGeneration
  );
  assert(destroyed.load(std::memory_order_relaxed) == 2);

  // Two structural requests before an audio block coalesce to the latest.
  auto firstPending = Exchange::prepare(newestRequest, 44, &destroyed);
  assert(
    exchange.publish(std::move(firstPending))
    == Exchange::PublishResult::Published
  );

  const auto finalRequest = exchange.beginRequest();
  auto finalState = Exchange::prepare(finalRequest, 55, &destroyed);
  assert(
    exchange.publish(std::move(finalState))
    == Exchange::PublishResult::Published
  );

  const auto* finalActive = exchange.acquireForAudioBlock();
  assert(finalActive != nullptr);
  assert(finalActive->value == 55);
  assert(exchange.activeGeneration() == finalRequest.generation);

  exchange.shutdownAfterAudioStopped();

  // All accepted/rejected states are eventually destroyed off the audio path.
  assert(destroyed.load(std::memory_order_relaxed) == 5);
  assert(exchange.ownedStateCount() == 0);

  return 0;
}
