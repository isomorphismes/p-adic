#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define LOG_TAG "WittNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define WITT_DEPTH 5
#define PI_F 3.14159265358979323846f
#define TAU_F 6.28318530717958647692f
#define INNER_R 0.085f
#define RING_W 0.073f

struct witt_state {
    struct android_app *app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int width;
    int height;
    bool gl_ready;
    bool redraw;
    GLuint program;
    GLint a_pos;
    GLint u_resolution;
    GLint u_time;
    GLint u_change_time;
    GLint u_changed_ring;
    GLint u_p;
    GLint u_digits;
    int p;
    int digits[WITT_DEPTH];
    int changed_ring;
    double epoch;
    double change_time;
};

static double monotonic_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
}

static double relative_seconds(const struct witt_state *state) {
    return monotonic_seconds() - state->epoch;
}

static const char *vertex_shader_source =
    "attribute vec2 aPos;\n"
    "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static const char *fragment_shader_source =
    "precision mediump float;\n"
    "uniform vec2 uResolution;\n"
    "uniform float uTime;\n"
    "uniform float uChangeTime;\n"
    "uniform float uChangedRing;\n"
    "uniform float uP;\n"
    "uniform float uDigits[5];\n"
    "const float PI = 3.14159265358979323846;\n"
    "const float TAU = 6.28318530717958647692;\n"
    "float digitAt(int i) {\n"
    "  if (i == 0) return uDigits[0];\n"
    "  if (i == 1) return uDigits[1];\n"
    "  if (i == 2) return uDigits[2];\n"
    "  if (i == 3) return uDigits[3];\n"
    "  return uDigits[4];\n"
    "}\n"
    "float segDist(vec2 p, vec2 a, vec2 b) {\n"
    "  vec2 pa = p - a; vec2 ba = b - a;\n"
    "  float h = clamp(dot(pa, ba) / max(dot(ba, ba), 0.000001), 0.0, 1.0);\n"
    "  return length(pa - ba * h);\n"
    "}\n"
    "vec2 nodeAt(int i, float span) {\n"
    "  float rr = 0.085 + (float(i) + 0.5) * 0.073;\n"
    "  float aa = 0.5 * PI + digitAt(i) * span;\n"
    "  return rr * vec2(cos(aa), sin(aa));\n"
    "}\n"
    "void main() {\n"
    "  float shortSide = min(uResolution.x, uResolution.y);\n"
    "  vec2 q = (gl_FragCoord.xy - 0.5 * uResolution) / shortSide;\n"
    "  float r = length(q); float a = atan(q.y, q.x);\n"
    "  float inner = 0.085; float ringW = 0.073; float outer = inner + 5.0 * ringW;\n"
    "  float span = TAU / uP;\n"
    "  float boundary0 = 0.5 * PI - 0.5 * span;\n"
    "  float rel = mod(a - boundary0 + 4.0 * TAU, TAU);\n"
    "  float sector = floor(rel / span);\n"
    "  float phase = mod(rel, span);\n"
    "  float angularDistance = r * min(phase, span - phase);\n"
    "  vec3 col = vec3(0.025, 0.035, 0.055);\n"
    "  float halo = 1.0 - smoothstep(0.0, 0.58, r);\n"
    "  col += vec3(0.018, 0.032, 0.060) * halo;\n"
    "  if (r >= inner && r <= outer) {\n"
    "    col = vec3(0.055, 0.082, 0.125);\n"
    "    for (int i = 0; i < 5; ++i) {\n"
    "      float r0 = inner + float(i) * ringW; float r1 = r0 + ringW;\n"
    "      if (r >= r0 && r < r1 && abs(sector - digitAt(i)) < 0.25)\n"
    "        col = vec3(0.255, 0.205, 0.045);\n"
    "    }\n"
    "    float sectorLine = 1.0 - smoothstep(0.0, 0.0026, angularDistance);\n"
    "    col = mix(col, vec3(0.20, 0.27, 0.37), sectorLine);\n"
    "  }\n"
    "  for (int j = 0; j <= 5; ++j) {\n"
    "    float rb = inner + float(j) * ringW;\n"
    "    float line = 1.0 - smoothstep(0.0, 0.0022, abs(r - rb));\n"
    "    col = mix(col, vec3(0.22, 0.31, 0.43), line);\n"
    "  }\n"
    "  for (int k = 0; k < 4; ++k) {\n"
    "    float path = 1.0 - smoothstep(0.0, 0.0050, segDist(q, nodeAt(k, span), nodeAt(k + 1, span)));\n"
    "    col = mix(col, vec3(0.96, 0.80, 0.28), path);\n"
    "  }\n"
    "  for (int n = 0; n < 5; ++n) {\n"
    "    float d = length(q - nodeAt(n, span));\n"
    "    float dotGlow = 1.0 - smoothstep(0.004, 0.012, d);\n"
    "    col = mix(col, vec3(1.0, 0.88, 0.38), dotGlow);\n"
    "  }\n"
    "  for (int g = 0; g < 5; ++g) {\n"
    "    float fg = float(g);\n"
    "    if (fg + 0.01 >= uChangedRing) {\n"
    "      float age = uTime - uChangeTime - 0.10 * (fg - uChangedRing);\n"
    "      if (age >= 0.0 && age <= 0.90) {\n"
    "        float rr = inner + (fg + 1.0) * ringW + age * 0.022;\n"
    "        float pulse = 1.0 - smoothstep(0.0, 0.0045, abs(r - rr));\n"
    "        float fade = 1.0 - age / 0.90;\n"
    "        col = mix(col, vec3(0.28, 0.80, 1.0), pulse * fade * 0.95);\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  float center = 1.0 - smoothstep(0.030, 0.050, r);\n"
    "  vec3 centerCol = (uP < 2.5) ? vec3(0.50,0.82,1.0) : (uP < 4.0) ? vec3(0.96,0.80,0.28) : vec3(0.76,0.53,1.0);\n"
    "  col = mix(col, centerCol, center);\n"
    "  gl_FragColor = vec4(col, 1.0);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[2048];
        GLsizei length = 0;
        glGetShaderInfoLog(shader, sizeof(log), &length, log);
        LOGE("shader compile failed: %.*s", (int)length, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static bool start_gl(struct witt_state *state) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    state->program = glCreateProgram();
    glAttachShader(state->program, vs);
    glAttachShader(state->program, fs);
    glBindAttribLocation(state->program, 0, "aPos");
    glLinkProgram(state->program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(state->program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[2048];
        GLsizei length = 0;
        glGetProgramInfoLog(state->program, sizeof(log), &length, log);
        LOGE("program link failed: %.*s", (int)length, log);
        glDeleteProgram(state->program);
        state->program = 0;
        return false;
    }

    state->a_pos = glGetAttribLocation(state->program, "aPos");
    state->u_resolution = glGetUniformLocation(state->program, "uResolution");
    state->u_time = glGetUniformLocation(state->program, "uTime");
    state->u_change_time = glGetUniformLocation(state->program, "uChangeTime");
    state->u_changed_ring = glGetUniformLocation(state->program, "uChangedRing");
    state->u_p = glGetUniformLocation(state->program, "uP");
    state->u_digits = glGetUniformLocation(state->program, "uDigits");
    if (state->a_pos < 0 || state->u_resolution < 0 || state->u_time < 0 ||
        state->u_change_time < 0 || state->u_changed_ring < 0 ||
        state->u_p < 0 || state->u_digits < 0) {
        LOGE("required GLES location missing");
        glDeleteProgram(state->program);
        state->program = 0;
        return false;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    state->epoch = monotonic_seconds();
    state->change_time = 0.0;
    state->changed_ring = 0;
    return true;
}

static void stop_gl(struct witt_state *state) {
    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }
}

static void draw_frame(struct witt_state *state) {
    static const GLfloat vertices[] = {
        -1.0f,-1.0f, 1.0f,-1.0f, -1.0f,1.0f,
        -1.0f, 1.0f, 1.0f,-1.0f, 1.0f,1.0f
    };
    GLfloat digits[WITT_DEPTH];
    for (int i = 0; i < WITT_DEPTH; ++i) digits[i] = (GLfloat)state->digits[i];

    glViewport(0, 0, state->width, state->height);
    glUseProgram(state->program);
    glUniform2f(state->u_resolution, (GLfloat)state->width, (GLfloat)state->height);
    glUniform1f(state->u_time, (GLfloat)relative_seconds(state));
    glUniform1f(state->u_change_time, (GLfloat)state->change_time);
    glUniform1f(state->u_changed_ring, (GLfloat)state->changed_ring);
    glUniform1f(state->u_p, (GLfloat)state->p);
    glUniform1fv(state->u_digits, WITT_DEPTH, digits);
    glEnableVertexAttribArray((GLuint)state->a_pos);
    glVertexAttribPointer((GLuint)state->a_pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray((GLuint)state->a_pos);
}

static bool animation_active(const struct witt_state *state) {
    return state->gl_ready && relative_seconds(state) - state->change_time < 1.45;
}

static void stop_surface(struct witt_state *state) {
    if (state->gl_ready) {
        stop_gl(state);
        state->gl_ready = false;
    }
    if (state->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (state->context != EGL_NO_CONTEXT) eglDestroyContext(state->display, state->context);
        if (state->surface != EGL_NO_SURFACE) eglDestroySurface(state->display, state->surface);
        eglTerminate(state->display);
    }
    state->display = EGL_NO_DISPLAY;
    state->surface = EGL_NO_SURFACE;
    state->context = EGL_NO_CONTEXT;
    state->width = state->height = 0;
    state->redraw = false;
}

static bool start_surface(struct witt_state *state) {
    if (state->app->window == NULL) return false;
    stop_surface(state);
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (state->display == EGL_NO_DISPLAY) return false;
    if (eglInitialize(state->display, NULL, NULL) != EGL_TRUE) { stop_surface(state); return false; }
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) { stop_surface(state); return false; }

    const EGLint attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,8, EGL_GREEN_SIZE,8, EGL_BLUE_SIZE,8, EGL_ALPHA_SIZE,8,
        EGL_NONE
    };
    EGLConfig config = NULL;
    EGLint count = 0;
    if (eglChooseConfig(state->display, attrs, &config, 1, &count) != EGL_TRUE || count < 1) {
        stop_surface(state); return false;
    }
    EGLint format = 0;
    eglGetConfigAttrib(state->display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(state->app->window, 0, 0, format);

    const EGLint ctxattrs[] = { EGL_CONTEXT_CLIENT_VERSION,2, EGL_NONE };
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, ctxattrs);
    if (state->context == EGL_NO_CONTEXT) { stop_surface(state); return false; }
    state->surface = eglCreateWindowSurface(state->display, config, state->app->window, NULL);
    if (state->surface == EGL_NO_SURFACE) { stop_surface(state); return false; }
    if (eglMakeCurrent(state->display, state->surface, state->surface, state->context) != EGL_TRUE) {
        stop_surface(state); return false;
    }
    eglSwapInterval(state->display, 1);
    EGLint width = 0, height = 0;
    eglQuerySurface(state->display, state->surface, EGL_WIDTH, &width);
    eglQuerySurface(state->display, state->surface, EGL_HEIGHT, &height);
    state->width = width; state->height = height;
    if (!start_gl(state)) { stop_surface(state); return false; }
    state->gl_ready = true;
    state->redraw = true;
    LOGI("surface ready %dx%d p=%d", width, height, state->p);
    return true;
}

static void resize_surface(struct witt_state *state) {
    if (!state->gl_ready || state->surface == EGL_NO_SURFACE) return;
    EGLint width = 0, height = 0;
    eglQuerySurface(state->display, state->surface, EGL_WIDTH, &width);
    eglQuerySurface(state->display, state->surface, EGL_HEIGHT, &height);
    if (width > 0 && height > 0) {
        state->width = width; state->height = height; state->redraw = true;
    }
}

static void mark_change(struct witt_state *state, int ring) {
    state->changed_ring = ring;
    state->change_time = relative_seconds(state);
    state->redraw = true;
}

static bool touch_witt(struct witt_state *state, float sx, float sy, bool allow_center) {
    if (state->width <= 0 || state->height <= 0) return false;
    const float short_side = (float)(state->width < state->height ? state->width : state->height);
    const float qx = (sx - 0.5f * (float)state->width) / short_side;
    const float qy = (0.5f * (float)state->height - sy) / short_side;
    const float r = sqrtf(qx*qx + qy*qy);

    if (allow_center && r < 0.055f) {
        state->p = state->p == 2 ? 3 : state->p == 3 ? 5 : 2;
        for (int i = 0; i < WITT_DEPTH; ++i) state->digits[i] %= state->p;
        mark_change(state, 0);
        LOGI("prime=%d", state->p);
        return true;
    }

    const float outer = INNER_R + (float)WITT_DEPTH * RING_W;
    if (r < INNER_R || r >= outer) return false;
    int ring = (int)floorf((r - INNER_R) / RING_W);
    if (ring < 0) ring = 0;
    if (ring >= WITT_DEPTH) ring = WITT_DEPTH - 1;

    const float span = TAU_F / (float)state->p;
    const float boundary0 = 0.5f * PI_F - 0.5f * span;
    float rel = atan2f(qy, qx) - boundary0;
    while (rel < 0.0f) rel += TAU_F;
    while (rel >= TAU_F) rel -= TAU_F;
    int digit = (int)floorf(rel / span);
    if (digit < 0) digit = 0;
    if (digit >= state->p) digit = state->p - 1;

    if (state->digits[ring] != digit) {
        state->digits[ring] = digit;
        mark_change(state, ring);
        LOGI("x_%d=%d", ring, digit);
        return true;
    }
    return false;
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    struct witt_state *state = (struct witt_state *)app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    const int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    const float x = AMotionEvent_getX(event, 0);
    const float y = AMotionEvent_getY(event, 0);
    if (action == AMOTION_EVENT_ACTION_DOWN) { (void)touch_witt(state,x,y,true); return 1; }
    if (action == AMOTION_EVENT_ACTION_MOVE) { (void)touch_witt(state,x,y,false); return 1; }
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) return 1;
    return 0;
}

static void handle_command(struct android_app *app, int32_t command) {
    struct witt_state *state = (struct witt_state *)app->userData;
    switch (command) {
        case APP_CMD_INIT_WINDOW: (void)start_surface(state); break;
        case APP_CMD_TERM_WINDOW: stop_surface(state); break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED: resize_surface(state); break;
        case APP_CMD_GAINED_FOCUS: state->redraw = true; break;
        default: break;
    }
}

void android_main(struct android_app *app) {
    struct witt_state state;
    memset(&state, 0, sizeof(state));
    state.app = app;
    state.display = EGL_NO_DISPLAY;
    state.surface = EGL_NO_SURFACE;
    state.context = EGL_NO_CONTEXT;
    state.p = 3;
    state.digits[0]=2; state.digits[1]=0; state.digits[2]=1; state.digits[3]=2; state.digits[4]=1;
    app->userData = &state;
    app->onAppCmd = handle_command;
    app->onInputEvent = handle_input;
    LOGI("native entry");

    for (;;) {
        const bool active = state.gl_ready && (state.redraw || animation_active(&state));
        const int timeout = active ? 0 : -1;
        int events = 0, ident = 0;
        struct android_poll_source *source = NULL;
        while ((ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source)) >= 0) {
            (void)ident;
            if (source != NULL) source->process(app, source);
            if (app->destroyRequested != 0) { stop_surface(&state); return; }
            if (state.gl_ready && (state.redraw || animation_active(&state))) break;
        }
        if (app->destroyRequested != 0) { stop_surface(&state); return; }
        if (state.gl_ready && (state.redraw || animation_active(&state))) {
            draw_frame(&state);
            if (eglSwapBuffers(state.display, state.surface) != EGL_TRUE) {
                stop_surface(&state);
                continue;
            }
            state.redraw = false;
        }
    }
}
