#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

static void
do_test(void)
{
        const int TEX_WIDTH = 256;
        const int TEX_HEIGHT = 1;
        uint8_t tex_data[TEX_WIDTH * TEX_HEIGHT * 4];
        uint16_t cpu_result[TEX_WIDTH * TEX_HEIGHT];
        uint16_t pbo_result[TEX_WIDTH * TEX_HEIGHT];
        uint8_t *p = tex_data;
        GLuint pbo, tex;
        int x, y;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        for (y = 0; y < TEX_HEIGHT; y++) {
                for (x = 0; x < TEX_WIDTH; x++) {
                        p[0] = x;
                        p[1] = 0;
                        p[2] = 0;
                        p[3] = 255;
                        p += 4;
                }
        }

        /* Initially upload the data without a PBO */
        glTexImage2D(GL_TEXTURE_2D,
                     0, /* level */
                     GL_RGB565,
                     TEX_WIDTH, TEX_HEIGHT,
                     0, /* border */
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     tex_data);

        glGetTexImage(GL_TEXTURE_2D, 0,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      cpu_result);

        glDeleteTextures(1, &tex);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        /* Upload the same texture but this time with a pbo */
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER,
                     sizeof tex_data,
                     tex_data,
                     GL_STATIC_DRAW);
        glTexImage2D(GL_TEXTURE_2D,
                     0, /* level */
                     GL_RGB565,
                     TEX_WIDTH, TEX_HEIGHT,
                     0, /* border */
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     NULL);
        glDeleteBuffers(1, &pbo);

        glGetTexImage(GL_TEXTURE_2D, 0,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                      pbo_result);

        for (x = 0; x < TEX_WIDTH; x++) {
                if (cpu_result[x] != pbo_result[x]) {
                        printf("%i %i %i\n",
                               x,
                               cpu_result[x] >> 11,
                               pbo_result[x] >> 11);
                }
        }

        glDeleteTextures(1, &tex);
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        SDL_Window *window;
        SDL_GLContext gl_context;
        int res;

        res = SDL_Init(SDL_INIT_VIDEO);
        if (res < 0) {
                fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
                ret = EXIT_FAILURE;
                goto out;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow("glthing",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  640, 480,
                                  SDL_WINDOW_OPENGL);

        if (window == NULL) {
                fprintf(stderr,
                        "Failed to create SDL window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_sdl;
        }

        gl_context = SDL_GL_CreateContext(window);
        if (gl_context == NULL) {
                fprintf(stderr,
                        "Failed to create GL context window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_window;
        }

        SDL_GL_MakeCurrent(window, gl_context);

        do_test();

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(gl_context);
out_window:
        SDL_DestroyWindow(window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
