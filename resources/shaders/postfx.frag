#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

vec4 uncharted2_tonemap_partial(vec4 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec4 uncharted2_filmic(vec4 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

void main()
{
  vec2 uv = surf.texCoord;
  uv.y = uv.y + 1;

  vec4 color = textureLod(colorTex, uv, 0);
  
  color = uncharted2_filmic(color);

  out_color = color;
}