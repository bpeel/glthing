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
        const int TEX_HEIGHT = 1;
        uint8_t tex_data[TEX_WIDTH * TEX_HEIGHT * 4];
        uint8_t cpu_result[TEX_WIDTH * TEX_HEIGHT * bpp];
        uint8_t pbo_result[TEX_WIDTH * TEX_HEIGHT * bpp];
        uint8_t *p = tex_data;
        GLuint pbo, tex;
        bool shown_format = false;
        int x, y;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        for (y = 0; y < TEX_HEIGHT; y++) {
                for (x = 0; x < TEX_WIDTH; x++) {
                        p[0] = x;
                        p[1] = x;
                        p[2] = x;
                        p[3] = x;
                        p += 4;
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

        for (x = 0; x < TEX_WIDTH; x++) {
                if (memcmp(cpu_result + x * bpp, pbo_result + x * bpp, bpp)) {
                        if (!shown_format) {
                                printf("internal_format = 0x%04x\n",
                                       internal_format);
                                shown_format = true;
                        }
                        printf("0x%02x 0x", x);
                        for (y = 0; y < bpp; y++)
                                printf("%02x", cpu_result[x * bpp + y]);
                        printf(" 0x");
                        for (y = 0; y < bpp; y++)
                                printf("%02x", pbo_result[x * bpp + y]);
                        fputc('\n', stdout);
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

        do_test(GL_RGB565, GL_RGBA, GL_UNSIGNED_BYTE,
                GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 2);
        do_test(GL_RGBA4, GL_RGBA, GL_UNSIGNED_BYTE,
                GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 2);
        do_test(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
                GL_RGBA, GL_UNSIGNED_BYTE, 4);
        do_test(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                GL_RED_INTEGER, GL_UNSIGNED_INT, 4);
        do_test(GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                GL_RED_INTEGER, GL_UNSIGNED_SHORT, 2);
        do_test(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                GL_RED_INTEGER, GL_UNSIGNED_BYTE, 1);
        do_test(GL_R16, GL_RED, GL_UNSIGNED_INT,
                GL_RED, GL_UNSIGNED_SHORT, 2);
        do_test(GL_R8, GL_RED, GL_UNSIGNED_INT,
                GL_RED, GL_UNSIGNED_BYTE, 1);
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
