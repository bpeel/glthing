#ifndef SHADER_DATA_H
#define SHADER_DATA_H

#include <epoxy/gl.h>

char *
shader_data_load_shader_source(const char *filename);

GLuint
shader_data_load_shader(GLenum type,
                        const char *filename);

GLuint
shader_data_load_program(GLenum shader_type,
                         ...);

#endif /* SHADER_DATA_H */
