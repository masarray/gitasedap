add_library(
  gitasedap_lab
  STATIC
    OfflineRenderer.cpp
    RealGuitarCalibration.cpp
    SignalGenerator.cpp
    WavFile.cpp
)

target_link_libraries(
  gitasedap_lab
  PUBLIC
    gitasedap_core
)

target_include_directories(
  gitasedap_lab
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)

target_compile_features(gitasedap_lab PUBLIC cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_lab)
