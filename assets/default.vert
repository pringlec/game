// Gouraud shading -- vertex shader
// Adapted from Angel

#version 330 core

layout(location=0) in vec4 vPosition;
layout(location=1) in vec3 vNormal;

// Transformation matrices
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;

// Light/Material  properties
uniform vec4 lightPosition; 
uniform vec4 ambientContrib;
uniform vec4 diffuseContrib;
uniform vec4 specularContrib;
uniform float shininess;

// Calculated vertex colour
out vec4 colour;

void main() {
 mat4 MV = V * M;
 vec4 vEyeSpacePosition = MV * vPosition;
 vec3 N = normalize(mat3(MV)*vNormal);
 vec4 aV = lightPosition + (-1.0)*vEyeSpacePosition;
 vec3 L = normalize(aV.xyz);
 vec3 E = normalize(vEyeSpacePosition.xyz);
 vec3 H = normalize(L+E);
 vec4 ambientColour = ambientContrib;
 float Kd = max(0, dot(L,N));
 float Ks = pow(max(0, (dot(N, H))),shininess);
 vec4 diffuseColour = Kd * diffuseContrib;
 vec4 specularColour = Ks * specularContrib;
 colour = ambientColour + diffuseColour + specularColour;
 gl_Position = P * MV * vPosition;
}