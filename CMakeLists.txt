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
set(DX_12  0)

include_directories(
			"${CMAKE_CURRENT_LIST_DIR}/"
			"${CMAKE_CURRENT_LIST_DIR}/Include/"
			"${CMAKE_CURRENT_LIST_DIR}/Source/"

			# ThirdParty header search paths
			${Vulkan_INCLUDE_DIRS}
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1/include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/assimp-4.1.0/include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-master"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-guizmo"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-node-editor/NodeEditor/Include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/glm"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/stb_image"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/D3D12MemoryAllocator/src"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/RecastDetour/Recast/Include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/RecastDetour/Detour/Include"
			"${CMAKE_CURRENT_LIST_DIR}/ThirdParty/Optik/src"
			${glmIncludeDirs}
		    )

# avoid building unnecessary stuff
set(GLFW_BUILD_DOCS 0)
set(GLFW_BUILD_TESTS 0)
set(GLFW_BUILD_EXAMPLES 0)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/GLFW3/glfw-3.2.1")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/assimp-4.1.0")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/imgui-node-editor/NodeEditor")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/RecastDetour")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ThirdParty/Optik")


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
		Source/Core/Vulkan/VulkanBarrierManager.cpp
		Source/Core/Vulkan/VulkanCommandContext.cpp
		Source/Core/Vulkan/VulkanAccelerationStructures.cpp)

	add_definitions(-DVULKAN -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
elseif(DX_12)
	set(BACKEND_SOURCE
		Source/Core/DX_12/DX_12RenderInstance.cpp
		Source/Core/DX_12/DX_12RenderDevice.cpp
		Source/Core/DX_12/DX_12Image.cpp
		Source/Core/DX_12/DX_12ImageView.cpp
		Source/Core/DX_12/DX_12Buffer.cpp
		Source/Core/DX_12/DX_12BufferView.cpp
		Source/Core/DX_12/DX_12Shader.cpp
		Source/Core/DX_12/DX_12SwapChain.cpp
		Source/Core/DX_12/DX_12ShaderResourceSet.cpp
		Source/Core/DX_12/DX_12Executor.cpp
		Source/Core/DX_12/DescriptorAllocator.cpp
		
		ThirdParty/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp
		)

	    add_definitions(-DDX_12 -DGLM_FORCE_DEPTH_ZERO_TO_ONE)
else()
message(FATAL_ERROR "no rendering back-end selected select VULKAN or DX_12")
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
	Source/Core/CommandContext.cpp
	Source/Core/AccelerationStructures.cpp
	Source/Core/ShaderCompiler.cpp
	
	${BACKEND_SOURCE}

	# RenderGraph files (backend independant)
	Source/RenderGraph/GraphicsTask.cpp
	Source/RenderGraph/RenderGraph.cpp

	# Utils
	Source/Core/ConversionUtils.cpp
	)

# Build just the base as a seperate target incase users want to build a different engine

add_library("${PROJECT_NAME}_BASE" ${BELL_BASE_SOURCE})

if(WIN32)
    set(BELL_BASE_DEPENDANCIES glfw)
else()
    set(BELL_BASE_DEPENDANCIES glfw stdc++fs)
endif(WIN32)

if(VULKAN)
    list(APPEND BELL_BASE_DEPENDANCIES ${Vulkan_LIBRARIES} $ENV{VULKAN_SDK}/lib/dxcompiler.lib)
elseif(DX_12)
	list(APPEND BELL_BASE_DEPENDANCIES D3d12 DXGI dxcompiler)
endif()

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
    Source/Engine/Animation.cpp
    Source/Engine/DefaultResourceSlots.cpp
    Source/Engine/RayTracedScene.cpp
    Source/Engine/RayTracingSamplers.cpp
    Source/Engine/CPUImage.cpp
    Source/Engine/PBR.cpp
    Source/Engine/NavigationMesh.cpp
    Source/Engine/UtilityTasks.cpp
    Source/Engine/VoxelTerrain.cpp
    Source/Engine/SceneNavigator.cpp
	Source/Engine/UberShaderStateCache.cpp
	Source/Engine/Allocators.cpp

    # Techniques
    Source/Engine/SSAOTechnique.cpp
    Source/Engine/GBufferTechnique.cpp
    Source/Engine/BlurXTechnique.cpp
    Source/Engine/BlurYTechnique.cpp
    Source/Engine/PreDepthTechnique.cpp
    Source/Engine/OverlayTechnique.cpp
    Source/Engine/SkyboxTechnique.cpp
    Source/Engine/ConvolveSkyboxTechnique.cpp
    Source/Engine/DFGGenerationTechnique.cpp
    Source/Engine/ImageBasedLightingTechnique.cpp
    Source/Engine/CompositeTechnique.cpp
    Source/Engine/ForwardIBLTechnique.cpp
    Source/Engine/LightFroxelationTechnique.cpp
    Source/Engine/DeferredAnalyticalLightingTechnique.cpp
    Source/Engine/ShadowMappingTechnique.cpp
    Source/Engine/ForwardLightingCombinedTechnique.cpp
    Source/Engine/TAATechnique.cpp
    Source/Engine/LineariseDepthTechnique.cpp
    Source/Engine/ScreenSpaceReflectionTechnique.cpp
    Source/Engine/VoxalizeTechnique.cpp
    Source/Engine/DebugVisualizationTechnique.cpp
    Source/Engine/TransparentTechnique.cpp
    Source/Engine/CascadeShadowMappingTechnique.cpp
    Source/Engine/DeferredProbeGITechnique.cpp
    Source/Engine/VisualizeLightProbesTechnique.cpp
    Source/Engine/OcclusionCullingTechnique.cpp
    Source/Engine/PathTracingTechnique.cpp
    Source/Engine/DownSampleColourTechnique.cpp
    Source/Engine/VoxelTerrainTechnique.cpp
	Source/Engine/InstanceIDTechnique.cpp
	Source/Engine/BuildAccelerationStructuresTechnique.cpp

    # used for texture loading
    ThirdParty/stb_image/stb_image.cpp
    ThirdParty/stb_image/stbi_image_write.cpp
    )


set(DearImGUI_SOURCE
    ThirdParty/imgui-master/imgui.cpp
    ThirdParty/imgui-master/imgui_draw.cpp
    ThirdParty/imgui-master/imgui_widgets.cpp
    ThirdParty/imgui-guizmo/ImGuizmo.cpp
    )


set(SHADERS
    ActiveFroxels.comp                         
	FullScreenTriangle.vert                   
	Overlay.vert
	GaussianBlur.hlsl                        
	PBR.hlsl
	CubemapParallax.hlsl                
	GBuffer.frag                              
	ShadowMap.vert
	Blit.frag                                  
	SkyBox.frag
	blurX.comp                                 
	DeferredAnalyticalLighting.comp     
	GBufferPassThrough.vert                   
	SkyBox.vert
	BlurX_Base.hlsl                            
	DeferredDFGIBL.frag                 
	GBufferPassThroughInstanced.vert          
	SkyBoxConvolve.comp
	blurX_SRGB.comp                            
	DepthOnly.vert                      
	SSAO.frag
	blurXR8.comp                               
	blurY.comp                                 
	DFGLutGenerate.comp                 
	Hammersley.hlsl                           
	TAAResolve.comp
	BlurY_Base.hlsl                            
	Empty.frag                          
	IndirectFroxelArgs.comp                   
	UniformBuffers.hlsl
	blurY_SRGB.comp                            
	ForwardCombinedLighting.frag        
	InPlaceCombine.comp                       
	Utilities.hlsl
	blurYR8.comp                               
	ForwardIBL.frag                     
	InPlaceCombineSRGB.comp                   
	VarianceShadowMap.frag
	ClusteredLighting.hlsl                     
	ForwardMaterial.vert                
	LightFroxelationClearCounters.comp        
	CompositeOverlay.frag
	CompositeOverlayTAA.frag
	NormalMapping.hlsl
	FroxelationGenerateLightLists.comp  
	Overlay.frag
	ShadowMapping.hlsl
	ResolveVarianceShadowMap.comp
	blurXrg32f.comp
	blurYrg32f.comp
	blurXr32f.comp
	blurYr32f.comp
	AlphaTestDepthOnly.frag
	MeshAttributes.hlsl
	VertexOutputs.hlsl
	LineariseDepth10.comp
	LineariseDepth11.comp
	FinalComposite.frag
	ScreenSpaceReflection.comp
	RayTracedReflections.comp
	Materials.hlsl
	Voxalize.vert
	Voxalize.geom
	Voxalize.geom.spv
	Voxalize.frag
	DebugVoxels.comp
	DebugAABB.vert
	DebugAABB.frag
	Transparent.frag
	ResolveCascadeVarianceShadowMap.comp
	Skinning.comp
	SkinnedBlendShapes.comp
	RayMarching.hlsl
	LightProbes.hlsl
	DeferredProbeGI.frag
	LightProbeVis.vert
	LightProbeVis.frag
	LightProbeVolumeVis.frag
	OcclusionCulling.comp
	RayTracing.hlsl
	PathTracer.comp
	RayTracedShadows.comp
	BilaterialUpsample.comp
	BilaterialUpsampleRGBA8.comp
	KdTree.hlsl
	BasicVertexTransform.vert
	LightDebug.frag
	MarchingCubes.comp
	InitialiseIndirectDraw.comp
	Terrain.vert
	TerrainDeferred.frag
	ModifyTerrain.comp
	ColourMapping.hlsl
	InstanceID.vert
	InstanceID.frag
	SetPickedInstance.comp
	Skinning.hlsl
    )

message(STATUS "shader directory ${BELL_SHADER_DIR}Shaders")
foreach(shader ${SHADERS})
  ADD_CUSTOM_COMMAND(OUTPUT "${BELL_SHADER_DIR}Shaders/${shader}"
    COMMAND cmake -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/Shaders/${shader}" "${BELL_SHADER_DIR}Shaders/${shader}"
    DEPENDS   "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/Shaders/${shader}"
    )
list(APPEND shader_dest "${BELL_SHADER_DIR}Shaders/${shader}")
 
endforeach(shader)
add_custom_target(COPY_SHADERS DEPENDS ${shader_dest})

if(MSVC)
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/Assets" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
else()
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/Assets" DESTINATION "${CMAKE_BINARY_DIR}/")
endif(MSVC)


add_library(${PROJECT_NAME} ${BELL_SOURCE} ${DearImGUI_SOURCE})
target_link_libraries(${PROJECT_NAME} BELL_BASE assimp Recast Detour OptickCore)
add_dependencies(${PROJECT_NAME} COPY_SHADERS)



set(BELL_EDITOR_SOURCE
	Source/Editor/main.cpp
	Source/Editor/Editor.cpp
	Source/Editor/MeshPicker.cpp
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
