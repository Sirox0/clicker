#version 450

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D star;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(star, uv);
    // zero-out transparent parts of the image
    outColor.rgb *= outColor.a;
}