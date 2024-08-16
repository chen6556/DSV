#ifndef TEXTENCODING_H
#define TEXTENCODING_H

#include <string>

namespace TextEncoding
{

bool is_utf8(const std::string &str);

bool is_gbk(const std::string &str);

}

#endif // TEXTENCODING_H
