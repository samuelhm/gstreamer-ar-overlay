#version 300 es

in vec2 a_position;
in vec2 a_texcoord;
out vec2 texcoord;

void main(void) {
    gl_Position = vec4(a_position, 0.0, 1.0);
    texcoord = a_texcoord;
}
