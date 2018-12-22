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
set(GLFW_BUILD_TESTS 0) # avoid building unnesary stuff
set(GLFW_BUILD_DOCS 0)
set(GLFW_BUILD_EXAMPLES 0)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1")

include_directories(
                        "${CMAKE_CURRENT_LIST_DIR}/Include/"
                        "${CMAKE_CURRENT_LIST_DIR}/Source/"

						# ThirdParty header search paths
						${Vulkan_INCLUDE_DIRS}
						"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1/include"
		    )

set(SOURCE
	# Core files (dependant on vulkan) 
	"Source/Core/RenderInstance.cpp"
	"Source/Core/RenderDevice.cpp"
	"Source/Core/Image.cpp"
	"Source/Core/Buffer.cpp"
	"Source/Core/MemoryManager.cpp"
	"Source/Core/BarrierManager.cpp"
	"Source/Core/SwapChain.cpp"

	#RenderGraph files (backend independant)
	"Source/RenderGraph/GraphicsTask.cpp"
	"Source/RenderGraph/ComputeTask.cpp"
	)

# Allows glm::perspective to work with vulkans coordinate system.
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

add_library(${PROJECT_NAME} ${SOURCE})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3)
