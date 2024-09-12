// Graphics layer abstraction.

#ifndef ED_GFX_INCLUDED
#define ED_GFX_INCLUDED
#include <stdint.h>

#include "ed_array.h"
#include "string.h"

bool keep_running = true;

#if defined(__APPLE__)
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <QuartzCore/CoreAnimation.h>
#include <QuartzCore/QuartzCore.h>
@interface EDGFXView : NSView
@property NSTrackingArea *tracking_area;
@end
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end
@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

#define wrapIdArray(T)                                                         \
  typedef id<T> _##T;                                                          \
  arrayTemplate(_##T);

wrapIdArray(MTLRenderPipelineState);
wrapIdArray(MTLBuffer);
wrapIdArray(MTLTexture);
#endif

typedef struct {
  float position[4];
  float size[4];
  float border_size[4];
  float color[4];
} GpuUiRect;
arrayTemplate(GpuUiRect);

typedef struct {
  float atlas_position[2];
  float size[2];
  float position[2];
  float y_offset;
  float _haha_alignment;
  float color[4];
} GpuGlyph;
arrayTemplate(GpuGlyph);

typedef struct {
  float screen_size[2];
  float font_size[2];
} GpuUniformParams;

#if defined(__APPLE__)
typedef struct {
  NSApplication *application;
  NSWindow *window;
  EDGFXView *view;
  bool keep_running;
  bool refresh_now;

  int mouse_x, mouse_y;
  bool mouse_left_down, mouse_right_down;

  // Metal objects
  id<MTLDevice> device;
  CAMetalLayer *metal_layer;
  id<MTLLibrary> library;
  id<MTLCommandQueue> command_queue;

  array(_MTLRenderPipelineState) pipelines;
  array(_MTLBuffer) buffers;
  array(_MTLTexture) textures;
} _metal_gfx_context;
#elif __linux__
#include "wayland-crap/xdg-shell.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "file_io.h"

// And I thought MacOS needed a lot of state to create a window
arrayTemplate(GLuint);
typedef struct {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_surface *surface;
  struct wl_compositor *compositor;
  struct wl_shm *shared_memory;
  struct wl_shm_pool *shared_memory_pool;
  struct xdg_wm_base *wm_base;
  struct wl_buffer *buffer;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  uint8_t *pixels;

  EGLDisplay egl_display;
  EGLConfig egl_config;
  EGLSurface egl_surface;
  EGLContext egl_context;
  struct wl_egl_window *egl_window;

  int mouse_x, mouse_y;
  int mouse_left_down, mouse_right_down;

  GLuint ui_rect_vertex_shader;
  GLuint ui_rect_fragment_shader;
  GLuint text_atlas_vertex_shader;
  GLuint text_atlas_fragment_shader;
  GLuint ui_rect_shader_program;
  GLuint text_atlas_shader_program;

  array(GLuint) buffers;
  array(GLuint) textures;
} _opengl_gfx_context_wayland;

static void _opengl_gfx_present_wayland(_opengl_gfx_context_wayland *cx);

typedef struct {
  Display *display;
  Window window;
  int screen;
} _opengl_gfx_context_x11;
#elif _WIN32
#include <GL\gl.h>
#include <opengl\glext.h>
#include <opengl\wglext.h>
#include <windows.h>

#define ED_FILE_IO_IMPLEMENTATION
#include "file_io.h"

static PFNGLGETSHADERIVPROC glShaderiv = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
static PFNGLDELETESHADERPROC glDeleteShader = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
static PFNGLCREATESHADERPROC glCreateShader = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader = NULL;
static PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
static PFNGLATTACHSHADERPROC glAttachShader = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
static PFNGLCREATETEXTURESPROC glCreateTextures = NULL;
static PFNGLTEXTUREPARAMETERIPROC glTextureParameteri = NULL;
static PFNGLTEXTURESTORAGE2DPROC glTextureStorage2D = NULL;
static PFNGLTEXTURESUBIMAGE2DPROC glTextureSubImage2D = NULL;
static PFNGLBINDTEXTUREUNITPROC glBindTextureUnit = NULL;
static PFNGLCREATEBUFFERSPROC glCreateBuffers = NULL;
static PFNGLNAMEDBUFFERSTORAGEPROC glNamedBufferStorage = NULL;
static PFNGLNAMEDBUFFERSUBDATAPROC glNamedBufferSubData = NULL;
static PFNGLBINDBUFFERBASEPROC glBindBufferBase = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
static PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = NULL;

arrayTemplate(GLuint);
typedef struct {
  GLuint ui_rect_vertex_shader;
  GLuint ui_rect_fragment_shader;
  GLuint text_atlas_vertex_shader;
  GLuint text_atlas_fragment_shader;
  GLuint ui_rect_shader_program;
  GLuint text_atlas_shader_program;

  array(GLuint) buffers;
  array(GLuint) textures;
} _opengl_gfx_context;

typedef struct {
  int mouse_x, mouse_y;
  int mouse_left_down, mouse_right_down;

  HWND window_handle;
  HDC device_context;

  _opengl_gfx_context gl;
} _win32_gfx_context;

static void _win32_gfx_present(_win32_gfx_context *cx);
#endif

typedef void (*_gfx_frame_func)(int mouse_x, int mouse_y, bool mouse_left_down,
                                bool mouse_right_down);
typedef struct {
#if defined(__APPLE__)
  _metal_gfx_context backend;
#elif __linux__
  // TODO: be able to use X11 or Wayland at runtime
  _opengl_gfx_context_wayland backend;
#elif _WIN32
  _win32_gfx_context backend;
#else
#error "Unsupported platform"
#endif

  uint32_t frame_width;
  uint32_t frame_height;
  _gfx_frame_func frame_func;

  array(GpuUiRect) gpu_ui_rects;
  array(GpuGlyph) gpu_glyphs;
  array(GpuGlyph) glyph_cache;
} gfx_context_t;
static gfx_context_t _gfx_context;

void gfx_queue_text(gfx_context_t *cx, string text, float position[2],
                    float max_x, float max_y, float color[4]);
void gfx_update_buffer(gfx_context_t *cx, size_t buffer_index, const void *data,
                       size_t len);
#ifdef ED_GFX_IMPLEMENTATION

#if defined(__APPLE__)
static void _metal_gfx_present(_metal_gfx_context *cx);
static void _metal_gfx_send_events(_metal_gfx_context *cx);

@implementation AppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication *)sender {
  keep_running = false;

  return NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  keep_running = false;

  return NO;
}
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSApplication *)sender {
  keep_running = false;
  return YES;
}

- (void)windowDidResize:(NSNotification *)notification {
  _gfx_context.frame_width =
      _gfx_context.backend.window.contentView.frame.size.width;
  _gfx_context.frame_height =
      _gfx_context.backend.window.contentView.frame.size.height;

  CGFloat scale = _gfx_context.backend.metal_layer.contentsScale;
  [_gfx_context.backend.metal_layer
      setDrawableSize:CGSizeMake(_gfx_context.frame_width * scale,
                                 _gfx_context.frame_height * scale)];

  _gfx_context.backend.refresh_now = true;
  _gfx_context.backend.view.needsDisplay = true;
}
@end

@implementation EDGFXView
- (BOOL)isOpaque {
  return YES;
}
- (void)updateLayer {
  _metal_gfx_present(&_gfx_context.backend);
}

- (BOOL)wantsLayer {
  return YES;
}
- (BOOL)wantsUpdateLayer {
  return YES;
}
- (NSViewLayerContentsRedrawPolicy)layerContentsRedrawPolicy {
  return NSViewLayerContentsRedrawOnSetNeedsDisplay;
}

- (id)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  NSRect rect =
      NSMakeRect(0, 0,
                 _gfx_context.frame_width =
                     _gfx_context.backend.window.contentView.frame.size.width,
                 _gfx_context.frame_height =
                     _gfx_context.backend.window.contentView.frame.size.height);

  self.tracking_area = [[NSTrackingArea alloc]
      initWithRect:rect
           options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                    NSTrackingActiveInKeyWindow)
             owner:self
          userInfo:nil];
  [self addTrackingArea:self.tracking_area];

  return self;
}

- (void)mouseDown:(NSEvent *)event {
  if ((unsigned long)event.type == NSEventTypeLeftMouseDown) {
    _gfx_context.backend.mouse_left_down = true;
  }
  if (event.type == NSEventTypeRightMouseUp) {
    _gfx_context.backend.mouse_right_down = true;
  }

  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(_gfx_context.backend.mouse_x,
                          _gfx_context.frame_height -
                              _gfx_context.backend.mouse_y,
                          _gfx_context.backend.mouse_left_down,
                          _gfx_context.backend.mouse_right_down);
  [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent *)event {
  if ((unsigned long)event.type == NSEventTypeLeftMouseUp) {
    _gfx_context.backend.mouse_left_down = false;
  }
  if (event.type == NSEventTypeRightMouseDown) {
    _gfx_context.backend.mouse_right_down = false;
  }

  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(_gfx_context.backend.mouse_x,
                          _gfx_context.frame_height -
                              _gfx_context.backend.mouse_y,
                          _gfx_context.backend.mouse_left_down,
                          _gfx_context.backend.mouse_right_down);
  [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent *)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];

  _gfx_context.backend.mouse_x = location.x;
  _gfx_context.backend.mouse_y = location.y;

  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(_gfx_context.backend.mouse_x,
                          _gfx_context.frame_height -
                              _gfx_context.backend.mouse_y,
                          _gfx_context.backend.mouse_left_down,
                          _gfx_context.backend.mouse_right_down);

  [self setNeedsDisplay:YES];
}

- (void)mouseMoved:(NSEvent *)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];

  _gfx_context.backend.mouse_x = location.x;
  _gfx_context.backend.mouse_y = location.y;

  [self setNeedsDisplay:YES];
  [self displayIfNeeded];
  [_gfx_context.backend.metal_layer setNeedsDisplay];
  [_gfx_context.backend.metal_layer displayIfNeeded];
}

- (void)updateTrackingAreas {
  [self removeTrackingArea:self.tracking_area];
  [self.tracking_area release];

  NSRect rect =
      NSMakeRect(0, 0,
                 _gfx_context.frame_width =
                     _gfx_context.backend.window.contentView.frame.size.width,
                 _gfx_context.frame_height =
                     _gfx_context.backend.window.contentView.frame.size.height);

  self.tracking_area = [[NSTrackingArea alloc]
      initWithRect:rect
           options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                    NSTrackingActiveInKeyWindow)
             owner:self
          userInfo:nil];
  [self addTrackingArea:self.tracking_area];
}
@end

static _metal_gfx_context _metal_gfx_init_context(uint32_t width,
                                                  uint32_t height) {
  NSApplication *application = [NSApplication sharedApplication];
  if (application == NULL) {
    fprintf(stderr, "NSApplication:sharedApplication failed\n");
    exit(1);
  }

  NSString *title = @"chat - [Slack Sux]";

  NSRect rect = NSMakeRect(0, 0, width, height);
  NSWindow *window = [[NSWindow alloc]
      initWithContentRect:rect
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable
                  backing:NSBackingStoreBuffered
                    defer:NO];
  EDGFXView *view = [[EDGFXView alloc] initWithFrame:rect];
  [view updateTrackingAreas];
  [window setTitle:title];
  [window setContentView:view];
  [window setDelegate:[[WindowDelegate alloc] init]];
  [window makeKeyAndOrderFront:NULL];

  // TODO: make this work
  // if (application.mainMenu == NULL) {
  //     NSMenu *menu = [[NSMenu alloc] initWithTitle:@"an_editor"];
  //     if (menu == NULL) {
  //         fprintf(stderr, "failed to create application menu\n");
  //         exit(1);
  //     }
  //     application.mainMenu = menu;
  // }
  [application setDelegate:[[AppDelegate alloc] init]];
  [application setActivationPolicy:NSApplicationActivationPolicyRegular];
  [application setPresentationOptions:NSApplicationPresentationDefault];
  [application finishLaunching];

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();

  CAMetalLayer *metal_layer = [CAMetalLayer layer];
  metal_layer.device = device;
  metal_layer.pixelFormat = MTLPixelFormatRGBA8Unorm;
  metal_layer.frame = CGRectMake(0, 0, width, height);
  metal_layer.needsDisplayOnBoundsChange = YES;
  metal_layer.presentsWithTransaction = YES;
  metal_layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;

  // TODO: set this to the display dpi scale
  metal_layer.contentsScale = 2.0;
  view.wantsLayer = YES;
  [view.layer addSublayer:metal_layer];
  view.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;

  NSError *libraryError = NULL;
  NSURL *libraryURL = [[NSBundle mainBundle] URLForResource:@"./shaders"
                                              withExtension:@"metallib"];
  if (libraryURL == NULL) {
    fprintf(stderr, "Couldn't find shaders library file\n");
    exit(1);
  }

  id<MTLLibrary> library =
      [device newLibraryWithURL:libraryURL error:&libraryError];

  if (library == NULL) {
    if (libraryError.description != NULL) {
      NSLog(@"Error description: %@\n", libraryError.description);
    }

    exit(1);
  }

  id<MTLCommandQueue> command_queue = [device newCommandQueue];
  id<MTLFunction> vertex_func = [library newFunctionWithName:@"vs_main"];
  id<MTLFunction> fragment_func = [library newFunctionWithName:@"fs_main"];
  id<MTLFunction> ui_rect_vertex_func =
      [library newFunctionWithName:@"ui_rect_vs"];
  id<MTLFunction> ui_rect_fragment_func =
      [library newFunctionWithName:@"ui_rect_fs"];

  MTLRenderPipelineDescriptor *pipeline_descriptor =
      [[MTLRenderPipelineDescriptor alloc] init];
  [pipeline_descriptor setVertexFunction:vertex_func];
  [pipeline_descriptor setFragmentFunction:fragment_func];
  pipeline_descriptor.colorAttachments[0].pixelFormat =
      MTLPixelFormatRGBA8Unorm;
  pipeline_descriptor.colorAttachments[0].alphaBlendOperation =
      MTLBlendOperationAdd;
  pipeline_descriptor.colorAttachments[0].sourceAlphaBlendFactor =
      MTLBlendFactorSourceAlpha;
  pipeline_descriptor.colorAttachments[0].destinationAlphaBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  pipeline_descriptor.colorAttachments[0].sourceRGBBlendFactor =
      MTLBlendFactorSourceAlpha;
  pipeline_descriptor.colorAttachments[0].destinationRGBBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  pipeline_descriptor.colorAttachments[0].blendingEnabled = true;

  MTLRenderPipelineDescriptor *ui_rect_pipeline_descriptor =
      [[MTLRenderPipelineDescriptor alloc] init];
  [ui_rect_pipeline_descriptor setVertexFunction:ui_rect_vertex_func];
  [ui_rect_pipeline_descriptor setFragmentFunction:ui_rect_fragment_func];
  ui_rect_pipeline_descriptor.colorAttachments[0].pixelFormat =
      MTLPixelFormatRGBA8Unorm;
  ui_rect_pipeline_descriptor.colorAttachments[0].alphaBlendOperation =
      MTLBlendOperationAdd;
  ui_rect_pipeline_descriptor.colorAttachments[0].sourceAlphaBlendFactor =
      MTLBlendFactorSourceAlpha;
  ui_rect_pipeline_descriptor.colorAttachments[0].destinationAlphaBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  ui_rect_pipeline_descriptor.colorAttachments[0].sourceRGBBlendFactor =
      MTLBlendFactorSourceAlpha;
  ui_rect_pipeline_descriptor.colorAttachments[0].destinationRGBBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  ui_rect_pipeline_descriptor.colorAttachments[0].blendingEnabled = true;

  NSError *pipeline_error = NULL;
  array(_MTLRenderPipelineState) pipelines =
      newArray(_MTLRenderPipelineState, 2);
  pushArray(_MTLRenderPipelineState, &pipelines,
            [device newRenderPipelineStateWithDescriptor:pipeline_descriptor
                                                   error:&pipeline_error]);
  if (pipeline_error != NULL) {
    if (pipeline_error.description != NULL) {
      NSLog(@"Error description: %@\n", pipeline_error.description);
    }

    exit(1);
  }
  pushArray(_MTLRenderPipelineState, &pipelines,
            [device
                newRenderPipelineStateWithDescriptor:ui_rect_pipeline_descriptor
                                               error:&pipeline_error]);

  array(_MTLBuffer) buffers = newArray(_MTLBuffer, 8);
  array(_MTLTexture) textures = newArray(_MTLTexture, 8);

  if (pipeline_error != NULL) {
    if (pipeline_error.description != NULL) {
      NSLog(@"Error description: %@\n", pipeline_error.description);
    }

    exit(1);
  }

  return (_metal_gfx_context){
      .application = application,
      .window = window,
      .view = view,
      .keep_running = true,

      .device = device,
      .metal_layer = metal_layer,
      .library = library,
      .command_queue = command_queue,
      .pipelines = pipelines,
      .buffers = buffers,
      .textures = textures,
  };
}

static void _metal_gfx_send_events(_metal_gfx_context *cx) {
  NSEvent *event = [cx->application nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];

  [cx->application sendEvent:event];
}

static void _metal_gfx_present(_metal_gfx_context *cx) {
  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(cx->mouse_x, _gfx_context.frame_height - cx->mouse_y,
                          cx->mouse_left_down, cx->mouse_right_down);

  if (_gfx_context.gpu_glyphs.size > 0) {
    gfx_update_buffer(&_gfx_context, 2, _gfx_context.gpu_glyphs.data,
                      _gfx_context.gpu_glyphs.size * sizeof(GpuGlyph));
  }
  if (_gfx_context.gpu_ui_rects.size > 0) {
    gfx_update_buffer(&_gfx_context, 4, _gfx_context.gpu_ui_rects.data,
                      _gfx_context.gpu_ui_rects.size * sizeof(GpuUiRect));
  }

  GpuUniformParams gpu_uniform_params = {
      .screen_size =
          {
              (float)_gfx_context.frame_width,
              (float)_gfx_context.frame_height,
          },
      .font_size =
          {
              (float)_FONT_WIDTH,
              (float)_FONT_HEIGHT,
          },
  };

  gfx_update_buffer(&_gfx_context, 3, &gpu_uniform_params,
                    sizeof(GpuUniformParams));

  @autoreleasepool {
    id<CAMetalDrawable> drawable = [cx->metal_layer nextDrawable];

    id<MTLCommandBuffer> command_buffer = [cx->command_queue commandBuffer];
    MTLRenderPassDescriptor *render_pass_desc =
        [MTLRenderPassDescriptor renderPassDescriptor];
    render_pass_desc.colorAttachments[0].texture = drawable.texture;
    render_pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
    render_pass_desc.colorAttachments[0].clearColor =
        MTLClearColorMake(0.1, 0.1, 0.1, 1);
    render_pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLRenderCommandEncoder> encoder =
        [command_buffer renderCommandEncoderWithDescriptor:render_pass_desc];

    if (_gfx_context.gpu_ui_rects.size > 0) {
      // UI Rects
      [encoder setRenderPipelineState:cx->pipelines.data[1]];
      // FIXME: allow these to be described by the user instead of
      // hardcoded
      [encoder setVertexBuffer:cx->buffers.data[0]
                        offset:0
                       atIndex:0]; // vertices
      [encoder setVertexBuffer:cx->buffers.data[4]
                        offset:0
                       atIndex:1]; // ui rects
      [encoder setVertexBuffer:cx->buffers.data[3]
                        offset:0
                       atIndex:2]; // uniforms
      [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                          indexCount:6
                           indexType:MTLIndexTypeUInt16
                         indexBuffer:cx->buffers.data[1]
                   indexBufferOffset:0
                       instanceCount:_gfx_context.gpu_ui_rects.size];
    }

    if (_gfx_context.gpu_glyphs.size > 0) {
      // UI Text
      [encoder setRenderPipelineState:cx->pipelines.data[0]];
      // FIXME: allow these to be described by the user instead of
      // hardcoded
      [encoder setVertexBuffer:cx->buffers.data[0]
                        offset:0
                       atIndex:0]; // vertices
      [encoder setVertexBuffer:cx->buffers.data[2]
                        offset:0
                       atIndex:1]; // glyph data
      [encoder setVertexBuffer:cx->buffers.data[3]
                        offset:0
                       atIndex:2]; // uniforms
      [encoder setFragmentTexture:cx->textures.data[0] atIndex:0];
      [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                          indexCount:6
                           indexType:MTLIndexTypeUInt16
                         indexBuffer:cx->buffers.data[1]
                   indexBufferOffset:0
                       instanceCount:_gfx_context.gpu_glyphs.size];
    }

    [encoder endEncoding];
    // FIXME: `afterMinimumDuration` causes the weird re-size scaling, but I
    // need to figure why the heck the NSView doesn't get the rendered
    // contents unless `afterMinimumDuration` is here
    if (cx->refresh_now) {
      [command_buffer presentDrawable:drawable];
      cx->refresh_now = false;
    } else {
      [command_buffer presentDrawable:drawable
                 afterMinimumDuration:1.0 / 144.0];
    }
    [command_buffer commit];

    [command_buffer waitUntilScheduled];
  }
}

static size_t _metal_gfx_push_texture_buffer(_metal_gfx_context *cx,
                                             uint32_t width, uint32_t height,
                                             const void *data, size_t len) {
  MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                   width:width
                                  height:height
                               mipmapped:false];
  _MTLTexture texture = [cx->device newTextureWithDescriptor:texture_desc];

  MTLRegion region = MTLRegionMake2D(0, 0, width, height);
  [texture replaceRegion:region
             mipmapLevel:0
                   slice:0
               withBytes:data
             bytesPerRow:width * sizeof(uint8_t)
           bytesPerImage:len];

  pushArray(_MTLTexture, &cx->textures, texture);
  return cx->textures.size - 1;
}

static void _metal_gfx_resize_texture_buffer(_metal_gfx_context *cx,
                                             uint32_t width,
                                             size_t texture_index,
                                             uint32_t height) {
  [cx->textures.data[texture_index] setPurgeableState:MTLPurgeableStateEmpty];
  [cx->textures.data[texture_index] release];

  MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                   width:width
                                  height:height
                               mipmapped:false];
  cx->textures.data[texture_index] =
      [cx->device newTextureWithDescriptor:texture_desc];
}

size_t _metal_gfx_push_vertex_buffer(_metal_gfx_context *cx, const void *data,
                                     size_t len) {
  pushArray(_MTLBuffer, &cx->buffers,
            [cx->device newBufferWithBytes:data
                                    length:len
                                   options:MTLResourceStorageModeShared]);

  return cx->buffers.size - 1;
}
static size_t _metal_gfx_allocate_vertex_buffer(_metal_gfx_context *cx,
                                                size_t len) {
  pushArray(_MTLBuffer, &cx->buffers,
            [cx->device newBufferWithLength:len
                                    options:MTLResourceStorageModeShared]);

  return cx->buffers.size - 1;
}
static void _metal_gfx_update_buffer(_metal_gfx_context *cx,
                                     size_t buffer_index, const void *data,
                                     size_t len) {
  void *buffer_contents = [cx->buffers.data[buffer_index] contents];

  // FIXME: actually check to see if this will fit in the buffer
  memcpy(buffer_contents, data, len);
}
#elif __linux__
#include <linux/input-event-codes.h>

static void _wayland_pointer_enter(void *data, struct wl_pointer *pointer,
                                   uint32_t serial, struct wl_surface *surface,
                                   wl_fixed_t x, wl_fixed_t y) {
  // fprintf(stderr, "pointer enter: %d, %d\n", x, y);
}
static void _wayland_pointer_leave(void *data, struct wl_pointer *pointer,
                                   uint32_t serial,
                                   struct wl_surface *surface) {
  // fprintf(stderr, "pointer leave\n");
}
static void _wayland_pointer_button(void *data, struct wl_pointer *pointer,
                                    uint32_t serial, uint32_t time,
                                    uint32_t button, uint32_t state) {
  _opengl_gfx_context_wayland *cx = data;

  if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
    if (button == BTN_LEFT) {
      cx->mouse_left_down = true;
    } else if (button == BTN_RIGHT) {
      cx->mouse_right_down = true;
    }
  } else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
    if (button == BTN_LEFT) {
      cx->mouse_left_down = false;
    } else if (button == BTN_RIGHT) {
      cx->mouse_right_down = false;
    }
  }
}
static void _wayland_pointer_axis(void *data, struct wl_pointer *pointer,
                                  uint32_t time, uint32_t axis,
                                  wl_fixed_t value) {}
static void _wayland_pointer_motion(void *data, struct wl_pointer *pointer,
                                    uint32_t time, wl_fixed_t x, wl_fixed_t y) {
  _opengl_gfx_context_wayland *cx = data;
  cx->mouse_x = wl_fixed_to_int(x);
  cx->mouse_y = wl_fixed_to_int(y);
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = _wayland_pointer_enter,
    .leave = _wayland_pointer_leave,
    .motion = _wayland_pointer_motion,
    .button = _wayland_pointer_button,
    .axis = _wayland_pointer_axis,
};

static void _wayland_xdg_toplevel_configure(void *data,
                                            struct xdg_toplevel *xdg_toplevel,
                                            int32_t width, int32_t height,
                                            struct wl_array *states) {}

static void _wayland_xdg_toplevel_close(void *data,
                                        struct xdg_toplevel *xdg_toplevel) {
  keep_running = false;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    _wayland_xdg_toplevel_configure,
    _wayland_xdg_toplevel_close,
};

static void _wayland_xdg_surface_configure(void *data,
                                           struct xdg_surface *xdg_surface,
                                           uint32_t serial) {
  xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    _wayland_xdg_surface_configure,
};

static void _wayland_xdg_wm_base_ping(void *data, struct xdg_wm_base *shell,
                                      uint32_t serial) {
  xdg_wm_base_pong(shell, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    _wayland_xdg_wm_base_ping,
};

static void _wayland_registry_handle_global(void *data,
                                            struct wl_registry *registry,
                                            uint32_t name,
                                            const char *interface,
                                            uint32_t version) {
  _opengl_gfx_context_wayland *d = data;

  fprintf(stderr, "global: %s\n", interface);

  if (strcmp(interface, "wl_compositor") == 0) {
    d->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 3);
  } else if (strcmp(interface, "wl_shm") == 0) {
    d->shared_memory = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, "xdg_wm_base") == 0) {
    d->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
  } else if (strcmp(interface, "wl_seat") == 0) {
    d->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
  }
}

static void _wayland_registry_handle_global_remove(void *data,
                                                   struct wl_registry *registry,
                                                   uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
    _wayland_registry_handle_global,
    _wayland_registry_handle_global_remove,
};

static void _opengl_gfx_message_callback(GLenum source, GLenum type, GLenum id,
                                         GLenum severity, GLsizei length,
                                         const GLchar *message,
                                         const void *user_param) {
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "", type, severity,
          message);
}

static void _opengl_gfx_check_shader_error(string msg, GLuint shader,
                                           GLuint status) {
  GLint good = 0;
  glGetShaderiv(shader, status, &good);
  if (good == GL_FALSE) {
    GLint max_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

    uint8_t *log_buffer = malloc(max_length + 1);
    glGetShaderInfoLog(shader, max_length, &max_length, log_buffer);
    glDeleteShader(shader);

    fprintf(stderr, "%.*s: %.*s\n", msg.len, msg.data, max_length, log_buffer);
    exit(1);
  }
}

static void _opengl_gfx_check_shader_program_error(string msg,
                                                   GLuint shader_program,
                                                   GLuint status) {
  GLint good = 0;
  glGetProgramiv(shader_program, status, &good);
  if (good == GL_FALSE) {
    GLint max_length = 0;
    glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);

    uint8_t *log_buffer = malloc(max_length + 1);
    glGetProgramInfoLog(shader_program, max_length, &max_length, log_buffer);
    glDeleteProgram(shader_program);

    fprintf(stderr, "%.*s: %.*s\n", msg.len, msg.data, max_length, log_buffer);
    exit(1);
  }
}

static GLuint _opengl_gfx_compile_shader(string file_path, GLuint shader_type) {
  GLuint shader = glCreateShader(shader_type);
  size_t shader_file_size = get_file_size(file_path);
  uint8_t *shader_file_data = malloc(shader_file_size + 1);
  load_file(file_path, shader_file_size, shader_file_data);
  shader_file_data[shader_file_size] = 0;
  // fprintf(stderr, "%s\n", shader_file_data);

  glShaderSource(shader, 1, &shader_file_data, NULL);
  glCompileShader(shader);

  _opengl_gfx_check_shader_error(_String("failed to compile shader"), shader,
                                 GL_COMPILE_STATUS);

  return shader;
}

static _opengl_gfx_context_wayland
_opengl_gfx_init_context_wayland(uint32_t width, uint32_t height) {
  _opengl_gfx_context_wayland cx = {0};

  cx.display = wl_display_connect(NULL);
  if (!cx.display) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    exit(1);
  }
  struct wl_registry *registry = wl_display_get_registry(cx.display);
  wl_registry_add_listener(registry, &registry_listener, &cx);

  // wait for all the globals to be registered
  wl_display_roundtrip(cx.display);
  xdg_wm_base_add_listener(cx.wm_base, &xdg_wm_base_listener,
                           &_gfx_context.backend);

  cx.pointer = wl_seat_get_pointer(cx.seat);
  wl_pointer_add_listener(cx.pointer, &pointer_listener, &_gfx_context.backend);

  cx.surface = wl_compositor_create_surface(cx.compositor);
  cx.xdg_surface = xdg_wm_base_get_xdg_surface(cx.wm_base, cx.surface);
  xdg_surface_add_listener(cx.xdg_surface, &xdg_surface_listener,
                           &_gfx_context.backend);

  cx.xdg_toplevel = xdg_surface_get_toplevel(cx.xdg_surface);
  xdg_toplevel_add_listener(cx.xdg_toplevel, &xdg_toplevel_listener,
                            &_gfx_context.backend);
  xdg_toplevel_set_title(cx.xdg_toplevel, "chat - [Slack sux]");
  xdg_toplevel_set_app_id(cx.xdg_toplevel, "nl.spacegirl.a_chat_client");

  wl_surface_commit(cx.surface);
  wl_display_roundtrip(cx.display);

  int buffer_size = width * height * 4;
  int fd = syscall(SYS_memfd_create, "buffer", 0);
  ftruncate(fd, buffer_size);

  cx.pixels =
      mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  for (int i = 0; i < buffer_size; i++) {
    cx.pixels[i] = 0;
  }

  cx.shared_memory_pool = wl_shm_create_pool(cx.shared_memory, fd, buffer_size);
  cx.buffer = wl_shm_pool_create_buffer(cx.shared_memory_pool, 0, width, height,
                                        width * 4, WL_SHM_FORMAT_ARGB8888);

  wl_surface_attach(cx.surface, cx.buffer, 0, 0);
  wl_surface_commit(cx.surface);

  /* Init EGL */
  EGLint major, minor, count, n, size;
  EGLConfig *configs;
  EGLint config_attribs[] = {
      EGL_SURFACE_TYPE,
      EGL_WINDOW_BIT,
      //
      EGL_RED_SIZE,
      8,
      //
      EGL_BLUE_SIZE,
      8,
      //
      EGL_GREEN_SIZE,
      8,
      //
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_BIT,
      //
      EGL_NONE,
  };

  static const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                           EGL_NONE};

  cx.egl_display = eglGetDisplay(cx.display);
  if (cx.egl_display == EGL_NO_DISPLAY) {
    fprintf(stderr, "Failed to create EGL display\n");
    exit(1);
  }
  fprintf(stderr, "Created EGL display\n");

  if (eglInitialize(cx.egl_display, &major, &minor) != EGL_TRUE) {
    fprintf(stderr, "Failed to initialize EGL display\n");
    exit(1);
  }
  fprintf(stderr, "EGL major: %d, minor: %d\n", major, minor);

  eglGetConfigs(cx.egl_display, NULL, 0, &count);
  configs = calloc(count, sizeof(EGLConfig));

  eglChooseConfig(cx.egl_display, config_attribs, configs, count, &n);

  for (int i = 0; i < n; ++i) {
    eglGetConfigAttrib(cx.egl_display, configs[i], EGL_BUFFER_SIZE, &size);
    fprintf(stderr, "EGL Buffer size: %d\n", size);

    eglGetConfigAttrib(cx.egl_display, configs[i], EGL_RED_SIZE, &size);
    fprintf(stderr, "EGL Red size: %d\n", size);

    cx.egl_config = configs[i];
    break;
  }

  eglBindAPI(EGL_OPENGL_API);
  cx.egl_context = eglCreateContext(cx.egl_display, cx.egl_config,
                                    EGL_NO_CONTEXT, context_attribs);

  cx.egl_window = wl_egl_window_create(cx.surface, width, height);
  if (cx.egl_window == EGL_NO_SURFACE) {
    fprintf(stderr, "Failed to create EGL window\n");
    exit(1);
  }
  fprintf(stderr, "Created EGL window\n");

  cx.egl_surface = eglCreateWindowSurface(cx.egl_display, cx.egl_config,
                                          cx.egl_window, NULL);
  if (eglMakeCurrent(cx.egl_display, cx.egl_surface, cx.egl_surface,
                     cx.egl_context) != EGL_TRUE) {
    fprintf(stderr, "eglMakeCurrent() failed\n");
  }

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(_opengl_gfx_message_callback, NULL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  cx.ui_rect_vertex_shader = _opengl_gfx_compile_shader(
      _String("shaders/ui_rect_vertex.glsl"), GL_VERTEX_SHADER);
  cx.ui_rect_fragment_shader = _opengl_gfx_compile_shader(
      _String("shaders/ui_rect_fragment.glsl"), GL_FRAGMENT_SHADER);
  cx.ui_rect_shader_program = glCreateProgram();
  glAttachShader(cx.ui_rect_shader_program, cx.ui_rect_vertex_shader);
  glAttachShader(cx.ui_rect_shader_program, cx.ui_rect_fragment_shader);
  glLinkProgram(cx.ui_rect_shader_program);
  _opengl_gfx_check_shader_program_error(
      _String("failed to link ui_rect shader program"),
      cx.ui_rect_shader_program, GL_LINK_STATUS);

  cx.text_atlas_vertex_shader = _opengl_gfx_compile_shader(
      _String("shaders/text_atlas_vertex.glsl"), GL_VERTEX_SHADER);
  cx.text_atlas_fragment_shader = _opengl_gfx_compile_shader(
      _String("shaders/text_atlas_fragment.glsl"), GL_FRAGMENT_SHADER);
  cx.text_atlas_shader_program = glCreateProgram();
  glAttachShader(cx.text_atlas_shader_program, cx.text_atlas_vertex_shader);
  glAttachShader(cx.text_atlas_shader_program, cx.text_atlas_fragment_shader);
  glLinkProgram(cx.text_atlas_shader_program);
  _opengl_gfx_check_shader_program_error(
      _String("failed to link text_atlas shader program"),
      cx.text_atlas_shader_program, GL_LINK_STATUS);

  cx.buffers = newArray(GLuint, 8);
  cx.textures = newArray(GLuint, 8);
  /* ******** */

  return cx;
}

static _opengl_gfx_context_x11 _opengl_gfx_init_context_11(uint32_t width,
                                                           uint32_t height) {
  Display *display = XOpenDisplay(NULL);
  if (display == NULL) {
    fprintf(stderr, "Failed to open X display\n");
    exit(1);
  }

  int screen = DefaultScreen(display);
  Window window = XCreateSimpleWindow(
      display, RootWindow(display, screen), 0, 0, width, height, 1,
      BlackPixel(display, screen), WhitePixel(display, screen));
  XSelectInput(display, window,
               ExposureMask | KeyPressMask | ButtonPressMask |
                   ButtonReleaseMask);
  XMapWindow(display, window);

  return (_opengl_gfx_context_x11){
      .display = display,
      .window = window,
      .screen = screen,
  };
}

static void _opengl_gfx_send_events_wayland(_opengl_gfx_context_wayland *cx) {
  wl_display_dispatch(cx->display);

  // TODO: don't just render like crazy, limit framerate
  _opengl_gfx_present_wayland(cx);
}

static void _opengl_gfx_send_events_x11(_opengl_gfx_context_x11 *cx) {
  XEvent e;
  XNextEvent(cx->display, &e);
  if (e.type == Expose) {
    XFillRectangle(cx->display, cx->window, DefaultGC(cx->display, cx->screen),
                   20, 20, 10, 10);
  }

  if (e.type == KeyPress) {
    keep_running = false;

    XCloseDisplay(cx->display);
  }
}

static void _opengl_gfx_present_wayland(_opengl_gfx_context_wayland *cx) {
  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(cx->mouse_x, cx->mouse_y, cx->mouse_left_down,
                          cx->mouse_right_down);

  if (_gfx_context.gpu_glyphs.size > 0) {
    gfx_update_buffer(&_gfx_context, 2, _gfx_context.gpu_glyphs.data,
                      _gfx_context.gpu_glyphs.size * sizeof(GpuGlyph));
  }
  if (_gfx_context.gpu_ui_rects.size > 0) {
    gfx_update_buffer(&_gfx_context, 4, _gfx_context.gpu_ui_rects.data,
                      _gfx_context.gpu_ui_rects.size * sizeof(GpuUiRect));
  }

  GpuUniformParams gpu_uniform_params = {
      .screen_size =
          {
              (float)_gfx_context.frame_width,
              (float)_gfx_context.frame_height,
          },
      .font_size =
          {
              (float)_FONT_WIDTH,
              (float)_FONT_HEIGHT,
          },
  };

  gfx_update_buffer(&_gfx_context, 3, &gpu_uniform_params,
                    sizeof(GpuUniformParams));
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, _gfx_context.frame_width, _gfx_context.frame_height);

  if (_gfx_context.gpu_ui_rects.size > 0) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cx->buffers.data[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cx->buffers.data[4]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cx->buffers.data[3]);
    glUseProgram(cx->ui_rect_shader_program);

    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices,
                            _gfx_context.gpu_ui_rects.size);
  }

  if (_gfx_context.gpu_glyphs.size > 0) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cx->buffers.data[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cx->buffers.data[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cx->buffers.data[3]);
    glUseProgram(cx->text_atlas_shader_program);

    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices,
                            _gfx_context.gpu_glyphs.size);
  }

  glFlush();

  if (eglSwapBuffers(cx->egl_display, cx->egl_surface) != EGL_TRUE) {
    fprintf(stderr, "eglSwapBuffers() failed\n");
  }
}

static size_t
_opengl_gfx_push_texture_buffer_wayland(_opengl_gfx_context_wayland *cx,
                                        uint32_t width, uint32_t height,
                                        const void *data, size_t len) {
  pushArray(GLuint, &cx->textures, 0);
  glCreateTextures(GL_TEXTURE_2D, 1, &cx->textures.data[cx->textures.size - 1]);
  glTextureParameteri(cx->textures.data[cx->textures.size - 1],
                      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(cx->textures.data[cx->textures.size - 1],
                      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureStorage2D(cx->textures.data[cx->textures.size - 1], 1, GL_RGBA8,
                     width, height);
  glTextureSubImage2D(cx->textures.data[cx->textures.size - 1], 0, 0, 0, width,
                      height, GL_RED, GL_UNSIGNED_BYTE, data);
  glBindTextureUnit(cx->textures.size - 1,
                    cx->textures.data[cx->textures.size - 1]);

  return cx->textures.size - 1;
}

static void
_opengl_gfx_resize_texture_buffer_wayland(_opengl_gfx_context_wayland *cx,
                                          uint32_t width, size_t texture_index,
                                          uint32_t height) {
  // TODO
  assert(false && "_opengl_gfx_resize_texture_buffer_wayland unimplemented");
}

size_t _opengl_gfx_push_vertex_buffer_wayland(_opengl_gfx_context_wayland *cx,
                                              const void *data, size_t len) {
  pushArray(GLuint, &cx->buffers, 0);
  glCreateBuffers(1, &cx->buffers.data[cx->buffers.size - 1]);
  glNamedBufferStorage(cx->buffers.data[cx->buffers.size - 1], len, data,
                       GL_DYNAMIC_STORAGE_BIT);

  return cx->buffers.size - 1;
}
static size_t
_opengl_gfx_allocate_vertex_buffer_wayland(_opengl_gfx_context_wayland *cx,
                                           size_t len) {
  pushArray(GLuint, &cx->buffers, 0);
  glCreateBuffers(1, &cx->buffers.data[cx->buffers.size - 1]);
  glNamedBufferStorage(cx->buffers.data[cx->buffers.size - 1], len, (void *)0,
                       GL_DYNAMIC_STORAGE_BIT);

  return cx->buffers.size - 1;
}
static void _opengl_gfx_update_buffer_wayland(_opengl_gfx_context_wayland *cx,
                                              size_t buffer_index,
                                              const void *data, size_t len) {
  glNamedBufferSubData(cx->buffers.data[buffer_index], 0, len, data);
}
#elif _WIN32
// #include <windows.h>

static void _opengl_gfx_message_callback(GLenum source, GLenum type, GLenum id,
                                         GLenum severity, GLsizei length,
                                         const GLchar *message,
                                         const void *user_param) {
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "", type, severity,
          message);
}

static void _opengl_gfx_check_shader_error(string msg, GLuint shader,
                                           GLuint status) {
  GLint good = 0;
  glGetShaderiv(shader, status, &good);
  if (good == GL_FALSE) {
    GLint max_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

    uint8_t *log_buffer = malloc(max_length + 1);
    glGetShaderInfoLog(shader, max_length, &max_length, log_buffer);
    glDeleteShader(shader);

    fprintf(stderr, "%.*s: %.*s\n", (int)msg.len, msg.data, max_length,
            log_buffer);
    exit(1);
  }
}

static void _opengl_gfx_check_shader_program_error(string msg,
                                                   GLuint shader_program,
                                                   GLuint status) {
  GLint good = 0;
  glGetProgramiv(shader_program, status, &good);
  if (good == GL_FALSE) {
    GLint max_length = 0;
    glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);

    uint8_t *log_buffer = malloc(max_length + 1);
    glGetProgramInfoLog(shader_program, max_length, &max_length, log_buffer);
    glDeleteProgram(shader_program);

    fprintf(stderr, "%.*s: %.*s\n", (int)msg.len, msg.data, max_length,
            log_buffer);
    exit(1);
  }
}

static GLuint _opengl_gfx_compile_shader(string file_path, GLuint shader_type) {
  GLuint shader = glCreateShader(shader_type);
  size_t shader_file_size = get_file_size(file_path);
  uint8_t *shader_file_data = malloc(shader_file_size + 1);
  load_file(file_path, shader_file_size, shader_file_data);
  shader_file_data[shader_file_size] = 0;
  // fprintf(stderr, "%s\n", shader_file_data);

  glShaderSource(shader, 1, &shader_file_data, NULL);
  glCompileShader(shader);

  _opengl_gfx_check_shader_error(_String("failed to compile shader"), shader,
                                 GL_COMPILE_STATUS);

  return shader;
}

static LRESULT CALLBACK _win32_gfx_window_proc(HWND window, UINT message,
                                               WPARAM wparam, LPARAM lparam) {
  LRESULT result = 0;

  switch (message) {
  case WM_SIZE: {
    UINT width = LOWORD(lparam);
    UINT height = HIWORD(lparam);

    _gfx_context.frame_width = width;
    _gfx_context.frame_height = height;
    break;
  }
  case WM_LBUTTONDOWN: {
    _gfx_context.backend.mouse_left_down = true;
    break;
  }
  case WM_LBUTTONUP: {
    _gfx_context.backend.mouse_left_down = false;
    break;
  }
  case WM_MOUSEMOVE: {
    UINT mouse_x = LOWORD(lparam);
    UINT mouse_y = HIWORD(lparam);

    _gfx_context.backend.mouse_x = mouse_x;
    _gfx_context.backend.mouse_y = mouse_y;
    break;
  }
  case WM_PAINT: {
    _win32_gfx_present(&_gfx_context.backend);
    break;
  }
  case WM_DESTROY: {
    keep_running = false;
    break;
  }
  case WM_CLOSE: {
    keep_running = false;
    break;
  }
  case WM_ACTIVATEAPP: {
    break;
  }
  default: {
    result = DefWindowProc(window, message, wparam, lparam);
  }
  }

  return result;
}

static _win32_gfx_context _win32_gfx_init_context(uint32_t width,
                                                  uint32_t height) {
  _win32_gfx_context cx = {0};

  HINSTANCE instance = GetModuleHandle(0);

  WNDCLASS win_class = {0};
  win_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  win_class.hInstance = instance;
  win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  win_class.lpszClassName = "GFXWindowClass";
  win_class.lpfnWndProc = _win32_gfx_window_proc;
  assert(RegisterClass(&win_class) &&
         "_win32_gfx_init_context: RegisterClass failed");

  HWND window_handle =
      CreateWindowEx(0, "GFXWindowClass", "chat - [Slack Sux]",
                     WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                     CW_USEDEFAULT, width, height, 0, 0, instance, 0);
  assert(window_handle && "_win32_gfx_init_context: CreateWindowEx failed");

  HDC device_context = GetDC(window_handle);
  assert(device_context && "_win32_gfx_init_context: GetDC failed");

  PIXELFORMATDESCRIPTOR pixel_format_desc;
  ZeroMemory(&pixel_format_desc, sizeof(pixel_format_desc));
  pixel_format_desc.nSize = sizeof(pixel_format_desc);
  pixel_format_desc.nVersion = 1;
  pixel_format_desc.dwFlags =
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
  pixel_format_desc.cColorBits = 32;
  pixel_format_desc.cAlphaBits = 32;
  pixel_format_desc.cDepthBits = 0;

  int pixel_format_id = ChoosePixelFormat(device_context, &pixel_format_desc);
  assert(pixel_format_id &&
         "_win32_gfx_init_context: ChoosePixelFormat failed");

  assert(SetPixelFormat(device_context, pixel_format_id, &pixel_format_desc) &&
         "_win32_gfx_init_context: SetPixelFormat failed");

  HGLRC fake_gl_cx = wglCreateContext(device_context);
  assert(fake_gl_cx &&
         "_win32_gfx_init_context: failed to create initial GL context");
  assert(wglMakeCurrent(device_context, fake_gl_cx) &&
         "_win32_gfx_init_context: wglMakeCurrent failed");

  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
      (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress(
          "wglCreateContextAttribsARB");
  assert(wglCreateContextAttribsARB &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'wglCreateContextAttribsARB'");

  const int major_min = 4, minor_min = 5;
  int context_attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                           major_min,
                           WGL_CONTEXT_MINOR_VERSION_ARB,
                           minor_min,
                           WGL_CONTEXT_PROFILE_MASK_ARB,
                           WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                           0};

  HGLRC gl_cx = wglCreateContextAttribsARB(device_context, 0, context_attribs);
  assert(gl_cx &&
         "_win32_gfx_init_context: failed to create proper GL context");

  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(fake_gl_cx);
  assert(wglMakeCurrent(device_context, gl_cx) &&
         "_win32_gfx_init_context: wglMakeCurrentFailed");

  assert((glGetShaderiv =
              (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glGetShaderiv'");
  assert((glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress(
              "glGetShaderInfoLog")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glGetShaderInfoLog'");
  assert((glDeleteShader =
              (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glDeleteShader'");
  assert((glGetProgramiv =
              (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glGetProgramiv'");
  assert((glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress(
              "glGetProgramInfoLog")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glGetProgramInfoLog'");
  assert((glDeleteProgram =
              (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glDeleteProgram'");
  assert((glCreateShader =
              (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glCreateShader'");
  assert((glShaderSource =
              (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glShaderSource'");
  assert((glCompileShader =
              (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glCompileShader'");
  assert((glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)
              wglGetProcAddress("glDebugMessageCallback")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glDebugMessageCallback'");
  assert((glCreateProgram =
              (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glCreateProgram'");
  assert((glAttachShader =
              (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glAttachShader'");
  assert((glLinkProgram =
              (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glLinkProgram'");
  assert((glCreateTextures =
              (PFNGLCREATETEXTURESPROC)wglGetProcAddress("glCreateTextures")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glCreateTextures'");
  assert((glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)wglGetProcAddress(
              "glTextureParameteri")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glTextureParameteri'");
  assert((glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)wglGetProcAddress(
              "glTextureStorage2D")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glTextureStorage2D'");
  assert((glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)wglGetProcAddress(
              "glTextureSubImage2D")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glTextureSubImage2D'");
  assert((glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)wglGetProcAddress(
              "glBindTextureUnit")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glBindTextureUnit'");
  assert((glCreateBuffers =
              (PFNGLCREATEBUFFERSPROC)wglGetProcAddress("glCreateBuffers")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glCreateBuffers'");
  assert((glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)wglGetProcAddress(
              "glNamedBufferStorage")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glNamedBufferStorage'");
  assert((glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)wglGetProcAddress(
              "glNamedBufferSubData")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glNamedBufferSubData'");
  assert((glBindBufferBase =
              (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glBindBufferBase'");
  assert(
      (glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram")) &&
      "_win32_gfx_init_context: failed to get proc address for 'glUseProgram'");
  assert((glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)
              wglGetProcAddress("glDrawElementsInstanced")) &&
         "_win32_gfx_init_context: failed to get proc address for "
         "'glDrawElementsInstanced'");

  SetWindowText(window_handle, (LPCSTR)glGetString(GL_VERSION));

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(_opengl_gfx_message_callback, NULL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  cx.gl.ui_rect_vertex_shader = _opengl_gfx_compile_shader(
      _String("shaders/ui_rect_vertex.glsl"), GL_VERTEX_SHADER);
  cx.gl.ui_rect_fragment_shader = _opengl_gfx_compile_shader(
      _String("shaders/ui_rect_fragment.glsl"), GL_FRAGMENT_SHADER);
  cx.gl.ui_rect_shader_program = glCreateProgram();
  glAttachShader(cx.gl.ui_rect_shader_program, cx.gl.ui_rect_vertex_shader);
  glAttachShader(cx.gl.ui_rect_shader_program, cx.gl.ui_rect_fragment_shader);
  glLinkProgram(cx.gl.ui_rect_shader_program);
  _opengl_gfx_check_shader_program_error(
      _String("failed to link ui_rect shader program"),
      cx.gl.ui_rect_shader_program, GL_LINK_STATUS);

  cx.gl.text_atlas_vertex_shader = _opengl_gfx_compile_shader(
      _String("shaders/text_atlas_vertex.glsl"), GL_VERTEX_SHADER);
  cx.gl.text_atlas_fragment_shader = _opengl_gfx_compile_shader(
      _String("shaders/text_atlas_fragment.glsl"), GL_FRAGMENT_SHADER);
  cx.gl.text_atlas_shader_program = glCreateProgram();
  glAttachShader(cx.gl.text_atlas_shader_program,
                 cx.gl.text_atlas_vertex_shader);
  glAttachShader(cx.gl.text_atlas_shader_program,
                 cx.gl.text_atlas_fragment_shader);
  glLinkProgram(cx.gl.text_atlas_shader_program);
  _opengl_gfx_check_shader_program_error(
      _String("failed to link text_atlas shader program"),
      cx.gl.text_atlas_shader_program, GL_LINK_STATUS);

  cx.gl.buffers = newArray(GLuint, 8);
  cx.gl.textures = newArray(GLuint, 8);

  cx.window_handle = window_handle;
  cx.device_context = device_context;

  return cx;
}

static void _win32_gfx_send_events(_win32_gfx_context *cx) {
  _win32_gfx_present(cx);

  MSG message;
  GetMessage(&message, 0, 0, 0);
  TranslateMessage(&message);
  DispatchMessage(&message);
}

static void _win32_gfx_present(_win32_gfx_context *cx) {
  _gfx_context.gpu_glyphs.size = 0;
  _gfx_context.gpu_ui_rects.size = 0;
  _gfx_context.frame_func(cx->mouse_x, cx->mouse_y, cx->mouse_left_down,
                          cx->mouse_right_down);

  if (_gfx_context.gpu_glyphs.size > 0) {
    gfx_update_buffer(&_gfx_context, 2, _gfx_context.gpu_glyphs.data,
                      _gfx_context.gpu_glyphs.size * sizeof(GpuGlyph));
  }
  if (_gfx_context.gpu_ui_rects.size > 0) {
    gfx_update_buffer(&_gfx_context, 4, _gfx_context.gpu_ui_rects.data,
                      _gfx_context.gpu_ui_rects.size * sizeof(GpuUiRect));
  }

  GpuUniformParams gpu_uniform_params = {
      .screen_size =
          {
              (float)_gfx_context.frame_width,
              (float)_gfx_context.frame_height,
          },
      .font_size =
          {
              (float)_FONT_WIDTH,
              (float)_FONT_HEIGHT,
          },
  };

  gfx_update_buffer(&_gfx_context, 3, &gpu_uniform_params,
                    sizeof(GpuUniformParams));
  // TODO: clear viewport
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, _gfx_context.frame_width, _gfx_context.frame_height);

  if (_gfx_context.gpu_ui_rects.size > 0) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cx->gl.buffers.data[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cx->gl.buffers.data[4]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cx->gl.buffers.data[3]);
    glUseProgram(cx->gl.ui_rect_shader_program);

    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices,
                            _gfx_context.gpu_ui_rects.size);
  }

  if (_gfx_context.gpu_glyphs.size > 0) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cx->gl.buffers.data[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cx->gl.buffers.data[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cx->gl.buffers.data[3]);
    glUseProgram(cx->gl.text_atlas_shader_program);

    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices,
                            _gfx_context.gpu_glyphs.size);
  }

  glFlush();
  SwapBuffers(cx->device_context);
}

static size_t _win32_gfx_push_texture_buffer(_win32_gfx_context *cx,
                                             uint32_t width, uint32_t height,
                                             const void *data, size_t len) {
  pushArray(GLuint, &cx->gl.textures, 0);
  glCreateTextures(GL_TEXTURE_2D, 1,
                   &cx->gl.textures.data[cx->gl.textures.size - 1]);
  glTextureParameteri(cx->gl.textures.data[cx->gl.textures.size - 1],
                      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(cx->gl.textures.data[cx->gl.textures.size - 1],
                      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureStorage2D(cx->gl.textures.data[cx->gl.textures.size - 1], 1,
                     GL_RGBA8, width, height);
  glTextureSubImage2D(cx->gl.textures.data[cx->gl.textures.size - 1], 0, 0, 0,
                      width, height, GL_RED, GL_UNSIGNED_BYTE, data);
  glBindTextureUnit(cx->gl.textures.size - 1,
                    cx->gl.textures.data[cx->gl.textures.size - 1]);

  return cx->gl.textures.size - 1;
}

static void _win32_gfx_resize_texture_buffer(_win32_gfx_context *cx,
                                             uint32_t width,
                                             size_t texture_index,
                                             uint32_t height) {
  // TODO
  assert(false && "_win32_gfx_resize_texture_buffer unimplemented");
}

size_t _win32_gfx_push_vertex_buffer(_win32_gfx_context *cx, const void *data,
                                     size_t len) {
  pushArray(GLuint, &cx->gl.buffers, 0);
  glCreateBuffers(1, &cx->gl.buffers.data[cx->gl.buffers.size - 1]);
  glNamedBufferStorage(cx->gl.buffers.data[cx->gl.buffers.size - 1], len, data,
                       GL_DYNAMIC_STORAGE_BIT);

  return cx->gl.buffers.size - 1;
}
static size_t _win32_gfx_allocate_vertex_buffer(_win32_gfx_context *cx,
                                                size_t len) {
  pushArray(GLuint, &cx->gl.buffers, 0);
  glCreateBuffers(1, &cx->gl.buffers.data[cx->gl.buffers.size - 1]);
  glNamedBufferStorage(cx->gl.buffers.data[cx->gl.buffers.size - 1], len,
                       (void *)0, GL_DYNAMIC_STORAGE_BIT);

  return cx->gl.buffers.size - 1;
}
static void _win32_gfx_update_buffer(_win32_gfx_context *cx,
                                     size_t buffer_index, const void *data,
                                     size_t len) {
  glNamedBufferSubData(cx->gl.buffers.data[buffer_index], 0, len, data);
}
#endif

void gfx_run_events(gfx_context_t *cx) {
#if defined(__APPLE__)
  _metal_gfx_send_events(&cx->backend);
#elif __linux__
  _opengl_gfx_send_events_wayland(&cx->backend);
#elif _WIN32
  _win32_gfx_send_events(&cx->backend);
#else
#error "Unsupported graphics backend"
#endif
}

size_t gfx_push_texture_buffer(gfx_context_t *cx, uint32_t width,
                               uint32_t height, const void *data, size_t len) {
#if defined(__APPLE__)
  return _metal_gfx_push_texture_buffer(&cx->backend, width, height, data, len);
#elif __linux__
  return _opengl_gfx_push_texture_buffer_wayland(&cx->backend, width, height,
                                                 data, len);
#elif _WIN32
  return _win32_gfx_push_texture_buffer(&cx->backend, width, height, data, len);
#else
#error "Unsupported graphics backend"
#endif
}

size_t gfx_push_vertex_buffer(gfx_context_t *cx, const void *data, size_t len) {
#if defined(__APPLE__)
  return _metal_gfx_push_vertex_buffer(&cx->backend, data, len);
#elif __linux__
  return _opengl_gfx_push_vertex_buffer_wayland(&cx->backend, data, len);
#elif _WIN32
  return _win32_gfx_push_vertex_buffer(&cx->backend, data, len);
#else
#error "Unsupported graphics backend"
#endif
}

size_t gfx_allocate_vertex_buffer(gfx_context_t *cx, size_t len) {
#if defined(__APPLE__)
  return _metal_gfx_allocate_vertex_buffer(&cx->backend, len);
#elif __linux__
  return _opengl_gfx_allocate_vertex_buffer_wayland(&cx->backend, len);
#elif _WIN32
  return _win32_gfx_allocate_vertex_buffer(&cx->backend, len);
#else
#error "Unsupported graphics backend"
#endif
}

// FIXME: abstract different backends
void gfx_update_buffer(gfx_context_t *cx, size_t buffer_index, const void *data,
                       size_t len) {
#if defined(__APPLE__)
  _metal_gfx_update_buffer(&cx->backend, buffer_index, data, len);
#elif __linux__
  _opengl_gfx_update_buffer_wayland(&cx->backend, buffer_index, data, len);
#elif _WIN32
  _win32_gfx_update_buffer(&cx->backend, buffer_index, data, len);
#else
#error "Unsupported graphics backend"
#endif
}

void gfx_queue_char(gfx_context_t *cx, uint8_t character, float position[2]) {
  GpuGlyph glyph;
  if (character >= 32) {
    glyph = cx->glyph_cache.data[character - 32];
  } else {
    glyph = cx->glyph_cache.data[cx->glyph_cache.size - 1];
  }

  glyph.position[0] = position[0];
  glyph.position[1] = position[1];

  pushArray(GpuGlyph, &cx->gpu_glyphs, glyph);
}

void gfx_queue_text(gfx_context_t *cx, string text, float position[2],
                    float max_x, float max_y, float color[4]) {
  float x = 0;
  float y = 0;
  for (size_t i = 0; i < text.len; ++i) {
    GpuGlyph glyph;
    if (text.data[i] >= 32) {
      glyph = cx->glyph_cache.data[text.data[i] - 32];
    } else {
      glyph = cx->glyph_cache.data[cx->glyph_cache.size - 1];
    }

    // size_t j;
    // float j_x = 0;
    // for (j = i; j < text.len; ++j) {
    //   GpuGlyph glyph;
    //   if (text.data[j] >= 32) {
    //     glyph = cx->glyph_cache.data[text.data[j] - 32];
    //   } else {
    //     glyph = cx->glyph_cache.data[cx->glyph_cache.size - 1];
    //   }

    //   if (text.data[j] == 32) {
    //     break;
    //   }

    //   j_x += glyph.size[0] / 2 + 4;
    // }

    // if (x + j_x >= max_x) {
    //     x = 0;
    //     y += 24;
    // }
    // if (y >= max_y) {
    //     break;
    // }

    glyph.color[0] = color[0];
    glyph.color[1] = color[1];
    glyph.color[2] = color[2];
    glyph.color[3] = color[3];
    glyph.position[0] = x + position[0];
    glyph.position[1] = y + position[1];
    x += glyph.size[0] / 2 + 4;

    pushArray(GpuGlyph, &cx->gpu_glyphs, glyph);
  }
}

void gfx_queue_ui_rect(gfx_context_t *cx, float position[2], float size[2],
                       float border_size, float color[4]) {
  GpuUiRect rect =
      (GpuUiRect){.position = {position[0], position[1]},
                  .size = {size[0], size[1]},
                  .border_size = {border_size, border_size},
                  .color = {color[0], color[1], color[2], color[3]}};

  pushArray(GpuUiRect, &cx->gpu_ui_rects, rect);
}

void gfx_add_glyph(gfx_context_t *cx, GpuGlyph glyph) {
  pushArray(GpuGlyph, &cx->glyph_cache, glyph);
}

uint32_t gfx_frame_width(gfx_context_t *cx) { return cx->frame_width; }

uint32_t gfx_frame_height(gfx_context_t *cx) { return cx->frame_height; }

int32_t gfx_mouse_x(gfx_context_t *cx) { return cx->backend.mouse_x; }

int32_t gfx_mouse_y(gfx_context_t *cx) { return cx->backend.mouse_y; }

void *gfx_init_context(_gfx_frame_func frame_func, uint32_t width,
                       uint32_t height) {
#if defined(__APPLE__)
  _metal_gfx_context backend = _metal_gfx_init_context(width, height);
#elif __linux__
  _opengl_gfx_context_wayland backend =
      _opengl_gfx_init_context_wayland(width, height);
#elif _WIN32
  _win32_gfx_context backend = _win32_gfx_init_context(width, height);
#else
#error "Unsupported graphics backend"
#endif

  _gfx_context.backend = backend;
  _gfx_context.frame_func = frame_func;
  _gfx_context.frame_width = width;
  _gfx_context.frame_height = height;

  _gfx_context.gpu_ui_rects = newArray(GpuUiRect, 2000);
  _gfx_context.gpu_glyphs = newArray(GpuGlyph, 8192 * 32);
  _gfx_context.glyph_cache = newArray(GpuGlyph, 97);

  const float vertices[] = {
      // positions       texture coords
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  1.0f,  1.0f, 0.0f,
      1.0f,  -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f,
  };
  const uint16_t indices[] = {0, 1, 2, 0, 2, 3};

  // NOTE: the order of these matter
  gfx_push_vertex_buffer(&_gfx_context, vertices, sizeof(vertices));
  gfx_push_vertex_buffer(&_gfx_context, indices, sizeof(indices));
  gfx_allocate_vertex_buffer(&_gfx_context, _gfx_context.gpu_glyphs.capacity *
                                                sizeof(GpuGlyph));
  gfx_allocate_vertex_buffer(&_gfx_context, sizeof(GpuUniformParams));
  gfx_allocate_vertex_buffer(&_gfx_context, _gfx_context.gpu_ui_rects.capacity *
                                                sizeof(GpuUiRect));

  return &_gfx_context;
}

#endif
#endif
