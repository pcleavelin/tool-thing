all: app native_runtime

# TODO: be platform agnostic
native_runtime: wasm3_source src/native_runtime/*.odin thirdparty/pcleavelin/*.h compile_shaders_metal
	clang -O0 -g -c -ObjC -Wall -Wextra -D_FONT_WIDTH=12 -D_FONT_HEIGHT=24 -DED_GFX_IMPLEMENTATION thirdparty/pcleavelin/gfx.h -o lib/libgfx.a
	odin build src/native_runtime/ -out:bin/runtime -extra-linker-flags="-L./lib -framework Cocoa -framework QuartzCore -framework CoreImage -framework Metal"

# TODO: be platform agnostic
compile_shaders_metal:
	mkdir -p bin/compiled_shaders
	xcrun -sdk macosx metal -o bin/compiled_shaders/text_atlas.ir -c shaders/text_atlas.metal
	xcrun -sdk macosx metallib -o bin/shaders.metallib bin/compiled_shaders/text_atlas.ir

app: src/app/*.odin
	odin build src/app/ -target:js_wasm32 -out:bin/app.wasm

wasm3_source: thirdparty/wasm3/source/*.c
	clang -O3 -g0 -Ithirdparty/wasm3/source -DDEBUG -Dd_m3HasTracer -Dd_m3RecordBacktraces -Dd_m3HasWASI -shared thirdparty/wasm3/source/*.c -o lib/libwasm3.so
