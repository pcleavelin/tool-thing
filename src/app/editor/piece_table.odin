package editor

import "core:mem"
import "core:slice"

Piece_Table :: struct {
    original: [dynamic]u8,
    added: [dynamic]u8,
    pieced_content: [dynamic]Content_Slice,
}

Content_Slice :: struct {
    type: Content_Slice_Type,
    start: int,
    end: int,
}

Content_Slice_Type :: enum {
    Original,
    Added
}

Piece_Table_Iterator :: struct {
    cursor: Piece_Table_Index,
    piece_table: ^Piece_Table,
    hit_end: bool,
}

Piece_Table_Index :: struct {
    piece_index: int,
    slice_index: int,
}

new_piece_table :: proc(allocator: mem.Allocator) -> Piece_Table {
    context.allocator = allocator

    return Piece_Table{
        original = slice.clone_to_dynamic([]u8{'\n'}),
        added = make([dynamic]u8, 0, 1024*1024),
        pieced_content = make([dynamic]Content_Slice, 0, 1024*1024),
    }
}

insert_content_string :: proc(piece_table: ^Piece_Table, cursor: Piece_Table_Index, to_be_inserted: string) {
    insert_content_bytes(piece_table, cursor, transmute([]u8)to_be_inserted)
}

insert_content_bytes :: proc(piece_table: ^Piece_Table, cursor: Piece_Table_Index, to_be_inserted: []u8) {
    cursor := cursor

    if len(to_be_inserted) == 0 {
        return
    }

    if len(piece_table.pieced_content) == 0 {
        cursor.piece_index = 0
        cursor.slice_index = 0
    } else if !cursor_within_bounds(cursor, piece_table) {
        return
    }

    it := new_piece_table_iterator(piece_table, cursor)

    length := append(&piece_table.added, ..to_be_inserted)
    inserted_slice := Content_Slice{
        type = .Added,
        start = len(piece_table.added) - length,
        end = len(piece_table.added),
    }

    if it.cursor.slice_index == 0 {
        inject_at(&piece_table.pieced_content, cursor.piece_index, inserted_slice)
    } else {
        end_slice := piece_table.pieced_content[cursor.piece_index]
        end_slice.start = it.cursor.slice_index

        piece_table.pieced_content[cursor.piece_index].end = it.cursor.slice_index

        inject_at(&piece_table.pieced_content, cursor.piece_index + 1, inserted_slice)
        inject_at(&piece_table.pieced_content, cursor.piece_index + 2, end_slice)
    }
}

insert_content :: proc{insert_content_string, insert_content_bytes}

new_piece_table_iterator_from_beginning :: proc(piece_table: ^Piece_Table) -> Piece_Table_Iterator {
    return Piece_Table_Iterator{
        cursor = Piece_Table_Index{
            piece_index = 0,
            slice_index = 0,
        },
        piece_table = piece_table,
        hit_end = false,
    }
}

new_piece_table_iterator_from_cursor :: proc(piece_table: ^Piece_Table, cursor: Piece_Table_Index) -> Piece_Table_Iterator {
    return Piece_Table_Iterator{
        cursor = cursor,
        piece_table = piece_table,
        hit_end = false,
    }
}

new_piece_table_iterator :: proc{new_piece_table_iterator_from_beginning, new_piece_table_iterator_from_cursor}

iterate_piece_table :: proc(it: ^Piece_Table_Iterator) -> (char: u8, index: Piece_Table_Index, cont: bool) {
    if it.hit_end || !cursor_within_bounds(it.cursor, it.piece_table) {
        return
    }

    cont = true
    index = it.cursor
    content_slice := it.piece_table.pieced_content[it.cursor.piece_index]
    switch content_slice.type {
        case .Original: {
            char = it.piece_table.original[content_slice.start + it.cursor.slice_index]
        }
        case .Added: {
            char = it.piece_table.added[content_slice.start + it.cursor.slice_index]
        }
    }

    it.cursor.slice_index += 1
    if it.cursor.slice_index >= content_slice_len(content_slice) {
        it.cursor.slice_index = 0
        it.cursor.piece_index += 1

        if it.cursor.piece_index >= len(it.piece_table.pieced_content) {
            it.hit_end = true
        }
    }

    return char, index, cont
}

cursor_within_bounds :: proc(cursor: Piece_Table_Index, piece_table: ^Piece_Table) -> bool {
    if cursor.piece_index < len(piece_table.pieced_content) {
        content_slice := piece_table.pieced_content[cursor.piece_index]

        if cursor.slice_index < content_slice_len(content_slice) {
            return true
        }
    }

    return false
}

content_slice_len :: proc(content_slice: Content_Slice) -> int {
    return content_slice.end - content_slice.start
}

