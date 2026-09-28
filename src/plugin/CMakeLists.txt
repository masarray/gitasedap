iplug_add_plugin(
  GitaSedap
  SOURCES
    GitaSedap.cpp
    GitaSedap.h
    config.h
  FORMATS
    VST3
)

target_link_libraries(GitaSedap-vst3 PRIVATE gitasedap_core)
target_include_directories(
  GitaSedap-vst3
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
)
target_compile_features(GitaSedap-vst3 PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(GitaSedap-vst3)
