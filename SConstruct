env = Environment(CC = 'cl',
                   CCFLAGS = ['/std:c++17','/EHsc'])
SConscript(['vkEngine/SConstruct'])

env.Program("out/test_env",
            ["./tests/test_env.cpp"],
        LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1', 'glfw3','vkengine'], 
        LIBPATH=['./vkEngine/out','./vkEngine/libs','D:/VulkanSDK/Lib'], 
        CPPPATH=['./include','./vkEngine/include','D:/VulkanSDK/Include'],
        SCONS_CXX_STANDARD="c++17")

env.Program("out/test_genterrain",
            ["./tests/test_genterrain.cpp","./src/terrain/terrain.cpp","./src/voxel/voxel_render.cpp","./src/vengine.cpp"],
        LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1', 'glfw3','vkengine'], 
        LIBPATH=['./vkEngine/out','./vkEngine/libs','D:/VulkanSDK/Lib'], 
        CPPPATH=['./include','./vkEngine/include','D:/VulkanSDK/Include'],
        SCONS_CXX_STANDARD="c++17")

env.Program("out/test_voxel",
            ["./tests/test_voxel.cpp","./src/terrain/terrain.cpp","./src/voxel/voxel_render.cpp","./src/vengine.cpp"],
        LIBS=['msvcrtd', 'libcmt', 'Gdi32', 'shell32', 'user32','vulkan-1', 'glfw3','vkengine'], 
        LIBPATH=['./vkEngine/out','./vkEngine/libs','D:/VulkanSDK/Lib'], 
        CPPPATH=['./include','./vkEngine/include','D:/VulkanSDK/Include'],
        SCONS_CXX_STANDARD="c++17")