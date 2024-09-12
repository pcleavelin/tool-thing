#version 440 core

struct Vertex {
    vec2 position;
    vec2 tex_coord;
};

struct VertexOutput {
    vec4 position;
    vec4 color;
    vec2 tex_coord;
};

struct Glyph {
    vec2 atlas_position;
    vec2 size;
    vec2 target_position;
    float y_offset;
    float _haha_alignment;
    vec4 color;
};

layout(std430, binding = 0) readonly buffer VertexBlock {
    Vertex vertices[];
};

layout(std430, binding = 1) readonly buffer GlyphBlock {
    Glyph glyphs[];
};

layout(std430, binding = 2) readonly buffer ParamsBlock {
    vec2 screen_size;
    vec2 font_size;
};

vec4 to_device_position(vec2 position, vec2 size) {
    return vec4(((position / size) * 2.0) - vec2(1.0), 1.0, 1.0);
}

layout(location = 0) out VertexOutput out_vertex;

void main() {
    Glyph glyph = glyphs[gl_InstanceID];

    vec2 scaled_size = ((vertices[gl_VertexID].position + 1.0) / 2.0) * (glyph.size/2.0);
    vec2 scaled_size_2 = ((vertices[gl_VertexID].position + 1.0) / 2.0) * (glyph.size);
    vec2 glyph_pos = scaled_size + glyph.target_position + vec2(0, glyph.y_offset/2.0+font_size.y);

    vec4 device_position = to_device_position(glyph_pos, screen_size);
    vec2 atlas_position = (scaled_size_2 + glyph.atlas_position) / 512.0;

    device_position.y = -device_position.y;

    out_vertex.position = device_position;
    out_vertex.color = glyph.color;
    out_vertex.tex_coord = atlas_position;

    gl_Position = device_position;
}
