#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace gitasedap::runtime
{

template <typename Key, typename Value, std::size_t Capacity>
class CoalescingQueue
{
  static_assert(Capacity > 0, "CoalescingQueue capacity must be non-zero");

public:
  enum class PushResult
  {
    Inserted,
    Replaced,
    Full
  };

  struct Item
  {
    Key key;
    Value value;
  };

  [[nodiscard]] PushResult pushOrReplace(Key key, Value value)
  {
    std::lock_guard lock(mMutex);

    for(auto& slot : mSlots)
    {
      if(slot.has_value() && slot->key == key)
      {
        slot->value = std::move(value);
        slot->sequence = nextSequenceLocked();
        return PushResult::Replaced;
      }
    }

    for(auto& slot : mSlots)
    {
      if(!slot.has_value())
      {
        slot.emplace(
          Entry{std::move(key), std::move(value), nextSequenceLocked()}
        );
        ++mSize;
        return PushResult::Inserted;
      }
    }

    return PushResult::Full;
  }

  [[nodiscard]] std::optional<Item> tryPopOldest()
  {
    std::lock_guard lock(mMutex);

    if(mSize == 0)
      return std::nullopt;

    std::size_t selected = Capacity;
    std::uint64_t selectedSequence = std::numeric_limits<std::uint64_t>::max();

    for(std::size_t i = 0; i < Capacity; ++i)
    {
      if(mSlots[i].has_value() && mSlots[i]->sequence < selectedSequence)
      {
        selected = i;
        selectedSequence = mSlots[i]->sequence;
      }
    }

    if(selected == Capacity)
      return std::nullopt;

    Item item{
      std::move(mSlots[selected]->key),
      std::move(mSlots[selected]->value)
    };

    mSlots[selected].reset();
    --mSize;
    return item;
  }

  [[nodiscard]] std::size_t size() const
  {
    std::lock_guard lock(mMutex);
    return mSize;
  }

  [[nodiscard]] constexpr std::size_t capacity() const noexcept
  {
    return Capacity;
  }

  [[nodiscard]] bool empty() const
  {
    return size() == 0;
  }

private:
  struct Entry
  {
    Key key;
    Value value;
    std::uint64_t sequence;
  };

  [[nodiscard]] std::uint64_t nextSequenceLocked() noexcept
  {
    // Sequence wrap is practically unreachable for control-plane work.
    // Preserve ordering deterministically if it is ever reached by renumbering
    // existing entries before issuing the next sequence.
    if(mNextSequence == std::numeric_limits<std::uint64_t>::max())
    {
      std::uint64_t sequence = 1;

      while(true)
      {
        std::size_t selected = Capacity;
        std::uint64_t selectedOld = std::numeric_limits<std::uint64_t>::max();

        for(std::size_t i = 0; i < Capacity; ++i)
        {
          if(
            mSlots[i].has_value()
            && mSlots[i]->sequence < selectedOld
            && mSlots[i]->sequence >= sequence
          )
          {
            selected = i;
            selectedOld = mSlots[i]->sequence;
          }
        }

        if(selected == Capacity)
          break;

        mSlots[selected]->sequence = sequence++;
      }

      mNextSequence = sequence;
    }

    return mNextSequence++;
  }

  mutable std::mutex mMutex;
  std::array<std::optional<Entry>, Capacity> mSlots{};
  std::size_t mSize{0};
  std::uint64_t mNextSequence{1};
};

} // namespace gitasedap::runtime
