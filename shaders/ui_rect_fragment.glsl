#version 440 core

struct UiRectFragment {
    highp vec4 device_position;
    highp vec2 position;
    highp vec2 size;
    highp vec2 border_size;
    highp vec2 screen_size;
    highp vec2 tex_coord;
    highp vec4 color;
};

in UiRectFragment out_rect;

layout(location = 0) out highp vec4 color;

highp float rect_sdf(
    highp vec2 absolute_pixel_position,
    highp vec2 origin,
    highp vec2 size,
    highp float corner_radius
) {
    highp vec2 half_size = size / 2.0;
    highp vec2 rect_center = origin + half_size;

    highp vec2 pixel_position = abs(absolute_pixel_position - rect_center);
    highp vec2 shrunk_corner_position = half_size - corner_radius;

    highp vec2 pixel_to_shrunk_corner = max(vec2(0), pixel_position - shrunk_corner_position);
    highp float distance_to_shrunk_corner = length(pixel_to_shrunk_corner);
    highp float distance = distance_to_shrunk_corner - corner_radius;

    return distance;
}

void main() {
    highp vec2 pixel_pos = out_rect.tex_coord.xy * out_rect.screen_size;

    highp float distance = rect_sdf(pixel_pos, out_rect.position, out_rect.size, out_rect.border_size.x);
    if (distance <= 0.0) {
        color = out_rect.color;
    } else {
        color = vec4(0);
    }
}
