package wasm3

import "core:mem"

foreign import wasm3 "system:wasm3"

Environment :: struct {}
Runtime :: struct {}
Module :: struct {}
Function :: struct {}
ImportContext :: struct {}

Result :: distinct cstring

RawFunction :: proc "c" (runtime: ^Runtime, ctx: ^ImportContext, stack_pointer: ^u64, mem: rawptr)

@(link_prefix="m3_")
@(default_calling_convention="c")
foreign wasm3 {
    NewEnvironment :: proc() -> ^Environment ---
    NewRuntime :: proc(env: ^Environment, stack_size: u32, userdata: rawptr) -> ^Runtime ---

    ParseModule :: proc(env: ^Environment, module: ^^Module, wasm_bytes: []byte, num_wasm_bytes: u32) -> Result ---
    LoadModule :: proc(runtime: ^Runtime, module: ^Module) -> Result ---

    LinkWASI :: proc(module: ^Module) -> Result ---
    LinkLibC :: proc(module: ^Module) -> Result ---
    RunStart :: proc(module: ^Module) -> Result ---

    FindFunction :: proc(function: ^^Function, runtime: ^Runtime, function_name: cstring) -> Result ---

    LinkRawFunction :: proc(module: ^Module, module_name: cstring, function_name: cstring, function_signature: cstring, function: RawFunction) -> Result ---

    CallV :: proc(function: ^Function, args: ..any) -> Result ---
    CallArgv :: proc(function: ^Function, argc: u32, argv: []cstring) -> Result ---
}

GetArg :: proc ($T: typeid, stack_pointer: ^^u64) -> T {
    data_ptr := cast(^T)(stack_pointer^)
    value := data_ptr^

    stack_pointer^ = mem.ptr_offset(stack_pointer^, size_of(T))

    return value
}
