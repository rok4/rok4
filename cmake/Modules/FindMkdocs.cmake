# - This module looks for mkdocs
# Find the command line mkdocs generator
#
# This modules defines
#  MKDOCS_FOUND
#  MKDOCS_EXECUTABLE

#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
# Copyright 2011 Peter Colberg
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_program(MKDOCS_EXECUTABLE NAMES mkdocs
  HINTS
  $ENV{MKDOCS_DIR}
  PATH_SUFFIXES bin
  DOC "Mkdocs - site generator from markdown files"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Mkdocs DEFAULT_MSG
  MKDOCS_EXECUTABLE
)

mark_as_advanced(
  MKDOCS_EXECUTABLE
)
