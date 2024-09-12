#version 440 core

struct VertexOutput {
    vec4 position;
    vec4 color;
    vec2 tex_coord;
};


layout(location = 0) in VertexOutput out_vertex;
layout(location = 1) uniform highp sampler2D atlas_texture;

out vec4 color;

void main() {
    float text_color = texture(atlas_texture, out_vertex.tex_coord).r;

    color = vec4(out_vertex.color.rgb * text_color, text_color);
}
