#pragma once


namespace GLSL
{
const char *base_vss = "#version 410 core\n"
    "layout (location = 0) in dvec3 pos;\n"
    "uniform int w;\n"
    "uniform int h;\n"
    "uniform dvec3 vec0;\n"
    "uniform dvec3 vec1;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4((pos.x * vec0.x + pos.y * vec0.y + vec0.z) / w - 1.0,"
            "1.0 - (pos.x * vec1.x + pos.y * vec1.y + vec1.z) / h, pos.z, 1.0);\n"
    "}\0";

const char *base_fss = "#version 410 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "void main()\n"
    "{\n"
    "   FragColor = color;\n"
    "}\0";

};