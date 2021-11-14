#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string.h>
#include <iostream>
#include <sstream>

class Logger
{
private:
	typedef std::ostream &(*ManipFn)(std::ostream &);
	typedef std::ios_base &(*FlagsFn)(std::ios_base &);
	std::stringstream LogStream;
	enum StreamDir
	{
		console,
		file,
		socket
	};
	StreamDir streamDir = console;
	std::string fname;
	int SocketFd;
	std::string GetTime();

public:
	enum LogLevel
	{
		INFO,
		WARN,
		ERROR
	};
	LogLevel BaseLogLevel = INFO;
	LogLevel SingleLogLevel;
	Logger(/* args */);
	void flush();
	void SetLogLevel(LogLevel);
	std::string GetSingleLogLevel();
	template <class T>
	Logger &operator<<(const T &output)
	{
		LogStream << output;
		return *this;
	}
	Logger &operator<<(ManipFn manip) /// endl, flush, setw, setfill, etc.
	{
		manip(LogStream);

		if (manip == static_cast<ManipFn>(std::flush) || manip == static_cast<ManipFn>(std::endl))
			this->flush();

		return *this;
	}
	Logger &operator<<(FlagsFn manip) /// setiosflags, resetiosflags
	{
		manip(LogStream);
		return *this;
	}
	Logger &operator()(LogLevel e)
	{
		this->BaseLogLevel = e;
		return *this;
	}
};

std::string Logger::GetTime()
{
	time_t rawtime;
	struct tm *info;
	char buffer[32];

	time(&rawtime);

	info = localtime(&rawtime);

	strftime(buffer, 32, "[%Y-%m-%d %H:%M:%S] ", info);
	return std::string(buffer);
}
std::string Logger::GetSingleLogLevel()
{
	if (SingleLogLevel == INFO)
	{
		return std::string("\033[34m[I]\033[0m");
	}
	if (SingleLogLevel == WARN)
	{
		return std::string("\033[43m[W]\033[0m");
	}
	if (SingleLogLevel == ERROR)
	{
		return std::string("\033[41m[E]\033[0m");
	}
	return std::string("Logger error");
}

void Logger::flush()
{
	//Send LogStream.str() to console, file, socket, or whatever you like here.
	if (streamDir == console)
	{
		std::cout << GetSingleLogLevel() << GetTime() << LogStream.str();
	}

	//Reset LogLevel
	if (SingleLogLevel != BaseLogLevel)
		SingleLogLevel = BaseLogLevel;

	LogStream.str(std::string());
	LogStream.clear();
}

Logger::Logger() : SingleLogLevel(INFO) {}
#endif //LOGGER_HPP