FILE(REMOVE_RECURSE
  "src/configure"
  "CMakeFiles/zlib-build"
  "install/lib/libz.a"
  "src/configure"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/zlib-build.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
