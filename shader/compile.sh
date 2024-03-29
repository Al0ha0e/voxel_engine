computeShaders=(edit1 edit2 gen_surface gen_terrain gen_vertex update_info)

for shader in ${computeShaders[@]}
    do
    echo $shader
    glslc $shader.comp -I . -o $shader.spv
    done

echo terrain_vert
glslc terrain.vert -I . -o terrain_vert.spv

echo terrain_frag
glslc terrain.frag -o terrain_frag.spv