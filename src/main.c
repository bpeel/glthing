#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "shader-data.h"

struct prim {
        GLuint buffer;
        GLuint array;
};

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;

        GLuint program;
        GLuint tex_location;
        struct prim square;

        GLuint tex;

        int last_w, last_h;

        bool quit;
};

struct vertex {
        float x, y;
};

static void
create_texture(struct glthing *glthing)
{
        const int TEX_SIZE = 256;
        uint8_t tex_data[TEX_SIZE * TEX_SIZE * 4];
        static const uint8_t color[] = { 70, 0x00, 0x00, 0xff };
        uint8_t *p = tex_data;
        GLuint pbo;
        int x, y;

        glGenTextures(1, &glthing->tex);
        glBindTexture(GL_TEXTURE_2D, glthing->tex);

        for (y = 0; y < TEX_SIZE; y++) {
                for (x = 0; x < TEX_SIZE; x++) {
                        memcpy(p, color, 4);
                        p += 4;
                }
        }

        /* Initially upload the data without a PBO */
        glTexImage2D(GL_TEXTURE_2D,
                     0, /* level */
                     GL_R3_G3_B2,
                     TEX_SIZE, TEX_SIZE,
                     0, /* border */
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     tex_data);

        /* Update a sub-region of the texture with the same data but
         * this time using a PBO */
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER,
                     sizeof tex_data,
                     tex_data,
                     GL_STATIC_DRAW);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0, /* level */
                        TEX_SIZE / 4, TEX_SIZE / 4,
                        TEX_SIZE / 2, TEX_SIZE / 2,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        NULL);
        glDeleteBuffers(1, &pbo);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);

        printf("%i %i %i\n",
               color[0],
               tex_data[0], tex_data[TEX_SIZE / 4 * 4 +
                                     TEX_SIZE / 4 * 4 * TEX_SIZE]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static void
create_square(struct prim *square)
{
        static const struct vertex vertex_data[] = {
                { -1, -1 },
                { 1, -1 },
                { -1, 1 },
                { 1, 1 }
        };

        glGenVertexArrays(1, &square->array);
        glBindVertexArray(square->array);

        glGenBuffers(1, &square->buffer);
        glBindBuffer(GL_ARRAY_BUFFER, square->buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof vertex_data,
                     vertex_data, /* data */
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2,
                              GL_FLOAT, GL_FALSE,
                              sizeof (struct vertex),
                              (void *) offsetof(struct vertex, x));

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
}

static void
destroy_prim(struct prim *prim)
{
        glDeleteVertexArrays(1, &prim->array);
        glDeleteBuffers(1, &prim->buffer);
}

static void
redraw(struct glthing *glthing)
{
        int w, h;

        SDL_GetWindowSize(glthing->window, &w, &h);

        if (w != glthing->last_w || h != glthing->last_h) {
                glViewport(0.0f, 0.0f, w, h);
                glthing->last_w = w;
                glthing->last_h = h;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        create_texture(glthing);

        glUseProgram(glthing->program);
        glBindVertexArray(glthing->square.array);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDeleteTextures(1, &glthing->tex);

        SDL_GL_SwapWindow(glthing->window);
}

static void
process_window_event(struct glthing *glthing,
                     const SDL_WindowEvent *event)
{
        switch (event->event) {
        case SDL_WINDOWEVENT_CLOSE:
                glthing->quit = true;
                break;
        }
}

static void
process_event(struct glthing *glthing,
              const SDL_Event *event)
{
        switch (event->type) {
        case SDL_WINDOWEVENT:
                process_window_event(glthing, &event->window);
                break;

        case SDL_QUIT:
                glthing->quit = true;
                break;
        }
}

static bool
main_loop(struct glthing *glthing)
{
        SDL_Event event;

        while (!glthing->quit) {
                if (SDL_PollEvent(&event))
                        process_event(glthing, &event);
                else
                        redraw(glthing);
        }

        return true;
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        struct glthing glthing;
        int res;

        memset(&glthing, 0, sizeof glthing);

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

        glthing.program = shader_data_load_program(GL_VERTEX_SHADER,
                                                   "vertex-shader.glsl",
                                                   GL_FRAGMENT_SHADER,
                                                   "fragment-shader.glsl",
                                                   NULL);
        if (glthing.program == 0)
                goto out_context;

        glthing.tex_location =
                glGetUniformLocation(glthing.program, "tex");

        create_square(&glthing.square);

        glUseProgram(glthing.program);
        glUniform1i(glthing.tex_location, 0);

        if (!main_loop(&glthing))
                ret = EXIT_FAILURE;

        destroy_prim(&glthing.square);

        glDeleteProgram(glthing.program);

out_context:
        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(glthing.gl_context);
out_window:
        SDL_DestroyWindow(glthing.window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
