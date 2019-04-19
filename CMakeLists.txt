cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "BELL")
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif(WIN32)

find_package(Vulkan REQUIRED)
# glfw doesn't have a good way to find itself on windows so include the source as a dependancy

# avoid building unnesary stuff
set(GLFW_BUILD_TESTS 0)
set(GLFW_BUILD_DOCS 0)
set(GLFW_BUILD_EXAMPLES 0)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1")

if(WIN32)
    set(glslangIncludeDirs "C:\\Program Files (x86)\\glslang\\include" )
	set(glslangLibs "C:\\Program Files (x86)\\glslang\\lib\\glslang.lib")

	set(glmIncludeDirs "C:\\Program Files (x86)\\glm\\include")
else()
    set(glslangLibs /usr/lib/libglslang.so /usr/lib/libglslang-default-resource-limits.so  /usr/lib/libSPIRV.so)
endif(WIN32)

if(USE_ASAN)
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

include_directories(
                        "${CMAKE_CURRENT_LIST_DIR}/Include/"
                        "${CMAKE_CURRENT_LIST_DIR}/Source/"

						# ThirdParty header search paths
						${Vulkan_INCLUDE_DIRS}
						"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1/include"
						${glslangIncludeDirs}
						${glmIncludeDirs}
		    )

set(BELL_BASE_SOURCE
	# Core files (dependant on vulkan) 
	Source/Core/RenderInstance.cpp
	Source/Core/RenderDevice.cpp
	Source/Core/Image.cpp
	Source/Core/Buffer.cpp
	Source/Core/MemoryManager.cpp
	Source/Core/BarrierManager.cpp
	Source/Core/SwapChain.cpp
	Source/Core/CommandPool.cpp
	Source/Core/Shader.cpp
	Source/Core/DescriptorManager.cpp

	#RenderGraph files (backend independant)
	Source/RenderGraph/GraphicsTask.cpp
	Source/RenderGraph/ComputeTask.cpp
	Source/RenderGraph/RenderGraph.cpp
	)

# Build just the base as a seperate target incase users want to build a different engine

add_library("${PROJECT_NAME}_BASE" ${BELL_BASE_SOURCE})

if(WIN32)
	set(BELL_BASE_DEPENDANCIES ${Vulkan_LIBRARIES} glfw ${glslangLibs})
else()
	set(BELL_BASE_DEPENDANCIES ${Vulkan_LIBRARIES} glfw stdc++fs ${glslangLibs})
endif(WIN32)

target_link_libraries("${PROJECT_NAME}_BASE" ${BELL_BASE_DEPENDANCIES})


# Build the main engine as a library

set(BELL_SOURCE
    Source/Engine/Engine.cpp
    Source/Engine/AABB.cpp
    Source/Engine/BVH.cpp
    Source/Engine/Camera.cpp
    Source/Engine/GraphResolver.cpp
    Source/Engine/StaticMesh.cpp
    Source/Engine/BufferBuilder.cpp
    Source/Engine/GeomUtils.cpp
    )


add_library(${PROJECT_NAME} ${BELL_SOURCE})
target_link_libraries(${PROJECT_NAME} BELL_BASE)
