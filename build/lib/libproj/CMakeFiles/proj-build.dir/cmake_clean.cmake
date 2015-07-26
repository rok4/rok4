FILE(REMOVE_RECURSE
  "src/configure"
  "CMakeFiles/proj-build"
  "install/lib/libproj.a"
  "src/configure"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/proj-build.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
