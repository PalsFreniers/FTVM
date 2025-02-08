#include <Logger.hpp>
#include <colors.hpp>
#include <cstdarg>
#include <iostream>
#include <stdexcept>
#include <bitset>

const char *getLogStr(LogLevel l) {
        switch(l) {
                case DEBUG: return BLU "[DEBUG]" CLR;
                case INFO: return GRN "[INFO]" CLR;
                case WARNING: return YLW "[WARNING]" CLR;
                case ERROR: return RED "[ERROR]" CLR;
                case FATAL: return RGB(127, 0, 0) "[FATAL]" CLR;
                default: return "[LOG]";
        }
}

Logger::Logger() : _name("") {}

Logger::Logger(const std::string &name) : _name(name) {}

Logger::Logger(const Logger &src) : _name(src._name) {}

Logger &Logger::operator=(const Logger &rhs) {
	this->_name = rhs._name;
	return *this;
}

Logger::~Logger() {}

void logFormat(const std::string str, size_t &i, std::va_list lst) {
        i++;
        switch(str[i]) {
                case '/': std::cerr << '/'; break;
                case 's': std::cerr << va_arg(lst, const char *); break;
                case 'c': std::cerr << (char)va_arg(lst, int); break;
                case 'd': std::cerr << va_arg(lst, int); break;
                case 'l': std::cerr << va_arg(lst, long); break;
                case 'x': std::cerr << "0x" << std::hex << va_arg(lst, int) << std::dec; break;
                case 'o': std::cerr << "0" << std::oct << va_arg(lst, int) << std::dec; break;
                case 'b': std::cerr << "0b" << std::bitset<sizeof(int) * 8>(va_arg(lst, int)) << std::dec; break;
                default: std::cerr << "/" << str[i]; break;
        }
}

void Logger::log(LogLevel lvl, const std::string fmt, ...) {
#ifdef RELEASE
        if(lvl == LogLevel::DEBUG) return;
#endif // RELEASE
        std::va_list lst;
        std::cerr << getLogStr(lvl) << (_name.empty() ? "" : " ") << _name << " => ";
        va_start(lst, fmt);
        for(size_t i = 0; i < fmt.length(); i++) {
                if(fmt[i] == '/' && i != fmt.length() - 1) logFormat(fmt, i, lst);
                else std::cerr << fmt[i];
        }
        va_end(lst);
        std::cerr << std::endl;
        if(lvl == LogLevel::FATAL) throw std::logic_error(RGB(127, 0, 0) "!![PANIC]!! fatal logger has been reached\n");
}
