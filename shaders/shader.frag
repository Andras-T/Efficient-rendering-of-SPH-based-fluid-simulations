#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

layout(binding = 4) uniform Model {
    vec4 color;
    vec4 wallColor;
    float size;
    float wallSize;
    int wall;
    float pad2;
} model;

void main() {
    // part of the wall
    if (fragColor.w < 0.0001) {
        // if show wall is off
        if (model.wall == 0) {
            discard;
        }
        else {
            outColor = model.wallColor;
        } 
    } else {
        outColor = model.color;
    }
}