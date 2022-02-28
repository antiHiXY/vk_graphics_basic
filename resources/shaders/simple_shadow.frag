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

void main()
{
  float intensity      =  0.0f;
  float cutOff         = cos(radians(4.5f));
  float outerCutOff    = cos(radians(8.5f));
  float lightConstant  = 1.0f;
  float lightLinear    = 0.09f;
  float lightQuadratic = 0.032f;
  
  const vec4 posLightClipSpace = Params.lightMatrix*vec4(surf.wPos, 1.0f); // 
  const vec3 posLightSpaceNDC  = posLightClipSpace.xyz/posLightClipSpace.w;    // for orto matrix, we don't need perspective division, you can remove it if you want; this is general case;
  const vec2 shadowTexCoord    = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);  // just shift coords from [-1,1] to [0,1]               
    
  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  const float shadow    = ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;
  
  const vec4 dark_violet = vec4(0.59f, 0.0f, 0.82f, 1.0f);
  const vec4 red  = vec4(0.95f, 0.1f, 0.1f, 1.0f);
  
  vec4 dynamicLightColor = mix(dark_violet, red, abs(sin(Params.time)));

  vec3 lightDir   = normalize(Params.lightPos - surf.wPos);
  vec4 lightColor = max(dot(surf.wNorm, lightDir), 0.0f) * dynamicLightColor;
  float theta     = dot(lightDir, normalize(-Params.lightDir));
  float epsilon   = cutOff - outerCutOff;

  if (theta > outerCutOff)
  {
    intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);
  }

  float distance  = length(Params.lightPos - surf.wPos);
  float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * (distance * distance));

  out_fragColor   = (lightColor*shadow + vec4(0.1f)) * vec4(Params.baseColor, 1.0f) * intensity * attenuation;
}
