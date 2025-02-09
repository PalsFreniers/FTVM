#pragma once

# include <string>

enum LogLevel {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
        LOG_FATAL,
};

const char *getLogStr(LogLevel l);

class Logger
{
public:
	Logger();
	Logger(const std::string &name);
	Logger(const Logger &src);
	Logger&operator=(const Logger &rhs);
	~Logger();

        void log(LogLevel lvl, const std::string fmt, ...);
private:
	std::string _name;
};

