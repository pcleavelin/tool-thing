#version 440 core

struct Vertex {
    vec2 position;
    vec2 tex_coord;
};

struct UiRect {
    vec4 position;
    vec4 size;
    vec4 border_size;
    vec4 color;
};

struct UiRectFragment {
    highp vec4 device_position;
    highp vec2 position;
    highp vec2 size;
    highp vec2 border_size;
    highp vec2 screen_size;
    highp vec2 tex_coord;
    highp vec4 color;
};

layout(std430, binding = 0) readonly buffer VertexBlock {
    Vertex vertices[];
};

layout(std430, binding = 1) readonly buffer RectBlock {
    UiRect rects[];
};
layout(std430, binding = 2) readonly buffer ParamsBlock {
    vec2 screen_size;
};

out UiRectFragment out_rect;

void main() {
    UiRect rect = rects[gl_InstanceID];

    out_rect.device_position = vec4(vertices[gl_VertexID].position, 1, 1);
    out_rect.position = rect.position.xy;
    out_rect.size = rect.size.xy;
    out_rect.border_size = rect.border_size.xy;
    out_rect.screen_size = screen_size;
    out_rect.tex_coord = vertices[gl_VertexID].tex_coord;
    out_rect.color = rect.color;

    gl_Position = vec4(vertices[gl_VertexID].position, 1, 1);
}
