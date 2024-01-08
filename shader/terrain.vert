#version 450
#extension GL_ARB_separate_shader_objects: enable
#extension GL_GOOGLE_include_directive : enable
#include "terrain_util.h"

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColor;

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;

layout(std430, set = 0, binding = 1) buffer GridInfoUBO {
    ivec3 GridCenter[16];
} ;

layout (std430, set = 0, binding = 2) buffer VertPositionSSBO {
    vec3 VertPosition[];
} ;

layout (std430, set = 0, binding = 3) buffer VertNormalSSBO {
    vec3 VertNormal[];
} ;

layout (std430, set = 0, binding = 4) buffer VertInfoSSBO {
    uint VertInfo[];
} ;

layout (std430, set = 0, binding = 5) buffer ContourSSBO {
    uint Contour[];
} ;

void main()
{
    vec3 position = vec3(0);
    ivec3 global_cell_pos;
    vec3 normal = vec3(0);
    uint ctor = Contour[gl_VertexIndex];
    uint idx = ctor & (~(7 << 29));
    uint level;
    // if(gl_VertexIndex % 3 == 0){
    //     level = idx / (gridStorageSize3 * 3 * blockSize3);
    //     uint info = EdgeInfo[idx];
    //     uint dim = (info >> 1) & 3;
    //     global_cell_pos = GetPosFromEIdx(idx, level, dim);
    //     float offset = EdgePosition[idx];
    //     if(dim == 0){
    //         position.x += offset;
    //     }else if(dim == 1){
    //         position.y += offset;
    //     }else{
    //         position.z += offset;
    //     }
    //     outNormal = EdgeNormal[idx];
    //     outColor = vec3(1.0,0.0,0.0);
    //     //outNormal = vec3(0.0f,0.0f,0.0f) + vec3(0.01 * level, 0.01 * level, 0.01 * level);
    // }else{
        level = idx / (gridStorageSize3 * blockSize3);
        uvec3 offset = GridCenter[level] & 3;
        global_cell_pos = ivec3(GetPosFromVIdx(idx, level, offset));
        position = VertPosition[idx];
        outNormal = VertNormal[idx];
        //outNormal = vec3(0.0f,0.0f,0.0f) + vec3(0.01 * level, 0.01 * level, 0.01 * level);
    // }
    position *= float(1 << level);
    ivec3 world_cell_pos = GetCellWorldPosition(global_cell_pos, GridCenter[level], level);
    // outNormal += vec3(0.2 * level, 0.2 * level, 0.2 * level);
    gl_Position = VPBlock.projection * VPBlock.view * vec4(world_cell_pos * 1.0f + position, 1.0f);

    uint idx0 = Contour[gl_VertexIndex - gl_VertexIndex % 3] & (~(7 << 29));
    uint info;
    ivec3 dirs[3] = {ivec3(1,0,0),ivec3(0,1,0),ivec3(0,0,1)};
    if((ctor & (1 << 31)) > 0){
        ivec3 global_cell_pos0 = ivec3(GetPosFromVIdx(idx0, level, offset));
        info = VertInfo[GetVertIdx(level, global_cell_pos0 + dirs[(ctor >> 29) & 3], offset)];
    }else{
        info = VertInfo[idx0];
    }

    vec3 colors[3] = {vec3(0.5,0.0,0),vec3(0,0.5,0),vec3(0,0,0.5)};
    outColor = colors[info & 0x3ff];
    // outColor = colors[(ctor >> 29) & 3];
    outColor += vec3(0.1 * level , 0.0f ,0.2f);
    // o.uv = TRANSFORM_TEX(v.uv, _MainTex);
    // UNITY_TRANSFER_FOG(o,o.vertex);
}