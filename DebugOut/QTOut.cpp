#include "QTOut.h"
#include <stdarg.h>
#include <stdio.h>


#ifdef WIN32
	#include <windows.h>
#endif

QTOut::QTOut(QListWidget *listWidget) :
	m_listWidget(listWidget)
{

}

QTOut::~QTOut() {

}


void QTOut::printf(const char* format, ...)
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

	m_listWidget->addItem ( buff );
}
