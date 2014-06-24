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

enum blit_mode {
        BLIT_MODE_BLIT,
        BLIT_MODE_FIXED_FUNCTION_TRIANGLES,
        BLIT_MODE_GLSL_TRIANGLES
};

static const char *
blit_mode_names[] = {
        "blit",
        "ff",
        "glsl"
};

#define DST_WIDTH 256

#define ARRAY_SIZE(x) (sizeof (x) / sizeof ((x)[0]))

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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
blit_fixed_function_triangles(GLuint src_tex, GLuint src_fbo,
                              GLuint dst_tex, GLuint dst_fbo)
{
        glViewport(0, 0, DST_WIDTH, 1);

        glBindTexture(GL_TEXTURE_2D, src_tex);
        glEnable(GL_TEXTURE_2D);

        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(0, 0);
        glVertex2i(-1, -1);
        glTexCoord2i(1, 0);
        glVertex2i(1, -1);
        glTexCoord2i(0, 1);
        glVertex2i(-1, 1);
        glTexCoord2i(1, 1);
        glVertex2i(1, 1);
        glEnd();

        glDisable(GL_TEXTURE_2D);
}

static GLuint
create_shader(GLenum type, const char *source)
{
        GLuint shader;
        GLint length, compile_status;
        GLsizei actual_length;
        GLchar *info_log;

        length = strlen(source);

        shader = glCreateShader(type);
        glShaderSource(shader, 1, (const GLchar **) &source, &length);

        glCompileShader(shader);

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        if (length > 0) {
                info_log = malloc(length);
                glGetShaderInfoLog(shader, length, &actual_length, info_log);
                if (*info_log) {
                        fprintf(stderr,
                                "Info log:\n%s\n",
                                info_log);
                }
                free(info_log);
        }

        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

        if (!compile_status) {
                fprintf(stderr, "compilation failed\n");
                glDeleteShader(shader);
                return 0;
        }

        return shader;
}

static GLuint
create_program(GLenum shader_type,
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
                shader = create_shader(shader_type,
                                       va_arg(ap, const char *));
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

static void
blit_glsl_triangles(GLuint src_tex, GLuint src_fbo,
                    GLuint dst_tex, GLuint dst_fbo)
{
        GLuint uniform;
        GLuint attrib;
        GLuint prog;

        static const char vertex_shader[] =
                "varying vec2 coord;\n"
                "attribute vec2 pos;\n"
                "\n"
                "void\n"
                "main()\n"
                "{\n"
                "        coord = pos;\n"
                "        gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);\n"
                "}\n";
        static const char fragment_shader[] =
                "varying vec2 coord;\n"
                "uniform sampler2D tex;\n"
                "\n"
                "void\n"
                "main()\n"
                "{\n"
                "        gl_FragColor = texture2D(tex, coord);\n"
                "}\n";

        static const GLfloat verts[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f,
        };

        prog = create_program(GL_VERTEX_SHADER, vertex_shader,
                              GL_FRAGMENT_SHADER, fragment_shader,
                              GL_NONE);

        if (prog == 0)
                return;

        glBindTexture(GL_TEXTURE_2D, src_tex);

        glUseProgram(prog);

        uniform = glGetUniformLocation(prog, "tex");
        glUniform1i(uniform, 0);

        attrib = glGetAttribLocation(prog, "pos");

        glEnableVertexAttribArray(attrib);
        glVertexAttribPointer(attrib,
                              2, /* size */
                              GL_FLOAT,
                              GL_FALSE, /* normalized */
                              2 * sizeof (float),
                              verts);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(attrib);

        glUseProgram(0);

        glDeleteProgram(prog);
}

static void
blit_blit(GLuint src_tex, GLuint src_fbo,
          GLuint dst_tex, GLuint dst_fbo)
{
        glBlitFramebuffer(0, 0, /* src x0 y0 */
                          2, 1, /* src x1 y1 */
                          0, 0, /* dst x0 y0 */
                          DST_WIDTH, 1, /* dst x0 y0 */
                          GL_COLOR_BUFFER_BIT,
                          GL_LINEAR);

}

static void
do_blit(enum blit_mode mode, GLubyte *dst_data)
{
        GLuint src_tex, dst_tex;
        GLuint src_fbo, dst_fbo;
        static const GLubyte src_data[2 * 4] = {
                0xff, 0x00, 0x00, 0xff,
                0x00, 0x00, 0xff, 0xff,
        };

        create_texture(&src_tex, &src_fbo, 2, 1, src_data);
        create_texture(&dst_tex, &dst_fbo, DST_WIDTH, 1, NULL);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);

        switch (mode) {
        case BLIT_MODE_BLIT:
                blit_blit(src_tex, src_fbo,
                          dst_tex, dst_fbo);
                break;
        case BLIT_MODE_FIXED_FUNCTION_TRIANGLES:
                blit_fixed_function_triangles(src_tex, src_fbo,
                                              dst_tex, dst_fbo);
                break;
        case BLIT_MODE_GLSL_TRIANGLES:
                blit_glsl_triangles(src_tex, src_fbo,
                                    dst_tex, dst_fbo);
                break;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &src_fbo);
        glDeleteFramebuffers(1, &dst_fbo);

        glBindTexture(GL_TEXTURE_2D, dst_tex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst_data);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteTextures(1, &src_tex);
        glDeleteTextures(1, &dst_tex);
}

static void
do_all_blits(void)
{
        GLubyte dst_data[ARRAY_SIZE(blit_mode_names)][DST_WIDTH * 4];
        int mode;
        int x, c;

        fputs("  ,  ", stdout);
        for (mode = 0; mode < ARRAY_SIZE(blit_mode_names); mode++) {
                do_blit(mode, dst_data[mode]);
                printf("%-8s,", blit_mode_names[mode]);
        }
        fputc('\n', stdout);

        for (x = 0; x < DST_WIDTH; x++) {
                printf("%02x,  ", x);
                for (mode = 0; mode < ARRAY_SIZE(blit_mode_names); mode++) {
                        for (c = 0; c < 4; c++)
                                printf("%02x", dst_data[mode][x * 4 + c]);
                        fputc(',', stdout);
                }
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

        do_all_blits();

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(glthing.gl_context);
out_window:
        SDL_DestroyWindow(glthing.window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
