#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (binding = 1) uniform sampler2D shadowMap;

vec3 T(float s) 
{
  return vec3(0.233, 0.455, 0.649) * exp(-s*s/0.0064) +
         vec3(0.1, 0.336, 0.344) * exp(-s*s/0.0484) +
         vec3(0.118, 0.198, 0.0) * exp(-s*s/0.187) +
         vec3(0.113, 0.007, 0.007) * exp(-s*s/0.567) +
         vec3(0.358, 0.004, 0.0) * exp(-s*s/1.99) +
         vec3(0.078, 0.0, 0.0) * exp(-s*s/7.41);
}

float dist (vec3 pos, vec3 norm)
{
  float d1 = textureLod(shadowMap, vec2(pos.x/pos.z, pos.y/pos.z), 0).x;
  float d2 = pos.z;
  return abs(d1 - d2);
}

void main()
{
  const vec4 posLightClipSpace = Params.lightMatrix*vec4(surf.wPos, 1.0f); // 
  const vec3 posLightSpaceNDC  = posLightClipSpace.xyz/posLightClipSpace.w;    // for orto matrix, we don't need perspective division, you can remove it if you want; this is general case;
  const vec2 shadowTexCoord    = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);  // just shift coords from [-1,1] to [0,1]     
    
  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  const float shadow    = ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;

  const vec4 red = vec4(0.9f, 0.1f, 0.1f, 1.0f);
  const vec4 green  = vec4(0.1f, 0.9f, 0.1f, 1.0f);
   
  vec3 lightDir   = normalize(Params.lightPos - surf.wPos);
  vec4 lightColor = max(dot(surf.wNorm, lightDir), 0.0f) * green;

  float dstnc = length(Params.lightPos - surf.wPos);
  float attenuation = 1.0f / (dstnc * dstnc);
  float scale = 15.f;
  float s = scale * dist(surf.wPos, surf.wNorm);
  float E = max(0.3f + dot(-surf.wNorm, lightDir), 0.0f);
  vec3 transmittance = T(s) * lightColor.rgb * attenuation * red.rgb * E;
  out_fragColor   = (lightColor*shadow + vec4(0.1f)) * red;
  out_fragColor += vec4(transmittance, 1.0f); 
}
