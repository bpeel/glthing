#version 330 core

layout(location = 0) out vec4 color;

in vec4 vcolor;

void
main()
{
        color = vcolor;
}
