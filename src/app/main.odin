package main

import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:strings"
import "core:sys/wasm/wasi"

import tk "toolkit"
import "ui"
import "editor"

backing_buffer := [1024*1024*4]byte{}
temp_buffer := [1024*1024]byte{}

arena := mem.Arena{}
temp_arena := mem.Arena{}
ui_allocator: mem.Allocator
temp_allocator: mem.Allocator

app: AppState = AppState{}

main :: proc() {
    mem.arena_init(&arena, backing_buffer[:])
    mem.arena_init(&temp_arena, temp_buffer[:])

    ui_allocator = mem.arena_allocator(&arena)
    temp_allocator = mem.arena_allocator(&temp_arena)

    context.allocator = ui_allocator
    context.temp_allocator = temp_allocator

    app.ui = ui.init(ui_allocator, temp_allocator)
    app.test_file_buffer = editor.new_file_buffer(ui_allocator)

    editor.insert_content(&app.test_file_buffer.content, editor.Piece_Table_Index{}, #load("editor/piece_table.odin"))
}

@(export)
present :: proc "contextless" (mouse_x: i32, mouse_y: i32, mouse_left_down: bool, mouse_right_down: bool, width, height: i32) {
    context = runtime.default_context()

    context.allocator = ui_allocator
    context.temp_allocator = temp_allocator

    app.ui.last_mouse_x = app.ui.mouse_x
    app.ui.last_mouse_y = app.ui.mouse_y
    app.ui.last_mouse_left_down = app.ui.mouse_left_down
    app.ui.last_mouse_right_down = app.ui.mouse_right_down

    app.ui.mouse_x = int(mouse_x)
    app.ui.mouse_y = int(mouse_y)
    app.ui.mouse_left_down = mouse_left_down
    app.ui.mouse_right_down = mouse_right_down

    font_width, font_height := 15, 24
    {
        background, _ := ui.push_box(
            &app.ui,
            "background",
            {
                .DrawBackground
            },
            .Vertical,
            ui.SemanticSize {
                .Fill,
                0,
            }
        )
        ui.push_parent(&app.ui, background)
        defer ui.pop_parent(&app.ui)

        header(&app.ui)
        main_container(&app.ui)
    }

    ui.compute_layout(
        &app.ui,
        {
            int(width),
            int(height),
        },
        font_width,
        font_height,
        app.ui.root
    )

    tk.fill_rect(0,0,800,600, "white")

    ui.draw(
        &app.ui,
        font_width,
        font_height,
        app.ui.root
    )

    ui.prune(&app.ui)

    runtime.free_all(context.temp_allocator)

    // FIXME: don't require this
    app.ui.clips = make([dynamic]ui.Rect, allocator = context.temp_allocator)
}

header :: proc(ctx: ^ui.Context) {
    header, _ := ui.push_box(
        ctx,
        "header",
        {},
        .Horizontal,
        {
            ui.SemanticSize {
                .Fill,
                0
            },
            ui.SemanticSize {
                .FitText,
                0
            }
        }
    )
    ui.push_parent(ctx, header)
    defer ui.pop_parent(ctx)

    ui.spacer(ctx, "header_top_left_spacer")
    ui.label(ctx, "Odin Tool Creator - [very wip]")
    ui.spacer(ctx, "header_top_right_spacer")
}

main_container :: proc(ctx: ^ui.Context) {
    main_container, _ := ui.push_box(
        ctx,
        "main_container",
        {},
        .Horizontal,
        ui.Fill
    )

    ui.push_parent(ctx, main_container)
    defer ui.pop_parent(ctx)

    right_panel(ctx)
    left_panel(ctx)
}

text_editor :: proc(ctx: ^ui.Context, label: string, file_buffer: ^editor.File_Buffer) {
    ui.custom(ctx, label, proc (box: ^ui.Box, user_data: rawptr) {
        tk.fill_rect(
            i32(box.computed_pos.x),
            i32(box.computed_pos.y),
            i32(box.computed_size.x),
            i32(box.computed_size.y),
            "black"
        )
        editor.draw_buffer(
            transmute(^editor.File_Buffer)user_data,
            i32(box.computed_pos.x),
            i32(box.computed_pos.y),
            i32(box.computed_size.x/15),
            i32(box.computed_size.y/24),
            15,
            24
        )
    }, &app.test_file_buffer)
}

left_panel :: proc(ctx: ^ui.Context) {
    left_panel, _ := ui.push_box(
        ctx,
        "left_panel",
        {},
        .Vertical,
        ui.Fill
    )

    ui.push_parent(ctx, left_panel)
    defer ui.pop_parent(ctx)

    ui.label(ctx, "Here is a text editor!")
    text_editor(ctx, "test_file_buffer", &app.test_file_buffer)
}

right_panel_clicked: bool = false
right_panel :: proc(ctx: ^ui.Context) {
    right_panel, _ := ui.push_box(
        ctx,
        "right_panel",
        {},
        .Vertical,
        ui.Fill
    )


    ui.push_parent(ctx, right_panel)
    defer ui.pop_parent(ctx)

    ui.spacer(ctx, "right_panel_top_spacer")
    if ui.button(ctx, "look at me!").clicked {
        right_panel_clicked = !right_panel_clicked
    }
    if right_panel_clicked {
        ui.label(ctx, "Peekaboo!")
    }
    ui.spacer(ctx, "right_panel_bottom_spacer")
}
