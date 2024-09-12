package gfx;

import "core:strings"
import "core:fmt"
import c "core:c"

when ODIN_OS == .Linux || ODIN_OS == .Darwin {
    foreign import gfx "system:gfx"
}
when ODIN_OS_STRING == "windows" {
    @require foreign import "system:user32.lib"
    @require foreign import "system:gdi32.lib"
    @require foreign import "system:kernel32.lib"
    @require foreign import "system:opengl32.lib"
    foreign import "../../bin/gfx.obj"
}

GfxFrameFunc :: proc "c" (mouse_x: int, mouse_y: int, mouse_left_down: bool, mouse_right_down: bool)

cx :: struct {}

GpuGlyph :: struct {
    atlas_position: [2]f32,
    size: [2]f32,
    position: [2]f32,
    y_offset: f32,
    _haha_alignment: f32,
    color: [2]f32,
}

@(private)
gfx_string :: struct {
    data: [^]u8,
    len: c.size_t,

    // unused
    owned: c.bool,
}

foreign gfx {
    keep_running: bool

    @(link_name="gfx_frame_width") frame_width :: proc "c" (gfx_cx: ^cx) -> u32 ---
    @(link_name="gfx_frame_height") frame_height :: proc "c" (gfx_cx: ^cx) -> u32 ---
    @(link_name="gfx_mouse_x") mouse_x :: proc "c" (gfx_cx: ^cx) -> i32 ---
    @(link_name="gfx_mouse_y") mouse_y :: proc "c" (gfx_cx: ^cx) -> i32 ---

    @(link_name="gfx_init_context") init_context :: proc "c" (frame_func: GfxFrameFunc, width: u32, height: u32) -> ^cx ---
    @(link_name="gfx_run_events") run_events :: proc "c" (gfx_cx: ^cx) ---
    @(link_name="gfx_add_glyph") add_glyph :: proc "c" (gfx_cx: ^cx, glyph: GpuGlyph) ---
    @(link_name="gfx_push_texture_buffer") push_texture_buffer :: proc "c" (gfx_cx: ^cx, width: u32, height: u32, data: [^]u8, len: u32) ---

    @(private)
    gfx_queue_text :: proc "c" (gfx_cx: ^cx, text: gfx_string, position: [^]f32, max_x: f32, max_y: f32, color: [^]f32) ---
    @(private)
    gfx_queue_ui_rect :: proc "c" (gfx_cx: ^cx, position: [^]f32, size: [^]f32, border_size: f32, color: [^]f32) ---
}

queue_text :: proc(gfx_cx: ^cx, text: string, position: [2]f32, max_x: f32, max_y: f32, color: [4]f32) {
    ctext_data := strings.clone_to_cstring(text) 
    text_ := gfx_string {
        data = transmute([^]u8)ctext_data,
        len = len(ctext_data),
    };

    p := []f32 { position[0], position[1] }
    c := []f32 { color[0], color[1], color[2], color[3] }

    gfx_queue_text(gfx_cx, text_, raw_data(p), max_x, max_y, raw_data(c))

    delete(ctext_data)
}

queue_ui_rect :: proc(gfx_cx: ^cx, position: [2]f32, size: [2]f32, border_size: f32, color: [4]f32) {
    p := []f32 { position[0], position[1] }
    s := []f32 { size[0], size[1] }
    c := []f32 { color[0], color[1], color[2], color[3] }

    gfx_queue_ui_rect(gfx_cx, raw_data(p), raw_data(s), border_size, raw_data(c));
}

