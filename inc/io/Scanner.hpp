#pragma once
#include <string>
#include <list>
#include <fstream>
#include <sstream>


class Scanner
{
public:
    static Scanner repeat(const Scanner &scanner, const size_t times = 0);
    static Scanner repeat(const std::string &scanner, const size_t times = 0);
    static Scanner repeat(const char scanner, const size_t times = 0);

    static Scanner optional(const Scanner &scanner);
    static Scanner optional(const std::string &scanner);
    static Scanner optional(const char scanner);

    static Scanner range(const int a, const int b);
    static Scanner range(const char a, const char b);

private:
    enum Type {NONE, OR, REPEAT, NOT, AND, LIST};
    std::list<Scanner> _scanners;
    size_t _repeat_tiems = 0;
    std::string _txt;
    Type _type = Type::NONE;
    bool _optional = false;

public:
    Scanner();
    Scanner(const char text);
    Scanner(const std::string &text);
    Scanner(const Scanner &scanner);
    ~Scanner();

    const bool &optional() const;

    const Scanner &operator=(const Scanner &scanner);
    const Scanner &operator=(const std::string &scanner);

    Scanner operator|(const Scanner &scanner) const;
    Scanner operator&(const Scanner &scanner) const;
    Scanner operator<<(const Scanner &scanner) const;
    friend Scanner operator!(const Scanner &scanner);

    Scanner operator|(const std::string &scanner) const;
    Scanner operator&(const std::string &scanner) const;
    Scanner operator<<(const std::string &scanner) const;
    friend Scanner operator!(const std::string &scanner);

    std::string operator()(std::fstream &file) const;
    std::string operator()(std::stringstream &str) const;
};


namespace Scanners
{
    const static Scanner digit = Scanner('1') | Scanner('2') | Scanner('3') | Scanner('4') | Scanner('5') |
        Scanner('6') | Scanner('7') | Scanner('8') | Scanner('9') | Scanner('0');
    const static Scanner digits = Scanner::repeat(digit);

    const static Scanner alpha_a = Scanner('a') | Scanner('b') | Scanner('c') | Scanner('d') | Scanner('e') |
        Scanner('f') | Scanner('g') | Scanner('h') | Scanner('i') | Scanner('j') | Scanner('k') | Scanner('l') |
        Scanner('m') | Scanner('n') | Scanner('o') | Scanner('p') | Scanner('q') | Scanner('r') | Scanner('s') |
        Scanner('t') | Scanner('u') | Scanner('v') | Scanner('w') | Scanner('x') | Scanner('y') | Scanner('z');
    const static Scanner alpha_b = Scanner('A') | Scanner('B') | Scanner('C') | Scanner('D') | Scanner('E') |
        Scanner('F') | Scanner('G') | Scanner('H') | Scanner('I') | Scanner('J') | Scanner('K') | Scanner('L') |
        Scanner('M') | Scanner('N') | Scanner('O') | Scanner('P') | Scanner('Q') | Scanner('R') | Scanner('S') |
        Scanner('T') | Scanner('U') | Scanner('V') | Scanner('W') | Scanner('X') | Scanner('Y') | Scanner('Z');
    const static Scanner alpha = alpha_a | alpha_b;
    const static Scanner alpha_as = Scanner::repeat(alpha_a);
    const static Scanner alpha_bs = Scanner::repeat(alpha_b);
    const static Scanner alphas = Scanner::repeat(alpha);

    const static Scanner digitalpha_a = digit | alpha_a;
    const static Scanner digitalpha_b = digit | alpha_b;
    const static Scanner digitalpha = digit | alpha;
    const static Scanner digitalpha_as = Scanner::repeat(digitalpha_a);
    const static Scanner digitalpha_bs = Scanner::repeat(digitalpha_b);
    const static Scanner digitalphas = Scanner::repeat(digitalpha);

    const static Scanner num = Scanner::optional('-') << digits << Scanner::optional(Scanner('.') << digits);
    const static Scanner any = Scanner::range(static_cast<char>(0), static_cast<char>(127));

    const static Scanner space = Scanner(' ');
    const static Scanner enter = Scanner('\n');
    const static Scanner tab = Scanner('\t');
};

