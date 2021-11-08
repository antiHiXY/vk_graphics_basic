#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.h"


layout (location = 0) in vec4 vPosNorm;
layout (location = 1) in vec4 vTexCoordAndTang;

layout (push_constant) uniform params_t
{
    mat4 mProjView;
} params;


layout (location = 0) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
    vec4 color;
} vOut;

layout (std430, binding = 1) buffer matricies 
{
    mat4 mat[];
};

layout (std430, binding = 2) buffer instanceMapping
{
    uint instMap[];
};

out gl_PerVertex { vec4 gl_Position; };
void main(void)
{
    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    const mat4 model = mat[instMap[gl_InstanceIndex]];

    vOut.wPos     = (model * vec4(vPosNorm.xyz, 1.0f)).xyz;
    vOut.wNorm    = mat3(transpose(inverse(model))) * wNorm.xyz;
    vOut.wTangent = mat3(transpose(inverse(model))) * wTang.xyz;
    vOut.texCoord = vTexCoordAndTang.xy;

    vOut.color    = vec4(exp(vOut.wPos) * exp(1.f / vOut.wNorm) * 5.f, 1.f);

    gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
}
