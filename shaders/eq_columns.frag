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

    // Dynamic indexing of uniform arrays works in GLES 3.0+
    float amplitude = u_amplitudes[band];
    amplitude = clamp(amplitude, 0.0, 1.0);
    amplitude = max(amplitude, 0.15);

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
