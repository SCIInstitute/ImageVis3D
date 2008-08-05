#include "ConsoleOut.h"
#include "Basics/Console.h"


#ifdef WIN32
	#include <windows.h>
#endif

void ConsoleOut::printf(const char* format, ...)
{
	// output string
	char buff[16384];
   
	// arguments
	va_list args;
	va_start(args, format);

	// build string
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif

	Console::printf("%s\n",buff);
}
