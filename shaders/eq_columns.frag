#version 100
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float u_a0;
uniform float u_a1;
uniform float u_a2;
uniform float u_a3;
uniform float u_a4;
uniform float u_a5;
uniform float u_a6;
uniform float u_a7;
uniform float u_a8;
uniform float u_a9;
uniform float u_a10;
uniform float u_a11;
uniform float u_a12;
uniform float u_a13;
uniform float u_a14;
uniform float u_a15;
uniform float u_texel_x;
uniform float u_texel_y;
uniform float u_blur;

vec3 neonColor(int band) {
    if (band == 0)  return vec3(0.0, 1.0, 1.0);
    if (band == 1)  return vec3(0.2, 0.8, 1.0);
    if (band == 2)  return vec3(0.2, 1.0, 0.4);
    if (band == 3)  return vec3(0.5, 1.0, 0.0);
    if (band == 4)  return vec3(1.0, 1.0, 0.0);
    if (band == 5)  return vec3(1.0, 0.7, 0.0);
    if (band == 6)  return vec3(1.0, 0.4, 0.0);
    if (band == 7)  return vec3(1.0, 0.1, 0.1);
    if (band == 8)  return vec3(1.0, 0.0, 0.5);
    if (band == 9)  return vec3(0.8, 0.0, 1.0);
    if (band == 10) return vec3(0.5, 0.0, 1.0);
    if (band == 11) return vec3(0.3, 0.0, 1.0);
    if (band == 12) return vec3(0.0, 0.3, 1.0);
    if (band == 13) return vec3(0.0, 0.6, 1.0);
    if (band == 14) return vec3(0.0, 0.9, 0.8);
    return vec3(0.0, 1.0, 1.0);
}

void main(void) {
    float colW = 1.0 / 16.0;
    int band = int(floor(v_texcoord.x / colW));
    if (band < 0) band = 0;
    if (band > 15) band = 15;

    float posInCol = (v_texcoord.x - float(band) * colW) / colW;
    float gapMargin = 0.08;
    if (posInCol < gapMargin || posInCol > (1.0 - gapMargin)) {
        gl_FragColor = texture2D(tex, v_texcoord);
        return;
    }

    float a;
    if      (band == 0)  a = u_a0;
    else if (band == 1)  a = u_a1;
    else if (band == 2)  a = u_a2;
    else if (band == 3)  a = u_a3;
    else if (band == 4)  a = u_a4;
    else if (band == 5)  a = u_a5;
    else if (band == 6)  a = u_a6;
    else if (band == 7)  a = u_a7;
    else if (band == 8)  a = u_a8;
    else if (band == 9)  a = u_a9;
    else if (band == 10) a = u_a10;
    else if (band == 11) a = u_a11;
    else if (band == 12) a = u_a12;
    else if (band == 13) a = u_a13;
    else if (band == 14) a = u_a14;
    else                  a = u_a15;

    float amp = clamp(a, 0.0, 1.0);
    amp = max(amp, 0.08);
    float yb = 1.0 - v_texcoord.y;
    if (yb >= amp) {
        gl_FragColor = texture2D(tex, v_texcoord);
        return;
    }

    vec3 ncol = neonColor(band);
    vec4 blurred = vec4(0.0);
    float total = 0.0;
    for (float x = -2.0; x <= 2.0; x += 1.0) {
        for (float y = -2.0; y <= 2.0; y += 1.0) {
            vec2 off = vec2(x * u_texel_x * u_blur, y * u_texel_y * u_blur);
            blurred += texture2D(tex, v_texcoord + off);
            total += 1.0;
        }
    }
    blurred /= total;

    float edgeV = 0.02;
    float edgeH = edgeV * 16.0;
    float distTop = amp - yb;
    float distBot = yb;
    float distLeft = (posInCol - gapMargin) / (1.0 - 2.0 * gapMargin);
    float distRight = 1.0 - distLeft;
    float edgeT = distTop < edgeV ? 1.0 - distTop / edgeV : 0.0;
    float edgeB = distBot < edgeV ? 1.0 - distBot / edgeV : 0.0;
    float edgeL = distLeft < edgeH ? 1.0 - distLeft / edgeH : 0.0;
    float edgeR = distRight < edgeH ? 1.0 - distRight / edgeH : 0.0;
    float edgeMax = max(max(edgeT, edgeB), max(edgeL, edgeR));
    if (edgeMax > 0.0) {
        gl_FragColor = mix(blurred, vec4(ncol, 1.0), edgeMax * 0.65);
    } else {
        gl_FragColor = blurred * vec4(mix(vec3(1.0), ncol, 0.12), 1.0);
    }
}
