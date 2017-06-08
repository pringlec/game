// Textured object -- frag shader
// Adapted from LearnOpenGL

#version 330 core

in vec2 texCoord;
out vec4 colour;

uniform sampler2D texture1;

void main() {
   colour = texture(texture1, texCoord);
}