package ui

// import "core:fmt"
import "base:runtime"
import "core:mem"
import "core:strings"
import "core:math"

import "../toolkit"

Context :: struct {
    persistent_allocator: mem.Allocator,
    temp_allocator: mem.Allocator,

    root: ^Box,
    current_parent: ^Box,
    persistent: map[string]Box,
    current_interaction_index: int,

    clips: [dynamic]Rect,

    last_mouse_x: int,
    last_mouse_y: int,

    mouse_x: int,
    mouse_y: int,

    mouse_left_down: bool,
    last_mouse_left_down: bool,

    mouse_right_down: bool,
    last_mouse_right_down: bool,
}

Rect :: struct {
    pos: [2]int,
    size: [2]int,
}

Key :: struct {
    label: string,
    value: int,
}

Interaction :: struct {
    hovering: bool,
    clicked: bool,
    dragging: bool,

    box_pos: [2]int,
    box_size: [2]int,
}

Flag :: enum {
    Clickable,
    Hoverable,
    Scrollable,
    DrawText,
    DrawBorder,
    DrawBackground,
    RoundedBorder,
    Floating,
    CustomDrawFunc,
}

SemanticSizeKind :: enum {
    FitText = 0,
    Exact,
    ChildrenSum,
    Fill,
    PercentOfParent,
}

SemanticSize :: struct {
    kind: SemanticSizeKind,
    value: int,
}

Axis :: enum {
    Horizontal = 0,
    Vertical = 1,
}

CustomDrawFunc :: proc(/* state: ^core.State, */ box: ^Box, user_data: rawptr)
Box :: struct {
    first: ^Box,
    last: ^Box,
    next: ^Box,
    prev: ^Box,
    parent: ^Box,

    key: Key,
    last_interacted_index: int,

    flags: bit_set[Flag],

    label: string,

    axis: Axis,
    semantic_size: [2]SemanticSize,
    computed_size: [2]int,
    computed_pos: [2]int,

    scroll_offset: int,

    hot: int,
    active: int,

    custom_draw_func: CustomDrawFunc,
    user_data: rawptr,
}

init :: proc(allocator: mem.Allocator, temp_allocator: mem.Allocator) -> Context {
    context.allocator = allocator
    context.temp_allocator = temp_allocator

    root := new(Box)
    root.key = gen_key(nil, "root", 69)
    root.label = strings.clone("root")

    return Context {
        persistent_allocator = allocator,
        temp_allocator = temp_allocator,
        root = root,
        current_parent = root,
        persistent = make(map[string]Box, allocator = allocator),
        clips = make([dynamic]Rect, allocator = temp_allocator),
    }
}

gen_key :: proc(ctx: ^Context, label: string, value: int) -> Key {
    if ctx != nil {
        context.allocator = ctx.temp_allocator
    }

    key_label: string

    if ctx != nil && (ctx.current_parent == nil || len(ctx.current_parent.key.label) < 1) {
        key_label = strings.clone(label)
    } else if ctx != nil {
        // key_label = fmt.aprintf("%s:%s", ctx.current_parent.key.label, label);
        bytes, _ := runtime.mem_alloc(len(label) + len(ctx.current_parent.key.label) + 1)
        for c, i in ctx.current_parent.key.label {
            bytes[i] = u8(c)
        }
        bytes[len(ctx.current_parent.key.label)] = u8(':')
        for c, i in label {
            bytes[len(ctx.current_parent.key.label) + i + 1] = u8(c)
        }
        key_label = string(bytes)
    } else {
        key_label = strings.clone(label)
    }

    return Key {
        label = key_label,
        value = value,
    }
}

@(private)
make_box :: proc(ctx: ^Context, key: Key, label: string, flags: bit_set[Flag], axis: Axis, semantic_size: [2]SemanticSize) -> ^Box {
    context.allocator = ctx.temp_allocator

    box: ^Box = nil

    if cached_box, exists := ctx.persistent[key.label]; exists {
        // NOTE(pcleavelin): its important to note that the _cached_ key _is not_ free'd
        // as that would invalid the maps reference to the key causing memory leaks because
        // the map would think that an entry doesn't exist (in some cases)
        delete(key.label)

        if cached_box.last_interacted_index < ctx.current_interaction_index-1 {
            box = new_clone(cached_box)

            box.last_interacted_index = ctx.current_interaction_index
            box.hot = 0
            box.active = 0
        } else {
            box = new_clone(cached_box)
        }
    } else {
        box = new(Box)
        ctx.persistent[key.label] = box^

        box.key = key
        box.last_interacted_index = ctx.current_interaction_index
    }

    box.label = strings.clone(label, context.temp_allocator)

    box.first = nil
    box.last = nil
    box.next = nil
    box.prev = ctx.current_parent.last
    box.parent = ctx.current_parent
    box.flags = flags
    box.axis = axis
    box.semantic_size = semantic_size

    if ctx.current_parent.last != nil {
        ctx.current_parent.last.next = box
    }
    if ctx.current_parent.first == nil {
        ctx.current_parent.first = box
    }

    ctx.current_parent.last = box

    return box
}

make_semantic_size :: proc(kind: SemanticSizeKind, value: int = 0) -> SemanticSize {
    return SemanticSize {
        kind = kind,
        value = value
    }
}

FitText :[2]SemanticSize: {
    SemanticSize {
        kind = .FitText,
    },
    SemanticSize {
        kind = .FitText,
    }
}

ChildrenSum :[2]SemanticSize: {
    SemanticSize {
        kind = .ChildrenSum,
    },
    SemanticSize {
        kind = .ChildrenSum,
    }
}

Fill :[2]SemanticSize: {
    SemanticSize {
        kind = .Fill,
    },
    SemanticSize {
        kind = .Fill,
    }
}

push_box :: proc(ctx: ^Context, label: string, flags: bit_set[Flag], axis: Axis = .Horizontal, semantic_size: [2]SemanticSize = FitText) -> (^Box, Interaction) {
    context.allocator = ctx.temp_allocator

    key := gen_key(ctx, label, 0)
    box := make_box(ctx, key, label, flags, axis, semantic_size)
    interaction := test_box(ctx, box)

    return box, interaction
}

push_parent :: proc(ctx: ^Context, box: ^Box) {
    context.allocator = ctx.temp_allocator

    ctx.current_parent = box

    push_clip(ctx, box.computed_pos, box.computed_size, !(.Floating in box.flags))
}

pop_parent :: proc(ctx: ^Context) {
    context.allocator = ctx.temp_allocator

    if ctx.current_parent.parent != nil {
        ctx.current_parent = ctx.current_parent.parent
    }

    pop_clip(ctx)
}

test_box :: proc(ctx: ^Context, box: ^Box) -> Interaction {
    hovering: bool

    mouse_is_clicked := !ctx.last_mouse_left_down && ctx.mouse_left_down
    mouse_is_released := ctx.last_mouse_left_down && !ctx.mouse_left_down
    mouse_is_dragging := !mouse_is_clicked && ctx.mouse_left_down && (ctx.last_mouse_x != ctx.mouse_x || ctx.last_mouse_y != ctx.mouse_y)

    if ctx.mouse_x >= box.computed_pos.x && ctx.mouse_x <= box.computed_pos.x + box.computed_size.x &&
        ctx.mouse_y >= box.computed_pos.y && ctx.mouse_y <= box.computed_pos.y + box.computed_size.y
    {
        // FIXME: fix the clips
        if false && len(ctx.clips) > 0 {
            clip := ctx.clips[len(ctx.clips)-1]

            if ctx.mouse_x >= clip.pos.x && ctx.mouse_x <= clip.pos.x + clip.size.x &&
                ctx.mouse_y >= clip.pos.y && ctx.mouse_y <= clip.pos.y + clip.size.y
            {
                hovering = true
            } else  {
                hovering = false
            }
        } else {
            hovering = true
        }
    }

    if hovering || box.active > 0 {
        box.hot += 1
    } else {
        box.hot = 0
    }

    if hovering && mouse_is_clicked && !mouse_is_dragging {
        box.active = 1
    } else if hovering && mouse_is_dragging && box.active > 0 {
        box.active += 1
    } else if !ctx.mouse_left_down && !mouse_is_released {
        box.active = 0
    }

    // TODO: change this to use the scroll wheel input
    if .Scrollable in box.flags && box.active > 1 && ctx.mouse_left_down {
        box.scroll_offset -= ctx.mouse_y - ctx.last_mouse_y

        box.scroll_offset = math.min(0, box.scroll_offset)
    }

    if box.hot > 0 || box.active > 0 {
        box.last_interacted_index = ctx.current_interaction_index
    }
    return Interaction {
        hovering = .Hoverable in box.flags && (hovering || box.active > 0),
        clicked = .Clickable in box.flags && (hovering && mouse_is_released && box.active > 0),
        dragging = box.active > 1,

        box_pos = box.computed_pos,
        box_size = box.computed_size,
    }
}

delete_box_children :: proc(ctx: ^Context, box: ^Box, keep_persistent: bool = true) {
    context.allocator = ctx.temp_allocator

    iter := BoxIter { box.first, 0 }

    for box in iterate_box(&iter) {
        delete_box(ctx, box, keep_persistent)
    }
}

delete_box :: proc(ctx: ^Context, box: ^Box, keep_persistent: bool = true) {
    context.allocator = ctx.temp_allocator

    delete_box_children(ctx, box, keep_persistent)

    if box.last_interacted_index < ctx.current_interaction_index-1 {
        // FIXME: uncomment this
        // delete_key(&ctx.persistent, box.key.label)
    }

    if !(box.key.label in ctx.persistent) || !keep_persistent {
        delete(box.key.label)
        free(box)
    } else {
        ctx.persistent[box.key.label] = box^
    }
}

prune :: proc(ctx: ^Context) {
    context.allocator = ctx.temp_allocator

    iter := BoxIter { ctx.root.first, 0 }

    for box in iterate_box(&iter) {
        delete_box_children(ctx, box)

        if !(box.key.label in ctx.persistent) && box != ctx.root {
            free(box)
        }
    }

    ctx.root.first = nil
    ctx.root.last = nil
    ctx.root.next = nil
    ctx.root.prev = nil
    ctx.root.parent = nil
    ctx.current_parent = ctx.root

    ctx.current_interaction_index += 1
}

// TODO: consider not using `ctx` here
ancestor_size :: proc(ctx: ^Context, box: ^Box, axis: Axis) -> int {
    if box == nil || box.parent == nil || .Floating in box.flags {
        return ctx.root.computed_size[axis]
    }

    switch box.parent.semantic_size[axis].kind {
        case .FitText: fallthrough
        case .Exact: fallthrough
        case .Fill: fallthrough
        case .PercentOfParent:
            return box.parent.computed_size[axis]

        case .ChildrenSum:
            return ancestor_size(ctx, box.parent, axis)
    }

    return 1337
}

prev_non_floating_sibling :: proc(ctx: ^Context, box: ^Box) -> ^Box {
    if box == nil {
        return nil
    } else if box.prev == nil {
        return nil
    } else if !(.Floating in box.prev.flags) {
        return box.prev
    } else {
        return prev_non_floating_sibling(ctx, box.prev)
    }
}

compute_layout :: proc(ctx: ^Context, canvas_size: [2]int, font_width: int, font_height: int, box: ^Box) {
    context.allocator = ctx.temp_allocator

    if box == nil { return }

    axis := Axis.Horizontal
    if box.parent != nil && !(.Floating in box.flags) {
        axis = box.parent.axis
        box.computed_pos = box.parent.computed_pos
        box.computed_pos[axis] += box.parent.scroll_offset
    }

    if .Floating in box.flags {
        // box.computed_pos = {0,0};
    } else if box.prev != nil {
        prev := prev_non_floating_sibling(ctx, box)

        if prev != nil {
            box.computed_pos[axis] = prev.computed_pos[axis] + prev.computed_size[axis]
        }
    }

    post_compute_size := [2]bool { false, false }
    compute_children := true
    if box == ctx.root {
        box.computed_size = canvas_size
    } else {
        switch box.semantic_size.x.kind {
            case .FitText: {
                box.computed_size.x = len(box.label) * font_width
            }
            case .Exact: {
                box.computed_size.x = box.semantic_size.x.value
            }
            case .ChildrenSum: {
                post_compute_size[int(Axis.Horizontal)] = true
            }
            case .Fill: {
                if .Floating in box.flags {
                    box.computed_size.x = ancestor_size(ctx, box, .Horizontal)
                }
            }
            case .PercentOfParent: {
                box.computed_size.x = int(f32(ancestor_size(ctx, box, .Horizontal))*(f32(box.semantic_size.x.value)/100.0))
            }
        }
        switch box.semantic_size.y.kind {
            case .FitText: {
                box.computed_size.y = font_height
            }
            case .Exact: {
                box.computed_size.y = box.semantic_size.y.value
            }
            case .ChildrenSum: {
                post_compute_size[Axis.Vertical] = true
            }
            case .Fill: {
                if .Floating in box.flags {
                    box.computed_size.y = ancestor_size(ctx, box, .Vertical)
                }
            }
            case .PercentOfParent: {
                box.computed_size.y = int(f32(ancestor_size(ctx, box, .Vertical))*(f32(box.semantic_size.y.value)/100.0))
            }
        }
    }

    if compute_children {
        iter := BoxIter { box.first, 0 }
        child_size: [2]int = {0,0}

        // NOTE: the number of fills for the opposite axis of this box needs to be 1
        // because it will never get incremented in the loop below and cause a divide by zero
        // and the number of fills for the axis of the box needs to start at zero or else it will
        // be n+1 causing incorrect sizes
        number_of_fills: [2]int = {1,1}
        number_of_fills[box.axis] = 0

        our_size := box.computed_size

        for child in iterate_box(&iter) {
            if .Floating in child.flags { continue }

            compute_layout(ctx, canvas_size, font_width, font_height, child)
            if child.semantic_size[box.axis].kind == .Fill {
                number_of_fills[box.axis] += 1
            } else {
                child_size[box.axis] += child.computed_size[box.axis]
            }
        }

        iter = BoxIter { box.first, 0 }
        for child in iterate_box(&iter) {
            if !(.Floating in child.flags) {
                for axis in 0..<2 {
                    if child.semantic_size[axis].kind == .Fill {
                        child.computed_size[axis] = (our_size[axis] - child_size[axis]) / number_of_fills[axis]
                    }
                }
            }

            compute_layout(ctx, canvas_size, font_width, font_height, child)
        }
    }

    if post_compute_size[Axis.Horizontal] {
        box.computed_size[Axis.Horizontal] = 0

        iter := BoxIter { box.first, 0 }
        for child in iterate_box(&iter) {
            if .Floating in child.flags { continue }

            switch box.axis {
                case .Horizontal: {
                    box.computed_size[Axis.Horizontal] += child.computed_size[Axis.Horizontal]
                }
                case .Vertical: {
                    if child.computed_size[Axis.Horizontal] > box.computed_size[Axis.Horizontal] {
                        box.computed_size[Axis.Horizontal] = child.computed_size[Axis.Horizontal]
                    }
                }
            }
        }
    }
    if post_compute_size[Axis.Vertical] {
        box.computed_size[Axis.Vertical] = 0

        iter := BoxIter { box.first, 0 }
        for child in iterate_box(&iter) {
            if .Floating in child.flags { continue }

            switch box.axis {
                case .Horizontal: {
                    if child.computed_size[Axis.Vertical] > box.computed_size[Axis.Vertical] {
                        box.computed_size[Axis.Vertical] = child.computed_size[Axis.Vertical]
                    }
                }
                case .Vertical: {
                    box.computed_size[Axis.Vertical] += child.computed_size[Axis.Vertical]
                }
            }
        }
    }
}

push_clip :: proc(ctx: ^Context, pos: [2]int, size: [2]int, inside_parent: bool = true) {
    context.allocator = ctx.temp_allocator

    rect := Rect { pos, size }

    if len(ctx.clips) > 0 && inside_parent {
        parent_rect := ctx.clips[len(ctx.clips)-1]

        if rect.pos.x >= parent_rect.pos.x &&
            rect.pos.y >= parent_rect.pos.y &&
            rect.pos.x < parent_rect.pos.x + parent_rect.size.x &&
            rect.pos.y < parent_rect.pos.y + parent_rect.size.y
        {
            //rect.pos.x = math.max(rect.pos.x, parent_rect.pos.x);
            //rect.pos.y = math.max(rect.pos.y, parent_rect.pos.y);

            rect.size.x = math.min(rect.pos.x + rect.size.x, parent_rect.pos.x + parent_rect.size.x)
            rect.size.y = math.min(rect.pos.y + rect.size.y, parent_rect.pos.y + parent_rect.size.y)

            rect.size.x -= rect.pos.x
            rect.size.y -= rect.pos.y
        } else {
            rect = parent_rect
        }
    }

    // sdl2.RenderSetClipRect(ctx.renderer, &sdl2.Rect {
    //     i32(rect.pos.x),
    //     i32(rect.pos.y),
    //     i32(rect.size.x),
    //     i32(rect.size.y)
    // });

    append(&ctx.clips, rect)
}

pop_clip :: proc(ctx: ^Context) {
    context.allocator = ctx.temp_allocator

    if len(ctx.clips) > 0 {
        pop(&ctx.clips)

        // sdl2.RenderSetClipRect(ctx.renderer, &sdl2.Rect {
        //     i32(rect.pos.x),
        //     i32(rect.pos.y),
        //     i32(rect.size.x),
        //     i32(rect.size.y)
        // });
    } else {
        // sdl2.RenderSetClipRect(ctx.renderer, nil);
    }
}

draw :: proc(ctx: ^Context, /* state: ^core.State, */ font_width: int, font_height: int, box: ^Box) {
    context.allocator = ctx.temp_allocator

    if box == nil { return }

    push_clip(ctx, box.computed_pos, box.computed_size, !(.Floating in box.flags))
    {
        defer pop_clip(ctx)

        if .Hoverable in box.flags && box.hot > 0 {
            // core.draw_rect_blend(
            //     state,
            //     box.computed_pos.x,
            //     box.computed_pos.y,
            //     box.computed_size.x,
            //     box.computed_size.y,
            //     .Background1,
            //     .Background2,
            //     f32(math.min(box.hot, 20))/20.0
            // );

            toolkit.fill_rect(
                i32(box.computed_pos.x),
                i32(box.computed_pos.y),
                i32(box.computed_size.x),
                i32(box.computed_size.y),
                "#504945"
            )
        } else if .DrawBackground in box.flags {
            // core.draw_rect(
            //     state,
            //     box.computed_pos.x,
            //     box.computed_pos.y,
            //     box.computed_size.x,
            //     box.computed_size.y,
            //     .Background1
            // );

            toolkit.fill_rect(i32(box.computed_pos.x), i32(box.computed_pos.y), i32(box.computed_size.x), i32(box.computed_size.y), "#282828")
        }

        if .DrawText in box.flags {
            // core.draw_text(state, box.label, box.computed_pos.x, box.computed_pos.y);

            toolkit.fill_text(i32(box.computed_pos.x), i32(box.computed_pos.y), box.label, "#fbf1c7")
        }

        if .CustomDrawFunc in box.flags && box.custom_draw_func != nil {
            box.custom_draw_func(/* state, */ box, box.user_data)
        }

        iter := BoxIter { box.first, 0 }
        for child in iterate_box(&iter) {
            draw(ctx, /* state, */ font_width, font_height, child)
        }
    }

    push_clip(ctx, box.computed_pos, box.computed_size)
    defer pop_clip(ctx)

    if .DrawBorder in box.flags {
        // core.draw_rect_outline(
        //     state,
        //     box.computed_pos.x,
        //     box.computed_pos.y,
        //     box.computed_size.x,
        //     box.computed_size.y,
        //     .Background4
        // );

        toolkit.outline_rect(
            i32(box.computed_pos.x),
            i32(box.computed_pos.y),
            i32(box.computed_size.x),
            i32(box.computed_size.y),
            "#504945"
        )
    }
}

BoxIter :: struct {
    box: ^Box,
    index: int,
}

iterate_box :: proc(iter: ^BoxIter, print: bool = false) -> (box: ^Box, idx: int, cond: bool) {
    if iter.box == nil {
        return nil, iter.index, false
    }

    box = iter.box
    idx = iter.index

    iter.box = iter.box.next
    iter.index += 1

    return box, iter.index, true
}

debug_print :: proc(ctx: ^Context, box: ^Box, depth: int = 0) {
    context.allocator = ctx.temp_allocator

    /*
    iter := BoxIter { box.first, 0 };

    for box, idx in iterate_box(&iter, true) {
        for _ in 0..<(depth*6) {
            fmt.print("-");
        }
        if depth > 0 {
            fmt.print(">");
        }
        fmt.println(idx, "Box _", box.label, "#", box.key.label, "ptr", transmute(rawptr)box); //, "_ first", transmute(rawptr)box.first, "parent", transmute(rawptr)box.parent, box.computed_size);
        debug_print(ctx, box, depth+1);
    }

    if depth == 0 {
        fmt.println("persistent");
        for p in ctx.persistent {
            fmt.println(p);
        }
    }
    */
}

spacer :: proc(ctx: ^Context, label: string, flags: bit_set[Flag] = {}, semantic_size: [2]SemanticSize = {{.Fill, 0}, {.Fill,0}}) -> Interaction {
    context.allocator = ctx.temp_allocator

    box, interaction := push_box(ctx, label, flags, semantic_size = semantic_size)

    return interaction
}

push_floating :: proc(ctx: ^Context, label: string, pos: [2]int, flags: bit_set[Flag] = {.Floating}, axis: Axis = .Horizontal, semantic_size: [2]SemanticSize = Fill) -> (^Box, Interaction) {
    context.allocator = ctx.temp_allocator

    box, interaction := push_box(ctx, label, flags, semantic_size = semantic_size)
    box.computed_pos = pos

    return box, interaction
}

push_rect :: proc(ctx: ^Context, label: string, background: bool = true, border: bool = true, axis: Axis = .Vertical, semantic_size: [2]SemanticSize = Fill) -> (^Box, Interaction) {
    context.allocator = ctx.temp_allocator

    return push_box(ctx, label, {.DrawBackground if background else nil, .DrawBorder if border else nil}, axis, semantic_size = semantic_size)
}

label :: proc(ctx: ^Context, label: string) -> Interaction {
    context.allocator = ctx.temp_allocator

    box, interaction := push_box(ctx, label, {.DrawText})

    return interaction
}

button :: proc(ctx: ^Context, label: string) -> Interaction {
    context.allocator = ctx.temp_allocator

    return advanced_button(ctx, label)
}

advanced_button :: proc(
    ctx: ^Context,
    label: string,
    flags: bit_set[Flag] = {.Clickable, .Hoverable, .DrawText, .DrawBorder, .DrawBackground},
    semantic_size: [2]SemanticSize = FitText) -> Interaction
{
    context.allocator = ctx.temp_allocator

    box, interaction := push_box(ctx, label, flags, semantic_size = semantic_size)

    return interaction
}

custom :: proc(ctx: ^Context, label: string, draw_func: CustomDrawFunc, user_data: rawptr) -> Interaction {
    context.allocator = ctx.temp_allocator

    box, interaction := push_box(ctx, label, {.CustomDrawFunc}, semantic_size = { make_semantic_size(.Fill), make_semantic_size(.Fill) })
    box.custom_draw_func = draw_func
    box.user_data = user_data

    return interaction
}
