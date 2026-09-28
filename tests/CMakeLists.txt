add_executable(
  gitasedap_parameter_spec_tests
  ParameterSpecTests.cpp
)

target_link_libraries(
  gitasedap_parameter_spec_tests
  PRIVATE
    gitasedap_core
)

target_compile_features(gitasedap_parameter_spec_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_parameter_spec_tests)

add_test(
  NAME gitasedap.parameter_spec
  COMMAND gitasedap_parameter_spec_tests
)

add_executable(
  gitasedap_state_schema_tests
  StateSchemaTests.cpp
)

target_link_libraries(
  gitasedap_state_schema_tests
  PRIVATE
    gitasedap_core
)

target_compile_features(gitasedap_state_schema_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_state_schema_tests)

add_test(
  NAME gitasedap.state_schema
  COMMAND gitasedap_state_schema_tests
)

add_executable(
  gitasedap_prepared_state_exchange_tests
  PreparedStateExchangeTests.cpp
)

target_link_libraries(
  gitasedap_prepared_state_exchange_tests
  PRIVATE
    gitasedap_runtime
)

target_compile_features(
  gitasedap_prepared_state_exchange_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_prepared_state_exchange_tests)

add_test(
  NAME gitasedap.prepared_state_exchange
  COMMAND gitasedap_prepared_state_exchange_tests
)

add_executable(
  gitasedap_coalescing_queue_tests
  CoalescingQueueTests.cpp
)

target_link_libraries(
  gitasedap_coalescing_queue_tests
  PRIVATE
    gitasedap_runtime
)

target_compile_features(
  gitasedap_coalescing_queue_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_coalescing_queue_tests)

add_test(
  NAME gitasedap.coalescing_queue
  COMMAND gitasedap_coalescing_queue_tests
)

add_executable(
  gitasedap_worker_pool_tests
  BoundedWorkerPoolTests.cpp
)

target_link_libraries(
  gitasedap_worker_pool_tests
  PRIVATE
    gitasedap_runtime
)

target_compile_features(
  gitasedap_worker_pool_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_worker_pool_tests)

add_test(
  NAME gitasedap.worker_pool
  COMMAND gitasedap_worker_pool_tests
)
