#version 450

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D depthSampler;
layout(location = 1) in vec2 fragTexCoord;

void main() {
	float depthValue = 1.0f - texture(depthSampler, fragTexCoord).r;
	outColor = vec4(texture(depthSampler, fragTexCoord).rgb, 1.0f);
}	