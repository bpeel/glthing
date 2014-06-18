#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <epoxy/gl.h>
#include <stdio.h>

#include "gol.h"
#include "shader-data.h"

#define GOL_N_TEXTURES 8

struct gol_texture {
        GLuint tex;
        GLuint fbo;
};

struct gol {
        struct gol_texture textures[GOL_N_TEXTURES];
        int width, height;
        GLuint update_program;
        GLuint paint_program;
        int current_texture;

        GLuint fullscreen_triangle_buffer;
        GLuint fullscreen_triangle_array;
};

static void
make_fullscreen_triangle(GLuint *buffer,
                         GLuint *array)
{
        static const GLfloat vertices[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
        };

        glGenBuffers(1, buffer);
        glBindBuffer(GL_ARRAY_BUFFER, *buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof vertices, vertices,
                     GL_STATIC_DRAW);

        glGenVertexArrays(1, array);
        glBindVertexArray(*array);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, /* index */
                              2, /* size */
                              GL_FLOAT,
                              GL_FALSE, /* normalized */
                              sizeof (GLfloat) * 2,
                              NULL);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
}

struct gol *
gol_new(int width, int height)
{
        struct gol *gol;
        uint8_t *tex_data;
        GLint uniform;
        int i;

        gol = malloc(sizeof *gol);

        gol->current_texture = 0;

        gol->update_program =
                shader_data_load_program(GL_VERTEX_SHADER,
                                         "gol-update-vertex.glsl",
                                         GL_FRAGMENT_SHADER,
                                         "gol-update-fragment.glsl",
                                         GL_NONE);
        if (gol->update_program == 0)
                goto error;

        glUseProgram(gol->update_program);
        uniform = glGetUniformLocation(gol->update_program, "tex");
        glUniform1i(uniform, 0);
        glUseProgram(0);

        gol->paint_program =
                shader_data_load_program(GL_VERTEX_SHADER,
                                         "gol-update-vertex.glsl",
                                         GL_FRAGMENT_SHADER,
                                         "gol-paint-fragment.glsl",
                                         GL_NONE);
        if (gol->paint_program == 0)
                goto error_update_program;

        glUseProgram(gol->paint_program);
        uniform = glGetUniformLocation(gol->paint_program, "tex");
        glUniform1i(uniform, 0);
        glUseProgram(0);

        tex_data = malloc(width * height);
        memset(tex_data, 0, width * height);

        /* Hack in a glider */
        tex_data[1] = 255;
        tex_data[2 + width] = 255;
        memset(tex_data + width * 2, 255, 3);

        gol->width = width;
        gol->height = height;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (i = 0; i < GOL_N_TEXTURES; i++) {
                glGenTextures(1, &gol->textures[i].tex);
                glBindTexture(GL_TEXTURE_2D, gol->textures[i].tex);
                glTexImage2D(GL_TEXTURE_2D,
                             0, /* level */
                             GL_R8,
                             width, height,
                             0, /* border */
                             GL_RED,
                             GL_UNSIGNED_BYTE,
                             tex_data);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);

                glGenFramebuffers(1, &gol->textures[i].fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, gol->textures[i].fbo);
                glFramebufferTexture(GL_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     gol->textures[i].tex,
                                     0 /* level */);

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
                    GL_FRAMEBUFFER_COMPLETE) {
                        fprintf(stderr, "framebuffer is not complete\n");
                        goto error_textures;
                }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        free(tex_data);

        make_fullscreen_triangle(&gol->fullscreen_triangle_buffer,
                                 &gol->fullscreen_triangle_array);

        return gol;

error_textures:
        free(tex_data);

        while (--i >= 0) {
                glDeleteTextures(1, &gol->textures[i].tex);
                glDeleteFramebuffers(1, &gol->textures[i].fbo);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteProgram(gol->paint_program);

error_update_program:
        glDeleteProgram(gol->update_program);

error:
        free(gol);

        return NULL;
}

void
gol_free(struct gol *gol)
{
        int i;

        for (i = 0; i < GOL_N_TEXTURES; i++) {
                glDeleteTextures(1, &gol->textures[i].tex);
                glDeleteFramebuffers(1, &gol->textures[i].fbo);
        }

        glDeleteProgram(gol->paint_program);
        glDeleteProgram(gol->update_program);

        free(gol);
}

void
gol_update(struct gol *gol)
{
        int next_texture = (gol->current_texture + 1) % GOL_N_TEXTURES;

        glViewport(0.0f, 0.0f, gol->width, gol->height);

        glUseProgram(gol->update_program);
        glBindTexture(GL_TEXTURE_2D, gol->textures[gol->current_texture].tex);
        glBindFramebuffer(GL_FRAMEBUFFER, gol->textures[next_texture].fbo);
        glBindVertexArray(gol->fullscreen_triangle_array);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);

        gol->current_texture = next_texture;
}

void
gol_paint(struct gol *gol)
{
        glUseProgram(gol->paint_program);
        glBindTexture(GL_TEXTURE_2D, gol->textures[gol->current_texture].tex);
        glBindVertexArray(gol->fullscreen_triangle_array);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
}
