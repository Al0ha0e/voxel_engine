#include <engine.hpp>
#include <terrain/generator.hpp>

int main()
{
    std::vector<vke_render::PassType> passes = {
        vke_render::BASE_RENDERER,
        vke_render::OPAQUE_RENDERER};
    std::vector<std::unique_ptr<vke_render::SubpassBase>> customPasses;
    std::vector<vke_render::RenderPassInfo> customPassInfo;
    vke_common::Engine *engine = vke_common::Engine::Init(800, 600, passes, customPasses, customPassInfo);
    VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;

    int levelNum = 3;

    vxe_voxel::WorldVoxel *voxel = new vxe_voxel::WorldVoxel(levelNum);
    vxe_terrain::TerrainGenerator generator(voxel);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vke_render::RenderEnvironment::GetInstance()->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    VkFence fence;
    vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);
    // generator.Generate(fence, glm::ivec3(vxe_voxel::gridStorageSize * levelNum, vxe_voxel::gridStorageSize, vxe_voxel::gridStorageSize << 2), commandBuffer);
    generator.GenerateBatch(fence, glm::ivec3(vxe_voxel::gridStorageSize * levelNum, vxe_voxel::gridStorageSize, vxe_voxel::gridStorageSize << 2), commandBuffer);

    return 0;
}