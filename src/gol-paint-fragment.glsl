#version 330

uniform sampler2D tex;

in vec2 tex_coord;

layout(location = 0) out vec4 result;

void
main()
{
        result = texture(tex, tex_coord);
}
