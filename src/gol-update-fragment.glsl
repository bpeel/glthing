#version 330

uniform sampler2D tex;

in vec2 tex_coord;

layout(location = 0) out float result;

void
main()
{
        float neighbours, self;
        vec2 step;

        step = 1.0 / textureSize(tex, 0);

        neighbours = texture(tex, tex_coord + vec2(-1.0, -1.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(0.0, -1.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(1.0, -1.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(-1.0, 0.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(1.0, 0.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(-1.0, 1.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(0.0, 1.0) * step).s;
        neighbours += texture(tex, tex_coord + vec2(1.0, 1.0) * step).s;

        self = texture(tex, tex_coord).s;

        if (self > 0.5) {
                if (neighbours < 1.5 || neighbours > 3.5)
                        result = 0.0;
                else
                        result = 1.0;
        } else {
                if (neighbours > 2.5 && neighbours < 3.5)
                        result = 1.0;
                else
                        result = 0.0;
        }
}
