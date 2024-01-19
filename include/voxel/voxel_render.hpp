#ifndef VOXEL_RENDER_H
#define VOXEL_RENDER_H

#include <render/environment.hpp>
#include <render/shader.hpp>
#include <render/subpass.hpp>
#include <voxel/voxel.hpp>

namespace vxe_voxel
{
    class WorldVoxelRenderer : public vke_render::SubpassBase
    {
    public:
        WorldVoxelRenderer() : descriptorSetInfo(nullptr, 0, 0, 0)
        {
            indirectCommand.firstInstance = 0;
            indirectCommand.firstVertex = 0;
            indirectCommand.instanceCount = 1;
        }

        ~WorldVoxelRenderer()
        {
            VkDevice logicalDevice = environment->logicalDevice;
            vkDestroyPipeline(logicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        }

        void Init(int subpassID, VkRenderPass renderPass) override
        {
            environment = vke_render::RenderEnvironment::GetInstance();
            SubpassBase::Init(subpassID, renderPass);
            indirectCommandBuffer = std::make_unique<vke_render::StagedBuffer>(sizeof(VkDrawIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

            shader = vke_render::RenderResourceManager::LoadVertFragShader("./shader/terrain_vert.spv", "./shader/terrain_frag.spv");
        }

        void SetVoxel(WorldVoxel *voxel)
        {
            worldVoxel = voxel;
            createDescriptorSet();
            createGraphicsPipeline();
        }

        void RegisterCamera(VkBuffer buffer) override
        {
            vke_render::DescriptorInfo &info = descriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = info.bufferSize;
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(descriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(environment->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
        void Render(VkCommandBuffer commandBuffer) override
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(environment->swapChainExtent.width);
            viewport.height = static_cast<float>(environment->swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = environment->swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0, 1, &descriptorSet,
                0, nullptr);
            indirectCommand.vertexCount = worldVoxel->vertexNum;
            indirectCommandBuffer->ToBuffer(0, &indirectCommand, sizeof(VkDrawIndirectCommand));
            vkCmdDrawIndirect(commandBuffer, indirectCommandBuffer->buffer, 0, 1, sizeof(VkDrawIndirectCommand));
        }

    private:
        vke_render::RenderEnvironment *environment;
        WorldVoxel *worldVoxel;
        std::shared_ptr<vke_render::VertFragShader> shader;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        std::vector<vke_render::DescriptorInfo> descriptorInfos;
        vke_render::DescriptorSetInfo descriptorSetInfo;
        VkDescriptorSet descriptorSet;
        std::unique_ptr<vke_render::StagedBuffer> indirectCommandBuffer;
        VkDrawIndirectCommand indirectCommand;

        void createDescriptorSet();
        void createGraphicsPipeline();
    };
}

#endif