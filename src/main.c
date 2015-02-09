#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

static GLuint
make_texture(void)
{
        const int TEX_WIDTH = 4096;
        uint8_t tex_data[TEX_WIDTH * 4];
        GLuint tex;
        int x;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_1D, tex);

        for (x = 0; x < TEX_WIDTH; x++) {
                tex_data[x] = rand();
        }

        glTexImage1D(GL_TEXTURE_1D,
                     0, /* level */
                     GL_RGBA,
                     TEX_WIDTH,
                     0, /* border */
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     tex_data);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        return tex;
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        SDL_Window *window;
        SDL_GLContext gl_context;
        SDL_Event event;
        static const GLfloat positions[] = {
                -1.0f, -1.0f,
                1.0f, -1.0f,
                -1.0f, 1.0f,
                1.0f, 1.0f
        };
        GLfloat tex_coords[4];
        GLuint tex;
        int res;
        int frame_num = 0;

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

        window = SDL_CreateWindow("glthing",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  1024, 768,
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

        tex = make_texture();

        glEnable(GL_TEXTURE_1D);

        glVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 2, positions);
        glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(1, GL_FLOAT, sizeof(GLfloat), tex_coords);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        while (true) {
                while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT)
                                goto done;
                }

                tex_coords[0] = tex_coords[2] = frame_num & 1;
                tex_coords[1] = tex_coords[3] = (frame_num ^ 1) & 1;

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                SDL_GL_SwapWindow(window);

                frame_num++;
        };

done:

        glDeleteTextures(1, &tex);

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(gl_context);
out_window:
        SDL_DestroyWindow(window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
