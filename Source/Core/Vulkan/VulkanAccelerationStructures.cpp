#include "VulkanAccelerationStructures.hpp"
#include "Engine/Engine.hpp"
#include "Engine/StaticMesh.h"

#include "Core/Vulkan/VulkanRenderDevice.hpp"

VulkanBottomLevelAccelerationStructure::VulkanBottomLevelAccelerationStructure(RenderEngine& engine, const StaticMesh& mesh, const std::string& name) :
    BottomLevelAccelerationStructureBase(engine, mesh, name),
    mEng(engine),
    mBVH(constructAccelerationStructure(engine, mesh, name))
{
}


VulkanAccelerationStructure VulkanBottomLevelAccelerationStructure::constructAccelerationStructure(RenderEngine& eng,
                                                                                                   const StaticMesh& mesh,
                                                                                                   const std::string& name)
{
    std::vector<vk::AccelerationStructureGeometryKHR> triangleInfo(mesh.getSubMeshCount());
    std::vector<uint32_t> primiviteCounts(mesh.getSubMeshCount());
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges(mesh.getSubMeshCount());
    const unsigned char* vertexData = mesh.getVertexData().data();
    const unsigned int* indexData = mesh.getIndexData().data();
    const uint32_t stride = mesh.getVertexStride();
    for(uint32_t subMesh_i = 0; subMesh_i < mesh.getSubMeshCount(); ++subMesh_i)
    {
        const SubMesh& subMesh = mesh.getSubMeshes()[subMesh_i];

        vk::AccelerationStructureGeometryTrianglesDataKHR triangleData{};
        triangleData.vertexFormat = vk::Format::eR32G32B32Sfloat;
        triangleData.vertexData.hostAddress = vertexData;
        triangleData.vertexStride = stride;
        triangleData.maxVertex = subMesh.mVertexCount;
        triangleData.indexType = vk::IndexType::eUint32;
        triangleData.indexData.hostAddress = indexData;
        triangleData.transformData.hostAddress = glm::value_ptr(subMesh.mTransform);

        vk::AccelerationStructureGeometryDataKHR geomData{};
        geomData.triangles = triangleData;
        vk::AccelerationStructureGeometryKHR geom{};
        geom.flags = vk::GeometryFlagBitsKHR::eOpaque; // TODO mark all as opaque for now.
        geom.geometryType = vk::GeometryTypeKHR::eTriangles;
        geom.geometry = geomData;

        vk::AccelerationStructureBuildRangeInfoKHR buildRange{};
        buildRange.primitiveCount = subMesh.mIndexCount / 3u;
        buildRange.primitiveOffset = subMesh.mIndexOffset * sizeof(uint32_t);
        buildRange.firstVertex = subMesh.mVertexOffset * stride;
        buildRange.transformOffset = 0;

        buildRanges[subMesh_i] = buildRange;
        triangleInfo[subMesh_i] = geom;
        primiviteCounts[subMesh_i] = subMesh.mIndexCount / 3u;
    }

    vk::AccelerationStructureBuildGeometryInfoKHR geomBuildInfo{};
    geomBuildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    geomBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geomBuildInfo.geometryCount = mesh.getSubMeshCount();
    geomBuildInfo.ppGeometries = nullptr;
    geomBuildInfo.pGeometries = triangleInfo.data();

    const VulkanRenderDevice* vkDevice =  static_cast<const VulkanRenderDevice*>(mEng.getDevice());

    vk::AccelerationStructureBuildSizesInfoKHR buildSize = vkDevice->getAccelerationStructureMemoryRequirements(vk::AccelerationStructureBuildTypeKHR::eHost,
            geomBuildInfo, primiviteCounts);

    VulkanBuffer backingBuffer(mEng.getDevice(), BufferUsage::AccelerationStructure, buildSize.accelerationStructureSize, buildSize.accelerationStructureSize, name);

    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.createFlags = {};
    createInfo.buffer = backingBuffer.getBuffer();
    createInfo.offset = 0;
    createInfo.size = backingBuffer.getSize();
    createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    // create acceleration structure.
    vk::AccelerationStructureKHR accelStructure = vkDevice->createAccelerationStructure(createInfo);

    geomBuildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    geomBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geomBuildInfo.srcAccelerationStructure = nullptr;
    geomBuildInfo.dstAccelerationStructure = accelStructure;
    geomBuildInfo.geometryCount = mesh.getSubMeshCount();
    geomBuildInfo.ppGeometries = nullptr;
    geomBuildInfo.pGeometries = triangleInfo.data();
    geomBuildInfo.scratchData = new unsigned char[buildSize.buildScratchSize];

    // Build geometry in to it.
    vk::AccelerationStructureBuildRangeInfoKHR* pRanges = buildRanges.data();
    vkDevice->buildAccelerationStructure(1, &geomBuildInfo, &pRanges);

    // free scratch data.
    delete[] geomBuildInfo.scratchData.hostAddress;

    return {accelStructure, backingBuffer};
}

