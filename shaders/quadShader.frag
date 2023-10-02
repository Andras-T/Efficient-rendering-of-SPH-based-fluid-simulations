#version 450

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D depthSampler;
layout(location = 1) in vec2 fragTexCoord;

void main() {
	float depthValue = 1 - texture(depthSampler, fragTexCoord).r;
	//if (depthValue < 0.001)
	//	discard;
	outColor = vec4(depthValue, depthValue, depthValue, 1.0f);
}