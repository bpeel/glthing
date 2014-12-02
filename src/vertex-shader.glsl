#version 330 core

#define N_COLORS (gl_MaxVertexAttribs - 1)

layout(location = 0) in vec2 pos;

layout(location = 1) in vec4 color[N_COLORS];

out vec4 vcolor;

void
main()
{
        vec4 result = vec4(0);
        int i;

        for (i = 0; i < N_COLORS; i++)
                result += color[i];
        result /= float(N_COLORS);

        gl_Position = vec4(pos.xy, 0.0, 1.0);
        vcolor = result;
}
