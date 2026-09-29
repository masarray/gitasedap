#include "TestSupport.h"
#include "runtime/CoalescingQueue.h"

#include <string>

namespace gsr = gitasedap::runtime;

int main()
{
  using Queue = gsr::CoalescingQueue<int, std::string, 2>;

  Queue queue;

  GS_REQUIRE(
    queue.pushOrReplace(1, "old")
    == Queue::PushResult::Inserted
  );

  GS_REQUIRE(
    queue.pushOrReplace(1, "new")
    == Queue::PushResult::Replaced
  );

  GS_REQUIRE(queue.size() == 1);

  GS_REQUIRE(
    queue.pushOrReplace(2, "second-key")
    == Queue::PushResult::Inserted
  );

  GS_REQUIRE(
    queue.pushOrReplace(3, "must-not-grow")
    == Queue::PushResult::Full
  );

  auto oldest = queue.tryPopOldest();
  GS_REQUIRE(oldest.has_value());

  GS_REQUIRE(oldest->key == 2);
  GS_REQUIRE(oldest->value == "second-key");

  auto latest = queue.tryPopOldest();
  GS_REQUIRE(latest.has_value());
  GS_REQUIRE(latest->key == 1);
  GS_REQUIRE(latest->value == "new");

  GS_REQUIRE(queue.empty());
  GS_REQUIRE(!queue.tryPopOldest().has_value());

  return 0;
}
