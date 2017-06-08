// Simple vertex shader for use with procedural texture
// Adapted from LearnOpenGL

#version 330 core

layout(location=0) in vec4 vPosition;
// layout(location=1) in vec3 vNormal;
layout (location = 1) in vec2 vTexCoord;

// Transformation matrices
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;

// Outputs for fragment shader
out vec2 texCoord;

void main() {
 mat4 MV = V * M;
 gl_Position = P * MV * vPosition;
 texCoord = vTexCoord;
}