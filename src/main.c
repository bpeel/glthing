#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "shader-data.h"

#define N_POINTS 100
#define N_COLORS 10

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;

        GLuint buffer;
        GLuint array[2];
        GLuint program;

        bool backwards;
        bool clear;

        bool quit;
};

struct color {
        float r, g, b, a;
};

struct vertex {
        float x, y;
        struct color colors[N_COLORS];
};

static void
gen_colors(struct color *colors,
           int n_colors)
{
        int i;

        for (i = 0; i < n_colors; i++) {
                colors->r = rand() / (float) RAND_MAX;
                colors->g = rand() / (float) RAND_MAX;
                colors->b = rand() / (float) RAND_MAX;
                colors++;
        }
}

static GLuint
create_points_buffer(void)
{
        struct vertex *v;
        GLuint buf;
        int i;

        glGenBuffers(1, &buf);
        glBindBuffer(GL_ARRAY_BUFFER, buf);
        glBufferData(GL_ARRAY_BUFFER,
                     N_POINTS * sizeof (struct vertex),
                     NULL, /* data */
                     GL_STATIC_DRAW);

        v = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        for (i = 0; i < N_POINTS; i++) {
                v->x = rand() / (float) RAND_MAX * 2.0f - 1.0f;
                v->y = rand() / (float) RAND_MAX * 2.0f - 1.0f;
                gen_colors(v->colors, N_COLORS);
                v++;
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return buf;
}

static GLuint
create_points_array(GLuint buffer,
                    bool backwards)
{
        GLuint ary;
        int attrib_num;
        int i;

        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glGenVertexArrays(1, &ary);
        glBindVertexArray(ary);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2,
                              GL_FLOAT, GL_FALSE,
                              sizeof (struct vertex),
                              (void *) offsetof(struct vertex, x));

        for (i = 0; i < N_COLORS; i++) {
                if (backwards)
                        attrib_num = N_COLORS - i;
                else
                        attrib_num = i + 1;

                glEnableVertexAttribArray(attrib_num);
                glVertexAttribPointer(attrib_num, 3,
                                      GL_UNSIGNED_BYTE, GL_TRUE,
                                      sizeof (struct vertex),
                                      (void *) offsetof(struct vertex,
                                                        colors[i].r));
        }

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return ary;
}

static void
redraw(struct glthing *glthing)
{
        int i;

        if (glthing->clear)
                glClear(GL_COLOR_BUFFER_BIT);

        /* Draw each point with an individual draw call and alternate
         * the vertex array objects each time so that the driver will
         * send the array state for every point
         */
        for (i = 0; i < N_POINTS; i++) {
                glBindVertexArray(glthing->array[i & 1]);
                glDrawArrays(GL_POINTS, i, 1);
        }


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
                while (SDL_PollEvent(&event))
                        process_event(glthing, &event);

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
        int i;

        memset(&glthing, 0, sizeof glthing);

        for (i = 1; i < argc; i++) {
                if (!strcmp(argv[i], "clear")) {
                        glthing.clear = true;
                } else if (!strcmp(argv[i], "backwards")) {
                        glthing.backwards = true;
                } else {
                        fprintf(stderr, "usage: glthing [clear] [backwards]\n");
                        exit(EXIT_FAILURE);
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

        glthing.buffer = create_points_buffer();
        for (i = 0; i < 2; i++)
                glthing.array[i] = create_points_array(glthing.buffer,
                                                       glthing.backwards);

        glUseProgram(glthing.program);

        if (!main_loop(&glthing))
                ret = EXIT_FAILURE;


        glDeleteBuffers(1, &glthing.buffer);
        glDeleteVertexArrays(2, glthing.array);
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
