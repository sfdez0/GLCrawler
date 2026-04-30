cmake_minimum_required ( VERSION 3.7 )
project ( GpO VERSION 0.0.2)
set( GPO_VERSION_ABI 1 )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

# Establecemos el estándar de C++ 17 para todo el proyecto
set(CMAKE_CXX_STANDARD 17) # C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Obligatorio
set(CMAKE_CXX_EXTENSIONS OFF) # Oficial sin extensiones específicas del compilador

if( "${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}" )
 message( FATAL "no in source building allowed." )
endif( )

list( APPEND CMAKE_MODULE_PATH
 ${PROJECT_SOURCE_DIR}/CMake
)

set( GPO_DESCRIPTION "GpO Base Project" )
set( GPO_LICENSE LGPL )

set(BUILD_SHARED_LIBS OFF)

find_package(OpenGL REQUIRED)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_subdirectory(libs/glm )

add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
add_subdirectory(libs/glad )

set(ASSIMP_NO_EXPORT ON)
if (WIN32)
	set(ASSIMP_BUILD_ZLIB ON)
else ()
	set(ASSIMP_BUILD_ZLIB OFF) # Configuración mac
endif()
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
add_subdirectory(libs/assimp )

include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} )
include_directories(libs/glad/include)
include_directories(libs/glm)
include_directories(libs/imgui)
include_directories(libs/assimp/include)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/KHR)
include_directories(${PROJECT_SOURCE_DIR}/include/stb)
include_directories(${PROJECT_SOURCE_DIR}/tests/assimp)

set(GPO_SOURCE_FILES include/stb/stb_image.h
					 include/GpO.h
					 include/maze.h
					 include/flame.h
					 include/lighting.h
					 include/torch_module.h
					 include/Particle.h
					 include/ParticleEmitter.h
					 include/ParticleSystem.h
					 src/GPO_proyecto_final.cpp
					 src/GPO_aux.cpp
					 src/maze.cpp
					 src/flame.cpp
					 src/lighting.cpp
					 src/torch_module.cpp
					 src/Particle.cpp
					 src/ParticleEmitter.cpp
					 src/ParticleSystem.cpp
					 tests/assimp/GPO_assimp_aux.h
					 tests/assimp/GPO_assimp_aux.cpp
					 libs/imgui/imgui.cpp
					 libs/imgui/imgui_draw.cpp
					 libs/imgui/imgui_tables.cpp
					 libs/imgui/imgui_widgets.cpp
					 libs/imgui/backends/imgui_impl_glfw.cpp
					 libs/imgui/backends/imgui_impl_opengl3.cpp)
add_executable(gpo_proyecto_final ${GPO_SOURCE_FILES})
target_link_libraries(gpo_proyecto_final PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface assimp)


set_target_properties(gpo_proyecto_final
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

set_target_properties(gpo_proyecto_final
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#GUI interface
set(GPO_GUI_SOURCE_FILES  	libs/imgui/imgui.cpp
							libs/imgui/imgui_demo.cpp
							libs/imgui/imgui_draw.cpp
							libs/imgui/imgui_tables.cpp
							libs/imgui/imgui_widgets.cpp
							libs/imgui/backends/imgui_impl_glfw.cpp
							libs/imgui/backends/imgui_impl_opengl3.cpp
							tests/gui/min_gui.cpp)

add_executable(gpo_min_gui ${GPO_GUI_SOURCE_FILES})
target_link_libraries(gpo_min_gui PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface)

set_target_properties(gpo_min_gui
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#Import
set(GPO_ASS_SOURCE_FILES  	include/stb/stb_image.h
							include/GpO.h
							src/GPO_aux.cpp
							tests/assimp/GPO_assimp_aux.h
							tests/assimp/GPO_assimp_aux.cpp
							tests/assimp/min_assimp.cpp
							)
add_executable(gpo_min_assimp ${GPO_ASS_SOURCE_FILES})
target_link_libraries(gpo_min_assimp PRIVATE ${OPENGL_LIBRARIES} glfw glm glad_interface assimp)
set_target_properties(gpo_min_assimp
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file(GLOB MY_CONTENT "data/*")
file(COPY ${MY_CONTENT} DESTINATION "${CMAKE_BINARY_DIR}/bin/data")
