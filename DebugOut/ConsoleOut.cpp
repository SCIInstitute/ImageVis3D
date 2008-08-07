#include "ConsoleOut.h"
#include "Basics/Console.h"

#ifdef WIN32
	#include <windows.h>
#endif

ConsoleOut::ConsoleOut() {
	Message("ConsoleOut::ConsoleOut:","Starting up ConsoleDebug out");
}

ConsoleOut::~ConsoleOut() {
	Message("ConsoleOut::~ConsoleOut:","Shutting down ConsoleDebug out");
}

void ConsoleOut::printf(const char* format, ...)
{
	if (!m_bShowOther) return;

	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	Console::printf("%s\n",buff);
}

void ConsoleOut::Message(const char* source, const char* format, ...) {
	if (!m_bShowMessages) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	Console::printf("MESSAGE (%s): %s\n",source, buff);
}

void ConsoleOut::Warning(const char* source, const char* format, ...) {
	if (!m_bShowWarnings) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	Console::printf("WARNING (%s): %s\n",source, buff);
}

void ConsoleOut::Error(const char* source, const char* format, ...) {
	if (!m_bShowErrors) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	Console::printf("ERROR (%s): %s\n",source, buff);
}
