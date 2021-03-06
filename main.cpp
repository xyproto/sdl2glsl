#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_video.h>

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#include <OpenGL/OpenGL.h>

#if ESSENTIAL_GL_PRACTICES_SUPPORT_GL3
#include <OpenGL/gl3.h>
#else // ESSENTIAL_GL_PRACTICES_SUPPORT_GL3
#include <OpenGL/gl.h>
#endif // ESSENTIAL_GL_PRACTICES_SUPPORT_GL3

#else // __APPLE__

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#endif // __APPLE

const int WIN_WIDTH = 640;
const int WIN_HEIGHT = 480;

const int WORLD_WIDTH = 320;
const int WORLD_HEIGHT = 240;

struct sprite {
    float x;
    float y;
    float vx;
    float vy;
};

const int MAX_SPRITES = 20;

#ifndef __APPLE__

// I'm avoiding the use of GLEW or some extensions handler, but that
// doesn't mean you should...
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;

bool initGLExtensions()
{
    glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
    glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
    glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");

    return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv
        && glGetShaderInfoLog && glDeleteShader && glAttachShader && glCreateProgram
        && glLinkProgram && glValidateProgram && glGetProgramiv && glGetProgramInfoLog
        && glUseProgram;
}

#endif

GLuint compileShader(const char* source, GLuint shaderType)
{
    std::cout << "Compiling shader:" << std::endl << source << std::endl;
    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, &source, nullptr);
    // Compile shader
    glCompileShader(result);

    // Check vertex shader for errors
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(result, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE) {
        std::cout << "Error when compiling shader: " << result << "!" << std::endl;
        GLint logLength;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar* log = (GLchar*)malloc(logLength);
            glGetShaderInfoLog(result, logLength, &logLength, log);
            std::cout << "Shader compile log:" << log << std::endl;
            free(log);
        }
        glDeleteShader(result);
        result = 0;
    } else {
        std::cout << "Shader compiled correctly. ID = " << result << std::endl;
    }
    return result;
}

GLuint compileProgram(const char* vtxFile, const char* fragFile)
{
    GLuint vtxShaderId, fragShaderId;

    auto programId = glCreateProgram();

    std::ifstream f(vtxFile);
    std::string source((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    vtxShaderId = compileShader(source.c_str(), GL_VERTEX_SHADER);

    f = std::ifstream(fragFile);
    source = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    fragShaderId = compileShader(source.c_str(), GL_FRAGMENT_SHADER);

    if (vtxShaderId && fragShaderId) {
        // Associate shader with program
        glAttachShader(programId, vtxShaderId);
        glAttachShader(programId, fragShaderId);
        glLinkProgram(programId);
        glValidateProgram(programId);

        // Check the status of the compile/link
        GLint logLen;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            char* log = (char*)malloc(logLen * sizeof(char));
            // Show any errors as appropriate
            glGetProgramInfoLog(programId, logLen, &logLen, log);
            std::cout << "Prog Info Log: " << std::endl << log << std::endl;
            free(log);
        }
    }
    if (vtxShaderId) {
        glDeleteShader(vtxShaderId);
    }
    if (fragShaderId) {
        glDeleteShader(fragShaderId);
    }
    return programId;
}

void presentBackBuffer(
    SDL_Renderer* renderer, SDL_Window* win, SDL_Texture* backBuffer, GLuint programId)
{
    GLint oldProgramId;
    // For obtaining the textureid (in driverdata->texture)
    // Detach the texture
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderClear(renderer);

    SDL_GL_BindTexture(backBuffer, nullptr, nullptr);
    if (programId != 0) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgramId);
        glUseProgram(programId);
    }

    GLfloat minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;

    // Coordinates of where to paint in the window.
    minx = 0.0f;
    miny = 0.0f;
    maxx = WIN_WIDTH;
    maxy = WIN_HEIGHT;

    minu = 0.0f;
    maxu = 1.0f;
    minv = 0.0f;
    maxv = 1.0f;

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(minu, minv);
    glVertex2f(minx, miny);
    glTexCoord2f(maxu, minv);
    glVertex2f(maxx, miny);
    glTexCoord2f(minu, maxv);
    glVertex2f(minx, maxy);
    glTexCoord2f(maxu, maxv);
    glVertex2f(maxx, maxy);
    glEnd();
    SDL_GL_SwapWindow(win);

    if (programId != 0) {
        glUseProgram(oldProgramId);
    }
}

#ifdef __APPLE__
void initializeFileSystem()
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX)) {
        std::cerr << "Couldn't get file system representation! " << std::endl;
    }
    CFRelease(resourcesURL);
    chdir(path);
}
#endif

int main()
{
#ifdef __APPLE__
    initializeFileSystem();
#endif

    if (SDL_Init(SDL_INIT_EVERYTHING | SDL_VIDEO_OPENGL) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Custom shader with SDL2 renderer!", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT, 0);

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    SDL_Renderer* renderer
        = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(renderer, &rendererInfo);

    if (!strncmp(rendererInfo.name, "opengl", 6)) {
        std::cout << "Es OpenGL!" << std::endl;
#ifndef __APPLE__
        // If you want to use GLEW or some other GL extension handler, do it here!
        if (!initGLExtensions()) {
            std::cout << "Couldn't init GL extensions!" << std::endl;
            SDL_Quit();
            exit(-1);
        }
#endif
    }

    // Compilar el shader y dejarlo listo para usar.
    auto programId = compileProgram(SHADERDIR "std.vertex", SHADERDIR "crt.fragment");
    std::cout << "programId = " << programId << std::endl;

    // Put your own bmp image here
    SDL_Surface* bmpSurf = IMG_Load(IMGDIR "image.png");
    SDL_Texture* bmpTex = SDL_CreateTextureFromSurface(renderer, bmpSurf);
    SDL_FreeSurface(bmpSurf);

    // Make a target texture to render too
    SDL_Texture* texTarget = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WORLD_WIDTH, WORLD_HEIGHT);

    int done = 0;
    int useShader = 0;
    SDL_Rect targetRect;

    targetRect.x = 0;
    targetRect.y = 0;
    targetRect.w = 32;
    targetRect.h = 32;

    struct sprite sprites[MAX_SPRITES];

    for (int i = 0; i < MAX_SPRITES; ++i) {
        sprites[i].x = static_cast<float>(rand() % (WORLD_WIDTH - targetRect.w));
        sprites[i].y = static_cast<float>(rand() % (WORLD_HEIGHT - targetRect.h));
        sprites[i].vx = static_cast<float>(rand() % 5 / 10.0f) - 0.2f;
        sprites[i].vy = static_cast<float>(rand() % 5 / 10.0f) - 0.2f;
    }

    // Set the background to white, for being able to see the shader
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    while (!done) {
        // Render to the texture
        SDL_SetRenderTarget(renderer, texTarget);
        SDL_RenderClear(renderer);

        // We paint MAX_SPRITES
        struct sprite* spr;
        for (int i = 0; i < MAX_SPRITES; ++i) {
            spr = &(sprites[i]);
            spr->x += spr->vx;
            if (spr->x < 0) {
                spr->x = 0;
                spr->vx *= -1;
            }
            if (spr->x > WORLD_WIDTH - targetRect.w) {
                spr->x = static_cast<float>(WORLD_WIDTH - targetRect.w);
                spr->vx *= -1;
            }
            spr->y += spr->vy;
            if (spr->y < 0) {
                spr->y = 0;
                spr->vy *= -1;
            }
            if (spr->y > WORLD_HEIGHT - targetRect.h) {
                spr->y = static_cast<float>(WORLD_HEIGHT - targetRect.h);
                spr->vy *= -1;
            }

            targetRect.x = static_cast<int>(spr->x);
            targetRect.y = static_cast<int>(spr->y);

            SDL_RenderCopy(renderer, bmpTex, nullptr, &targetRect);
        }

        presentBackBuffer(renderer, win, texTarget, programId);

        /* This could go in a separate function */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    useShader ^= 1;
                    std::cout << "useShader = " << (useShader ? "true" : "false") << std::endl;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    done = 1;
                }
            }
        }
    }

    SDL_DestroyTexture(texTarget);
    SDL_DestroyTexture(bmpTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
