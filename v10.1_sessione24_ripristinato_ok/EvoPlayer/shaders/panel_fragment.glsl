#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uPanel;
uniform float uAlpha;
void main() {
    vec4 col = texture(uPanel, TexCoord);
    FragColor = vec4(col.rgb, col.a * uAlpha);
}
