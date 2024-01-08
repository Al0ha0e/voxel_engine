#version 450
#extension GL_ARB_separate_shader_objects: enable
#extension GL_GOOGLE_include_directive : enable
#include "terrain_util.h"

// layout(location = 0) in vec3 fragColor;
layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;


void main(){
    vec3 _LightColor0 = vec3(0.8,0.8,0.8);
    vec3 _Diffuse = vec3(0.9,0.9,0.9);
    vec3 ambient = inColor;
    vec3 worldNormal = normalize(normal);
    vec3 worldLightDir = normalize(vec3(0.2,0.3,0.4));
    vec3 diffuse = _LightColor0.rgb * _Diffuse.rgb * max(dot(worldNormal,worldLightDir),0);//_Diffuse.rgb * (dot(worldNormal, worldLightDir) * 0.5 + 0.5);
    vec3 color = ambient + diffuse;
    //return fixed4(abs(worldNormal) * 0.5f + ambient,1.0f);
    outColor = vec4(color, 1.0f);
}