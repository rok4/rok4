FILE(REMOVE_RECURSE
  "src/configure"
  "CMakeFiles/tiff-build"
  "install/lib/libtiff.a"
  "src/configure"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/tiff-build.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
