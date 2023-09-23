#version 450

struct Particle {
    vec4 position;
    vec4 color;
    vec3 velocity;
    float movable;
    vec3 acc;
    float density;
};

layout(binding = 5) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float deltaTime;
} ubo;

layout(binding = 4) uniform Model {
    vec4 color;
    vec4 wallColor;
    float size;
    float wallSize;
    int wall;
    float pad;
} model;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 fragColor;
layout(location = 2) out vec4 pos;

layout(set = 0, binding = 0) buffer SphereBuffer {
    vec4 sphereData[];
};

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
    Particle particlesIn[];
};

void main() {
    int instanceIndex = gl_InstanceIndex;
    Particle p = particlesIn[instanceIndex];
    
    vec4 vertexPosition = sphereData[gl_VertexIndex] + p.position;
    gl_Position = ubo.proj * ubo.view * ubo.model * vertexPosition;
    
    pos = sphereData[gl_VertexIndex];
    fragColor = p.color;
}