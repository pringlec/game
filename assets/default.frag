// Gouraud shading -- frag shader
// Adapted from Angel

#version 330 core

in vec4 colour;
void main() {
   gl_FragColor = colour;
}