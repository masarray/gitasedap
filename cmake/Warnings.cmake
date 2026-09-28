include_guard(GLOBAL)

function(gitasedap_enable_project_warnings target)
  if(MSVC)
    target_compile_options(
      ${target}
      PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
  else()
    target_compile_options(
      ${target}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
    )
  endif()
endfunction()
