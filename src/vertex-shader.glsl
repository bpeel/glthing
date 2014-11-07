#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

uniform vec2 offset;

out vec4 vcolor;

void
main()
{
        vcolor = color;
        gl_Position = vec4(pos.xy + offset, pos.z, 1.0);
}
