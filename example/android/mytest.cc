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

#include <android_out.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "model.hpp"
#include "shader.hpp"

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
float *buildOrthographicMatrix(float *outMatrix, float halfHeight, float aspect,
                               float near, float far) {
  float halfWidth = halfHeight * aspect;

  // column 1
  outMatrix[0] = 1.f / halfWidth;
  outMatrix[1] = 0.f;
  outMatrix[2] = 0.f;
  outMatrix[3] = 0.f;

  // column 2
  outMatrix[4] = 0.f;
  outMatrix[5] = 1.f / halfHeight;
  outMatrix[6] = 0.f;
  outMatrix[7] = 0.f;

  // column 3
  outMatrix[8] = 0.f;
  outMatrix[9] = 0.f;
  outMatrix[10] = -2.f / (far - near);
  outMatrix[11] = -(far + near) / (far - near);

  // column 4
  outMatrix[12] = 0.f;
  outMatrix[13] = 0.f;
  outMatrix[14] = 0.f;
  outMatrix[15] = 1.f;

  return outMatrix;
}

float *buildIdentityMatrix(float *outMatrix) {
  // column 1
  outMatrix[0] = 1.f;
  outMatrix[1] = 0.f;
  outMatrix[2] = 0.f;
  outMatrix[3] = 0.f;

  // column 2
  outMatrix[4] = 0.f;
  outMatrix[5] = 1.f;
  outMatrix[6] = 0.f;
  outMatrix[7] = 0.f;

  // column 3
  outMatrix[8] = 0.f;
  outMatrix[9] = 0.f;
  outMatrix[10] = 1.f;
  outMatrix[11] = 0.f;

  // column 4
  outMatrix[12] = 0.f;
  outMatrix[13] = 0.f;
  outMatrix[14] = 0.f;
  outMatrix[15] = 1.f;

  return outMatrix;
}

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
  void update_render_area();

  int width_, height_;
  std::unique_ptr<Shader> shader_;
  bool shaderNeedsNewProjectionMatrix_;
  std::vector<Model> models_;
  // void DrawExample();
};

void Renderer::update_render_area() {
  EGLint width;
  eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

  EGLint height;
  eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);

    // make sure that we lazily recreate the projection matrix before we render
    shaderNeedsNewProjectionMatrix_ = true;
  }
}

void Renderer::Render() {
  update_render_area();

  if (shaderNeedsNewProjectionMatrix_) {
    // a placeholder projection matrix allocated on the stack. Column-major
    // memory layout
    float projectionMatrix[16] = {0};

    // build an orthographic projection matrix for 2d rendering
    buildOrthographicMatrix(projectionMatrix, 2.f,
                            static_cast<float>(width_) / height_, -1.f, 1.f);

    // send the matrix to the shader
    // Note: the shader must be active for this to work. Since we only have one
    // shader for this demo, we can assume that it's active.
    shader_->setProjectionMatrix(projectionMatrix);

    // make sure the matrix isn't generated every frame
    shaderNeedsNewProjectionMatrix_ = false;
  }
  glClear(GL_COLOR_BUFFER_BIT);

  if (!models_.empty()) {
    for (const auto &model : models_) {
      shader_->drawModel(model);
    }
  }
  eglSwapBuffers(display_, surface_);
}

void Renderer::LoadModels(android_app *app) {
  /*
   * This is a square:
   * 0 --- 1
   * | \   |
   * |  \  |
   * |   \ |
   * 3 --- 2
   */
  std::vector<Vertex> vertices = {
      Vertex(Vector3{{1, 1, 0}}, Vector2{{0, 0}}),    // 0
      Vertex(Vector3{{-1, 1, 0}}, Vector2{{1, 0}}),   // 1
      Vertex(Vector3{{-1, -1, 0}}, Vector2{{1, 1}}),  // 2
      Vertex(Vector3{{1, -1, 0}}, Vector2{{0, 1}})    // 3
  };
  std::vector<Index> indices = {0, 1, 2, 0, 2, 3};

  // loads an image and assigns it to the square.
  //
  // Note: there is no texture management in this sample, so if you reuse an
  // image be careful not to load it repeatedly. Since you get a shared_ptr you
  // can safely reuse it in many models.
  auto assetManager = app->activity->assetManager;
  auto spAndroidRobotTexture =
      TextureAsset::loadAsset(assetManager, "brick_01.png");

  // Create a model and put it in the back of the render list.
  models_.emplace_back(vertices, indices, spAndroidRobotTexture);
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
  shader_ = std::unique_ptr<Shader>(Shader::loadShader(
      vertex, fragment, "inPosition", "inUV", "uProjection"));
  shader_->activate();
  glClearColor(CORNFLOWER_BLUE);

  // enable alpha globally for now, you probably don't want to do this in a game
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  LoadModels(app);
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
