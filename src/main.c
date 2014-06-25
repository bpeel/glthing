#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;
};

#define DST_WIDTH 256
#define DST_HEIGHT 1

#define SRC_WIDTH 2
#define SRC_HEIGHT 1

/* Texture coordinates for the left and right vertices. We want the
 * center of the leftmost pixel in the destination to be the center of
 * the left texel and the center of the rightmost pixel to be the other
 * texel */
#define TX_LEFT (0.5f - (SRC_WIDTH - 1.0f) / (2 * DST_WIDTH - 1))
#define TX_RIGHT (SRC_WIDTH - 0.5 + (SRC_WIDTH - 1.0f) / (2 * DST_WIDTH - 1))

static void
create_texture(GLenum target,
               GLuint *tex,
               GLuint *fbo,
               int width, int height,
               const GLubyte *data)
{
        glGenTextures(1, tex);
        glBindTexture(target, *tex);
        glTexImage2D(target,
                     0, /* level */
                     GL_RGBA,
                     width, height,
                     0, /* border */
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     data);

        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(target,
                        GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(target,
                        GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               target,
                               *tex,
                               0 /* level */);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
               GL_FRAMEBUFFER_COMPLETE);
}

static void
do_render(GLenum target, GLubyte *dst_data)
{
        GLuint src_tex, dst_tex;
        GLuint src_fbo, dst_fbo;
        static const GLubyte src_data[SRC_WIDTH * SRC_HEIGHT * 4] = {
                0x00, 0x00, 0x00, 0xff,
                0xff, 0x00, 0x00, 0xff,
        };
        float tx_left, tx_right;

        create_texture(target, &src_tex, &src_fbo,
                       SRC_WIDTH, SRC_HEIGHT, src_data);
        create_texture(target, &dst_tex, &dst_fbo,
                       DST_WIDTH, DST_HEIGHT, NULL);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);

        glViewport(0, 0, DST_WIDTH, 1);

        glBindTexture(target, src_tex);
        glEnable(target);

        tx_left = TX_LEFT;
        tx_right = TX_RIGHT;

        if (target == GL_TEXTURE_2D) {
                tx_left /= SRC_WIDTH;
                tx_right /= SRC_WIDTH;
        }

        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(tx_left, 0.5f);
        glVertex2i(-1, -1);
        glTexCoord2f(tx_right, 0.5f);
        glVertex2i(1, -1);
        glTexCoord2f(tx_left, 0.5f);
        glVertex2i(-1, 1);
        glTexCoord2f(tx_right, 0.5f);
        glVertex2i(1, 1);
        glEnd();

        glDisable(target);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &src_fbo);
        glDeleteFramebuffers(1, &dst_fbo);

        glBindTexture(target, dst_tex);
        glGetTexImage(target,
                      0, /* level */
                      GL_RGBA, GL_UNSIGNED_BYTE,
                      dst_data);
        glBindTexture(target, 0);

        glDeleteTextures(1, &src_tex);
        glDeleteTextures(1, &dst_tex);
}

static void
run_test(GLenum target)
{
        GLubyte dst_data[DST_WIDTH * DST_HEIGHT * 4];
        int max_diff = 0, diff;
        int i;

        if (target == GL_TEXTURE_2D)
                printf("Using a 2D texture\n");
        else if (target == GL_TEXTURE_RECTANGLE)
                printf("Using a rectangle texture\n");

        do_render(target, dst_data);

        for (i = 0; i < DST_WIDTH; i++) {
                diff = abs((int) dst_data[i * 4] - i);
                if (diff > max_diff)
                        max_diff = diff;
                printf("%02x%c",
                       dst_data[i * 4],
                       i != dst_data[i * 4] ? '*' : ' ');
                if ((i + 1) % 16 == 0)
                        fputc('\n', stdout);
                else
                        fputc(' ', stdout);
        }

        printf("Maximum diff = %i (%f%%)\n",
               max_diff,
               max_diff * 100.0f / 255.0f);
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        struct glthing glthing;
        GLenum target = GL_TEXTURE_RECTANGLE;
        int res;

        if (argc > 1) {
                if (!strcmp(argv[1], "-r")) {
                        target = GL_TEXTURE_RECTANGLE;
                } else if (!strcmp(argv[1], "-2")) {
                        target = GL_TEXTURE_2D;
                } else {
                        fprintf(stderr, "usage: glthing [-r or -2]\n");
                        return 1;
                }
        }

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

        run_test(target);

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(glthing.gl_context);
out_window:
        SDL_DestroyWindow(glthing.window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
