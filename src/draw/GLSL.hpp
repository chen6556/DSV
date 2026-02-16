#pragma once


namespace GLSL
{
const char *const base_vss = "#version 450 core\n"
                             "layout (location = 0) in dvec3 pos;\n"
                             "uniform vec2 window;\n"
                             "uniform dmat3 ctm;\n"
                             "void main()\n"
                             "{\n"
                             "   const dvec3 result = ctm * dvec3(pos.x, pos.y, 1.0);\n"
                             "   gl_Position = vec4(result.x / window.x - 1.0, 1.0 - result.y / window.y, pos.z, 1.0);\n"
                             "}\0";

const char *const base_fss = "#version 450 core\n"
                             "out vec4 FragColor;\n"
                             "uniform vec4 color;\n"
                             "void main()\n"
                             "{\n"
                             "   FragColor = color;\n"
                             "}\0";

}; // namespace GLSL