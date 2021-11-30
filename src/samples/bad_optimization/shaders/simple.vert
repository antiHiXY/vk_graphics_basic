#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

vec3 DecodeNormal(uint a_data)
{
  const uint a_enc_x = (a_data  & 0x0000FFFFu);
  const uint a_enc_y = ((a_data & 0xFFFF0000u) >> 16);
  const float sign   = (a_enc_x & 0x0001u) != 0 ? -1.0f : 1.0f;

  const int usX = int(a_enc_x & 0x0000FFFEu);
  const int usY = int(a_enc_y & 0x0000FFFFu);

  const int sX  = (usX <= 32767) ? usX : usX - 65536;
  const int sY  = (usY <= 32767) ? usY : usY - 65536;

  const float x = sX*(1.0f / 32767.0f);
  const float y = sY*(1.0f / 32767.0f);
  const float z = sign*sqrt(max(1.0f - x*x - y*y, 0.0f));

  return vec3(x, y, z);
}


layout(location = 0) in vec4 vPosNorm;
layout(location = 1) in vec4 vTexCoordAndTang;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
} params;


layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} vOut;

out gl_PerVertex { vec4 gl_Position; };
void main(void)
{
    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    mat3 inversed = mat3(transpose(inverse(params.mModel)));
    vOut.wPos     = (params.mModel * vec4(vPosNorm.xyz, 1.0f)).xyz;
    vOut.wNorm    = normalize(inversed * wNorm.xyz);
    vOut.wTangent = normalize(inversed * wTang.xyz);
    vOut.texCoord = vTexCoordAndTang.xy;

    gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
}