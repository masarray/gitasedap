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

add_executable(
  gitasedap_dsp_laboratory_tests
  DspLaboratoryTests.cpp
)

target_link_libraries(
  gitasedap_dsp_laboratory_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_dsp_laboratory_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_dsp_laboratory_tests)

add_test(
  NAME gitasedap.dsp_laboratory
  COMMAND gitasedap_dsp_laboratory_tests
)

add_executable(
  gitasedap_wav_file_tests
  WavFileTests.cpp
)

target_link_libraries(
  gitasedap_wav_file_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_wav_file_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_wav_file_tests)

add_test(
  NAME gitasedap.wav_file
  COMMAND gitasedap_wav_file_tests
)

add_executable(
  gitasedap_parameter_torture_tests
  ParameterTortureTests.cpp
)

target_link_libraries(
  gitasedap_parameter_torture_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_parameter_torture_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_parameter_torture_tests)

add_test(
  NAME gitasedap.parameter_torture
  COMMAND gitasedap_parameter_torture_tests
)

add_executable(
  gitasedap_deadline_benchmark_tests
  DeadlineBenchmarkTests.cpp
)

target_link_libraries(
  gitasedap_deadline_benchmark_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_deadline_benchmark_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_deadline_benchmark_tests)

add_test(
  NAME gitasedap.deadline_benchmark
  COMMAND gitasedap_deadline_benchmark_tests
)

add_executable(
  gitasedap_signal_generator_tests
  SignalGeneratorTests.cpp
)

target_link_libraries(
  gitasedap_signal_generator_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_signal_generator_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_signal_generator_tests)

add_test(
  NAME gitasedap.signal_generator
  COMMAND gitasedap_signal_generator_tests
)

add_executable(
  gitasedap_source_adapter_tests
  SourceAdapterTests.cpp
)

target_link_libraries(
  gitasedap_source_adapter_tests
  PRIVATE
    gitasedap_dsp
    gitasedap_lab
)

target_compile_features(gitasedap_source_adapter_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_source_adapter_tests)

add_test(
  NAME gitasedap.source_adapter
  COMMAND gitasedap_source_adapter_tests
)

add_executable(
  gitasedap_smoothing_tests
  SmoothingTests.cpp
)

target_link_libraries(
  gitasedap_smoothing_tests
  PRIVATE
    gitasedap_dsp
)

target_compile_features(gitasedap_smoothing_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_smoothing_tests)

add_test(
  NAME gitasedap.smoothing
  COMMAND gitasedap_smoothing_tests
)

add_executable(
  gitasedap_body_profile_tests
  BodyProfileTests.cpp
)

target_link_libraries(
  gitasedap_body_profile_tests
  PRIVATE
    gitasedap_dsp
)

target_compile_features(gitasedap_body_profile_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_body_profile_tests)

add_test(
  NAME gitasedap.body_profile
  COMMAND gitasedap_body_profile_tests
)

add_executable(
  gitasedap_hybrid_body_engine_tests
  HybridBodyEngineTests.cpp
)

target_link_libraries(
  gitasedap_hybrid_body_engine_tests
  PRIVATE
    gitasedap_dsp
    gitasedap_lab
)

target_compile_features(gitasedap_hybrid_body_engine_tests PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_hybrid_body_engine_tests)

add_test(
  NAME gitasedap.hybrid_body_engine
  COMMAND gitasedap_hybrid_body_engine_tests
)

add_executable(
  gitasedap_body_profile_runtime_tests
  BodyProfileRuntimeTests.cpp
)

target_link_libraries(
  gitasedap_body_profile_runtime_tests
  PRIVATE
    gitasedap_runtime
)

target_compile_features(
  gitasedap_body_profile_runtime_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_body_profile_runtime_tests)

add_test(
  NAME gitasedap.body_profile_runtime
  COMMAND gitasedap_body_profile_runtime_tests
)


add_executable(
  gitasedap_state_migration_tests
  StateMigrationTests.cpp
)

target_link_libraries(
  gitasedap_state_migration_tests
  PRIVATE
    gitasedap_core
)

target_compile_features(
  gitasedap_state_migration_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_state_migration_tests)

add_test(
  NAME gitasedap.state_migration
  COMMAND gitasedap_state_migration_tests
)

add_executable(
  gitasedap_profile_comparison_tests
  ProfileComparisonTests.cpp
)

target_link_libraries(
  gitasedap_profile_comparison_tests
  PRIVATE
    gitasedap_dsp
    gitasedap_lab
)

target_compile_features(
  gitasedap_profile_comparison_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_profile_comparison_tests)

add_test(
  NAME gitasedap.profile_comparison
  COMMAND gitasedap_profile_comparison_tests
)

add_executable(
  gitasedap_factory_profile_runtime_tests
  FactoryProfileRuntimeTests.cpp
)

target_link_libraries(
  gitasedap_factory_profile_runtime_tests
  PRIVATE
    gitasedap_runtime
)

target_compile_features(
  gitasedap_factory_profile_runtime_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(gitasedap_factory_profile_runtime_tests)

add_test(
  NAME gitasedap.factory_profile_runtime
  COMMAND gitasedap_factory_profile_runtime_tests
)


add_executable(
  gitasedap_real_guitar_calibration_tests
  RealGuitarCalibrationTests.cpp
)

target_link_libraries(
  gitasedap_real_guitar_calibration_tests
  PRIVATE
    gitasedap_lab
)

target_compile_features(
  gitasedap_real_guitar_calibration_tests
  PRIVATE
    cxx_std_20
)
gitasedap_enable_project_warnings(
  gitasedap_real_guitar_calibration_tests
)

add_test(
  NAME gitasedap.real_guitar_calibration
  COMMAND gitasedap_real_guitar_calibration_tests
)
