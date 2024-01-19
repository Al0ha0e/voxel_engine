#ifndef VXE_ENGINE_H
#define VXE_ENGINE_H

#include <engine.hpp>
#include <terrain/terrain.hpp>
#include <voxel/voxel_render.hpp>
#include <render/render_pass.hpp>

namespace vxe_common
{
    class Engine
    {
    private:
        static Engine *instance;
        Engine() {}
        ~Engine() {}
        Engine(const Engine &);
        Engine &operator=(const Engine);

    public:
        static Engine *GetInstance()
        {
            if (instance == nullptr)
                instance = new Engine();
            return instance;
        }

        vke_common::Engine *engine;
        vxe_terrain::Terrain *terrain;
        vxe_voxel::WorldVoxelRenderer *terrainRenderer;

        static Engine *Init(int width, int height, int levelNum)
        {
            instance = new Engine();
            std::unique_ptr<vxe_voxel::WorldVoxelRenderer> terrainRenderer = std::make_unique<vxe_voxel::WorldVoxelRenderer>();
            instance->terrainRenderer = terrainRenderer.get();
            std::vector<vke_render::PassType> passes = {
                // vke_render::BASE_RENDERER,
                vke_render::CUSTOM_RENDERER,
                vke_render::OPAQUE_RENDERER};
            std::vector<std::unique_ptr<vke_render::SubpassBase>> customPasses;
            customPasses.push_back(std::move(terrainRenderer));
            std::vector<vke_render::RenderPassInfo> customPassInfo{
                vke_render::RenderPassInfo(
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)};

            instance->engine = vke_common::Engine::Init(width, height, passes, customPasses, customPassInfo);

            instance->terrain = vxe_terrain::Terrain::Init(levelNum);
            instance->terrainRenderer->SetVoxel(instance->terrain->worldVoxel.get());
            return instance;
        }

        void Update();

        static void Dispose()
        {
            instance->terrain->Dispose();
            instance->engine->Dispose();
            delete instance;
        }
    };
}

#endif