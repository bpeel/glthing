#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "shader-data.h"

struct grid {
        GLuint buffer;
        GLuint array;
};

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;

        GLuint program;
        GLuint offset_location;
        struct grid grid;

        int last_w, last_h;

        bool quit;
};

struct vertex {
        float x, y, z;
        uint8_t r, g, b;
};

#define GRID_SIZE 20
#define N_GRID_SQUARES (GRID_SIZE * GRID_SIZE / 2)

static void
create_grid(struct grid *grid)
{
        struct vertex *v;
        uint16_t *ind;
        float xp, yp;
        int y, x, i;

        glGenVertexArrays(1, &grid->array);
        glBindVertexArray(grid->array);

        glGenBuffers(1, &grid->buffer);
        glBindBuffer(GL_ARRAY_BUFFER, grid->buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     N_GRID_SQUARES *
                     (4 * sizeof (struct vertex) +
                      6 * sizeof (uint16_t)),
                     NULL, /* data */
                     GL_STATIC_DRAW);

        v = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        for (y = 0; y < GRID_SIZE; y++) {
                yp = 2.0f * y / GRID_SIZE - 1.0f;

                for (x = 0; x < GRID_SIZE / 2; x++) {
                        if (y & 1)
                                xp = (4.0f * x + 2.0f) / GRID_SIZE - 1.0f;
                        else
                                xp = 4.0f * x / GRID_SIZE - 1.0f;

                        v[0].x = xp;
                        v[0].y = yp;
                        v[1].x = xp + 2.0 / GRID_SIZE;
                        v[1].y = yp;
                        v[2].x = xp;
                        v[2].y = yp + 2.0 / GRID_SIZE;
                        v[3].x = xp + 2.0 / GRID_SIZE;
                        v[3].y = yp + 2.0 / GRID_SIZE;

                        for (i = 0; i < 4; i++) {
                                v[i].z = 0.5f;
                                v[i].r = 0;
                                v[i].g = 0;
                                v[i].b = 255;
                        }

                        v += 4;
                }
        }

        ind = (uint16_t *) v;

        for (i = 0; i < N_GRID_SQUARES; i++) {
                *(ind++) = i * 4;
                *(ind++) = i * 4 + 1;
                *(ind++) = i * 4 + 2;
                *(ind++) = i * 4 + 2;
                *(ind++) = i * 4 + 1;
                *(ind++) = i * 4 + 3;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3,
                              GL_FLOAT, GL_FALSE,
                              sizeof (struct vertex),
                              (void *) offsetof(struct vertex, x));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3,
                              GL_UNSIGNED_BYTE, GL_TRUE,
                              sizeof (struct vertex),
                              (void *) offsetof(struct vertex, r));

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid->buffer);

        glBindVertexArray(0);
}

static void
destroy_grid(struct grid *grid)
{
        glDeleteVertexArrays(1, &grid->array);
        glDeleteBuffers(1, &grid->buffer);
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniform2f(glthing->offset_location, 0.0f, 0.0f);
        glBindVertexArray(glthing->grid.array);
        glDrawRangeElements(GL_TRIANGLES,
                            0, N_GRID_SQUARES * 4 - 1,
                            N_GRID_SQUARES * 6,
                            GL_UNSIGNED_SHORT,
                            (void *) (N_GRID_SQUARES * 4 *
                                      sizeof (struct vertex)));

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

        glthing.offset_location =
                glGetUniformLocation(glthing.program, "offset");

        create_grid(&glthing.grid);

        glUseProgram(glthing.program);

        if (!main_loop(&glthing))
                ret = EXIT_FAILURE;

        destroy_grid(&glthing.grid);

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
