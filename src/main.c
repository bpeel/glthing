#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

static void
do_test(GLenum internal_format,
        GLenum unpack_format,
        GLenum unpack_type,
        GLenum pack_format,
        GLenum pack_type,
        int bpp)
{
        const int TEX_WIDTH = 256;
        const int TEX_HEIGHT = 256;
        uint16_t tex_data[TEX_WIDTH * TEX_HEIGHT * 2];
        uint16_t *p = tex_data;
        uint8_t cpu_result[TEX_WIDTH * TEX_HEIGHT * bpp];
        uint8_t pbo_result[TEX_WIDTH * TEX_HEIGHT * bpp];
        GLuint pbo, tex;
        int x, y;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        for (y = 0; y < TEX_HEIGHT; y++) {
                for (x = 0; x < TEX_WIDTH; x++) {
                        p[1] = p[0] = x + y * TEX_WIDTH;
                        p += 2;
                }
        }

        /* Initially upload the data without a PBO */
        glTexImage2D(GL_TEXTURE_2D,
                     0, /* level */
                     internal_format,
                     TEX_WIDTH, TEX_HEIGHT,
                     0, /* border */
                     unpack_format, unpack_type,
                     tex_data);

        glGetTexImage(GL_TEXTURE_2D, 0,
                      pack_format, pack_type,
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
                     internal_format,
                     TEX_WIDTH, TEX_HEIGHT,
                     0, /* border */
                     unpack_format, unpack_type,
                     NULL);
        glDeleteBuffers(1, &pbo);

        glGetTexImage(GL_TEXTURE_2D, 0,
                      pack_format, pack_type,
                      pbo_result);

        printf("16-bit input value,8-bit value without pbo,with pbo,"
               "expected value,winner\n");

        for (x = 0; x < TEX_WIDTH * TEX_HEIGHT; x++) {
                int src_value = (int16_t) x;
                int cpu_value = (int8_t) cpu_result[x * bpp];
                int pbo_value = (int8_t) pbo_result[x * bpp];
                float expected_value = src_value * 127 / 32767.0f;
                int int_expected_value = round(expected_value);

                if (cpu_value != pbo_value ||
                    int_expected_value != cpu_value) {
                        printf("%i,%i,%i,%f,%s\n",
                               src_value, cpu_value, pbo_value, expected_value,
                               int_expected_value == cpu_value ? "cpu" :
                               int_expected_value == pbo_value ? "pbo" :
                               "none");
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

        do_test(GL_RG8_SNORM, GL_RG, GL_SHORT,
                GL_RG, GL_BYTE, 2);

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(gl_context);
out_window:
        SDL_DestroyWindow(window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
