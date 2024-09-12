package toolkit

foreign import toolkit "toolkit"
foreign import tk_ui "toolkitUi"

@(private)
@(default_calling_convention="contextless")
foreign toolkit {
}

@(private)
@(default_calling_convention="contextless")
foreign tk_ui {
	@(link_name="fill_rect")
	_fill_rect :: proc(x: i32, y: i32, width: i32, height: i32, color_ptr: []byte) ---
	@(link_name="outline_rect")
	_outline_rect :: proc(x: i32, y: i32, width: i32, height: i32, color_ptr: []byte) ---

	@(link_name="fill_text")
	_fill_text :: proc(x: i32, y: i32, text_ptr: []byte, color_ptr: []byte) ---

	@(link_name="fill_codepoint")
	_fill_codepoint :: proc(x: i32, y: i32, codepoint: u8) ---
}

fill_rect :: proc(x: i32, y: i32, width: i32, height: i32, color: string) {
	_fill_rect(x, y, width, height, transmute([]byte)color)
}

outline_rect :: proc(x: i32, y: i32, width: i32, height: i32, color: string) {
	_outline_rect(x, y, width, height, transmute([]byte)color)
}

fill_text :: proc(x: i32, y: i32, text: string, color: string) {
	_fill_text(x, y, transmute([]byte)text, transmute([]byte)color)
}

fill_codepoint :: proc(x: i32, y: i32, codepoint: u8) {
	_fill_codepoint(x, y, codepoint)
}
