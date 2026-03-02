# The master CMake script of third libs.
#
# Using CMake install() & find_package() utilies is awkward, cause it depends on compiled static & dynamic libs, which differ from one platform to another.
# Rather than that, we use add_library(static) & target_link_libraries() to simplify libraries integration and cross-compilation.
#
# This fact reflects one significant belief of Pillow Basics —— useless things ain't used.
# We will never pursue "advanced techs" unless they are better or needed.

set(LIBRARIES DirectXMath UTF8cc LodePNG PhysX HashLib cgltf stb_truetype)
if(WIN32)
    list(APPEND LIBRARIES DirectX12Agility DXCompiler XeSS)
endif()
set(LIBRARIES ${LIBRARIES} PARENT_SCOPE) # Write back to the parent scope.

function(get_prebuilt_path dependencyPath)
   set(PREBUILT_PATH "${dependencyPath}/Prebuilt")
   if(WIN32)
      set(PREBUILT_PATH "${PREBUILT_PATH}/Windows-x64")
   elseif(ANDROID)
      set(PREBUILT_PATH "${PREBUILT_PATH}/Android-Armv8a")
   endif()
   return(PROPAGATE PREBUILT_PATH)
endfunction()

# DirectXMath
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.cc"
   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025/*.inl"
)
add_library(DirectXMath STATIC ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath-apr2025" FILES ${SOURCES})

# UTF8-CPP
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/utfcpp-4.0.6/*.h"
)
add_library(UTF8cc INTERFACE ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/utfcpp-4.0.6" FILES ${SOURCES})

# LodePNG
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/lodepng-apr2025/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/lodepng-apr2025/*.cc"
)
add_library(LodePNG STATIC ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/lodepng-apr2025" FILES ${SOURCES})

# PhysX
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/PhysX-4.1/*.h")
add_library(PhysX INTERFACE ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/PhysX-4.1" FILES ${SOURCES})
set(PREBUILT_PATH "")
get_prebuilt_path("${CMAKE_CURRENT_SOURCE_DIR}/PhysX-4.1")
if(WIN32)
   set(LIBS
      optimized "${PREBUILT_PATH}/Release/PhysXCharacterKinematic_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXCommon_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXCooking_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXExtensions_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXFoundation_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXVehicle_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysXPvdSDK_static_64.lib"
      optimized "${PREBUILT_PATH}/Release/PhysX_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXCharacterKinematic_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXCommon_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXCooking_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXExtensions_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXFoundation_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXVehicle_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysXPvdSDK_static_64.lib"
      debug "${PREBUILT_PATH}/Checked/PhysX_static_64.lib"
   )
elseif(ANDROID)
   set(LIBS
      optimized "${PREBUILT_PATH}/Release/libPhysXCharacterKinematic_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXCommon_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXCooking_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXExtensions_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXFoundation_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXVehicle_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysXPvdSDK_static.a"
      optimized "${PREBUILT_PATH}/Release/libPhysX_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXCharacterKinematic_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXCommon_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXCooking_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXExtensions_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXFoundation_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXVehicle_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysXPvdSDK_static.a"
      debug "${PREBUILT_PATH}/Checked/libPhysX_static.a"
   )
endif()
target_link_libraries(PhysX INTERFACE ${LIBS})

# HashLib
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/HashLib/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/HashLib/*.cc"
)
add_library(HashLib STATIC ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/HashLib" FILES ${SOURCES})

# cgltf
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/cgltf-1.15/*.h"
)
add_library(cgltf INTERFACE ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/cgltf-1.15" FILES ${SOURCES})

# stb_truetype
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
   "${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype/*.h"
)
add_library(stb_truetype INTERFACE ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype" FILES ${SOURCES})

if(WIN32)
   # DirectX12Agility
   file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      "${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618/*.cc"
      )
   add_library(DirectX12Agility STATIC ${SOURCES})
   source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618" FILES ${SOURCES})
   set(D3D12_RUNTIMES
      "${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618/binary/D3D12Core.dll"
      "$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618/binary/d3d12SDKLayers.dll>"
      "$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/DX12Agility-1.618/binary/D3D12StateObjectCompiler.dll>"
   )
   set(D3D12_RUNTIMES ${D3D12_RUNTIMES} PARENT_SCOPE)

   # DXCompiler
   # Use a standalone DirectX shader compiler to avoid compatibility issues.
   file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026/*.h")
   add_library(DXCompiler INTERFACE ${SOURCES})
   source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026" FILES ${SOURCES})
   SET(LIBS
      general "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026/binary/dxil.lib"
      general "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026/binary/dxcompiler.lib"
   )
   target_link_libraries(DXCompiler INTERFACE ${LIBS})
   list(APPEND RUNTIMES
      "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026/binary/dxil.dll"
      "${CMAKE_CURRENT_SOURCE_DIR}/dxc_feb2026/binary/dxcompiler.dll"
   )

   # XeSS
   file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/*.h")
   add_library(XeSS INTERFACE ${SOURCES})
   source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1" FILES ${SOURCES})
   SET(LIBS
      general "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxess.lib"
      general "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxell.lib"
      general "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxess_fg.lib"
   )
   target_link_libraries(XeSS INTERFACE ${LIBS})
   list(APPEND RUNTIMES
      "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxess.dll"
      "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxell.dll"
      "${CMAKE_CURRENT_SOURCE_DIR}/XeSS_2.1.1/binary/libxess_fg.dll"
   )
endif()

# Write back to the parent scope.
set(RUNTIMES ${RUNTIMES} PARENT_SCOPE)