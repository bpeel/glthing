#include "config.h"

#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;
        struct triangle *triangle;
        GLuint program;

        bool quit;
};

struct vertex {
        float x, y;
};

struct triangle {
        GLuint vertex_array;
        GLuint buffer;
};

#define ATTRIB_POS 0

static char *
load_shader_source(const char *filename)
{
        FILE *file;
        char *source;
        long int size;
        int res;

        file = fopen(filename, "r");
        if (file == NULL) {
                fprintf(stderr, "%s: %s\n", filename, strerror(errno));
                return NULL;
        }

        res = fseek(file, 0L, SEEK_END);
        if (res) {
                fprintf(stderr, "%s: %s\n", filename, strerror(errno));
                fclose(file);
                return NULL;
        }

        size = ftell(file);
        if (size == -1) {
                fprintf(stderr, "%s: %s\n", filename, strerror(errno));
                fclose(file);
                return NULL;
        }

        res = fseek(file, 0L, SEEK_SET);
        if (res) {
                fprintf(stderr, "%s: %s\n", filename, strerror(errno));
                fclose(file);
                return NULL;
        }

        source = malloc(size + 1);
        source[size] = '\0';

        res = fread(source, 1, size, file);
        if (res != size) {
                if (ferror(file))
                        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
                else
                        fprintf(stderr, "%s: unexpected EOF\n", filename);
                fclose(file);
                free(source);
                return NULL;
        }

        fclose(file);

        return source;
}

static GLuint
load_shader(GLenum type,
            const char *filename)
{
        GLuint shader;
        GLint length, compile_status;
        GLsizei actual_length;
        char *source;
        GLchar *info_log;

        source = load_shader_source(filename);
        if (source == NULL)
                return 0;

        length = strlen(source);

        shader = glCreateShader(type);
        glShaderSource(shader, 1, (const GLchar **) &source, &length);

        free(source);

        glCompileShader(shader);

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        if (length > 0) {
                info_log = malloc(length);
                glGetShaderInfoLog(shader, length, &actual_length, info_log);
                if (*info_log) {
                        fprintf(stderr,
                                "Info log for %s:\n%s\n",
                                filename,
                                info_log);
                }
                free(info_log);
        }

        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

        if (!compile_status) {
                fprintf(stderr, "%s: compilation failed\n", filename);
                glDeleteShader(shader);
                return 0;
        }

        return shader;
}

static GLuint
create_program_with_shaders(GLenum shader_type,
                            ...)
{
        GLint length, link_status;
        GLsizei actual_length;
        GLchar *info_log;
        GLuint program;
        GLuint shader;
        va_list ap;

        va_start(ap, shader_type);

        program = glCreateProgram();

        while (shader_type != GL_NONE) {
                shader = load_shader(shader_type, va_arg(ap, const char *));
                if (shader == 0) {
                        glDeleteProgram(program);
                        va_end(ap);
                        return 0;
                }

                glAttachShader(program, shader);
                glDeleteShader(shader);

                shader_type = va_arg(ap, GLenum);
        }

        va_end(ap);

        glBindAttribLocation(program, ATTRIB_POS, "pos");

        glLinkProgram(program);

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        if (length > 0) {
                info_log = malloc(length);
                glGetProgramInfoLog(program, length, &actual_length, info_log);
                if (*info_log) {
                        fprintf(stderr,
                                "Link info log:\n%s\n",
                                info_log);
                }
                free(info_log);
        }

        glGetProgramiv(program, GL_LINK_STATUS, &link_status);

        if (!link_status) {
                fprintf(stderr, "program link failed\n");
                glDeleteProgram(program);
                return 0;
        }

        return program;
}

static GLuint
create_program(void)
{
        return create_program_with_shaders(GL_VERTEX_SHADER,
                                           "vertex-shader.glsl",
                                           GL_FRAGMENT_SHADER,
                                           "fragment-shader.glsl",
                                           GL_NONE);
}

static struct triangle *
create_triangle(void)
{
        static const struct vertex triangle_data[3] = {
                { -0.5f, -0.5f },
                { 0.5f, -0.5f },
                { 0.0f, 0.5f }
        };
        struct triangle *triangle;

        triangle = malloc(sizeof *triangle);

        glGenBuffers(1, &triangle->buffer);
        glBindBuffer(GL_ARRAY_BUFFER, triangle->buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof triangle_data,
                     triangle_data,
                     GL_STATIC_DRAW);

        glGenVertexArrays(1, &triangle->vertex_array);
        glBindVertexArray(triangle->vertex_array);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(ATTRIB_POS,
                              2, /* size */
                              GL_FLOAT,
                              GL_FALSE, /* normalized */
                              sizeof (struct vertex),
                              (void *) offsetof(struct vertex, x));

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return triangle;
}

static void
free_triangle(struct triangle *triangle)
{
        glDeleteVertexArrays(1, &triangle->vertex_array);
        glDeleteBuffers(1, &triangle->buffer);
        free(triangle);
}

static void
redraw(struct glthing *glthing)
{
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(glthing->program);

        glBindVertexArray(glthing->triangle->vertex_array);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

        glBindVertexArray(0);

        glUseProgram(0);

        SDL_GL_SwapWindow(glthing->window);
}

static void
process_window_event(struct glthing *glthing,
                     const SDL_WindowEvent *event)
{
        switch (event->event) {
        case SDL_WINDOWEVENT_EXPOSED:
                redraw(glthing);
                break;

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
        int res;

        while (!glthing->quit) {
                res = SDL_WaitEvent(&event);
                if (res < 0) {
                        fprintf(stderr, "SDL_WaitEvent: %s\n", SDL_GetError());
                        return false;
                }

                process_event(glthing, &event);
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

        glthing.program = create_program();
        if (glthing.program == 0) {
                ret = EXIT_FAILURE;
                goto out_context;
        }

        glthing.triangle = create_triangle();

        if (!main_loop(&glthing))
                ret = EXIT_FAILURE;

        free_triangle(glthing.triangle);

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
