#pragma once


namespace GLSL
{
const char *const base_vss = "#version 450 core\n"
                             "layout (location = 0) in dvec2 pos;\n"
                             "layout (location = 1) in dvec2 texCoord;\n"
                             "uniform vec2 window;\n"
                             "uniform dmat3 ctm;\n"
                             "uniform int enableTex;\n"
                             "out vec2 TexCoord;\n"
                             "void main()\n"
                             "{\n"
                             "   const dvec3 result = ctm * dvec3(pos.x, pos.y, 1.0);\n"
                             "   gl_Position = vec4(result.x / window.x - 1.0, 1.0 - result.y / window.y, 1.0, 1.0)"
                             " * step(enableTex, 0) + vec4(pos.x, pos.y, 1.0, 1.0) * step(1, enableTex);\n"
                             "   TexCoord = vec2(texCoord);\n"
                             "}\0";

const char *const base_fss = "#version 450 core\n"
                             "out vec4 FragColor;\n"
                             "uniform vec4 color;\n"
                             "uniform int enableTex;\n"
                             "uniform sampler2D textTexture;\n"
                             "in vec2 TexCoord;\n"
                             "void main()\n"
                             "{\n"
                             "   FragColor = color * step(enableTex, 0) + texture(textTexture, TexCoord) * step(1, enableTex);\n"
                             "}\0";

}; // namespace GLSL