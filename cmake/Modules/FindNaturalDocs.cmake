# - This module looks for naturaldocs
# Find the command line NaturalDocs generator
#
# This modules defines
#  NATURALDOCS_FOUND
#  NATURALDOCS_EXECUTABLE

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

find_program(NATURALDOCS_EXECUTABLE NAMES naturaldocs
  HINTS
  $ENV{NATURALDOCS_DIR}
  PATH_SUFFIXES bin
  DOC "NaturalDocs - documentation generator"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(NaturalDocs DEFAULT_MSG
  NATURALDOCS_EXECUTABLE
)

mark_as_advanced(
  NATURALDOCS_EXECUTABLE
)
