// Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD
// License you choose.
//  NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK
//  extern

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <android/imagedecoder.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <map>
#include <vector>
#include <memory>
#include <android_out.hpp>

#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1
// Vertex shader, you'd typically load this from assets
static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform mat4 uProjection;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}
)vertex";

// Fragment shader, you'd typically load this from assets
static const char *fragment = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV);
}
)fragment";

namespace {
class InputManager {
 public:
  std::map<std::string, bool> GetKeysState();
};

class Renderer {
 public:
  void Init(android_app *app);
  void Render();
  EGLDisplay display_;
  EGLSurface surface_;
  EGLContext context_;
private:
  void LoadModels(android_app *app);
  // void DrawExample();
};

void Renderer::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(display_, surface_);
}

void Renderer::LoadModels(android_app *app) {
  auto* asset_manager = app->activity->assetManager;
  AAsset* asset = AAssetManager_open(asset_manager, "brick_01.png", AASSET_MODE_STREAMING);
  AImageDecoder* decoder;
  int result = AImageDecoder_createFromAAsset(asset, &decoder);
  if (result != ANDROID_IMAGE_DECODER_SUCCESS) {
    aout << "Failed to load asset" << result << std::endl;
    return;
  }
  const AImageDecoderHeaderInfo* info = AImageDecoder_getHeaderInfo(decoder);
  int32_t width = AImageDecoderHeaderInfo_getWidth(info);
  int32_t height = AImageDecoderHeaderInfo_getHeight(info);

  auto format =
    (AndroidBitmapFormat) AImageDecoderHeaderInfo_getAndroidBitmapFormat(info);

  size_t stride = AImageDecoder_getMinimumStride(decoder);

  void* pixels = malloc(height * stride);

  result = AImageDecoder_decodeImage(decoder, pixels, stride, height * stride);

  if (result != ANDROID_IMAGE_DECODER_SUCCESS) {
    aout << "Failed to decode asset" << result << std::endl;
    return;
  }

  AImageDecoder_delete(decoder);
  AAsset_close(asset);
  free(pixels);
}

void Renderer::Init(android_app *app) {
  // Choose your render attributes
  constexpr EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                                EGL_OPENGL_ES3_BIT,
                                EGL_SURFACE_TYPE,
                                EGL_WINDOW_BIT,
                                EGL_BLUE_SIZE,
                                8,
                                EGL_GREEN_SIZE,
                                8,
                                EGL_RED_SIZE,
                                8,
                                EGL_DEPTH_SIZE,
                                24,
                                EGL_NONE};

  // The default display is probably what you want on Android
  auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display, nullptr, nullptr);

  // figure out how many configs there are
  EGLint numConfigs;
  eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

  // get the list of configurations
  std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
  eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs,
                  &numConfigs);

  // Find a config we like.
  // Could likely just grab the first if we don't care about anything else in
  // the config. Otherwise hook in your own heuristic
  auto config = *std::find_if(
      supportedConfigs.get(), supportedConfigs.get() + numConfigs,
      [&display](const EGLConfig &config) {
        EGLint red, green, blue, depth;
        if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red) &&
            eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green) &&
            eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue) &&
            eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {
          aout << "Found config with " << red << ", " << green << ", " << blue
               << ", " << depth << std::endl;
          return red == 8 && green == 8 && blue == 8 && depth == 24;
        }
        return false;
      });

  aout << "Found " << numConfigs << " configs" << std::endl;
  aout << "Chose " << config << std::endl;

  // create the proper window surface
  EGLint format;
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
  EGLSurface surface =
      eglCreateWindowSurface(display, config, app->window, nullptr);

  // Create a GLES 3 context
  EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext context =
      eglCreateContext(display, config, nullptr, contextAttribs);

  // get some window metrics
  auto madeCurrent = eglMakeCurrent(display, surface, surface, context);

  display_ = display;
  surface_ = surface;
  context_ = context;

  // make width and height invalid so it gets updated the first frame in @a
  // updateRenderArea()

  // setup any other gl related global states
  glClearColor(CORNFLOWER_BLUE);

  // enable alpha globally for now, you probably don't want to do this in a game
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

struct GameState {
  std::unique_ptr<InputManager> input_manager_;
  std::unique_ptr<Renderer> renderer_;
};

void handle_cmd(android_app *pApp, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_INIT_WINDOW: {
      // A new window is created, associate a renderer with it. You may replace
      // this with a "game" class if that suits your needs. Remember to change
      // all instances of userData if you change the class here as a
      // reinterpret_cast is dangerous this in the android_main function and the
      // APP_CMD_TERM_WINDOW handler case.
      GameState *state = new GameState();
      state->input_manager_ = std::make_unique<InputManager>();
      state->renderer_ = std::make_unique<Renderer>();
      state->renderer_->Init(pApp);
      pApp->userData = state;
      break;
    }
    case APP_CMD_TERM_WINDOW: {
      if (pApp->userData) {
        auto *state = reinterpret_cast<GameState *>(pApp->userData);
        delete state;
        pApp->userData = nullptr;
      }
      break;
    }
      // The window is being destroyed. Use this to clean up your userData to
      // avoid leaking resources.
      //
      // We have to check if userData is assigned just in case this comes in
      // really quickly
    default:
      break;
  }
}

}  // namespace

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
  // // Can be removed, useful to ensure your code is running
  // aout << "Welcome to android_main" << std::endl;

  // register an event handler for Android events
  pApp->onAppCmd = handle_cmd;

  // aout << "App is running" << std::endl;
  // This sets up a typical game/event loop. It will run until the app is
  // destroyed.
  int events;
  android_poll_source *pSource;
  aout << "Before the loop" << std::endl;
  do {
    // Process all pending events before running game logic.
    if (ALooper_pollAll(0, nullptr, &events,
                        reinterpret_cast<void **>(&pSource)) >= 0) {
      if (pSource) {
        pSource->process(pApp, pSource);
      }
    }

    // Check if any user data is associated. This is assigned in handle_cmd
    if (pApp->userData) {
      // We know that our user data is a Renderer, so reinterpret cast it. If
      auto *state = reinterpret_cast<GameState *>(pApp->userData);
      state->renderer_->Render();
      aout << "Running inside the loop" << std::endl;
      // you change your user data remember to change it here
      // aout << "Loop" << std::endl;
    }
  } while (!pApp->destroyRequested);
}
