#pragma once

#include <iostream>

namespace gitasedap::test
{

inline bool check(
  bool condition,
  const char* expression,
  const char* file,
  int line
)
{
  if(condition)
    return true;

  std::cerr
    << file << ":" << line
    << ": requirement failed: " << expression
    << "\n";

  return false;
}

} // namespace gitasedap::test

#define GS_REQUIRE(expression) \
  do \
  { \
    if(!::gitasedap::test::check( \
      static_cast<bool>(expression), \
      #expression, \
      __FILE__, \
      __LINE__ \
    )) \
    { \
      return 1; \
    } \
  } while(false)
