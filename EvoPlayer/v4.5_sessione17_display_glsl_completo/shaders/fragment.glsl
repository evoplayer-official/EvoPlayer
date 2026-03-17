#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
uniform float uEmission;
void main() {
    vec3 col = uColor + uColor * uEmission * 1.8;
    FragColor = vec4(col, 1.0);
}
