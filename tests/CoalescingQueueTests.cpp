#include "runtime/CoalescingQueue.h"

#include <cassert>
#include <string>

namespace gsr = gitasedap::runtime;

int main()
{
  using Queue = gsr::CoalescingQueue<int, std::string, 2>;

  Queue queue;

  assert(
    queue.pushOrReplace(1, "old")
    == Queue::PushResult::Inserted
  );

  assert(
    queue.pushOrReplace(1, "new")
    == Queue::PushResult::Replaced
  );

  assert(queue.size() == 1);

  assert(
    queue.pushOrReplace(2, "second-key")
    == Queue::PushResult::Inserted
  );

  assert(
    queue.pushOrReplace(3, "must-not-grow")
    == Queue::PushResult::Full
  );

  auto oldest = queue.tryPopOldest();
  assert(oldest.has_value());

  // Replacing key 1 refreshes its sequence, so key 2 is now older.
  assert(oldest->key == 2);
  assert(oldest->value == "second-key");

  auto latest = queue.tryPopOldest();
  assert(latest.has_value());
  assert(latest->key == 1);
  assert(latest->value == "new");

  assert(queue.empty());
  assert(!queue.tryPopOldest().has_value());

  return 0;
}
