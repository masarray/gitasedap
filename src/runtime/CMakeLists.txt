find_package(Threads REQUIRED)

add_library(
  gitasedap_runtime
  STATIC
    BodyProfileRuntime.cpp
    BoundedWorkerPool.cpp
)

target_link_libraries(
  gitasedap_runtime
  PUBLIC
    gitasedap_core
    gitasedap_dsp
  PRIVATE
    Threads::Threads
)

target_include_directories(
  gitasedap_runtime
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)

target_compile_features(gitasedap_runtime PUBLIC cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_runtime)
