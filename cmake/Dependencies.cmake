include_guard(GLOBAL)
include(FetchContent)

# Dependency policy:
# - exact immutable commit, never a floating branch
# - local checkout override is supported for fast developer iteration
# - dependency preparation stays outside the real-time product code
set(
  GITASEDAP_IPLUG2_DIR
  ""
  CACHE PATH
  "Optional path to an existing iPlug2 checkout. Empty downloads the pinned revision."
)

set(
  GITASEDAP_IPLUG2_COMMIT
  "d54f69050f517e43b941d88c2a170f0a840b9ee4"
  CACHE STRING
  "Pinned iPlug2 commit"
)

set(IPLUG2_CXX_STANDARD 20 CACHE STRING "C++ standard used by iPlug2" FORCE)

if(GITASEDAP_IPLUG2_DIR)
  set(IPLUG2_DIR "${GITASEDAP_IPLUG2_DIR}" CACHE PATH "iPlug2 root" FORCE)
else()
  FetchContent_Declare(
    iplug2
    GIT_REPOSITORY https://github.com/iPlug2/iPlug2.git
    GIT_TAG "${GITASEDAP_IPLUG2_COMMIT}"
    GIT_PROGRESS TRUE
  )

  FetchContent_GetProperties(iplug2)
  if(NOT iplug2_POPULATED)
    FetchContent_Populate(iplug2)
  endif()

  set(IPLUG2_DIR "${iplug2_SOURCE_DIR}" CACHE PATH "iPlug2 root" FORCE)
endif()

include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)
