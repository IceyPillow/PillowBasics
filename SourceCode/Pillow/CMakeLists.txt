# Resursively searches, auto refreshes when building.
# Notes: In this command, "${WHAT_DIR}/*.h" means find all .h files in WHAT_DIR and its sub folders.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
#   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

#message("SOURCES: ${SOURCES}")

add_executable(Pillow WIN32 ${SOURCES})

target_include_directories(Pillow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link static libraries.
# DirectX 12 is decoupled from Windows SDK now (DirectX12 Agility SDK + DirectX Shader Compiler).
# target_link_libraries(Pillow PRIVATE dxgi.lib D3D12.lib d3dcompiler.lib)
target_link_libraries(Pillow PRIVATE dxgi.lib D3D12.lib imm32.lib Xinput.lib)

#Copy the 3rd-party runtimes.
message(STATUS "")
message(STATUS "RUNTIMES (Copyable Runtime Libraries) =")
foreach(item IN LISTS RUNTIMES)
    message(STATUS "${item}")
endforeach()
message(STATUS "")
add_custom_command(TARGET Pillow
   POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${RUNTIMES}
         "$<TARGET_FILE_DIR:Pillow>" # Generator Expressions in CMake
   COMMAND_EXPAND_LISTS # Filter the list
   VERBATIM # Adjust the command arguments, so they can be passed precisely
   COMMENT "Copying the 3rd-party runtimes to the output dir..."
   )
if(WIN32)
   message(STATUS "D3D12_RUNTIMES (Only On Win64) =")
   foreach(item IN LISTS D3D12_RUNTIMES)
      message(STATUS "${item}")
   endforeach()
    # Generator Expressions in CMake
   set(D3D12_DEPLOY_DIR "$<TARGET_FILE_DIR:Pillow>/D3D12/")
   add_custom_command(TARGET Pillow
   POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E make_directory "${D3D12_DEPLOY_DIR}"
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${D3D12_RUNTIMES} "${D3D12_DEPLOY_DIR}"
   COMMAND_EXPAND_LISTS # Filter the list
   VERBATIM # Adjust the command arguments, so they can be passed precisely
   COMMENT "Copying the DirectX 12 Agility runtimes to the output dir..."
   )
message(STATUS "")
endif()

# Link the 3rd-party libraries.
target_link_libraries(Pillow PRIVATE ${LIBRARIES})
#message("LIBS="  ${LIBRARIES})
set_target_properties(${LIBRARIES} PROPERTIES FOLDER "3rdParty")
#target_link_directories(Pillow PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty")
target_include_directories(Pillow PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty")

# Build an IDE hierarchy.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
