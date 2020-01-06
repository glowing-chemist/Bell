cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "BELL")
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g")
endif(WIN32)

find_package(Vulkan REQUIRED)
# glfw doesn't have a good way to find itself on windows so include the source as a dependancy

set(VULKAN 1) # vulkan default backend

include_directories(
			"${CMAKE_CURRENT_LIST_DIR}/"
			"${CMAKE_CURRENT_LIST_DIR}/Include/"
			"${CMAKE_CURRENT_LIST_DIR}/Source/"

			# ThirdParty header search paths
			${Vulkan_INCLUDE_DIRS}
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1/include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/assimp-4.1.0/include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-master"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glslang"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-node-editor/NodeEditor/Include"
		    "${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glm"	
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/stb_image"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glad/include"
			${glslangIncludeDirs}
			${glmIncludeDirs}
		    )

# avoid building unnesary stuff
set(GLFW_BUILD_DOCS 0)
set(GLFW_BUILD_TESTS 0)
set(GLFW_BUILD_EXAMPLES 0)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/assimp-4.1.0")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glslang")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-node-editor/NodeEditor")


if(USE_ASAN)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

if(VULKAN)

	set(BACKEND_SOURCE 
		Source/Core/Vulkan/VulkanImage.cpp
		Source/Core/Vulkan/VulkanImageView.cpp
		Source/Core/Vulkan/VulkanBuffer.cpp
		Source/Core/Vulkan/VulkanBufferView.cpp
		Source/Core/Vulkan/MemoryManager.cpp
		Source/Core/Vulkan/DescriptorManager.cpp
		Source/Core/Vulkan/CommandPool.cpp
		Source/Core/Vulkan/VulkanPipeline.cpp
		Source/Core/Vulkan/VulkanShaderResourceSet.cpp
		Source/Core/Vulkan/VulkanExecutor.cpp
		Source/Core/Vulkan/VulkanRenderDevice.cpp
		Source/Core/Vulkan/VulkanRenderInstance.cpp
		Source/Core/Vulkan/VulkanShader.cpp
		Source/Core/Vulkan/VulkanSwapChain.cpp
		Source/Core/Vulkan/VulkanBarrierManager.cpp)

		add_definitions(-DVULKAN)

elseif(OPENGL)
	set(BACKEND_SOURCE
		ThirdParty/glad/src/glad.c
		Source/Core/OpenGL/OpenGLRenderInstance.cpp
		Source/Core/OpenGL/OpenGLRenderDevice.cpp
		Source/Core/OpenGL/OpenGLImage.cpp
		Source/Core/OpenGL/OpenGLImageView.cpp
		Source/Core/OpenGL/OpenGLBuffer.cpp
		Source/Core/OpenGL/OpenGLBufferView.cpp
		Source/Core/OpenGL/OpenGLShader.cpp)

	add_definitions(-DOPENGL)
else()
	message(FATAL_ERROR "SELECT A BACKEND VULKAN/OPENGL")
endif()

set(BELL_BASE_SOURCE
	# Core files (dependant on vulkan) 
	Source/Core/RenderInstance.cpp
	Source/Core/Image.cpp
	Source/Core/ImageView.cpp
	Source/Core/Buffer.cpp
	Source/Core/BufferView.cpp
	Source/Core/BarrierManager.cpp
	Source/Core/SwapChain.cpp
	Source/Core/Shader.cpp
	Source/Core/Sampler.cpp
	Source/Core/PipelineCache.cpp
	Source/Core/ShaderResourceSet.cpp
	
	${BACKEND_SOURCE}

	# RenderGraph files (backend independant)
	Source/RenderGraph/GraphicsTask.cpp
	Source/RenderGraph/ComputeTask.cpp
	Source/RenderGraph/RenderGraph.cpp

	# Utils
	Source/Core/ConversionUtils.cpp
	)

# Build just the base as a seperate target incase users want to build a different engine

add_library("${PROJECT_NAME}_BASE" ${BELL_BASE_SOURCE})

if(WIN32)
    set(BELL_BASE_DEPENDANCIES ${Vulkan_LIBRARIES} glfw glslang SPIRV)
else()
    set(BELL_BASE_DEPENDANCIES ${Vulkan_LIBRARIES} glfw stdc++fs glslang SPIRV)
endif(WIN32)

target_link_libraries("${PROJECT_NAME}_BASE" ${BELL_BASE_DEPENDANCIES})


# Build the main engine as a library

set(BELL_SOURCE
    Source/Engine/Engine.cpp
    Source/Engine/AABB.cpp
    Source/Engine/OctTree.cpp
    Source/Engine/Camera.cpp
    Source/Engine/GraphResolver.cpp
    Source/Engine/StaticMesh.cpp
    Source/Engine/GeomUtils.cpp
    Source/Engine/Scene.cpp

    # Techniques
    Source/Engine/SSAOTechnique.cpp
    Source/Engine/GBufferTechnique.cpp
    Source/Engine/GBufferMaterialTechnique.cpp
    Source/Engine/BlurXTechnique.cpp
    Source/Engine/BlurYTechnique.cpp
    Source/Engine/PreDepthTechnique.cpp
    Source/Engine/BlinnPhongTechnique.cpp
    Source/Engine/OverlayTechnique.cpp
    Source/Engine/SkyboxTechnique.cpp
    Source/Engine/ConvolveSkyboxTechnique.cpp
    Source/Engine/DFGGenerationTechnique.cpp
    Source/Engine/AnalyticalImageBasedLightingTechnique.cpp
    Source/Engine/ImageBasedLightingTechnique.cpp
    Source/Engine/CompositeTechnique.cpp
    Source/Engine/ForwardIBLTechnique.cpp
    Source/Engine/LightFroxelationTechnique.cpp
    Source/Engine/DeferredAnalyticalLightingTechnique.cpp
    Source/Engine/ShadowMappingTechnique.cpp

    Source/Engine/CommandContext.cpp

    # used for texture loading
    ThirdParty/stb_image/stb_image.cpp
    )

set(DearImGUI_SOURCE
    ThirdParty/imgui-master/imgui.cpp
    ThirdParty/imgui-master/imgui_draw.cpp
    ThirdParty/imgui-master/imgui_widgets.cpp
    )

file(COPY "${CMAKE_CURRENT_LIST_DIR}/Source/Engine/Shaders" DESTINATION "${CMAKE_BINARY_DIR}/")


add_library(${PROJECT_NAME} ${BELL_SOURCE} ${DearImGUI_SOURCE})
target_link_libraries(${PROJECT_NAME} BELL_BASE assimp)


set(BELL_EDITOR_SOURCE
	Source/Editor/main.cpp
	Source/Editor/Editor.cpp
	Source/Editor/NodeIMGUI.cpp
	Source/Editor/MeshesIMGUI.cpp
	Source/Editor/imguifilebrowser.cpp
	Source/Editor/ImGuiNodeEditor.cpp
	)

add_executable(BELL_EDITOR ${BELL_EDITOR_SOURCE})
target_link_libraries(BELL_EDITOR BELL imgui_node_editor)


# Example targets TODO move in to seperate cmakelist
add_executable(PASS_EXAMPLE "Examples/PassRegistration.cpp")
target_link_libraries(PASS_EXAMPLE BELL)

add_executable(COMMAND_CONTEXT_EXAMPLE "Examples/CommandContext.cpp")
target_link_libraries(COMMAND_CONTEXT_EXAMPLE BELL)
