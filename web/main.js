// wait for the DOM to be loaded
document.addEventListener("DOMContentLoaded", function() {
    console.log("DOM loaded");
    const canvas = document.getElementById("canvas");
    const ctx = canvas.getContext("2d");

    const text_canvas = document.createElement("canvas");
    const text_ctx = text_canvas.getContext("2d");
    text_canvas.width = 94 * 2 * 15
    text_canvas.height = 24 + 7

    const font = new FontFace("Departure Mono", "url(/departure-mono.woff)");
    document.fonts.add(font);
    font.load();

    document.fonts.ready.then(() => {
        // Generate font texture on canvas
        text_ctx.font = "24px Departure Mono";
        text_ctx.fillStyle = "grey";
        console.log(text_ctx.measureText("A"))
        // text_ctx.fillStyle = "#fbf1c7";
        text_ctx.fillText("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, 24);
        // text_ctx.fillText("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 0, 24);
    });

    // const text_texture = text_canvas.transferToImageBitmap();

    mouse_x = 0;
    mouse_y = 0;
    mouse_left_down = false;
    mouse_right_down = false;

    window.addEventListener("resize", () => {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        canvas.style.width = window.innerWidth + "px";
        canvas.style.height = window.innerHeight + "px";
    });

    canvas.addEventListener("mousemove", (event) => {
        event.preventDefault();

        mouse_x = event.clientX - canvas.offsetLeft;
        mouse_y = event.clientY - canvas.offsetTop;
    });

    canvas.addEventListener("mousedown", (event) => {
        event.preventDefault();

        mouse_left_down = true;
    });

    canvas.addEventListener("mouseup", (event) => {
        event.preventDefault();

        mouse_left_down = false;
    });

    const read_string = (instance, ptr, len) => {
        const bytes = new Uint8Array(instance.exports.memory.buffer, ptr, len);
        return new TextDecoder("utf-8").decode(bytes);
    };

    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;

    let instance;
    const importObject = {
        odin_env: { write: (fd, ptr, len) => {
                const view = new DataView(instance.exports.memory.buffer);

                const string = read_string(instance, ptr, len);
                
                if (fd == 1) {
                    console.log(string);
                } else if (fd == 2) {
                    console.error(string);
                } else { 
                    throw new Error("Invalid fd to 'write'" + stripNewline(string));
                }
        } },
        wasi_snapshot_preview1: {
            fd_write: (fd, iovs, iovs_len, nwritten) => {
                const view = new DataView(instance.exports.memory.buffer);

                for (let i = 0; i < iovs_len; i++) {
                    const item_base = iovs + i * 8;
                    const vector_pointer = view.getUint32(item_base, true);
                    const vector_len = view.getUint32(item_base+4, true);

                    let string = "";
                    for (let i = 0; i < vector_len; i++) {
                        string += String.fromCharCode(view.getUint8(vector_pointer + i, true));
                    }

                    if (string.length > 0 && string[0] != '\n') {
                        console.log(string);
                    }
                }
            },
            args_get: (a, b) => { return 0; },
            args_sizes_get: (a, b) => { return 0; },
            // fd_close: (fd) => { return 0; },
            fd_filestat_get: (a, b) => { return 1; },
            // fd_pread: (a, b, c, d, e) => { return 0; },
            fd_prestat_dir_name: (a, b, c) => { return 1; },
            fd_prestat_get: (a, b) => { return 0; },
            // fd_pwrite: (a, b, c, d, e) => { return 0; },
            // fd_read: (a, b, c, d) => { return 0; },
            // fd_seek: (a, b, c, d) => { return 0; },
            // fd_write: (a, b, c, d) => { return 0; },
        },
        toolkit: {
            super_test: (ptr, len) => {
                const view = new DataView(instance.exports.memory.buffer);

                let string = "";
                for (let i = 0; i < len; i++) {
                    string += String.fromCharCode(view.getUint8(ptr + i, true));
                }

                document.getElementById("file-upload-button").innerHTML = string;
            }
        },
        toolkitUi: {
            fill_rect: (x, y, width, height, color_ptr, color_len) => {
                const color = read_string(instance, color_ptr, color_len);
                // console.log("x:", x, "y:", y, "width:", width, "height:", height, "color:", color);

                ctx.fillStyle = color;
                ctx.fillRect(x, y, width, height);
            },
            outline_rect: (x, y, width, height, color_ptr, color_len) => {
                const color = read_string(instance, color_ptr, color_len);
                // console.log("x:", x, "y:", y, "width:", width, "height:", height, "color:", color);

                ctx.fillStyle = color;
                ctx.lineWidth = 1;
                ctx.strokeRect(x, y, width, height);
            },
            fill_text: (x, y, text_ptr, text_len, color_ptr, color_len) => {
                const color = read_string(instance, color_ptr, color_len);
                const text = read_string(instance, text_ptr, text_len);
                // console.log("x:", x, "y:", y, "text:", text, "color:", color);

                 ctx.font = "24px Departure Mono";
                 ctx.fillStyle = color;
                 ctx.fillText(text, x, y+24);
                // drawImage(image, sx, sy, sWidth, sHeight, dx, dy, dWidth, dHeight)
                // ctx.drawImage(text_canvas, 0, 0, 15, 24, x, y, 15, 24);
            },
            fill_codepoint: (x, y, codepoint) => {
                c = codepoint - 33
                ctx.drawImage(text_canvas, c * 15.272720336914062, 0, 15, 24 + 7, x, y, 15, 24+7);
            },
        }
    };

    render = () => {
        instance.exports.present(mouse_x, mouse_y, mouse_left_down, mouse_right_down, canvas.width, canvas.height);

        requestAnimationFrame(render);
    };

    wasm_promise = fetch("/wasm")

    instance = WebAssembly.instantiateStreaming(wasm_promise, importObject).then((result) => {
        console.log("WebAssembly module instantiated");

        instance = result.instance;
        console.log(instance);

        instance.exports._start();
        render();
    });
});
