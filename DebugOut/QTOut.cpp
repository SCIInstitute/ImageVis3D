#include "QTOut.h"
#include <stdarg.h>
#include <stdio.h>


#ifdef WIN32
	#include <windows.h>
#endif

QTOut::QTOut(QListWidget *listWidget) :
	m_listWidget(listWidget)
{
	Message("QTOut::QTOut","Starting up QTListviewDebug out");
}

QTOut::~QTOut() {
	Message("QTOut::~QTOut","Shutting down QTListviewDebug out");
}

void QTOut::printf(const char* format, ...)
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
	m_listWidget->addItem ( buff );
}

void QTOut::_printf(const char* format, ...)
{
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	m_listWidget->addItem ( buff );
}

void QTOut::Message(const char* source, const char* format, ...) {
	if (!m_bShowMessages) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	_printf("MESSAGE (%s): %s",source, buff);
}

void QTOut::Warning(const char* source, const char* format, ...) {
	if (!m_bShowWarnings) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	_printf("WARNING (%s): %s",source, buff);
}

void QTOut::Error(const char* source, const char* format, ...) {
	if (!m_bShowErrors) return;
	char buff[16384];
	va_list args;
	va_start(args, format);
#ifdef WIN32
	_vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
	vsnprintf( buff, sizeof(buff), format, args);
#endif
	_printf("ERROR (%s): %s",source, buff);
}
