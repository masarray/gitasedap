#include "TestSupport.h"
#include "runtime/PreparedStateExchange.h"

#include <atomic>
#include <utility>

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
  GS_REQUIRE(exchange.isCurrent(request1));

  auto state1 = Exchange::prepare(request1, 11, &destroyed);
  GS_REQUIRE(
    exchange.publish(std::move(state1))
    == Exchange::PublishResult::Published
  );

  const auto* active1 = exchange.acquireForAudioBlock();
  GS_REQUIRE(active1 != nullptr);
  GS_REQUIRE(active1->value == 11);
  GS_REQUIRE(exchange.activeGeneration() == request1.generation);
  GS_REQUIRE(destroyed.load(std::memory_order_relaxed) == 0);

  const auto request2 = exchange.beginRequest();
  auto state2 = Exchange::prepare(request2, 22, &destroyed);
  GS_REQUIRE(
    exchange.publish(std::move(state2))
    == Exchange::PublishResult::Published
  );

  const auto* active2 = exchange.acquireForAudioBlock();
  GS_REQUIRE(active2 != nullptr);
  GS_REQUIRE(active2->value == 22);

  GS_REQUIRE(destroyed.load(std::memory_order_relaxed) == 0);
  GS_REQUIRE(exchange.drainReclaimable() == 1);
  GS_REQUIRE(destroyed.load(std::memory_order_relaxed) == 1);

  const auto staleRequest = exchange.beginRequest();
  auto stale = Exchange::prepare(staleRequest, 33, &destroyed);

  const auto newestRequest = exchange.beginRequest();
  GS_REQUIRE(!exchange.isCurrent(staleRequest));
  GS_REQUIRE(exchange.isCurrent(newestRequest));

  GS_REQUIRE(
    exchange.publish(std::move(stale))
    == Exchange::PublishResult::StaleGeneration
  );
  GS_REQUIRE(destroyed.load(std::memory_order_relaxed) == 2);

  auto firstPending = Exchange::prepare(newestRequest, 44, &destroyed);
  GS_REQUIRE(
    exchange.publish(std::move(firstPending))
    == Exchange::PublishResult::Published
  );

  const auto finalRequest = exchange.beginRequest();
  auto finalState = Exchange::prepare(finalRequest, 55, &destroyed);
  GS_REQUIRE(
    exchange.publish(std::move(finalState))
    == Exchange::PublishResult::Published
  );

  const auto* finalActive = exchange.acquireForAudioBlock();
  GS_REQUIRE(finalActive != nullptr);
  GS_REQUIRE(finalActive->value == 55);
  GS_REQUIRE(exchange.activeGeneration() == finalRequest.generation);

  exchange.shutdownAfterAudioStopped();

  GS_REQUIRE(destroyed.load(std::memory_order_relaxed) == 5);
  GS_REQUIRE(exchange.ownedStateCount() == 0);

  return 0;
}
