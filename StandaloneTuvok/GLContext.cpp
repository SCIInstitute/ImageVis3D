#include "GLContext.h"
#include "3rdParty/GLEW/GL/glew.h"

#ifdef DETECTED_OS_WINDOWS
#include "3rdParty/GLEW/GL/wglew.h"
#else
#include "3rdParty/GLEW/GL/glxew.h"
#endif

GLContext::GLContext(UINT32 width, UINT32 height, BYTE colorBits, BYTE depthBits, BYTE stencilBits, bool useDoubleBuffer, std::wostream * errorOutput) : 
#ifdef DETECTED_OS_WINDOWS
						deviceContext(NULL), previousDeviceContext(NULL), renderingContext(NULL), previousRenderingContext(NULL), window(NULL), 
#else
						window(None), previousWindow(None), renderingContext(NULL), previousRenderingContext(NULL), display(NULL), 
#endif
						valid(false), errorStream(errorOutput)
{

	if (width == 0 || height == 0 || !(colorBits == 8 || colorBits == 16 || colorBits == 24 || colorBits == 32) || 
									!(depthBits == 0 || depthBits == 8 || depthBits == 16 || depthBits == 24 || depthBits == 32) || 
									!(stencilBits == 0 || stencilBits == 8)) 
	{
		outputError(L"Invalid parameters passed to constructor");
		return;
	}
	
#ifdef DETECTED_OS_WINDOWS
	window = CreateWindowExW(WS_EX_TOOLWINDOW, L"Static", L"GLContextWindow", 
								WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN , 
								0, 0, width, height, 0, 0, GetModuleHandle(NULL), 0);
	if (!window)
	{
		outputLastError();
		return;
	}

	ShowWindow(window, SW_HIDE);

	deviceContext = GetDC(window);

	if (!deviceContext)
	{
		outputLastError(); // GetDC does not emit an error according to the documentation, but this should still work
		return;
	}

	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_TYPE_RGBA;
	if (useDoubleBuffer) pfd.dwFlags |= PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = colorBits;
	pfd.cRedBits = 0;
	pfd.cRedShift = 0;
	pfd.cGreenBits = 0;
	pfd.cGreenShift = 0;
	pfd.cBlueBits = 0;
	pfd.cBlueShift = 0;
	pfd.cAlphaBits = 0;
	pfd.cAlphaShift = 0;
	pfd.cAccumBits = 0;
	pfd.cAccumRedBits = 0;
	pfd.cAccumGreenBits = 0;
	pfd.cAccumBlueBits = 0;
	pfd.cAccumAlphaBits = 0;
	pfd.cDepthBits = depthBits;
	pfd.cStencilBits = stencilBits;
	pfd.cAuxBuffers = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.bReserved = 0;
	pfd.dwLayerMask = 0;
	pfd.dwVisibleMask = 0;
	pfd.dwDamageMask = 0;

	int pixelFormat = ChoosePixelFormat(deviceContext, &pfd);

	if (!pixelFormat)
	{
		outputLastError();
		return;
	}
	
	PIXELFORMATDESCRIPTOR pfdResult;
	DescribePixelFormat(deviceContext, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfdResult);

	if (!(pfdResult.dwFlags & PFD_SUPPORT_OPENGL))
	{
		outputError(L"No OpenGL support");
		return;
	}

	if (useDoubleBuffer && !(pfdResult.dwFlags & PFD_DOUBLEBUFFER)) outputWarning(L"No double buffer support");

	std::wostringstream ss;
	if (pfdResult.cColorBits != colorBits) 
	{
		ss << L"Color bits requested: " << colorBits << L", actual color bits: " << pfdResult.cColorBits;
		outputWarning(ss.str());
	}
	ss.str(L"");
	if (pfdResult.cDepthBits != depthBits) 
	{
		ss << L"Depth bits requested " << depthBits << L", actual depth bits: " << pfdResult.cDepthBits;
		outputWarning(ss.str());
	}
	ss.str(L"");
	if (pfdResult.cStencilBits != stencilBits) 
	{
		ss << L"Stencil bits requested " << stencilBits << L", actual stencil bits: " << pfdResult.cStencilBits;
		outputWarning(ss.str());
	}
	
	if (!SetPixelFormat(deviceContext, pixelFormat, &pfd))
	{
		outputLastError();
		return;
	}

	renderingContext = wglCreateContext(deviceContext);

	if (!renderingContext)
	{
		outputLastError();
		return;
	}

	// we prefer to remember this once in the beginning for now
	// this is more predictable to handle
	// but we can still get into quite some confusion when using several GLContexts interleaved, so probably make this more intelligent later
	previousDeviceContext = wglGetCurrentDC();
	previousRenderingContext = wglGetCurrentContext();
#else
	display = XOpenDisplay(0);

	if (!display)
	{
		outputError(L"Could not create display");
		return;
	}

	if (!glXQueryExtension(display, NULL, NULL) || !glxQueryVersion(display, NULL, NULL))
	{
		outputError(L"Could not find valid glx extension");
		return;
	}

	int sizePerChannel = (int)colorBits / 4;
	int doubleBufferAttribute = (useDoubleBuffer ? GLX_DOUBLEBUFFER : GLX_USE_GL); // GLX_USE_GL is just ignored as it is true on default
	int attributeList[] = 
	{ 
		GLX_RGBA, 
		doubleBufferAttribute, 
		GLX_RED_SIZE, sizePerChannel, 
		GLX_GREEN_SIZE, sizePerChannel, 
		GLX_BLUE_SIZE, sizePerChannel, 
		GLX_ALPHA_SIZE, sizePerChannel, 
		GLX_DEPTH_SIZE, (int)depthBits, 
		GLX_STENCIL_SIZE, (int)stencilBits,
		None
	};
	XVisualInfo * vi = glXChooseVisual(display, DefaultScreen(display), attributeList);

	if (!vi)
	{
		outputError(L"Could not create visual");
		return;
	}
	// add warnings here whether requested visual attributes could be supported (using glXGetConfig())

	// we should not need our own error handling for those, as they should use the default handler and thus exit on error
	XSetWindowAttributes swa;
	swa.colormap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	window = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XFreeColorMap(swa.colormap):
	XMapWindow(display, window);

	renderingContext = glxCreateContext(display, vi, NULL, true);
	XFree(vi);

	if (!renderingContext)
	{
		outputError(L"Could not create glx context");
		return;
	}

	previousWindow = glXGetCurrentDrawable();
	previousRenderingContext = glXGetCurrentContext();
#endif
	valid = true;
}

GLContext::~GLContext()
{
// add error messages here later as those methods may fail as well
#ifdef DETECTED_OS_WINDOWS
	if (deviceContext == wglGetCurrentDC() && renderingContext == wglGetCurrentContext()) restorePrevious();
	wglDeleteContext(renderingContext);
	ReleaseDC(window, deviceContext);
	DestroyWindow(window);
#else
	if (window == glXGetCurrentDrawable() && renderingContext == glXGetCurrentContext()) restorePrevious();
	glXDestroyContext(display, renderingContext);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
#endif
}

// for the next two, we go the windows way for now
// this means that if we have a valid context and they still fail, the current context of the thread will be unset (see comments below)
// NOTE: this has been tested and the msdn documentation http://msdn.microsoft.com/en-us/library/dd374387%28VS.85%29.aspx is wrong in this case
// if an error occurs, the threads current context will not be made not current (even though the doc states exactly that)
// so, we roll back and below methods will now leave the current context as is on error
bool GLContext::set()
{
	if (!isValid()) return false;
#ifdef DETECTED_OS_WINDOWS
	if (!wglMakeCurrent(deviceContext, renderingContext)) // if this fails, it automatically unsets the threads context
		// so remember to call restorePrevious to fall back to the context which was current when this context was created
		// or call set of the context which was current before this call
		// otherwise, no context will be set at all
		// WRONG, see comment above
	{
		outputLastError();
		return false;
	}
#else
	if (!glXMakeCurrent(display, window, renderingContext))
	{
		outputError(L"Could not set glx context");
		//glXMakeCurrent(display, None, NULL); // we call this to unset the threads context which is not done automatically (unlike wglMakeCurrent)
		return false;
	}
#endif
	return true;
}

bool GLContext::restorePrevious()
{
	if (!isValid()) return false;
#ifdef DETECTED_OS_WINDOWS
	if (!wglMakeCurrent(previousDeviceContext, previousRenderingContext))
	{
		outputLastError();
		return false;
	}
#else
	if (!glXMakeCurrent(display, previousWindow, previousRenderingContext))
	{
		outputError(L"Could not restore previous glx context");
		//glXMakeCurrent(display, None, NULL);
		return false;
	}
#endif
	return true;
}

bool GLContext::copyState(const GLContext & fromContext)
{
	if (!isValid()) return false;
#ifdef DETECTED_OS_WINDOWS
	if (!wglCopyContext(fromContext.renderingContext, renderingContext, GL_ALL_ATTRIB_BITS)) 
	{
		outputLastError();
		return false;
	}
#else
	glXCopyContext(display, fromContext.renderingContext, renderingContext, GLX_ALL_ATTRIB_BITS);
#endif
	return true;
}

bool GLContext::swapBuffers()
{
	if (!isValid()) return false;
#ifdef DETECTED_OS_WINDOWS
	if (!wglSwapLayerBuffers(deviceContext, WGL_SWAP_MAIN_PLANE))
	{
		outputLastError();
		return false;
	}
#else
	glXSwapBuffers(display, window);
#endif
	return true;
}

void GLContext::outputWarning(const std::wstring & warning) const
{
	if (errorStream) *errorStream << "Warning: " << warning << std::endl;
}

void GLContext::outputError(const std::wstring & error) const
{
	if (errorStream) *errorStream << "Error: " << error << std::endl;
}

void GLContext::outputLastError() const
{
	if (errorStream)
	{
#ifdef DETECTED_OS_WINDOWS
		DWORD lastError = GetLastError();
		LPVOID msgBuffer;
		FormatMessageW
		(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&msgBuffer,
			0, NULL
		);
		outputError(std::wstring((LPTSTR)msgBuffer));
		LocalFree(msgBuffer);
#else
#endif
	}
}
