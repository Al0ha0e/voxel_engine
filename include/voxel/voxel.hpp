#ifndef VOXEL_H
#define VOXEL_H

#include <render/compute.hpp>
#include <render/buffer.hpp>
#include <render/resource.hpp>
#include <engine.hpp>

namespace vxe_voxel
{

    const int chunkSize = 16;
    const int gridStorageSize = 4;
    const int gridRenderSize = 3;
    const int blockSize = chunkSize * chunkSize * chunkSize;
    const int gridSize = gridStorageSize * gridStorageSize * gridStorageSize;

    class WorldVoxel
    {
    public:
        int levelNum;
        int vertexNum;

        vke_render::HostCoherentBuffer surfaceGenerationInfoBuffer;
        vke_render::HostCoherentBuffer gridCenter;
        vke_render::HostCoherentBuffer gridSubMin;
        vke_render::StagedBuffer edgePositionBuffer;
        vke_render::StagedBuffer edgeNormalBuffer;
        vke_render::StagedBuffer edgeInfoBuffer;
        vke_render::StagedBuffer vertPositionBuffer;
        vke_render::StagedBuffer vertNormalBuffer;
        vke_render::StagedBuffer vertInfoBuffer;
        vke_render::StagedBuffer contourBuffer;
        vke_render::StagedBuffer levelCounter;

        WorldVoxel() {}
        WorldVoxel(int levelNum) : levelNum(levelNum)
        {
            initBuffers();
            initVertexGenerator();
            initSurfaceGenerator();
        }

        void GenerateContour(VkFence fence, VkCommandBuffer commandBuffer)
        {
            vertexNum = 0;
            levelCounter.ToBuffer(0, &vertexNum, 4);
            generateVertices(fence, commandBuffer);
            generateSurface(fence, commandBuffer);
            levelCounter.FromBuffer(0, &vertexNum, 4);
        }

    private:
        vke_render::ComputeShader *vertexGeneratiorShader;
        vke_render::ComputeTask *vertexGenerationTask;
        uint64_t vertexGenerationTaskID;

        vke_render::ComputeShader *surfaceGeneratiorShader;
        vke_render::ComputeTask *surfaceGenerationTask;
        uint64_t surfaceGenerationTaskID;

        void initBuffers()
        {
            int totSize = blockSize * gridSize * levelNum;
            int totSize3d = totSize * 3;
            surfaceGenerationInfoBuffer = vke_render::HostCoherentBuffer(sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            gridCenter = vke_render::HostCoherentBuffer(16 * sizeof(int) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            gridSubMin = vke_render::HostCoherentBuffer(16 * sizeof(int) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            edgePositionBuffer = vke_render::StagedBuffer(totSize3d * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            edgeNormalBuffer = vke_render::StagedBuffer(totSize3d * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            edgeInfoBuffer = vke_render::StagedBuffer(totSize3d * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            vertPositionBuffer = vke_render::StagedBuffer(totSize * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            vertNormalBuffer = vke_render::StagedBuffer(totSize * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            vertInfoBuffer = vke_render::StagedBuffer(totSize * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            contourBuffer = vke_render::StagedBuffer(totSize3d * 3 * 3 * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            levelCounter = vke_render::StagedBuffer(sizeof(int64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        void initVertexGenerator()
        {
            vertexGeneratiorShader = vke_render::RenderResourceManager::LoadComputeShader("./shader/gen_vertex.spv");

            std::vector<vke_render::DescriptorInfo> descriptorInfos;
            VkDescriptorSetLayoutBinding binding{};
            // GridCenter
            vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, gridCenter.bufferSize));
            // EdgePosition
            vke_render::InitDescriptorSetLayoutBinding(binding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, edgePositionBuffer.bufferSize));
            // EdgeNormal
            vke_render::InitDescriptorSetLayoutBinding(binding, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, edgeNormalBuffer.bufferSize));
            // EdgeInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, edgeInfoBuffer.bufferSize));
            // VertPosition
            vke_render::InitDescriptorSetLayoutBinding(binding, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, vertPositionBuffer.bufferSize));
            // VertNormal
            vke_render::InitDescriptorSetLayoutBinding(binding, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, vertNormalBuffer.bufferSize));
            // VertInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, vertInfoBuffer.bufferSize));

            std::vector<VkBuffer> buffers = {
                gridCenter.buffer,
                edgePositionBuffer.buffer, edgeNormalBuffer.buffer, edgeInfoBuffer.buffer,
                vertPositionBuffer.buffer, vertNormalBuffer.buffer, vertInfoBuffer.buffer};

            vertexGenerationTask = new vke_render::ComputeTask(vertexGeneratiorShader, std::move(descriptorInfos));
            vertexGenerationTaskID = vertexGenerationTask->AddInstance(
                std::move(buffers),
                std::move(std::vector<VkSemaphore>{}),
                std::move(std::vector<VkPipelineStageFlags>{}),
                std::move(std::vector<VkSemaphore>{}));
        }

        void initSurfaceGenerator()
        {
            surfaceGeneratiorShader = vke_render::RenderResourceManager::LoadComputeShader("./shader/gen_surface.spv");
            std::vector<vke_render::DescriptorInfo> descriptorInfos;
            VkDescriptorSetLayoutBinding binding{};
            // GridCenter
            vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, gridCenter.bufferSize));
            // GenerationInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, surfaceGenerationInfoBuffer.bufferSize));
            // EdgeInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, edgeInfoBuffer.bufferSize));
            // GridSubMin
            vke_render::InitDescriptorSetLayoutBinding(binding, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, gridSubMin.bufferSize));
            // LevelCounter
            vke_render::InitDescriptorSetLayoutBinding(binding, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, levelCounter.bufferSize));
            // Contour
            vke_render::InitDescriptorSetLayoutBinding(binding, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, contourBuffer.bufferSize));

            surfaceGenerationInfoBuffer.ToBuffer(0, &levelNum, 4);

            std::vector<VkBuffer> buffers = {
                gridCenter.buffer, surfaceGenerationInfoBuffer.buffer, edgeInfoBuffer.buffer,
                gridSubMin.buffer, levelCounter.buffer, contourBuffer.buffer};

            surfaceGenerationTask = new vke_render::ComputeTask(surfaceGeneratiorShader, std::move(descriptorInfos));
            surfaceGenerationTaskID = surfaceGenerationTask->AddInstance(
                std::move(buffers),
                std::move(std::vector<VkSemaphore>{}),
                std::move(std::vector<VkPipelineStageFlags>{}),
                std::move(std::vector<VkSemaphore>{}));
        }

        void generateVertices(VkFence fence, VkCommandBuffer commandBuffer)
        {
            glm::ivec3 dim3(gridStorageSize * levelNum, gridStorageSize, gridStorageSize << 2);
            vertexGenerationTask->Dispatch(vertexGenerationTaskID, commandBuffer, dim3, fence);
        }

        void generateSurface(VkFence fence, VkCommandBuffer commandBuffer)
        {
            glm::ivec3 dim3(gridStorageSize * levelNum, gridStorageSize, gridStorageSize << 2);
            surfaceGenerationTask->Dispatch(surfaceGenerationTaskID, commandBuffer, dim3, fence);
        }
    };
}

#endif