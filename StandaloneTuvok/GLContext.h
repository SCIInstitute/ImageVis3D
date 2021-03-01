#ifndef __GL_CONTEXT
#define __GL_CONTEXT

#include <StdTuvokDefines.h>
#include <sstream>

#ifdef DETECTED_OS_WINDOWS
#include <windows.h>
#endif

class GLContext
{
public:
	GLContext(UINT32 width, UINT32 height, BYTE colorBits = 32, BYTE depthBits = 24, BYTE stencilBits = 8, bool useDoubleBuffer = true, std::wostream * errorOutput = NULL);
	~GLContext();

	bool isValid() const { return valid; }
	bool set();
	bool restorePrevious();
	bool copyState(const GLContext & fromContext);
	bool swapBuffers();

private:
#ifdef DETECTED_OS_WINDOWS
	HDC deviceContext, previousDeviceContext;
	HGLRC renderingContext, previousRenderingContext;
	HWND window;
#else
	Window window, previousWindow;
	GLXContext renderingContext, previousRenderingContext;
	Display * display;
#endif

	bool valid;
	std::wostream * errorStream;

	void outputWarning(const std::wstring & warning) const;
	void outputError(const std::wstring & error) const;
	void outputLastError() const;
};

#endif
