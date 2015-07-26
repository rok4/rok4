FILE(REMOVE_RECURSE
  "build/.libs/libjpeg.a"
  "CMakeFiles/jpeg-configure"
  "build/config.status"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/jpeg-configure.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
