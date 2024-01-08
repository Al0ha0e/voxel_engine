#ifndef TERRAIN_EDITOR_H
#define TERRAIN_EDITOR_H

#include <voxel/voxel.hpp>

namespace vxe_terrain
{
    struct EditInfo
    {
        glm::vec4 info0;
        glm::uvec4 info1;
    };

    class TerrainEditor
    {
    public:
        vke_render::HostCoherentBuffer editInfoBuffer;
        enum EditorType
        {
            EDIT_DEC,
            EDIT_INC
        };

        TerrainEditor() {}
        TerrainEditor(vxe_voxel::WorldVoxel *voxel)
            : worldVoxel(voxel),
              editInfoBuffer(sizeof(EditInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            initEditor("./shader/edit1.spv");
            initEditor("./shader/edit2.spv");

            updateInfoShader = vke_render::RenderResourceManager::LoadComputeShader("./shader/update_info.spv");
            std::vector<vke_render::DescriptorInfo> descriptorInfos;
            VkDescriptorSetLayoutBinding binding{};
            // GridCenter
            vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->gridCenter.bufferSize));
            // EdgeInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->edgeInfoBuffer.bufferSize));
            // VertInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, voxel->vertInfoBuffer.bufferSize));

            std::vector<VkBuffer> buffers = {
                voxel->gridCenter.buffer,
                voxel->edgeInfoBuffer.buffer,
                voxel->vertInfoBuffer.buffer};

            updateInfoTask = new vke_render::ComputeTask(updateInfoShader, std::move(descriptorInfos));
            updateInfoTaskID = updateInfoTask->AddInstance(
                std::move(buffers),
                std::move(std::vector<VkSemaphore>{}),
                std::move(std::vector<VkPipelineStageFlags>{}),
                std::move(std::vector<VkSemaphore>{}));
        }

        void EditTerrain(VkFence fence, glm::ivec3 dim3, VkCommandBuffer commandBuffer,
                         EditorType type, EditInfo editInfo)
        {
            editInfoBuffer.ToBuffer(0, &editInfo, sizeof(EditInfo));
            terrainEditTasks[type]->Dispatch(terrainEditTaskIDs[type], commandBuffer, dim3, fence);
            updateInfoTask->Dispatch(updateInfoTaskID, commandBuffer, dim3, fence);
        }

    private:
        vxe_voxel::WorldVoxel *worldVoxel;
        std::vector<vke_render::ComputeShader *> terrainEditorShaders;
        std::vector<vke_render::ComputeTask *> terrainEditTasks;
        std::vector<uint64_t> terrainEditTaskIDs;

        vke_render::ComputeShader *updateInfoShader;
        vke_render::ComputeTask *updateInfoTask;
        uint64_t updateInfoTaskID;

        void initEditor(std::string pth)
        {
            std::vector<vke_render::DescriptorInfo> descriptorInfos;
            VkDescriptorSetLayoutBinding binding{};
            // GridCenter
            vke_render::InitDescriptorSetLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, worldVoxel->gridCenter.bufferSize));
            // GenerationInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, editInfoBuffer.bufferSize));
            // EdgePosition
            vke_render::InitDescriptorSetLayoutBinding(binding, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, worldVoxel->edgePositionBuffer.bufferSize));
            // EdgeNormal
            vke_render::InitDescriptorSetLayoutBinding(binding, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, worldVoxel->edgeNormalBuffer.bufferSize));
            // EdgeInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, worldVoxel->edgeInfoBuffer.bufferSize));
            // VertInfo
            vke_render::InitDescriptorSetLayoutBinding(binding, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
            descriptorInfos.push_back(vke_render::DescriptorInfo(binding, worldVoxel->vertInfoBuffer.bufferSize));

            std::vector<VkBuffer> buffers = {
                worldVoxel->gridCenter.buffer, editInfoBuffer.buffer,
                worldVoxel->edgePositionBuffer.buffer, worldVoxel->edgeNormalBuffer.buffer, worldVoxel->edgeInfoBuffer.buffer,
                worldVoxel->vertInfoBuffer.buffer};

            vke_render::ComputeShader *editShader = vke_render::RenderResourceManager::LoadComputeShader(pth);
            vke_render::ComputeTask *editTask = new vke_render::ComputeTask(editShader, std::move(descriptorInfos));
            uint64_t editTaskID = editTask->AddInstance(
                std::move(buffers),
                std::move(std::vector<VkSemaphore>{}),
                std::move(std::vector<VkPipelineStageFlags>{}),
                std::move(std::vector<VkSemaphore>{}));

            terrainEditorShaders.push_back(editShader);
            terrainEditTasks.push_back(editTask);
            terrainEditTaskIDs.push_back(editTaskID);
        }
    };
}

#endif