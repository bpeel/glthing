#version 330

layout(location = 0) in vec2 coord;

out vec2 tex_coord;

void
main()
{
        gl_Position = vec4(coord * 2.0 - 1.0, 0.0, 1.0);
        tex_coord = coord;
}
