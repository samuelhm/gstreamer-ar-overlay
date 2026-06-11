#version 300 es
precision mediump float;

in vec2 texcoord;
uniform sampler2D tex;
uniform float u_amplitudes[16];
uniform vec2 u_texel_size;
uniform float u_blur_radius;
out vec4 fragColor;

void main(void) {
    float bandCount = 16.0;
    float barRatio = 0.80;
    float gap = (1.0 - barRatio) * 0.5;
    float colW = 1.0 / bandCount;
    int band = int(floor(texcoord.x / colW));
    if (band < 0) band = 0;
    if (band > 15) band = 15;

    float colStart = float(band) * colW;
    float posInCol = (texcoord.x - colStart) / colW;

    if (posInCol < gap || posInCol > (1.0 - gap)) {
        fragColor = texture(tex, texcoord);
        return;
    }

    // Constant-index lookup for all 16 bands
    float amplitude;
    if      (band == 0)  amplitude = u_amplitudes[0];
    else if (band == 1)  amplitude = u_amplitudes[1];
    else if (band == 2)  amplitude = u_amplitudes[2];
    else if (band == 3)  amplitude = u_amplitudes[3];
    else if (band == 4)  amplitude = u_amplitudes[4];
    else if (band == 5)  amplitude = u_amplitudes[5];
    else if (band == 6)  amplitude = u_amplitudes[6];
    else if (band == 7)  amplitude = u_amplitudes[7];
    else if (band == 8)  amplitude = u_amplitudes[8];
    else if (band == 9)  amplitude = u_amplitudes[9];
    else if (band == 10) amplitude = u_amplitudes[10];
    else if (band == 11) amplitude = u_amplitudes[11];
    else if (band == 12) amplitude = u_amplitudes[12];
    else if (band == 13) amplitude = u_amplitudes[13];
    else if (band == 14) amplitude = u_amplitudes[14];
    else                 amplitude = u_amplitudes[15];

    amplitude = clamp(amplitude, 0.0, 1.0);
    amplitude = max(amplitude, 0.08);

    float yFromBottom = 1.0 - texcoord.y;

    if (yFromBottom >= amplitude) {
        fragColor = texture(tex, texcoord);
        return;
    }

    vec4 blurred = vec4(0.0);
    float total = 0.0;
    float r = u_blur_radius;
    for (float x = -2.0; x <= 2.0; x += 1.0) {
        for (float y = -2.0; y <= 2.0; y += 1.0) {
            vec2 off = vec2(x * u_texel_size.x * r,
                            y * u_texel_size.y * r);
            blurred += texture(tex, texcoord + off);
            total += 1.0;
        }
    }
    blurred /= total;

    float distToEdge = amplitude - yFromBottom;
    float edgeWidth = 0.006;
    if (distToEdge < edgeWidth) {
        float glow = 1.0 - (distToEdge / edgeWidth);
        vec4 neon = vec4(0.0, 1.0, 0.8, 1.0);
        fragColor = mix(blurred, neon, glow * 0.5);
    } else {
        fragColor = blurred;
    }
}
