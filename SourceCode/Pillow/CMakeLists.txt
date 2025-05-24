# Resursively searches, auto refreshes when building.
# Notes: In this command, "${WHAT_DIR}/*.h" means find all .h files in WHAT_DIR and its sub folders.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

#message("SOURCES: ${SOURCES}")

add_executable(Pillow WIN32 ${SOURCES})

# link static libraries.
target_link_libraries(Pillow PRIVATE dxgi.lib D3D12.lib d3dcompiler.lib)
# Build an IDE hierarchy.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})