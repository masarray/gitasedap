include_guard(GLOBAL)
include(FetchContent)

# Dependency policy:
# - exact immutable commits, never floating branches
# - local iPlug2 checkout override for fast developer iteration
# - public SDK source is prepared before iPlug2 creates plugin targets
# - dependency setup stays outside the real-time product code
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

set(
  GITASEDAP_VST3SDK_COMMIT
  "3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96"
  CACHE STRING
  "Pinned Steinberg VST3 SDK commit"
)

set(IPLUG2_CXX_STANDARD 20 CACHE STRING "C++ standard used by iPlug2" FORCE)

if(GITASEDAP_IPLUG2_DIR)
  set(IPLUG2_DIR "${GITASEDAP_IPLUG2_DIR}" CACHE PATH "iPlug2 root" FORCE)
else()
  FetchContent_Declare(
    gitasedap_iplug2
    GIT_REPOSITORY https://github.com/iPlug2/iPlug2.git
    GIT_TAG "${GITASEDAP_IPLUG2_COMMIT}"
    GIT_SHALLOW FALSE
    GIT_PROGRESS TRUE
  )

  FetchContent_GetProperties(gitasedap_iplug2)
  if(NOT gitasedap_iplug2_POPULATED)
    FetchContent_Populate(gitasedap_iplug2)
  endif()

  set(IPLUG2_DIR "${gitasedap_iplug2_SOURCE_DIR}" CACHE PATH "iPlug2 root" FORCE)
endif()

# iPlug2 deliberately keeps public plug-in SDKs outside its main repository.
# Prepare the exact VST3 SDK revision into the path iPlug2 expects, before
# importing iPlug2::VST3. Only the submodules required for VST3 builds are
# fetched; samples/tutorials/vstgui are intentionally excluded.
set(_gitasedap_vst3sdk_dir "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK")

if(NOT EXISTS "${_gitasedap_vst3sdk_dir}/pluginterfaces")
  FetchContent_Declare(
    gitasedap_vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG "${GITASEDAP_VST3SDK_COMMIT}"
    GIT_SHALLOW FALSE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES
      base
      cmake
      pluginterfaces
      public.sdk
    GIT_SUBMODULES_RECURSE TRUE
    SOURCE_DIR "${_gitasedap_vst3sdk_dir}"
  )

  FetchContent_GetProperties(gitasedap_vst3sdk)
  if(NOT gitasedap_vst3sdk_POPULATED)
    FetchContent_Populate(gitasedap_vst3sdk)
  endif()
endif()

include("${IPLUG2_DIR}/iPlug2.cmake")
find_package(iPlug2 REQUIRED)
