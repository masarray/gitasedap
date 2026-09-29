add_executable(
  gitasedap_dsp_lab
  main.cpp
)

target_link_libraries(
  gitasedap_dsp_lab
  PRIVATE
    gitasedap_lab
)

target_compile_features(gitasedap_dsp_lab PRIVATE cxx_std_20)
gitasedap_enable_project_warnings(gitasedap_dsp_lab)
