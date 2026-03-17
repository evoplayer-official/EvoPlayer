#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D uText;
uniform vec3 uTextColor;
void main() {
    float alpha = texture(uText, TexCoords).r;
    FragColor = vec4(uTextColor, alpha);
}
