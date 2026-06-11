precision mediump float;

varying vec2 texcoord;
uniform sampler2D tex;
uniform float u_amplitudes[64];
uniform vec2 u_texel_size;
uniform float u_blur_radius;

vec4 blur25(sampler2D sampler, vec2 uv, vec2 texel, float radius) {
    vec4 color = vec4(0.0);
    float total = 0.0;

    for (float x = -2.0; x <= 2.0; x += 1.0) {
        for (float y = -2.0; y <= 2.0; y += 1.0) {
            vec2 offset = vec2(x * texel.x * radius, y * texel.y * radius);
            color += texture2D(sampler, uv + offset);
            total += 1.0;
        }
    }
    return color / total;
}

void main(void) {
    float bandCount = 64.0;
    int band = int(floor(texcoord.x * bandCount));
    if (band < 0) band = 0;
    if (band > 63) band = 63;

    float amplitude = u_amplitudes[band];
    amplitude = clamp(amplitude, 0.0, 1.0);

    float yFromBottom = 1.0 - texcoord.y;
    float edgeWidth = 0.006;

    if (yFromBottom < amplitude) {
        vec4 blurred = blur25(tex, texcoord, u_texel_size, u_blur_radius);

        float distToEdge = amplitude - yFromBottom;
        if (distToEdge < edgeWidth) {
            float glow = 1.0 - (distToEdge / edgeWidth);
            vec4 neon = vec4(0.0, 1.0, 0.8, 1.0) * glow;
            gl_FragColor = mix(blurred, neon, glow * 0.5);
        } else {
            gl_FragColor = blurred;
        }
    } else {
        gl_FragColor = texture2D(tex, texcoord);
    }
}
