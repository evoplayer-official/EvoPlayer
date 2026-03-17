#version 330 core
in vec2 vUV;
out vec4 FragColor;

void main() {
    vec2 uv = vUV;

    // Base vetro nero profondo con gradiente sottile
    float grad = 1.0 - uv.y * 0.18;
    vec3 base = vec3(0.012, 0.015, 0.022) * grad;

    // Cornice LED azzurro ghiaccio #D6EEFF — spessa e luminosa
    float borderW = 0.008;
    float distL = uv.x;
    float distR = 1.0 - uv.x;
    float distT = 1.0 - uv.y;
    float distB = uv.y;
    float border = min(min(distL, distR), min(distT, distB));
    // glow interno — alone morbido
    float glowInner = 1.0 - smoothstep(0.0, 0.012, border);
    // linea LED netta
    float ledLine = smoothstep(0.0, borderW, border) - smoothstep(borderW, borderW*2.5, border);
    vec3 ledColor = vec3(0.84, 0.93, 1.0);
    base += ledColor * glowInner * 0.45;
    base += ledColor * ledLine * 3.5;

    // Riflesso speculare in alto (vetro glossy)
    float reflY = 1.0 - uv.y;
    float reflX = abs(uv.x - 0.5) * 2.0;
    float refl = smoothstep(0.85, 1.0, reflY) * (1.0 - reflX * reflX) * 0.07;
    base += vec3(refl * 0.6, refl * 0.75, refl);

    // Riflesso diagonale sottile (effetto luce laterale)
    float diagRefl = smoothstep(0.0, 0.3, uv.x) * smoothstep(0.6, 0.3, uv.x);
    diagRefl *= smoothstep(0.7, 1.0, uv.y) * 0.04;
    base += vec3(diagRefl * 0.4, diagRefl * 0.6, diagRefl);

    // Scanlines sottili stile display professionale
    float scanline = sin(uv.y * 300.0) * 0.012 + 0.988;
    base *= scanline;

    // Vignettatura angoli
    float vignette = uv.x * (1.0 - uv.x) * uv.y * (1.0 - uv.y);
    vignette = pow(vignette * 16.0, 0.3);
    base *= vignette;

    FragColor = vec4(base, 1.0);
}
