#ifndef TERRAIN_H
#define TERRAIN_H

#include <terrain/generator.hpp>
#include <terrain/editor.hpp>
#include <gameobject.hpp>

namespace vxe_terrain
{

    class Terrain
    {
    private:
        static Terrain *instance;
        Terrain() {}
        ~Terrain() {}
        Terrain(const Terrain &);
        Terrain &operator=(const Terrain);

    public:
        static Terrain *GetInstance()
        {
            if (instance == nullptr)
                instance = new Terrain();
            return instance;
        }

        vke_common::GameObject *observer;
        std::unique_ptr<vxe_voxel::WorldVoxel> worldVoxel;
        std::unique_ptr<TerrainGenerator> generator;
        std::unique_ptr<TerrainEditor> editor;

        static Terrain *Init(int levelNum)
        {
            instance = new Terrain();
            instance->worldVoxel = std::make_unique<vxe_voxel::WorldVoxel>(levelNum);
            instance->generator = std::make_unique<TerrainGenerator>(instance->worldVoxel.get());
            instance->editor = std::make_unique<TerrainEditor>(instance->worldVoxel.get());

            memset(instance->centers, 0, sizeof(glm::ivec4) * 16);
            memset(instance->prevCenters, 0, sizeof(glm::ivec4) * 16);

            vxe_voxel::WorldVoxel &worldVoxel = *(instance->worldVoxel);
            worldVoxel.gridCenter.ToBuffer(0, instance->centers, worldVoxel.gridCenter.bufferSize);
            for (int i = 1; i < levelNum; i++)
                instance->centers[i] = glm::vec4(8, 8, 8, 0);
            worldVoxel.gridSubMin.ToBuffer(0, instance->centers, worldVoxel.gridSubMin.bufferSize);

            VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = vke_render::RenderEnvironment::GetInstance()->commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &(instance->commandBuffer)) != VK_SUCCESS)
                throw std::runtime_error("failed to allocate command buffers!");

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = 0;
            vkCreateFence(logicalDevice, &fenceInfo, nullptr, &(instance->fence));

            glm::ivec3 dim3(vxe_voxel::gridStorageSize * levelNum, vxe_voxel::gridStorageSize, vxe_voxel::gridStorageSize << 2);
            instance->generator->GenerateBatch(instance->fence, dim3, instance->commandBuffer);

            worldVoxel.GenerateContour(instance->fence, instance->commandBuffer);
            instance->needDraw = false;
            return instance;
        }

        static void Dispose()
        {
            VkDevice logicalDevice = vke_render::RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyFence(logicalDevice, instance->fence, nullptr);
            delete instance;
        }

        static void SetObserver(vke_common::GameObject *observer)
        {
            instance->observer = observer;
        }

        static void EditTerrain(TerrainEditor::EditorType type, EditInfo editInfo)
        {
            glm::ivec3 dim3(
                vxe_voxel::gridStorageSize * instance->worldVoxel->levelNum,
                vxe_voxel::gridStorageSize,
                vxe_voxel::gridStorageSize << 2);
            instance->editor->EditTerrain(instance->fence, dim3, instance->commandBuffer, type, editInfo);
            instance->needDraw = true;
        }

        void Update();

    private:
        VkFence fence;
        VkCommandBuffer commandBuffer;
        glm::ivec4 centers[16];
        glm::ivec4 prevCenters[16];
        bool needDraw;
    };
};

#endif