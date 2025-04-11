# Resursively searches, auto refreshes when building.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

#message("SOURCES: ${SOURCES}")

add_executable(Crucible ${SOURCES})

# Build an IDE hierarchy.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

set_target_properties(Crucible PROPERTIES
  WIN32_EXECUTABLE true
)