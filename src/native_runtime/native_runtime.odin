package main

import "base:runtime"
import "core:fmt"
import "core:os"
import "core:path/filepath"

import stbtt "vendor:stb/truetype";

import wasm3 "./wasm3"
import "gfx"

gfx_cx: ^gfx.cx = nil

load_wasm_source :: proc(file_path: string) -> ([]byte, string) {
    fd, err := os.open(file_path)
    if err != os.ERROR_NONE {
        return nil, fmt.aprintf("failed to open file: errno=%x", err)
        // return FileBuffer{}, make_error(ErrorType.FileIOError, fmt.aprintf("failed to open file: errno=%x", err))
    }
    defer os.close(fd)

    fi, fstat_err := os.fstat(fd)
    if fstat_err != os.ERROR_NONE {
        return nil, fmt.aprintf("failed to read file: errno=%x", fstat_err)
        // return FileBuffer{}, make_error(ErrorType.FileIOError, fmt.aprintf("failed to get file info: errno=%x", fstat_err))
    }

    if original_content, success := os.read_entire_file_from_handle(fd); success {
        return original_content, ""
    }

    return nil, "failed to read entire file from handle"
}

main :: proc() {
    env := wasm3.NewEnvironment()
    rt := wasm3.NewRuntime(env, 8192, nil)
    module: ^wasm3.Module = nil

    wasm_bytes, err := load_wasm_source("bin/app.wasm")
    if err != "" {
        fmt.println(err)
        os.exit(1)
    }

    if result := wasm3.ParseModule(env, &module, wasm_bytes, u32(len(wasm_bytes))); result != nil {
        fmt.println("ParseModule:", result)
        os.exit(1)
    }
    if result := wasm3.LoadModule(rt, module); result != nil {
        fmt.println("LoadModule:", result)
        os.exit(1)
    }

    if result := wasm3.LinkLibC(module); result != nil {
        fmt.println("LinkWASI:", result)
        os.exit(1)
    }

    if result := wasm3.LinkWASI(module); result != nil {
        fmt.println("LinkWASI:", result)
        os.exit(1)
    }

    // if result := wasm3.LinkRawFunction(module, "testing", "_internal_quit", "v(i)", m3_quit); result != nil {
    //     fmt.println("LinkRawFunction:", result)
    //     os.exit(1)
    // }

    // if result := wasm3.RunStart(module); result != nil {
    //     fmt.println("RunStart:", result)
    //     os.exit(1)
    // }

    func: ^wasm3.Function = nil
    if result := wasm3.FindFunction(&func, rt, "_start"); result != nil {
        fmt.println("FindFunction:", result)
        os.exit(1)
    }

    if result := wasm3.CallArgv(func, 0, nil); result != nil {
        fmt.println("CallArgv:", result)
        os.exit(1)
    }

    // init_gfx(frame_func, 800, 600)

    // for gfx.keep_running {
    //     gfx.run_events(gfx_cx)
    // }
}

init_gfx :: proc(frame_func: gfx.GfxFrameFunc, width: u32, height: u32) -> ^gfx.cx {
    gfx_cx = gfx.init_context(frame_func, 800, 600)

    font_data, success := os.read_entire_file_from_filename("./bin/DepartureMono-Regular.otf");
    if !success {
        fmt.eprintln("failed to read font file");
        os.exit(1);
    }

    font: stbtt.fontinfo;
    if stbtt.InitFont(&font, raw_data(font_data), 0) == false {
        fmt.eprintln("failed to init font");
        os.exit(1);
    }

    bitmap_size: i32 = 512;
    bitmap := make([]u8, bitmap_size * bitmap_size);

    rasterized_font_height: i32 = 24;

    ascent, descent, line_gap: i32;
    scale := stbtt.ScaleForPixelHeight(&font, f32(rasterized_font_height * 2));
    stbtt.GetFontVMetrics(&font, &ascent, &descent, &line_gap);

    gfx.add_glyph(gfx_cx, gfx.GpuGlyph {
        size = {f32(rasterized_font_height/4), 1},
        y_offset = f32(-rasterized_font_height),
    });

    x := rasterized_font_height / 4;
    y: i32 = 0;
    for i in 33..<33+96 {
        width, height, xoff, yoff: i32;
        glyph_bitmap := stbtt.GetCodepointBitmap(
            &font, scale, scale, rune(i), &width, &height, &xoff, &yoff);

        if (x + width) >= bitmap_size {
            x = 0;
            y += i32(f32(ascent - descent + line_gap) * scale);
        }

        xxx: i32 = x;
        for xx in 0..<width {
            yyy: i32 = y;
            for yy in 0..<height {
                bitmap[xxx + yyy * bitmap_size] =
                    glyph_bitmap[xx + yy * width];

                yyy += 1;
            }

            xxx += 1;
        }

        gfx.add_glyph(gfx_cx,
            gfx.GpuGlyph {
                atlas_position = {f32(x), f32(y)},
                size = {f32(width), f32(height)},
                position = {f32(x), f32(y)},
                y_offset = f32(yoff),
            }
        );

        x += width;
    }
    gfx.push_texture_buffer(gfx_cx, u32(bitmap_size), u32(bitmap_size), raw_data(bitmap), u32(bitmap_size * bitmap_size));

    return gfx_cx
}

frame_func :: proc "c" (mouse_x: int, mouse_y: int, mouse_left_down: bool, mouse_right_down: bool) {
    context = runtime.default_context()

    for y in 0..<100 {
        for x in 0..<100 {
            gfx.queue_text(gfx_cx, "Helope!", { f32(x)*100, f32(x)*24 + f32(y)*26}, 1000, 1000, { 1, 1, 1, 1 });
        }
    }
}

m3_quit :: proc "c" (wasm_runtime: ^wasm3.Runtime, ctx: ^wasm3.ImportContext, sp: ^u64, mem: rawptr) {
    context = runtime.default_context()
    fmt.println("m3_quit")

    sp := sp

    code := wasm3.GetArg(u32, &sp)
    fmt.println("Quit with code: ", code)
    os.exit(int(code))
}
