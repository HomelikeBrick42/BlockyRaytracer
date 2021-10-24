#define GLAD_GL_IMPLEMENTATION
#include "gl.h"

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include <Windows.h>

typedef struct Vector3f {
    float x;
    float y;
    float z;
} Vector3f;

Vector3f Vector3f_Add(Vector3f a, Vector3f b) {
    Vector3f result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

Vector3f Vector3f_Subtract(Vector3f a, Vector3f b) {
    Vector3f result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Vector3f Vector3f_MultiplyScalar(Vector3f v, float s) {
    Vector3f result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

float Vector3f_SqrLength(Vector3f v) {
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

float Vector3f_Length(Vector3f v) {
    return sqrtf(Vector3f_SqrLength(v));
}

Vector3f Vector3f_Normalized(Vector3f v) {
    float inverseLength = 1.0f / Vector3f_Length(v);
    return (Vector3f){
        .x = v.x * inverseLength,
        .y = v.y * inverseLength,
        .z = v.z * inverseLength,
    };
}

Vector3f Vector3f_Cross(Vector3f a, Vector3f b) {
    Vector3f result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);
    return result;
}

GLADapiproc GladLoadProc(const char* name);

static bool Running     = true;
static int WindowWidth  = 640;
static int WindowHeight = 480;
static int XMouseDelta  = 0;
static int YMouseDelta  = 0;
static bool WPressed    = false;
static bool SPressed    = false;
static bool APressed    = false;
static bool DPressed    = false;
static bool EPressed    = false;
static bool QPressed    = false;

static LRESULT CALLBACK WindowMessageCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_QUIT:
        case WM_DESTROY:
        case WM_CLOSE: {
            Running = false;
        } break;

        case WM_SIZE: {
            RECT clientRect = {};
            GetClientRect(window, &clientRect);
            int clientWidth  = clientRect.right - clientRect.left;
            int clientHeight = clientRect.bottom - clientRect.top;
            if (clientWidth > 0 && clientHeight > 0) {
                WindowWidth  = clientWidth;
                WindowHeight = clientHeight;
                glViewport(0, 0, WindowWidth, WindowHeight);
            }
        } break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            bool pressed = message == WM_SYSKEYDOWN || message == WM_KEYDOWN;

            switch (wParam) {
                case 'W': {
                    WPressed = pressed;
                } break;

                case 'S': {
                    SPressed = pressed;
                } break;

                case 'A': {
                    APressed = pressed;
                } break;

                case 'D': {
                    DPressed = pressed;
                } break;

                case 'E': {
                    EPressed = pressed;
                } break;

                case 'Q': {
                    QPressed = pressed;
                } break;
            }

            result = DefWindowProcA(window, message, wParam, lParam);
        } break;

        case WM_INPUT: {
            HRAWINPUT inputHandle = (HRAWINPUT)lParam;
            UINT size;
            GetRawInputData(inputHandle, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
            BYTE bytes[size];
            GetRawInputData(inputHandle, RID_INPUT, bytes, &size, sizeof(RAWINPUTHEADER));
            RAWINPUT* input = (RAWINPUT*)bytes;

            XMouseDelta += input->data.mouse.lLastX;
            YMouseDelta += input->data.mouse.lLastY;
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return result;
}

static void OpenGLMessageCallback(
    unsigned source, unsigned type, unsigned id, unsigned severity, int length, const char* message, const void* userParam) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_MEDIUM:
        case GL_DEBUG_SEVERITY_HIGH: {
            fprintf(stderr, "%s\n", message);
        } break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        case GL_DEBUG_SEVERITY_LOW:
        default: {
            printf("%s\n", message);
        } break;
    }
}

int main() {
    const char* windowClassName = "RayTracingWindowClass";

    HANDLE instance = GetModuleHandleA(NULL);

    WNDCLASSEXA windowClass = {
        .cbSize        = sizeof(windowClass),
        .style         = CS_OWNDC,
        .lpfnWndProc   = WindowMessageCallback,
        .hInstance     = instance,
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = windowClassName,
    };
    if (!RegisterClassExA(&windowClass)) {
        fflush(stdout);
        fprintf(stderr, "Failed register window class\n");
        return EXIT_FAILURE;
    }

    RECT windowRect   = {};
    windowRect.left   = 100;
    windowRect.right  = windowRect.left + WindowWidth;
    windowRect.top    = 100;
    windowRect.bottom = windowRect.top + WindowHeight;
    AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

    int windowWidth  = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    HWND window = CreateWindowExA(WS_EX_APPWINDOW,
                                  windowClassName,
                                  "Ray Tracing",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  windowWidth,
                                  windowHeight,
                                  NULL,
                                  NULL,
                                  instance,
                                  NULL);

    if (!window) {
        fflush(stdout);
        fprintf(stderr, "Failed to create window\n");
        return EXIT_FAILURE;
    }

    HDC deviceContext = GetDC(window);

    if (!deviceContext) {
        fflush(stdout);
        fprintf(stderr, "Failed to get device context\n");
        return EXIT_FAILURE;
    }

    RAWINPUTDEVICE rawInputDevice = {
        .usUsagePage = 0x01,
        .usUsage     = 0x02,
    };

    if (!RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE))) {
        fflush(stdout);
        fprintf(stderr, "Failed to register raw input device\n");
        return EXIT_FAILURE;
    }

    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {
        .nSize        = sizeof(pixelFormatDescriptor),
        .nVersion     = 1,
        .dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType   = PFD_TYPE_RGBA,
        .cColorBits   = 32,
        .cDepthBits   = 24,
        .cStencilBits = 8,
        .iLayerType   = PFD_MAIN_PLANE,
    };

    int format = ChoosePixelFormat(deviceContext, &pixelFormatDescriptor);
    if (format == 0) {
        fflush(stdout);
        fprintf(stderr, "Failed to choose pixel format\n");
        return EXIT_FAILURE;
    }

    if (!SetPixelFormat(deviceContext, format, &pixelFormatDescriptor)) {
        fflush(stdout);
        fprintf(stderr, "Failed to set pixel format\n");
        return EXIT_FAILURE;
    }

    HGLRC openglContext = wglCreateContext(deviceContext);
    if (!openglContext) {
        fflush(stdout);
        fprintf(stderr, "Failed to create opengl context\n");
        return EXIT_FAILURE;
    }

    if (!wglMakeCurrent(deviceContext, openglContext)) {
        fflush(stdout);
        fprintf(stderr, "Failed to bind opengl context\n");
        return EXIT_FAILURE;
    }

    if (!gladLoadGL(GladLoadProc)) {
        fflush(stdout);
        fprintf(stderr, "Failed to load opengl functions\n");
        return EXIT_FAILURE;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLMessageCallback, NULL);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);

    GLuint vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    const char* vertexShaderSource = "#version 440 core\n"
                                     "\n"
                                     "layout(location = 0) out vec2 v_Coord;"
                                     "\n"
                                     "void main() {\n"
                                     "    float x = gl_VertexID & 1;\n"
                                     "    float y = gl_VertexID & 2;\n"
                                     "    v_Coord = vec2(x, y);\n"
                                     "    gl_Position = vec4(x * 2.0 - 1.0, y * 2.0 - 1.0, 0.0, 1.0);"
                                     "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    const char* fragmentShaderSource = "#version 440 core\n"
                                       "\n"
                                       "layout(location = 0) out vec4 o_Color;\n"
                                       "\n"
                                       "layout(location = 0) in vec2 v_Coord;\n"
                                       "\n"
                                       "layout(location = 0) uniform vec2 u_WindowSize;\n"
                                       "layout(location = 1) uniform vec3 u_CameraPosition;\n"
                                       "layout(location = 2) uniform vec3 u_CameraForward;\n"
                                       "layout(location = 3) uniform vec3 u_CameraRight;\n"
                                       "layout(location = 4) uniform vec3 u_CameraUp;\n"
                                       "\n"
                                       "struct Hit {\n"
                                       "    bool Hit;"
                                       "    float Distance;"
                                       "    vec3 Point;"
                                       "    vec3 Normal;"
                                       "};\n"
                                       "\n"
                                       "Hit IntersectSphere(vec3 rayPos, vec3 rayDir, vec3 spherePos, float sphereRadius) {\n"
                                       "    Hit hit;\n"
                                       "    hit.Hit = false;\n"
                                       "    float t = dot(spherePos-rayPos, rayDir);\n"
                                       "    vec3 p = rayPos + (rayDir * t);\n"
                                       "    float y = length(spherePos - p);\n"
                                       "    if (y < sphereRadius) {\n"
                                       "        float x = sqrt(sphereRadius * sphereRadius - y * y);\n"
                                       "        hit.Distance = t - x;\n"
                                       "        if (hit.Distance < 0.0) {\n"
                                       "            return hit;\n"
                                       "        }\n"
                                       "        hit.Point = rayPos + (rayDir * hit.Distance);\n"
                                       "        hit.Normal = (hit.Point - spherePos) / sphereRadius;\n"
                                       "        hit.Hit = true;\n"
                                       "    }\n"
                                       "    return hit;\n"
                                       "}\n"
                                       "\n"
                                       "Hit IntersectAABB(vec3 rayPos, vec3 rayDir, vec3 boxPos, vec3 boxSize) {\n"
                                       "    Hit hit;\n"
                                       "    hit.Hit = false;\n"
                                       "    vec3 m = 1.0 / rayDir;\n"
                                       "    vec3 n = m * (rayPos - boxPos);\n"
                                       "    vec3 k = abs(m) * boxSize;\n"
                                       "    vec3 t1 = -n - k;\n"
                                       "    vec3 t2 = -n + k;\n"
                                       "    float tN = max(max(t1.x, t1.y), t1.z);\n"
                                       "    float tF = min(min(t2.x, t2.y), t2.z);\n"
                                       "    if(tN > tF || tF < 0.0 || tN < 0.0) {\n"
                                       "        return hit;\n"
                                       "    }\n"
                                       "    // TODO: hit.Distance\n"
                                       "    // TODO: hit.Point\n"
                                       "    hit.Normal = normalize(-sign(rayDir) * step(t1.yzx,t1.xyz) * step(t1.zxy,t1.xyz));\n"
                                       "    hit.Hit = true;\n"
                                       "    return hit;\n"
                                       "}\n"
                                       "\n"
                                       "void main() {\n"
                                       "    vec3 rayDir = normalize(\n"
                                       "        u_CameraRight * (v_Coord.x * 2.0 - 1.0) * (u_WindowSize.x / u_WindowSize.y) +\n"
                                       "        u_CameraUp * (v_Coord.y * 2.0 - 1.0) +\n"
                                       "        u_CameraForward\n"
                                       "    );\n"
                                       /*
                                       "    vec3 boxPos = vec3(0.0, 0.0, 2.0);\n"
                                       "    vec3 boxSize = vec3(1.0);\n"
                                       "    Hit hit = IntersectAABB(u_CameraPosition, rayDir, boxPos, boxSize);\n"
                                       "    if (hit.Hit) {\n"
                                       "        o_Color = vec4(hit.Normal, 1.0);\n"
                                       "    } else {\n"
                                       "        o_Color = vec4(0.1, 0.1, 0.1, 1.0);\n"
                                       "    }\n"
                                        */
                                       "    vec3 spherePos = vec3(0.0, 0.0, 12.0);\n"
                                       "    float sphereRadius = 10.0;\n"
                                       "    Hit hit = IntersectSphere(u_CameraPosition, rayDir, spherePos, sphereRadius);\n"
                                       "    if (hit.Hit) {\n"
                                       "        vec3 boxPos = round(hit.Point);\n"
                                       "        vec3 boxSize = vec3(0.4);\n"
                                       "        hit = IntersectAABB(u_CameraPosition, rayDir, boxPos, boxSize);\n"
                                       "        if (hit.Hit) {\n"
                                       "            o_Color = vec4(hit.Normal, 1.0);\n"
                                       "        } else {\n"
                                       "            o_Color = vec4(0.1, 0.1, 0.1, 1.0);\n"
                                       "        }\n"
                                       "    } else {\n"
                                       "        o_Color = vec4(0.1, 0.1, 0.1, 1.0);\n"
                                       "    }\n"
                                       "}\n";

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);

    glUseProgram(shader);

    typedef struct Camera {
        Vector3f Position;
        Vector3f Forward;
        Vector3f Right;
        Vector3f Up;
    } Camera;

    float CameraYaw   = 0.0f;
    float CameraPitch = 0.0f;

    Camera camera = {
        .Position = { 0.0f, 0.0f, 0.0f },
        .Forward  = { 0.0f, 0.0f, 1.0f },
        .Right    = { 1.0f, 0.0f, 0.0f },
        .Up       = { 0.0f, 1.0f, 0.0f },
    };

    ShowWindow(window, SW_SHOW);

    float dt = 0.0f;
    LARGE_INTEGER largeInteger;

    QueryPerformanceFrequency(&largeInteger);
    double inverseFrequency = 1.0 / (double)largeInteger.QuadPart;

    QueryPerformanceCounter(&largeInteger);
    double lastFrame = (double)largeInteger.QuadPart * inverseFrequency;

    while (Running) {
        MSG message;
        while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        QueryPerformanceCounter(&largeInteger);
        double time = (double)largeInteger.QuadPart * inverseFrequency;
        dt          = (float)(time - lastFrame);
        lastFrame   = time;

        // Camera
        {
            CameraYaw = fmodf(CameraYaw + ((float)XMouseDelta * 0.5f), 360.0f);
            CameraPitch += (float)YMouseDelta * 0.5f;
            if (CameraPitch > 89.0f) {
                CameraPitch = 89.0f;
            }
            if (CameraPitch < -89.0f) {
                CameraPitch = -89.0f;
            }

            camera.Forward.x = sinf(CameraYaw * (float)(M_PI / 180.0)) * cosf(CameraPitch * (float)(M_PI / 180.0));
            camera.Forward.y = -sinf(CameraPitch * (float)(M_PI / 180.0));
            camera.Forward.z = cosf(CameraYaw * (float)(M_PI / 180.0)) * cosf(CameraPitch * (float)(M_PI / 180.0));
            camera.Forward   = Vector3f_Normalized(camera.Forward);
            camera.Right = Vector3f_Normalized(Vector3f_Cross((Vector3f){ 0.0f, 1.0f, 0.0f }, camera.Forward));
            camera.Up    = Vector3f_Normalized(Vector3f_Cross(camera.Forward, camera.Right));

            float moveSpeed = 5.0f * dt;
            if (WPressed)
                camera.Position = Vector3f_Add(camera.Position, Vector3f_MultiplyScalar(camera.Forward, moveSpeed));
            if (SPressed)
                camera.Position = Vector3f_Subtract(camera.Position, Vector3f_MultiplyScalar(camera.Forward, moveSpeed));
            if (APressed)
                camera.Position = Vector3f_Subtract(camera.Position, Vector3f_MultiplyScalar(camera.Right, moveSpeed));
            if (DPressed)
                camera.Position = Vector3f_Add(camera.Position, Vector3f_MultiplyScalar(camera.Right, moveSpeed));
            if (EPressed)
                camera.Position = Vector3f_Add(camera.Position, Vector3f_MultiplyScalar(camera.Up, moveSpeed));
            if (QPressed)
                camera.Position = Vector3f_Subtract(camera.Position, Vector3f_MultiplyScalar(camera.Up, moveSpeed));

            XMouseDelta = 0;
            YMouseDelta = 0;
        }

        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glProgramUniform2f(shader, 0, (GLfloat)WindowWidth, (GLfloat)WindowHeight);
        glProgramUniform3fv(shader, 1, 1, (GLfloat*)&camera.Position);
        glProgramUniform3fv(shader, 2, 1, (GLfloat*)&camera.Forward);
        glProgramUniform3fv(shader, 3, 1, (GLfloat*)&camera.Right);
        glProgramUniform3fv(shader, 4, 1, (GLfloat*)&camera.Up);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        SwapBuffers(deviceContext);
    }

    return EXIT_SUCCESS;
}

GLADapiproc GladLoadProc(const char* name) {
    void* ptr = (void*)wglGetProcAddress(name);
    if (ptr == 0 || (ptr == (void*)0x1) || (ptr == (void*)0x2) || (ptr == (void*)0x3) || (ptr == (void*)-1)) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        ptr            = (void*)GetProcAddress(module, name);
    }
    return ptr;
}
