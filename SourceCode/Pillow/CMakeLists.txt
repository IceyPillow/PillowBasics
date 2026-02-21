# Resursively searches, auto refreshes when building.
# Notes: In this command, "${WHAT_DIR}/*.h" means find all .h files in WHAT_DIR and its sub folders.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

#message("SOURCES: ${SOURCES}")

add_executable(Pillow WIN32 ${SOURCES})

target_include_directories(Pillow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link static libraries.
target_link_libraries(Pillow PRIVATE dxgi.lib D3D12.lib d3dcompiler.lib)

#Copy the 3rd-party runtimes.
message("RUNTIMES=" ${RUNTIMES})
add_custom_command(TARGET Pillow
   POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${RUNTIMES}
         $<TARGET_FILE_DIR:Pillow>
   COMMENT "Copying the 3rd-party runtimes to the output directory..."
)

# Link the 3rd-party libraries.
target_link_libraries(Pillow PRIVATE ${LIBRARIES})
#message("LIBS="  ${LIBRARIES})
set_target_properties(${LIBRARIES} PROPERTIES FOLDER "3rdParty")
#target_link_directories(Pillow PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty")
target_include_directories(Pillow PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty")

# Build an IDE hierarchy.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
