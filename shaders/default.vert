attribute vec2 a_position;
attribute vec2 a_texcoord;
varying vec2 texcoord;

void main(void) {
  gl_Position = vec4(a_position, 0.0, 1.0);
  texcoord = a_texcoord;
}
