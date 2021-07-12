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
        triangleData.vertexFormat = vk::Format::eR32G32B32A32Sfloat;
        triangleData.vertexData.hostAddress = vertexData;
        triangleData.vertexStride = stride;
        triangleData.maxVertex = subMesh.mVertexCount;
        triangleData.indexType = vk::IndexType::eUint32;
        triangleData.indexData.hostAddress = indexData;
        triangleData.transformData.hostAddress = nullptr;//glm::value_ptr(subMesh.mTransform);

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

    Buffer backingBuffer(eng->getDevice(), BufferUsage::AccelerationStructure, buildSize.accelerationStructureSize, buildSize.accelerationStructureSize, name);

    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.createFlags = {};
    createInfo.buffer = static_cast<VulkanBuffer*>(backingBuffer.getBase())->getBuffer();
    createInfo.offset = 0;
    createInfo.size = backingBuffer->getSize();
    createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    // create acceleration structure.
    vk::AccelerationStructureKHR accelStructure = vkDevice->createAccelerationStructure(createInfo);

    geomBuildInfo.srcAccelerationStructure = nullptr;
    geomBuildInfo.dstAccelerationStructure = accelStructure;
    geomBuildInfo.scratchData.hostAddress = new unsigned char[buildSize.buildScratchSize];

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
    mInstances.clear();
    mBottomLevelStructures.clear();
    mBVH->mAccelerationHandle = nullptr;
}


void VulkanTopLevelAccelerationStructure::addInstance(const MeshInstance* instance)
{
    mInstances.push_back(instance);
    mBottomLevelStructures.insert({instance,
                                   static_cast<const VulkanBottomLevelAccelerationStructure *>(instance->getAccelerationStructure()->getBase())->getAccelerationStructureHandle()});
}

void VulkanTopLevelAccelerationStructure::buildStructureOnCPU(RenderEngine* eng)
{
    std::vector<vk::AccelerationStructureGeometryKHR> instanceInfos(mInstances.size());
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges(mInstances.size());
    std::vector<vk::AccelerationStructureInstanceKHR> instances(mInstances.size());
    uint32_t instance_i = 0;
    for(const MeshInstance* instance : mInstances)
    {
        vk::AccelerationStructureInstanceKHR &instanceInfo = instances[instance_i];
        instanceInfo.accelerationStructureReference = *reinterpret_cast<const uint64_t *>(&mBottomLevelStructures[instance]);
        instanceInfo.flags = {};
        instanceInfo.instanceCustomIndex = instance->getID();
        instanceInfo.instanceShaderBindingTableRecordOffset = 0; // TODO figure out how to handle this.
        instanceInfo.mask = 0xFF;
        vk::TransformMatrixKHR transform{};
        {
            float3x4 trans = glm::transpose(instance->getTransMatrix());
            memcpy(transform.matrix.data(), glm::value_ptr(trans), sizeof(float3x4));
        }
        instanceInfo.transform = transform;

        vk::AccelerationStructureGeometryInstancesDataKHR instanceData{};
        instanceData.arrayOfPointers = false;
        instanceData.data.hostAddress = &instanceInfo;

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

        ++instance_i;
    }

    vk::AccelerationStructureBuildGeometryInfoKHR geomBuildInfo{};
    geomBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geomBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geomBuildInfo.geometryCount = 1;
    geomBuildInfo.ppGeometries = nullptr;
    geomBuildInfo.pGeometries = instanceInfos.data();

    const VulkanRenderDevice* vkDevice =  static_cast<const VulkanRenderDevice*>(eng->getDevice());

    vk::AccelerationStructureBuildSizesInfoKHR buildSize = vkDevice->getAccelerationStructureMemoryRequirements(vk::AccelerationStructureBuildTypeKHR::eHost,
                                                                                                                geomBuildInfo,
                                                                                                                {static_cast<uint32_t>(mBottomLevelStructures.size())});

    Buffer backingBuffer(eng->getDevice(), BufferUsage::AccelerationStructure, buildSize.accelerationStructureSize, buildSize.accelerationStructureSize, "Top level acceleration structure");

    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.createFlags = {};
    createInfo.buffer = static_cast<VulkanBuffer*>(backingBuffer.getBase())->getBuffer();
    createInfo.offset = 0;
    createInfo.size = backingBuffer->getSize();
    createInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;

    // create acceleration structure.
    vk::AccelerationStructureKHR accelStructure = vkDevice->createAccelerationStructure(createInfo);

    geomBuildInfo.srcAccelerationStructure = nullptr;
    geomBuildInfo.dstAccelerationStructure = accelStructure;
    geomBuildInfo.geometryCount = 1;
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

