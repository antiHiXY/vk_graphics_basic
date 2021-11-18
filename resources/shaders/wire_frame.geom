#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;

layout (location = 0) in GS_IN
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} vIn[];

layout (line_strip, max_vertices = 2) out;

layout (location = 0) out GS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} vOut;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
    for (int i = 0; i < 2; ++i)
    {
        gl_Position = gl_in[i].gl_Position;

        vOut.wPos = vIn[i].wPos;
        vOut.wNorm = vIn[i].wNorm;
        vOut.wTangent = vIn[i].wTangent;
        vOut.texCoord = vIn[i].texCoord;

        EmitVertex();
    }

    EndPrimitive();
}