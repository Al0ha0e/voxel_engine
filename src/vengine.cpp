#include <vengine.hpp>

namespace vxe_common
{
    Engine *Engine::instance = nullptr;

    void Engine::Update()
    {
        terrain->Update();
        engine->Update();
    }
}