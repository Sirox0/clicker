#version 450

layout(push_constant) uniform PC {
    float curStarScale;
};

layout(location = 0) out vec2 uv;

vec2 positions[6] = vec2[6](
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] * curStarScale, 0.0, 1.0);
    uv = clamp(positions[gl_VertexIndex] * 2, 0.0, 1.0);
}