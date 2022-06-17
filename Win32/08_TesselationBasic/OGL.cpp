// header files
#include<windows.h>
#include<stdlib.h>	// for exit()
#include<stdio.h>	// for file I/O functions
#include"OGL.h"

// header for the graphics library extension wrangler
#include<GL/glew.h>

// OpenGL headers
#include<GL/gl.h>
#include<gl/GLU.h>

// OpenGL libraries
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glu32.lib")

// macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// global function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// global variable declarations
FILE *gpLog = NULL;
HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;
BOOL gbFullScreen = FALSE;
BOOL gbActiveWindow = FALSE;

// shaders and programs
static const GLchar *vertex_shader_source[] =
{
	"#version 460 core                                                  \n"
	"                                                                   \n"
	"layout (location = 0) in vec4 offset;                              \n"
	"                                                                   \n"
	"void main(void)                                                    \n"
	"{                                                                  \n"
	"   const vec4 vertices[] = vec4[3](vec4( 0.25, -0.25, 0.5, 1.0),   \n"
	"                                   vec4(-0.25, -0.25, 0.5, 1.0),   \n"
	"                                   vec4( 0.25,  0.25, 0.5, 1.0));  \n"
	"                                                                   \n"
	"   gl_Position = vertices[gl_VertexID] + offset;                   \n"
	"}                                                                  \n"
};
static const GLchar *tesselation_control_shader_source[] =
{
	"#version 460 core                                                            \n"
	"                                                                             \n"
	"layout (vertices = 3) out;                                                   \n"
	"                                                                             \n"
	"void main(void)                                                              \n"
	"{                                                                            \n"
	"   if(gl_InvocationID == 0)                                                  \n"
	"   {                                                                         \n"
	"      gl_TessLevelInner[0] = 5.0;                                            \n"
	"      gl_TessLevelOuter[0] = 5.0;                                            \n"
	"      gl_TessLevelOuter[1] = 5.0;                                            \n"
	"      gl_TessLevelOuter[2] = 5.0;                                            \n"
	"   }                                                                         \n"
	"                                                                             \n"
	"   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n"
	"}                                                                            \n"
};
static const GLchar *tesselation_evaluation_shader_source[] =
{
	"#version 460 core                                         \n"
	"                                                          \n"
	"layout (triangles, equal_spacing, cw) in;                 \n"
	"                                                          \n"
	"void main(void)                                           \n"
	"{                                                         \n"
	"   gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position + \n"
	"                  gl_TessCoord.y * gl_in[1].gl_Position + \n"
	"                  gl_TessCoord.z * gl_in[2].gl_Position); \n"
	"}                                                         \n"
};
static const GLchar *fragment_shader_source[] =
{
	"#version 460 core                          \n"
	"                                           \n"
	"out vec4 color;                            \n"
	"                                           \n"
	"void main(void)                            \n"
	"{                                          \n"
	"   color = vec4(0.0f, 0.8f, 1.0f, 1.0f);   \n"
	"}                                          \n"
};
GLuint rendering_program;

// required for programmable pipeline of OpenGL
GLuint vertex_array_object;

// rendering global
GLfloat color[4];
GLfloat positionVertexAttrib[4];

// entry-point function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// function prototypes
	int initialize(void);	// declaring according to the order of use
	void display(void);
	void update(void);
	void uninitialize(void);

	// variable declarations
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szAppName[] = TEXT("MyWindow");
	BOOL bDone = FALSE;
	int iRetVal = 0;
	int cxScreen, cyScreen;

	// code
	if (fopen_s(&gpLog, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("fopen_s: failed to open log file"), TEXT("File I/O Error"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	else
	{
		fprintf(gpLog, "fopen_s: log file opened successfully\n");
	}

	// initialization of the wndclass structure
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));

	// registering the wndclass
	RegisterClassEx(&wndclass);

	// getting the screen size
	cxScreen = GetSystemMetrics(SM_CXSCREEN);
	cyScreen = GetSystemMetrics(SM_CYSCREEN);

	// create the window
	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szAppName,
		TEXT("OpenGL Superbible Examples: Kaivalya Vishwakumar Deshpande"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		(cxScreen - WIN_WIDTH) / 2,
		(cyScreen - WIN_HEIGHT) / 2,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);
	ghwnd = hwnd;
	
	// initialize
	iRetVal = initialize();
	if (iRetVal == -1)
	{
		fprintf(gpLog, "ChoosePixelFormat(): failed\n");
		uninitialize();
		exit(EXIT_FAILURE);
	}
	else if (iRetVal == -2)
	{
		fprintf(gpLog, "SetPixelFormat(): failed\n");
		uninitialize();
		exit(EXIT_FAILURE);
	}
	else if (iRetVal == -3)
	{
		fprintf(gpLog, "wglCreateContext(): failed\n");
		uninitialize();
		exit(EXIT_FAILURE);
	}
	else if (iRetVal == -4)
	{
		fprintf(gpLog, "wglMakeCurrent(): failed\n");
		uninitialize();
		exit(EXIT_FAILURE);
	}
	else if (iRetVal == -5)
	{
		fprintf(gpLog, "glewInit(): failed\n");
		uninitialize();
		exit(EXIT_FAILURE);
	}
	else
	{
		fprintf(gpLog, "created OpenGL context successfully and made it the current context\n");
		fprintf(gpLog, "using GLEW %s\n", glewGetString(GLEW_VERSION));
	}

	// show the window
	ShowWindow(hwnd, iCmdShow);

	// foregrounding and focussing the window
	SetForegroundWindow(hwnd);	// using ghwnd is obviously fine, but by common sense ghwnd is for global use while we have hwnd locally available in WndProc and here
	SetFocus(hwnd);

	// game loop
	while (bDone != TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = TRUE;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (gbActiveWindow)
			{
				// render the scene
				display();

				// update the scene
				update();
			}
		}
	}

	// uninitialize
	uninitialize();

	return (int)msg.wParam;
}

// callback function
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// function prototypes
	void ToggleFullScreen(void);
	void resize(int, int);

	// code
	switch (iMsg)
	{
	case WM_SETFOCUS:
		gbActiveWindow = TRUE;
		fprintf(gpLog, "window in focus\n");
		break;
	case WM_KILLFOCUS:
		gbActiveWindow = FALSE;
		fprintf(gpLog, "window out of focus\n");
		break;
	case WM_ERASEBKGND:
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 27:
			fprintf(gpLog, "destroying after receiving esc\n");
			DestroyWindow(hwnd);
			break;
		default:
			break;
		}
		break;
	case WM_CHAR:
		switch (wParam)
		{
		case 'F':
		case 'f':
			ToggleFullScreen();
			break;
		default:
			break;
		}
		break;
	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_CLOSE:	// disciplined code: sent as a signal that a window or an application should terminate
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void ToggleFullScreen(void)
{
	// variable declarations
	static DWORD dwStyle;
	static WINDOWPLACEMENT wp;
	MONITORINFO mi;

	// code
	wp.length = sizeof(WINDOWPLACEMENT);

	if (gbFullScreen == FALSE)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);

		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi.cbSize = sizeof(MONITORINFO);

			if (GetWindowPlacement(ghwnd, &wp) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			ShowCursor(FALSE);
			gbFullScreen = TRUE;
			fprintf(gpLog, "fullscreen mode on\n");
		}
	}
	else
	{
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wp);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);
		gbFullScreen = FALSE;
		fprintf(gpLog, "fullscreen mode off\n");
	}
}

int initialize(void)
{
	// function prototypes
	GLuint compile_shaders(void);
	void resize(int, int);

	// variable declarations
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex = 0;
	GLenum ret;
	RECT rc;

	// code
	// initialize PIXELFORMATDESCRIPTOR
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;	// R
	pfd.cGreenBits = 8;	// G
	pfd.cBlueBits = 8;	// B
	pfd.cAlphaBits = 8;	// A

	// get DC
	ghdc = GetDC(ghwnd);

	// choose pixel format
	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
	if (iPixelFormatIndex == 0)
		return -1;

	// set chosen pixel format
	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
		return -2;

	// create OpenGL rendering context
	ghrc = wglCreateContext(ghdc);
	if (ghrc == NULL)
		return -3;

	// make the rendering context the current context
	if (wglMakeCurrent(ghdc, ghrc) == FALSE)
		return -4;
	
	// initialize glew
	ret = glewInit();
	if (ret != GLEW_OK)
		return -5;

	// compile the shaders
	rendering_program = compile_shaders();

	// create and bind the vertex array object
	glCreateVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	// warmup resize
	GetClientRect(ghwnd, &rc);
	resize(rc.right - rc.left, rc.bottom - rc.top);

	return 0;
}

GLuint compile_shaders(void)
{
	// function prototypes
	void uninitialize(void);

	// variable declarations
	GLuint vertex_shader = 0;
	GLuint fragment_shader = 0;
	GLuint tesselation_control_shader = 0;
	GLuint tesselation_evaluation_shader = 0;
	GLuint program = 0;
	int len;
	char buf[1024];

	// code
	// create and compile the vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderInfoLog(vertex_shader, sizeof(buf), &len, buf);
	if (len)
		fprintf(gpLog, "\nVertex shader: compilation errors:\n%s", buf);
	else
		fprintf(gpLog, "Vertex shader compiled without errors\n");

	// create and compile the tesselation control shader
	tesselation_control_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tesselation_control_shader, 1, tesselation_control_shader_source, NULL);
	glCompileShader(tesselation_control_shader);
	glGetShaderInfoLog(tesselation_control_shader, sizeof(buf), &len, buf);
	if (len)
		fprintf(gpLog, "\nTesselation control shader: compilation errors:\n%s", buf);
	else
		fprintf(gpLog, "Tesselation control shader compiled without errors\n");

	// create and compile the tesselation evaluation shader
	tesselation_evaluation_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tesselation_evaluation_shader, 1, tesselation_evaluation_shader_source, NULL);
	glCompileShader(tesselation_evaluation_shader);
	glGetShaderInfoLog(tesselation_evaluation_shader, sizeof(buf), &len, buf);
	if (len)
		fprintf(gpLog, "\nTesselation evaluation shader: compilation errors:\n%s", buf);
	else
		fprintf(gpLog, "Tesselation evaluation shader compiled without errors\n");

	// create and compile the fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderInfoLog(fragment_shader, sizeof(buf), &len, buf);
	if(len)
		fprintf(gpLog, "\nFragment shader: compilation errors:\n%s", buf);
	else
		fprintf(gpLog, "Fragment shader compiled without errors\n");

	// create a program object, attach the shaders to it, and link it
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, tesselation_control_shader);
	glAttachShader(program, tesselation_evaluation_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramInfoLog(program, sizeof(buf), &len, buf);
	if(len)
		fprintf(gpLog, "\nProgram: linking errors:\n%s\n", buf);
	else
		fprintf(gpLog, "Program linked without errors\n");

	// delete created shader objects for they are now copied into the program
	glDeleteShader(fragment_shader);
	glDeleteShader(tesselation_evaluation_shader);
	glDeleteShader(tesselation_control_shader);
	glDeleteShader(vertex_shader);

	return program;
}

void resize(int width, int height)
{
	// code
	if (height == 0)
		height = 1;	// to prevent a divide by zero when calculating the width/height ratio

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void display(void)
{	
	// code
	glClearBufferfv(GL_COLOR, 0, color);
	
	glUseProgram(rendering_program);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_PATCHES, 0, 3);

	SwapBuffers(ghdc);
}

void update(void)
{
	// code
	color[0] = 0.0f;
	color[1] = 0.2f;
	color[2] = 0.0f;
	color[3] = 1.0f;

	positionVertexAttrib[0] = 0.0f;
	positionVertexAttrib[1] = 0.0f;
	positionVertexAttrib[2] = 0.0f;
	positionVertexAttrib[3] = 0.0f;
	glVertexAttrib4fv(0, positionVertexAttrib);
}

void uninitialize(void)
{
	// function prototypes
	void ToggleFullScreen(void);

	// code
	if (vertex_array_object)
	{
		glDeleteVertexArrays(1, &vertex_array_object);
		vertex_array_object = 0;
	}

	if (rendering_program)
	{
		glDeleteProgram(rendering_program);
		rendering_program = 0;
	}

	if (gbFullScreen)
	{
		ToggleFullScreen();
	}

	if (wglGetCurrentContext() == ghrc)
	{
		wglMakeCurrent(NULL, NULL);
	}

	if (ghrc)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
	}

	if (ghdc)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	if (ghwnd)
	{
		DestroyWindow(ghwnd);	// if unitialize() was not called from WM_DESTROY
		ghwnd = NULL;
	}

	if (gpLog)
	{
		fprintf(gpLog, "fclose: closing log file\n");
		fclose(gpLog);
		gpLog = NULL;
	}
}
