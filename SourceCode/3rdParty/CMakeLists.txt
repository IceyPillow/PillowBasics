# The master CMake script of third libs.
#
# Using CMake install() & find_package() utilies is awkward, cause it depends on compiled static & dynamic libs, which differ from one platform to another.
# Rather than that, we use add_library(static) & target_link_libraries() to simplify libraries integration and cross-compilation.
#
# This fact reflects one significant belief of Pillow Basics —— useless things ain't used.
# We will never pursue "advanced techs" unless they are better or needed.

file(GLOB SOURCES CONFIGURE_DEPENDS
   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.cc"
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.inl"
)
add_library(DirectXMath STATIC ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

file(GLOB SOURCES CONFIGURE_DEPENDS
   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/lodepng-apr2025/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/lodepng-apr2025/*.cc"
)
add_library(LodePNG STATIC ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_subdirectory(OpenAL-1.24.3)

set_target_properties(DirectXMath LodePNG PROPERTIES FOLDER "3rdParty")
set_target_properties(
   OpenAL
   alsoft.common
   alsoft.common-examples
   alsoft.fmt
   clang-tidy-check
   PROPERTIES FOLDER "3rdParty/OpenAL-Soft-1.24.3"
)