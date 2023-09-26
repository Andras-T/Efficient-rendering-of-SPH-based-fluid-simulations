#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 0) out vec2 fragTexCoord;

void main() {
	fragTexCoord = (inPosition.xy + vec2(1.0f, 1.0f)) * 0.5f;
	gl_Position = inPosition;
}