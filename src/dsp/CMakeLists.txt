add_library(
  gitasedap_dsp
  STATIC
    BodyProfile.cpp
    BodyProfileCompiler.cpp
    HybridBodyEngine.cpp
    SourceAdapter.cpp
)

target_link_libraries(
  gitasedap_dsp
  PUBLIC
    gitasedap_core
)

target_include_directories(
  gitasedap_dsp
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)

target_compile_features(gitasedap_dsp PUBLIC cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_dsp)
