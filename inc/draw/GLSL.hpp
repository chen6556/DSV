#pragma once

namespace GLSL
{
const char *base_vss = "#version 450 core\n"
    "layout (location = 0) in dvec2 pos;\n"
    "uniform int w;\n"
    "uniform int h;\n"
    "uniform dvec3 vec0;\n"
    "uniform dvec3 vec1;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4((pos.x * vec0.x + pos.y * vec0.y + vec0.z) / w - 1.0,"
            "1.0 - (pos.x * vec1.x + pos.y * vec1.y + vec1.z) / h, 0.0, 1.0);\n"
    "}\0";

const char *shape_fss = "#version 450 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.9765f, 0.9765f, 0.9765f, 1.0f);\n"
    "}\0";

const char *path_normal_fss = "#version 450 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n"
    "}\0";

const char *path_selected_fss = "#version 450 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
    "}\0";

const char *point_fss = "#version 450 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);\n"
    "}\0";

};