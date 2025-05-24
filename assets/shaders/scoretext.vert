#version 450

layout(location = 0) in vec4 posuv;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(posuv.xy, 0.0, 1.0);
    uv = posuv.zw;
}