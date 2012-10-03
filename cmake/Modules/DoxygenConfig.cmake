FILE(GLOB dox_configs ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.*.in)
if(dox_configs)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    add_custom_target(doc)
    MESSAGE("-- Detecting documentation language")
    FOREACH(doxconf ${dox_configs})
        STRING(LENGTH ${doxconf} doxconf_length)
        MATH( EXPR doxconf_pos "${doxconf_length} - 5")
        STRING(SUBSTRING ${doxconf} ${doxconf_pos} 2 doxconf_lang)
        MESSAGE("  - adding ${doxconf_lang}")
        set (doxconf_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${doxconf_lang})
        configure_file(${doxconf} ${doxconf_out} @ONLY)
        FILE(STRINGS ${doxconf_out} doxconf_outdir_line REGEX "OUTPUT_DIRECTORY.*=.*$")
        STRING(REGEX REPLACE "OUTPUT_DIRECTORY.*= *" "" doxconf_outdir ${doxconf_outdir_line})
        add_custom_target(doc-${doxconf_lang}
           mkdir -p ${doxconf_outdir}
           COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${doxconf_lang} 
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
           COMMENT "Generating API documentation with Doxygen" VERBATIM)
        add_dependencies(doc doc-${doxconf_lang})
        INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${doxconf_outdir}"
        DESTINATION doc/${PROJECT_NAME})
    ENDFOREACH(doxconf)
  endif(DOXYGEN_FOUND)
endif(dox_configs)