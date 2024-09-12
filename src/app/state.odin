package main

import "ui"
import "editor"

AppState :: struct {
    ui: ui.Context,

    test_file_buffer: editor.File_Buffer,
    // TODO:
    // plugins: map[string]Plugin,
}
