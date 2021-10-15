#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
	vec2 texCoord;
} surf;

void main()
{
	int kernelSize   = 3;
	float k_1		 = 1.f;
	float k_2		 = 5.f;
	ivec2 size		 = textureSize(colorTex, 0);
	float dx		 = 1.f / float(size.x);
	float dy		 = 1.f / float(size.y);
	vec4 clr		 = vec4(0.f);
	float weightSum  = 0.f;
	vec2 tex		 = surf.texCoord;
	vec4 originColor = texture(colorTex, tex);

	for (int i = -kernelSize; i <= kernelSize; i++)
		for (int j = -kernelSize; j <= kernelSize; j++)
		{
			vec4 curr = texture(colorTex, tex + vec2(i * dx, j * dy));
			float f = sqrt(float(i) * float(i) + float(j) * float(j));
			float weight = exp(-k_1 * f * f  -k_2 * length(curr - originColor) * length(curr - originColor));

			clr += weight * curr;
			weightSum += weight;
		}

	color = clr / weightSum;

	//color = textureLod(colorTex, surf.texCoord, 0);
}
