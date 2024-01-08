#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <render/compute.hpp>
#include <render/buffer.hpp>
#include <render/resource.hpp>
#include <voxel/voxel.hpp>

namespace vxe_terrain
{
    class TerrainGenerator
    {
    public:
        vke_render::HostCoherentBuffer generationInfoBuffer;
        vke_render::StagedBuffer localBlockPos;

        TerrainGenerator() {}

        TerrainGenerator(vxe_voxel::WorldVoxel *voxel)
            : worldVoxel(voxel),
              generationInfoBuffer(sizeof(int) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
              localBlockPos(vxe_voxel::gridSize * sizeof(int) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            terrainGeneratorShader = vke_render::RenderResourceManager::LoadComputeShader("./shader/gen_terrain.spv");

            std::vector<vke_render::DescriptorInfo> descriptorInfos;
            VkDescriptorSetLayoutBinding binding{};
            // GridCenter
            vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->gridCenter.bufferSize));
            // GenerationInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, generationInfoBuffer.bufferSize));
            // LocalBlockPos
            vke_render::InitDescriptorSetLayoutBinding(binding, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, localBlockPos.bufferSize));
            // EdgePosition
            vke_render::InitDescriptorSetLayoutBinding(binding, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->edgePositionBuffer.bufferSize));
            // EdgeNormal
            vke_render::InitDescriptorSetLayoutBinding(binding, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->edgeNormalBuffer.bufferSize));
            // EdgeInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->edgeInfoBuffer.bufferSize));
            // VertInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->vertInfoBuffer.bufferSize));

            std::vector<VkBuffer> buffers = {
                voxel->gridCenter.buffer, generationInfoBuffer.buffer, localBlockPos.buffer,
                voxel->edgePositionBuffer.buffer, voxel->edgeNormalBuffer.buffer, voxel->edgeInfoBuffer.buffer,
                voxel->vertInfoBuffer.buffer};

            terrainGenerationTask = new vke_render::ComputeTask(terrainGeneratorShader, std::move(descriptorInfos));
            terrainGenerationTaskID = terrainGenerationTask->AddInstance(
                std::move(buffers),
                std::move(std::vector<VkSemaphore>{}),
                std::move(std::vector<VkPipelineStageFlags>{}),
                std::move(std::vector<VkSemaphore>{}));
        }

        void GenerateBatch(VkFence fence, glm::ivec3 dim3, VkCommandBuffer commandBuffer)
        {
            int isbatch = 1;
            generationInfoBuffer.ToBuffer(0, &isbatch, 4);
            terrainGenerationTask->Dispatch(terrainGenerationTaskID, commandBuffer, dim3, fence);
        }

        void Generate(
            VkFence fence,
            glm::ivec3 dim3,
            VkCommandBuffer commandBuffer,
            int level,
            std::vector<glm::ivec4> &tempPositions)
        {
            int info[2];
            info[0] = 0;
            info[1] = level;
            generationInfoBuffer.ToBuffer(0, info, 8);
            localBlockPos.ToBuffer(0, tempPositions.data(), tempPositions.size() * sizeof(glm::vec4));
            terrainGenerationTask->Dispatch(terrainGenerationTaskID, commandBuffer, dim3, fence);
        }

    private:
        vxe_voxel::WorldVoxel *worldVoxel;
        vke_render::ComputeShader *terrainGeneratorShader;
        vke_render::ComputeTask *terrainGenerationTask;
        uint64_t terrainGenerationTaskID;

        int GetEdgeIdx(int level, int dim, glm::uvec3 block, glm::uvec3 cell)
        {
            return (((level * 64 + ((block.z * 4 + block.y) * 4 + block.x)) * 3 + dim) << 12) + (((cell.z << 4) + cell.y) << 4) + cell.x;
        }
    };
}

#endif