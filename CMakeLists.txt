cmake_minimum_required(VERSION 3.4)
set(PROJECT_NAME "BELL")
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")

include_directories(
			"./Include/"
			"./Source/"
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
target_link_libraries(${PROJECT_NAME} vulkan)
