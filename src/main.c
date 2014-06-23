#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;
};

static void
create_texture(GLuint *tex,
               GLuint *fbo,
               int width, int height,
               const GLubyte *data)
{
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexImage2D(GL_TEXTURE_2D,
                     0, /* level */
                     GL_RGBA,
                     width, height,
                     0, /* border */
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     data);

        glGenFramebuffers(1, fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               *tex,
                               0 /* level */);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
               GL_FRAMEBUFFER_COMPLETE);
}

static void
do_blit(void)
{
        GLuint src_tex, dst_tex;
        GLuint src_fbo, dst_fbo;
        static const GLubyte src_data[2 * 4] = {
                0xff, 0x00, 0x00, 0xff,
                0x00, 0x00, 0xff, 0xff,
        };
        GLubyte dst_data[256 * 4];
        int i, j;

        create_texture(&src_tex, &src_fbo, 2, 1, src_data);
        create_texture(&dst_tex, &dst_fbo, 256, 1, NULL);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);

        glBlitFramebuffer(0, 0, /* src x0 y0 */
                          2, 1, /* src x1 y1 */
                          0, 0, /* dst x0 y0 */
                          256, 1, /* dst x0 y0 */
                          GL_COLOR_BUFFER_BIT,
                          GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &src_fbo);
        glDeleteFramebuffers(1, &dst_fbo);

        glBindTexture(GL_TEXTURE_2D, dst_tex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst_data);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteTextures(1, &src_tex);
        glDeleteTextures(1, &dst_tex);

        for (i = 0; i < 256; i++) {
                printf("%02x:  ", i);
                for (j = 0; j < 4; j++)
                        printf("%02x", dst_data[i * 4 + j]);
                fputc('\n', stdout);
        }
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        struct glthing glthing;
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

        glthing.window = SDL_CreateWindow("glthing",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640, 480,
                                          SDL_WINDOW_OPENGL);

        if (glthing.window == NULL) {
                fprintf(stderr,
                        "Failed to create SDL window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_sdl;
        }

        glthing.gl_context = SDL_GL_CreateContext(glthing.window);
        if (glthing.gl_context == NULL) {
                fprintf(stderr,
                        "Failed to create GL context window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_window;
        }

        SDL_GL_MakeCurrent(glthing.window, glthing.gl_context);

        do_blit();

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(glthing.gl_context);
out_window:
        SDL_DestroyWindow(glthing.window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
