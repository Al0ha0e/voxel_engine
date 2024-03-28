#ifndef VOXEL_H
#define VOXEL_H

#include <render/compute.hpp>
#include <render/buffer.hpp>
#include <resource.hpp>
#include <engine.hpp>
#include <iostream>

#define SHOW_BUFFERSIZE(buffer)                       \
    std::cout << #buffer << "\t" << buffer.bufferSize \
              << "B\t" << buffer.bufferSize / 1024.0 << "K\t" << buffer.bufferSize / (1024 * 1024.0) << "M\n";

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
        int totSize;
        int totSize3d;
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

        WorldVoxel(int levelNum)
            : levelNum(levelNum),
              totSize(blockSize * gridSize * levelNum),
              totSize3d(totSize * 3),
              surfaceGenerationInfoBuffer(sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              gridCenter(16 * sizeof(int) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              gridSubMin(16 * sizeof(int) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              edgePositionBuffer(totSize3d * sizeof(char), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              edgeNormalBuffer(totSize3d * sizeof(short), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              edgeInfoBuffer(totSize3d * sizeof(char), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              vertPositionBuffer(totSize * sizeof(char) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              vertNormalBuffer(totSize * sizeof(short), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              vertInfoBuffer(totSize * sizeof(short), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              contourBuffer(totSize3d * 3 * sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), // * 3 * 2 * 1/2
              levelCounter(sizeof(int64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            initVertexGenerator();
            initSurfaceGenerator();
            showBufferSize();
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
        std::shared_ptr<vke_render::ComputeShader> vertexGeneratiorShader;
        std::unique_ptr<vke_render::ComputeTask> vertexGenerationTask;
        uint64_t vertexGenerationTaskID;

        std::shared_ptr<vke_render::ComputeShader> surfaceGeneratiorShader;
        std::unique_ptr<vke_render::ComputeTask> surfaceGenerationTask;
        uint64_t surfaceGenerationTaskID;

        void showBufferSize()
        {
            std::cout << "---------------------\n";
            std::cout << "LEVELCNT: " << levelNum << " CHUNKCNT: " << gridSize * levelNum << " VOXELCNT: " << blockSize * gridSize * levelNum << "\n";
            SHOW_BUFFERSIZE(surfaceGenerationInfoBuffer)
            SHOW_BUFFERSIZE(gridCenter)
            SHOW_BUFFERSIZE(gridSubMin)
            SHOW_BUFFERSIZE(edgePositionBuffer)
            SHOW_BUFFERSIZE(edgeNormalBuffer)
            SHOW_BUFFERSIZE(edgeInfoBuffer)
            SHOW_BUFFERSIZE(vertPositionBuffer)
            SHOW_BUFFERSIZE(vertNormalBuffer)
            SHOW_BUFFERSIZE(vertInfoBuffer)
            SHOW_BUFFERSIZE(contourBuffer)
            SHOW_BUFFERSIZE(levelCounter)
            size_t sum = 0;
            sum += surfaceGenerationInfoBuffer.bufferSize + gridCenter.bufferSize + gridSubMin.bufferSize;
            sum += edgePositionBuffer.bufferSize + edgeNormalBuffer.bufferSize + edgeInfoBuffer.bufferSize;
            sum += vertPositionBuffer.bufferSize + vertNormalBuffer.bufferSize + vertInfoBuffer.bufferSize;
            sum += contourBuffer.bufferSize + levelCounter.bufferSize;
            std::cout << "TOT: " << sum << " B\t" << sum / 1024.0 << " K\t" << sum / (1024.0 * 1024.0) << " M"
                      << "\n";
            std::cout << "---------------------\n";
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

            vertexGenerationTask = std::make_unique<vke_render::ComputeTask>(vertexGeneratiorShader, std::move(descriptorInfos));
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

            surfaceGenerationTask = std::make_unique<vke_render::ComputeTask>(surfaceGeneratiorShader, std::move(descriptorInfos));
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