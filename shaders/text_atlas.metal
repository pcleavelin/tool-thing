#include <metal_stdlib>

using namespace metal;

struct VertexInput {
    float2 position;
    float2 tex_coord;
};

struct VertexOutput {
    float4 position [[position]];
    float4 color;
    float2 tex_coord;
};

struct Glyph {
    float2 atlas_position;
    float2 size;
    float2 target_position;
    float y_offset;
    float _haha_alignment;
    float4 color;
};

struct UniformParams {
    float2 screen_size;
};

float4 to_device_position(float2 position, float2 size) {
    return float4(((position / size) * 2.0) - float2(1.0), 1.0, 1.0);
}

vertex VertexOutput
vs_main(
        uint vertex_id [[vertex_id]],
        uint glyph_id [[instance_id]],
        constant VertexInput *vertices [[buffer(0)]],
        constant Glyph *glyphs [[buffer(1)]],
        constant UniformParams &params [[buffer(2)]]
)
{
    VertexOutput out;

    Glyph glyph = glyphs[glyph_id];

    float2 scaled_size = ((vertices[vertex_id].position + 1.0) / 2.0) * (glyph.size/2.0);
    float2 scaled_size_2 = ((vertices[vertex_id].position + 1.0) / 2.0) * (glyph.size);
    float2 glyph_pos = scaled_size + glyph.target_position + float2(0, glyph.y_offset/2.0+24);

    float4 device_position = to_device_position(glyph_pos, params.screen_size);
    float2 atlas_position = (scaled_size_2 + glyph.atlas_position) / 512.0;

    device_position.y = -device_position.y;

    out.position = device_position;
    out.color = glyph.color;
    out.tex_coord = atlas_position;

    return out;
}

fragment float4 fs_main(VertexOutput in [[stage_in]],
                            texture2d<float, access::sample> texture [[texture(0)]])
{
    constexpr sampler texture_sampler (mag_filter::linear, min_filter::linear);

    float text_color = texture.sample(texture_sampler, in.tex_coord).r;
    return float4(in.color.rgb, text_color);
}

struct UiRect {
    float4 position;
    float4 size;
    float4 border_size;
    float4 color;
};

struct UiRectFragment {
    float4 device_position [[position]];
    float2 position;
    float2 size;
    float2 border_size;
    float2 screen_size;
    float2 tex_coord;
    float4 color;
};

vertex UiRectFragment
ui_rect_vs(
        uint vertex_id [[vertex_id]],
        uint rect_id [[instance_id]],
        constant VertexInput *vertices [[buffer(0)]],
        constant UiRect *rects [[buffer(1)]],
        constant UniformParams &params [[buffer(2)]]
)
{
    UiRect rect = rects[rect_id];

    float2 scaled_size = ((vertices[vertex_id].position.xy + 1.0) / 2.0) * (rect.size.xy);
    float2 rect_pos = scaled_size + rect.position.xy;

    float4 device_position = to_device_position(rect_pos, params.screen_size);
    device_position.y = -device_position.y;

    return UiRectFragment {
        device_position,
        rect.position.xy,
        rect.size.xy,
        rect.border_size.xy,
        params.screen_size,
        vertices[vertex_id].tex_coord,
        rect.color,
    };
}

float rect_sdf(
    float2 absolute_pixel_position,
    float2 origin,
    float2 size,
    float corner_radius
) {
    float2 half_size = size / 2;
    float2 rect_center = origin + half_size;

    float2 pixel_position = abs(absolute_pixel_position - rect_center);
    float2 shrunk_corner_position = half_size - corner_radius;

    float2 pixel_to_shrunk_corner = max(float2(0), pixel_position - shrunk_corner_position);
    float distance_to_shrunk_corner = length(pixel_to_shrunk_corner);
    float distance = distance_to_shrunk_corner - corner_radius;

    return distance;
}

fragment float4 ui_rect_fs(UiRectFragment in [[stage_in]])
{
    float2 pixel_pos = (in.device_position.xy + 1.0) / 2.0;

    float distance = rect_sdf(pixel_pos, in.position, in.size, in.border_size.x);
    if (distance <= 0.0) {
        return in.color;
    } else {
        return float4(0);
    }
}
