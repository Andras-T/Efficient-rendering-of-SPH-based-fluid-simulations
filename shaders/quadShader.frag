#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragTexCoord;

void main() {
	outColor = vec4(fragTexCoord.x, fragTexCoord.y, (fragTexCoord.x + fragTexCoord.y) * 0.5f, 1.0f);
}