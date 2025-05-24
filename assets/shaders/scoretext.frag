#version 450

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D text;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vec3(texture(text, uv).r), 1.0);
}