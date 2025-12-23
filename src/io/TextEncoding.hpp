#pragma once

#include <string>


namespace TextEncoding
{

bool is_utf8(const std::string &str);

bool is_gbk(const std::string &str);

#if defined(__linux__) || defined(__GNUC__)
bool encoding_convert(const char* charset_src, const char* charset_dest,
    const char* inbuf, size_t insize, char* outbuf, size_t outsize);
#endif

std::string gbk_to_utf8(const std::string &str);

std::string uft8_to_gbk(const std::string &str);

}