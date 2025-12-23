#if defined(_MSC_VER) || defined(WIN64)
#include <icu.h>
#pragma comment(lib, "icu.lib")
#elif defined(__linux__) || defined(__GNUC__)
#include <iconv.h>
#endif
#include "io/TextEncoding.hpp"


bool TextEncoding::is_utf8(const std::string &str)
{
    // UFT8可用1-6个字节编码，ASCII用一个字节
    unsigned int bytes_count = 0;
    unsigned char uc;
    bool all_ascii = true;
    for (char c : str)
    {
        uc = static_cast<unsigned char>(c);
        // 判断是否ASCII编码，如果不是，说明有可能是UTF8，ASCII用7位编码，最高位标记为0，0xxxxxxx
        if (bytes_count == 0 && (uc & 0x80) != 0)
        {
            all_ascii = false;
        }
        if (bytes_count == 0)
        {
            //如果不是ASCII码，应该是多字节符，计算字节数
            if (uc >= 0x80)
            {
                if (uc >= 0xFC && uc <= 0xFD)
                {
                    bytes_count = 6;
                }
                else if (uc >= 0xF8)
                {
                    bytes_count = 5;
                }
                else if (uc >= 0xF0)
                {
                    bytes_count = 4;
                }
                else if (uc >= 0xE0)
                {
                    bytes_count = 3;
                }
                else if (uc >= 0xC0)
                {
                    bytes_count = 2;
                }
                else
                {
                    return false;
                }
                --bytes_count;
            }
        }
        else
        {
            // 多字节符的非首字节，应为 10xxxxxx
            if ((uc & 0xC0) != 0x80)
            {
                return false;
            }
            // 减到为零为止
            --bytes_count;
        }
    }

    // 违返UTF8编码规则
    if (bytes_count != 0)
    {
        return false;
    }
    if (all_ascii)
    {
        // 如果全部都是ASCII，也是UTF8
        return true;
    }

    return true;
}

bool TextEncoding::is_gbk(const std::string &str)
{
    // GBK可用1 - 2个字节编码，中文两个，英文一个
		unsigned int bytes_count = 0;
		unsigned char uc;
		// 如果全部都是ASCII
		bool all_ascii = true;
		for (char c : str)
		{
			uc = static_cast<unsigned char>(c);
			if ((uc & 0x80) != 0 && bytes_count == 0)
			{
				// 判断是否ASCII编码，如果不是，说明有可能是GBK
				all_ascii = false;
			}
			if (bytes_count == 0)
			{
				if (uc >= 0x80)
				{
					if (uc >= 0x81 && uc <= 0xFE)
					{
						bytes_count = 2;
					}
					else
					{
						return false;
					}
					--bytes_count;
				}
			}
			else
			{
				if (uc < 0x40 || uc > 0xFE)
				{
					return false;
				}
				--bytes_count;
			}
		}

		if (bytes_count != 0)
		{
			// 违返规则
			return false;
		}
		if (all_ascii)
		{
			// 如果全部都是ASCII，也是GBK
			return true;
		}

		return true;
}

#if defined(__linux__) || defined(__GNUC__)
bool TextEncoding::encoding_convert(const char *charset_src, const char *charset_dest,
    const char *inbuf, size_t insize, char *outbuf, size_t outsize)
{
    iconv_t cd;
    char** pin = &inbuf;
    char** pout = &outbuf;
    cd = iconv_open(charset_dest, charset_src);
    if (0 == cd)
    {
        std::cerr << charset_src << " to " << charset_dest
            << " conversion not available" << std::endl; 
        return false;
    }

    if (-1 == static_cast<int>(iconv(cd, pin, &insize, pout, &outsize)))
    {
   
        std::cerr << "conversion failure" << std::endl;
        return false;
    }

    iconv_close(cd);
    **pout = '\0';
    return true;
}
#endif

std::string TextEncoding::gbk_to_utf8(const std::string &str)
{
#if defined(_MSC_VER) || defined(WIN64)
    UErrorCode err;
    UConverter *gbkconv = ucnv_open("GBK", &err);
    UChar *unicode = new UChar[str.length() * 2 + 1];
    const int32_t unilen = ucnv_toUChars(gbkconv, unicode, str.length() * 2, str.data(), str.length(), &err);
    ucnv_close(gbkconv);
    UConverter *u8conv = ucnv_open("UTF8", &err);
    char *data = new char[str.length() * 2 + 1];
    const int32_t len = ucnv_fromUChars(u8conv, data, str.size() * 2, unicode, unilen, &err);
    ucnv_close(u8conv);
    std::string res(data, len);
    delete[] unicode;
    delete[] data;
    return res;
#elif defined(__linux__) || defined(__GNUC__)
    size_t len = str.size() * 2 + 1;
    char* temp = new char[len];
    if (encoding_convert("gbk", "utf-8", const_cast<char*>(str.c_str()), str.size(), temp, len))
    {
        std::string res;
        res.append(temp);
        delete[] temp;
        return res;
    }
    else
    {
        delete[] temp;
        return str;
    }
#else
    std::cerr << "Unhandled operating system." << std::endl;
    return str;
#endif
}

std::string TextEncoding::uft8_to_gbk(const std::string &str)
{
#if defined(_MSC_VER) || defined(WIN64)
    UErrorCode err;
    UConverter *u8conv = ucnv_open("UTF8", &err);
    UChar *unicode = new UChar[str.length() * 2 + 1];
    const int32_t unilen = ucnv_toUChars(u8conv, unicode, str.length() * 2, str.data(), str.length(), &err);
    ucnv_close(u8conv);
    UConverter *gbkconv = ucnv_open("GBK", &err);
    char *data = new char[unilen * 2 + 1];
    const int32_t len = ucnv_fromUChars(gbkconv, data, str.size() * 2, unicode, unilen, &err);
    ucnv_close(gbkconv);
    std::string res(data, len);
    delete[] unicode;
    delete[] data;
    return res;
#elif defined(__linux__) || defined(__GNUC__)
    size_t len = str.size() * 2 + 1;
    char* temp = new char[len];
    if (encoding_convert("utf-8", "gbk", const_cast<char*>(str.c_str()), str.size(), temp, len))
    {
        std::string res;
        res.append(temp);
        delete[] temp;
        return res;
    }
    else
    {
        delete[] temp;
        return str;
    }
#else
    std::cerr << "Unhandled operating system." << std::endl;
    return str;
#endif
}