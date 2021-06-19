#include "VulkanAccelerationStructures.hpp"
#include "Engine/Engine.hpp"
#include "Engine/StaticMesh.h"

#include "Core/Vulkan/VulkanRenderDevice.hpp"

VulkanBottomLevelAccelerationStructure::VulkanBottomLevelAccelerationStructure(RenderEngine* engine, const StaticMesh& mesh, const std::string& name) :
    BottomLevelAccelerationStructureBase(engine, mesh, name),
    mBVH(constructAccelerationStructure(engine, mesh, name))
{
}


VulkanAccelerationStructure VulkanBottomLevelAccelerationStructure::constructAccelerationStructure(RenderEngine* eng,
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
        geom.flags = vk::GeometryFlagBitsKHR::eOpaque; // TODO mark all as opaque for now. add alpha testing later.
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

    const VulkanRenderDevice* vkDevice =  static_cast<const VulkanRenderDevice*>(eng->getDevice());

    vk::AccelerationStructureBuildSizesInfoKHR buildSize = vkDevice->getAccelerationStructureMemoryRequirements(vk::AccelerationStructureBuildTypeKHR::eHost,
            geomBuildInfo, primiviteCounts);

    VulkanBuffer backingBuffer(eng->getDevice(), BufferUsage::AccelerationStructure, buildSize.accelerationStructureSize, buildSize.accelerationStructureSize, name);

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

VulkanBottomLevelAccelerationStructure::~VulkanBottomLevelAccelerationStructure()
{
    getDevice()->destroyBottomLevelAccelerationStructure(*this);
}


VulkanTopLevelAccelerationStructure::VulkanTopLevelAccelerationStructure(RenderEngine* engine) :
        TopLevelAccelerationStructureBase(engine)
{

}


VulkanTopLevelAccelerationStructure::~VulkanTopLevelAccelerationStructure()
{
    getDevice()->destroyTopLevelAccelerationStructure(*this);
}


void VulkanTopLevelAccelerationStructure::reset()
{
    getDevice()->destroyTopLevelAccelerationStructure(*this);
    mBottomLevelStructures.clear();
    mBVH->mAccelerationHandle = nullptr;
}


void VulkanTopLevelAccelerationStructure::addBottomLevelStructure(const BottomLevelAccelerationStructure& accelStruct)
{
    mBottomLevelStructures.push_back(accelStruct);
}

void VulkanTopLevelAccelerationStructure::buildStructureOnCPU(RenderEngine* eng)
{
    std::vector<vk::AccelerationStructureGeometryKHR> instanceInfos(mBottomLevelStructures.size());
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges(mBottomLevelStructures.size());
    std::vector<vk::AccelerationStructureKHR> bottomLevelStructures(mBottomLevelStructures.size());
    std::vector<uint32_t> instanceCount(mBottomLevelStructures.size());
    for(uint32_t instance_i = 0; instance_i < mBottomLevelStructures.size(); ++instance_i)
    {
        vk::AccelerationStructureGeometryInstancesDataKHR instanceData{};
        instanceData.arrayOfPointers = false;
        instanceData.data.hostAddress = bottomLevelStructures.data() + instance_i;
        bottomLevelStructures[instance_i] = static_cast<VulkanBottomLevelAccelerationStructure*>(mBottomLevelStructures[instance_i].getBase())->getAccelerationStructureHandle();
        instanceCount[instance_i] = 1;

        vk::AccelerationStructureGeometryDataKHR geomData{};
        geomData.instances = instanceData;
        vk::AccelerationStructureGeometryKHR geom{};
        geom.geometryType = vk::GeometryTypeKHR::eInstances;
        geom.geometry = geomData;

        vk::AccelerationStructureBuildRangeInfoKHR buildRange{};
        buildRange.primitiveCount = 1;
        buildRange.transformOffset = 0;

        buildRanges[instance_i] = buildRange;
        instanceInfos[instance_i] = geom;
    }

    vk::AccelerationStructureBuildGeometryInfoKHR geomBuildInfo{};
    geomBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geomBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geomBuildInfo.geometryCount = mBottomLevelStructures.size();
    geomBuildInfo.ppGeometries = nullptr;
    geomBuildInfo.pGeometries = instanceInfos.data();

    const VulkanRenderDevice* vkDevice =  static_cast<const VulkanRenderDevice*>(eng->getDevice());

    vk::AccelerationStructureBuildSizesInfoKHR buildSize = vkDevice->getAccelerationStructureMemoryRequirements(vk::AccelerationStructureBuildTypeKHR::eHost,
                                                                                                                geomBuildInfo, instanceCount);

    VulkanBuffer backingBuffer(eng->getDevice(), BufferUsage::AccelerationStructure, buildSize.accelerationStructureSize, buildSize.accelerationStructureSize, "");

    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.createFlags = {};
    createInfo.buffer = backingBuffer.getBuffer();
    createInfo.offset = 0;
    createInfo.size = backingBuffer.getSize();
    createInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;

    // create acceleration structure.
    vk::AccelerationStructureKHR accelStructure = vkDevice->createAccelerationStructure(createInfo);

    geomBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geomBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geomBuildInfo.srcAccelerationStructure = nullptr;
    geomBuildInfo.dstAccelerationStructure = accelStructure;
    geomBuildInfo.geometryCount = mBottomLevelStructures.size();
    geomBuildInfo.ppGeometries = nullptr;
    geomBuildInfo.pGeometries = instanceInfos.data();
    geomBuildInfo.scratchData = new unsigned char[buildSize.buildScratchSize];

    // Build geometry in to it.
    vk::AccelerationStructureBuildRangeInfoKHR* pRanges = buildRanges.data();
    vkDevice->buildAccelerationStructure(1, &geomBuildInfo, &pRanges);

    // free scratch data.
    delete[] geomBuildInfo.scratchData.hostAddress;

    mBVH = {accelStructure, backingBuffer};
}


void VulkanTopLevelAccelerationStructure::buildStructureOnGPU(Executor*)
{
    // TODO
    BELL_TRAP;
}

