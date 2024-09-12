package editor

import "core:mem"
import "core:slice"

import tk "../toolkit"

MAX_GLYPH_BUFFER_WIDTH :: 1024
MAX_GLYPH_BUFFER_HEIGHT :: 1024
MAX_INPUT_BUFFER_SIZE :: 1024

File_Buffer :: struct {
    allocator: mem.Allocator,

    file_info: File_Info,
    content: Piece_Table,
    input_buffer: [MAX_INPUT_BUFFER_SIZE]u8,

    glyph_buffer_width: i32,
    glyph_buffer_height: i32,
    glyph_buffer: []Glyph,

    cursor: Cursor,
}

File_Info :: struct {
    virtual_directory: string,
    virtual_path: string,
    extension: string,
}

Glyph :: struct {
    codepoint: u8,

    // TODO
    // color: Palette_Color,
}

Cursor :: struct {
    index: Piece_Table_Index,
    cached_line: int,
    cached_col: int,
}

new_file_buffer :: proc(allocator: mem.Allocator, file_info: File_Info = File_Info{}) -> File_Buffer {
    context.allocator = allocator

    return File_Buffer{
        allocator = allocator,
        file_info = file_info,
        content = new_piece_table(allocator),
        input_buffer = [1024]u8{},
        glyph_buffer_width = 1024,
        glyph_buffer_height = 1024,
        glyph_buffer = make([]Glyph, MAX_GLYPH_BUFFER_WIDTH * MAX_GLYPH_BUFFER_HEIGHT),
        cursor = Cursor{},
    }
}

draw_buffer :: proc(file_buffer: ^File_Buffer, x, y, num_cols, num_rows, font_width, font_height: i32) {
    file_buffer.glyph_buffer_width = num_cols if num_cols <= MAX_GLYPH_BUFFER_WIDTH else MAX_GLYPH_BUFFER_WIDTH
    file_buffer.glyph_buffer_height = num_rows if num_rows <= MAX_GLYPH_BUFFER_HEIGHT else MAX_GLYPH_BUFFER_HEIGHT
    update_glyph_buffer(file_buffer)

    begin := 0

    for row in 0..<file_buffer.glyph_buffer_height {
        text_y := y + font_height * row

        for col in 0..<file_buffer.glyph_buffer_width {
            text_x := x + font_width * col
            glyph := file_buffer.glyph_buffer[col + row * file_buffer.glyph_buffer_width]

            if glyph.codepoint == 0 { break }

            // TODO: color
            tk.fill_codepoint(text_x, text_y, glyph.codepoint)
        }
    }
}

update_glyph_buffer :: proc(file_buffer: ^File_Buffer) {
    for &glyph in file_buffer.glyph_buffer {
        glyph = Glyph{}
    }

    begin: i32 = 0
    drawn_col: i32 = 0
    drawn_row: i32 = 0

    it := new_piece_table_iterator(&file_buffer.content, Piece_Table_Index{})
    for character in iterate_piece_table(&it) {
        if character == '\r' { continue }

        screen_row := drawn_row - begin
        if drawn_row >= begin && drawn_row >= file_buffer.glyph_buffer_height { break }

        if character == '\n' {
            drawn_col = 0
            drawn_row += 1
            continue
        }

        if drawn_row >= begin && drawn_col < file_buffer.glyph_buffer_width {
            file_buffer.glyph_buffer[drawn_col + screen_row * file_buffer.glyph_buffer_width] = Glyph{
                codepoint = character,
            }
        }

        drawn_col += 1
    }
}
