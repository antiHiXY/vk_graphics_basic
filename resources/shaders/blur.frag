#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec2 out_M1M2;

layout(binding = 0) uniform sampler2D depthMap;

layout(push_constant) uniform params_t
{
    uint radius;
} params;

void main()
{
    const vec2 dims = vec2(textureSize(depthMap, 0));

    const int r = int(params.radius);
    uint counter = 0;

    out_M1M2 = vec2(0, 0);
    for (int i = -r; i <= r; ++i)
    {
        for (int j = -r; j <= r; ++j)
        {
            vec2 offsetCoords = (gl_FragCoord.xy + vec2(i, j)) / dims;

            float depth = textureLod(depthMap, offsetCoords, 0).x;
            out_M1M2 += vec2(depth, depth*depth);
            counter++;

        }
    }
    out_M1M2 /= vec2(counter);
}