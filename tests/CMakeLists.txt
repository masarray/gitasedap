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
