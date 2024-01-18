// Last updated: <2024/01/19 02:13:31 +0900>
//
// stars GL screen saver by mieki256.
//
// Original open-source example screensaver by Rachel Grey, lemming@alum.mit.edu.
//
// Use Windows10 x64 22H2 + MinGW (gcc 6.3.0) + scrnsave.h + libscrnsave.a
//
// Build:
//
// xxd -i texture.png > texture.h
// windres resource.rc resource.o
// g++ -c ssstars_opengl.cpp -o ssstars_opengl.o -O3
// g++ ssstars_opengl.o resource.o -o ssstars_opengl.scr -static -lstdc++ -lgcc -lscrnsave -lopengl32 -lglu32 -lgdi32 -lcomctl32 -lshlwapi -lwinmm -mwindows

#define _USE_MATH_DEFINES

// use SHGetSpecialFolderPath()
#define _WIN32_IE 0x0400

#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <windows.h>
#include <scrnsave.h>
#include <math.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "resource.h"

// use stb library
// https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// include png image binary
#include "texture.h"

// include font data
#include "fontdata_profont.h"

// get rid of these warnings: truncation from const double to float conversion from double to float
// #pragma warning(disable: 4305 4244)

// config ini file name and directory
#define INIFILENAME _T("ssstars_opengl.ini")
#define INIDIR _T("ssstars_opengl")

// ini file section name
#define SECNAME _T("ssstars_opengl_config")

#define USE_DEPTHMASK 0
#define USE_FOG 1

// Define a Windows timer
#define TIMER 1

#define OBJ_MAX 4000

#define DEG2RAD(x) (double)((x) * M_PI / 180.0)

// setting value
static int waitValue = 15;
static int speedValue = 1000;
static int numberValue = 1000;
static int fps_display = 1;

// globals for size of screen
static int Width, Height;

// fps count work
static DWORD rec_time;
static DWORD prev_time;
static int count_frame;
static int count_fps;

// global work
static GLuint texture;

static GLfloat dist = 300.0;
static GLfloat fovy = 90.0;

// star object work
typedef struct
{
  GLfloat x;
  GLfloat y;
  GLfloat z;
  GLfloat dist;
  GLfloat spd;

  int kind;
  GLfloat tx;
  GLfloat ty;
  GLfloat tw;
  GLfloat th;
} star;

static star objw[OBJ_MAX];

// prototype
static void initCountFps(void);
static void closeCountFps(void);
static void countFps(void);
static double getRand(void);
void initXYPos(star *);
void initObjs(void);
void updateObjs(float);
GLuint createTextureFromPngInMemory(const unsigned char *, int);
void drawText(char *);
void Render(HDC);
void SetupAnimation(int Width, int Height);
static bool InitGL(HWND hWnd, HDC &hDC, HGLRC &hRC);
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC);
void CleanupAnimation();
BOOL getIniFilePath(TCHAR *filepath);
void writeConfigToIniFile(void);
void setConfigValueToGlobalWork(void);
BOOL getConfigFromIniFile(void);
int clamp(int, int, int);
void setValueOnDialog(HWND, int, int, int, int);
void getValueFromDialog(HWND hDlg);

// ========================================
// Screen Saver Procedure
LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam)
{
  static HDC hDC;
  static HGLRC hRC;
  static RECT rect;

  switch (message)
  {
  case WM_CREATE:
    GetClientRect(hWnd, &rect); // get window dimensions
    Width = rect.right;
    Height = rect.bottom;

    initCountFps();         // initialize FPS counter
    getConfigFromIniFile(); // get config value from ini file

    // setup OpenGL, then animation
    if (!InitGL(hWnd, hDC, hRC))
      break;

    SetupAnimation(Width, Height);

    SetTimer(hWnd, TIMER, waitValue, NULL); // set timer
    return 0;

  case WM_TIMER:
    Render(hDC); // animate
    return 0;

  case WM_DESTROY:
    closeCountFps();
    KillTimer(hWnd, TIMER);
    CleanupAnimation();
    CloseGL(hWnd, hDC, hRC);
    return 0;
  }

  return DefScreenSaverProc(hWnd, message, wParam, lParam);
}

// ========================================
// screen saver config dialog procedure
BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                       WPARAM wParam, LPARAM lParam)
{
  // InitCommonControls();
  // would need this for slider bars or other common controls

  switch (message)
  {
  case WM_INITDIALOG:
    // Initialize config dialog
    LoadString(hMainInstance, IDS_DESCRIPTION, szAppName, 40);
    getConfigFromIniFile();
    setValueOnDialog(hDlg, waitValue, speedValue, numberValue, fps_display);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
      // ok button pressed
      getValueFromDialog(hDlg);
      writeConfigToIniFile();
      EndDialog(hDlg, LOWORD(wParam) == IDOK);
      return TRUE;

    case IDCANCEL:
      // cancel button pressed
      EndDialog(hDlg, LOWORD(wParam) == IDOK);
      return TRUE;

    case IDC_RESET:
      // reset button pressed
      setValueOnDialog(hDlg, 15, 1000, 1000, 1);
      return TRUE;
    }

    return FALSE;
  } // end command switch

  return FALSE;
}

// ========================================
// needed for SCRNSAVE.LIB (or libscrnsave.a)
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}

// ========================================
// Init count FPS work
static void initCountFps(void)
{
  timeBeginPeriod(1);
  rec_time = timeGetTime();
  prev_time = rec_time;
  count_fps = 0;
  count_frame = 0;
}

// Close count FPS
static void closeCountFps(void)
{
  timeEndPeriod(1);
}

// count FPS
static void countFps(void)
{
  count_frame++;
  DWORD t = timeGetTime() - rec_time;

  if (t >= 1000)
  {
    rec_time += 1000;
    count_fps = count_frame;
    count_frame = 0;
  }
  else if (t < 0)
  {
    rec_time = timeGetTime();
    count_fps = 0;
    count_frame = 0;
  }
}

// get random value. (0.0 - 1.0)
static double getRand(void)
{
  return ((double)rand() / RAND_MAX); // retrun 0.0 - 1.0
}

// initialize object x, y position
void initXYPos(star *o)
{
  float h = o->z * tan((fovy / 2.0) * M_PI / 180.0);
  float aspect = (float)Width / (float)Height;
  o->x = (getRand() * 2.0 - 1.0) * h * aspect;
  o->y = (getRand() * 2.0 - 1.0) * h;
}

// initialize object work
void initObjs(void)
{
  for (int i = 0; i < OBJ_MAX; i++)
  {
    objw[i].dist = dist;
    objw[i].spd = ((float)speedValue) / 1000.0;

    int k = rand() % 4;
    objw[i].kind = k;
    objw[i].tx = 0.5 * (k & 0x01);
    objw[i].ty = 0.5 * ((k >> 1) & 0x01);
    objw[i].tw = 0.5;
    objw[i].th = 0.5;

    objw[i].z = getRand() * (-1.0 * dist);
    initXYPos(&(objw[i]));
  }
}

// update object work
void updateObjs(float delta)
{
  for (int i = 0; i < numberValue; i++)
  {
    objw[i].z += (objw[i].spd * 60.0 * delta);

    if (objw[i].z >= 0.0)
    {
      objw[i].z -= objw[i].dist;
      initXYPos(&(objw[i]));
      continue;
    }

    // get position on screen
    GLfloat sz = (float)(Height / 2.0) / tan((fovy / 2.0) * M_PI / 180.0);
    GLfloat sx = objw[i].x * sz / objw[i].z;
    GLfloat sy = objw[i].y * sz / objw[i].z;
    float wh = (float)(Width / 2.0) * 1.2;
    float hh = (float)(Height / 2.0) * 1.2;

    if (sx < -wh || wh < sx || sy < -hh || hh < sy)
    {
      // outside display area
      objw[i].z = -objw[i].dist - 15.0;
      initXYPos(&(objw[i]));
      continue;
    }
  }
}

// Initialize OpenGL
static bool InitGL(HWND hWnd, HDC &hDC, HGLRC &hRC)
{
  hDC = GetDC(hWnd);

#if 0
  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory(&pfd, sizeof pfd);
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;

  //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; //blaine's
  // pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;

  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
#else

  try
  {
    PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
            1,                             // version number
            // support window, OpenGL, double bufferd
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,  // RGBA type
            32,             // color
            0, 0,           // R
            0, 0,           // G
            0, 0,           // B
            0, 0,           // A
            0,              // accumulation buffer
            0, 0, 0, 0,     // accum bits ignored
            24,             // depth
            8,              // stencil buffer
            0,              // auxiliary buffer
            PFD_MAIN_PLANE, // main layer
            0,              // reserved
            0, 0, 0         // layermask, visiblemask, damagemask
        };

    int format = ChoosePixelFormat(hDC, &pfd);

    if (format == 0)
      throw "";

    if (!SetPixelFormat(hDC, format, &pfd))
      throw "";

    hRC = wglCreateContext(hDC);

    if (!hRC)
      throw "";
  }
  catch (...)
  {
    ReleaseDC(hWnd, hDC);
    return false;
  }

#endif

  wglMakeCurrent(hDC, hRC);

  return true;
}

// Shut down OpenGL
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(hRC);
  ReleaseDC(hWnd, hDC);
}

/**
 * Load texture from png image on memory
 *
 * @param[in] _pngData png binary on memory
 * @param[in] _pngDataLen png binary size
 * return GLuint OpenGL texture ID. if 0, process fails
 */
GLuint createTextureFromPngInMemory(const unsigned char *_pngData, int _pngLen)
{
  GLuint texture;
  int width = 0, height = 0, bpp = 0;
  unsigned char *data = NULL;

  data = stbi_load_from_memory(_pngData, _pngLen, &width, &height, &bpp, 4);

  // create OpenGL texture
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  glClearColor(0, 0, 0, 0);
  glShadeModel(GL_SMOOTH);

  // set texture repeat
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // set texture filter
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

  // Release allocated all memory
  stbi_image_free(data);

  return texture;
}

// setup animation
void SetupAnimation(int Width, int Height)
{
  // window resizing stuff
  glViewport(0, 0, (GLsizei)Width, (GLsizei)Height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Calculate The Aspect Ratio Of The Window
  float aspect = (float)Width / (float)Height;
  GLfloat znear = 0.1;
  GLfloat zfar = dist + 50.0;
  gluPerspective(fovy, aspect, znear, zfar);
  glMatrixMode(GL_MODELVIEW);

  glLoadIdentity();

  // background
  glClearColor(0.0, 0.0, 0.0, 0.0); // 0.0s is black
  glClearDepth(1.0);

  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  // glShadeModel(GL_FLAT);
  glShadeModel(GL_SMOOTH);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK);

#if USE_FOG == 1
  // set fog
  GLfloat fog_color[4] = {0.0, 0.0, 0.0, 0.0};
  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_START, dist * 0.75);
  glFogf(GL_FOG_END, dist);
  glFogfv(GL_FOG_COLOR, fog_color);
#endif

  // create OpenGL texture from PNG image
  texture = createTextureFromPngInMemory((unsigned char *)&texture_png, texture_png_len);

  if (!texture)
  {
    fprintf(stderr, "Failed create texture\n");
    exit(1);
  }

  initObjs();
}

// cleanup animation
void CleanupAnimation()
{
  // didn't create any objects, so no need to clean them up
}

// main loop. draw OpenGL
void Render(HDC hDC)
{
  DWORD now_time, deltam;
  float delta;

  // get delta time (millisecond)
  now_time = timeGetTime();
  deltam = now_time - prev_time;
  if (deltam <= 0)
    deltam = waitValue;

  // get delta time (second)
  delta = (float)deltam / 1000.0;
  prev_time = now_time;

  updateObjs(delta);

  glClearColor(0.0, 0.0, 0.0, 0.0); // set background color
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity(); // Reset The View

  // set camera
  gluLookAt(0.0, 0.0, 0.0, 0.0, 0.0, -10.0, 0.0, 1.0, 0.0);

  glEnable(GL_BLEND);

  // enable texture
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_ONE, GL_ONE);

  glColor4f(1.0, 1.0, 1.0, 1.0); // set color

  // draw objects
#if USE_DEPTHMASK == 1
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
#else
  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);
#endif

  GLfloat v = 2.0;

  for (int i = 0; i < numberValue; i++)
  {
    GLfloat tx, ty, tw, th;
    star *o = &(objw[i]);
    tx = o->tx;
    ty = o->ty;
    tw = o->tw;
    th = o->th;

    glPushMatrix();
    glTranslatef(o->x, o->y, o->z); // translate

    {
      GLdouble m[16];
      glGetDoublev(GL_MODELVIEW_MATRIX, m);
      m[0] = m[5] = m[10] = 1.0;
      m[1] = m[2] = m[4] = m[6] = m[8] = m[9] = 0.0;
      glLoadMatrixd(m);
    }

    glBegin(GL_QUADS);
    glTexCoord2f(tx, ty);  // set u, v
    glVertex3f(-v, -v, 0); // Top Left
    glTexCoord2f(tx + tw, ty);
    glVertex3f(v, -v, 0.0); // Top Right
    glTexCoord2f(tx + tw, ty + th);
    glVertex3f(v, v, 0.0); // Bottom Right
    glTexCoord2f(tx, ty + th);
    glVertex3f(-v, v, 0.0); // Bottom Left
    glEnd();
    glPopMatrix();
  }

  glDisable(GL_TEXTURE_2D);

#if USE_DEPTHMASK == 0
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
#endif

  // draw FPS text
  if (fps_display != 0)
  {
    glColor4f(1, 1, 1, 1);           // set color
    glRasterPos3f(-0.1, 1.0, -1.08); // set postion
    {
      char buf[512];
      sprintf(buf, "%d FPS", count_fps);
      drawText(buf);
    }
  }

  SwapBuffers(hDC);

  countFps(); // calc FPS
}

// draw text
void drawText(char *str)
{
  GLsizei w = (GLsizei)fontdata_width;
  GLsizei h = (GLsizei)fontdata_height;
  GLfloat xorig, yorig;
  GLfloat xmove, ymove;
  xorig = 0;
  yorig = 0;
  xmove = w;
  ymove = 0;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Without this, drawing will be incorrect

  int slen = strlen(str);

  for (int i = 0; i < slen; i++)
  {
    int c = str[i];

    if (c == 0)
      break;

    if (c < 0x20 || c > 0x7f)
      c = 0x20;

    c -= 0x20;
    glBitmap(w, h, xorig, yorig, xmove, ymove, &(fontdata[c][0]));
  }
}

BOOL getIniFilePath(TCHAR *filepath)
{
  TCHAR appdataFolderPath[MAX_PATH];
  TCHAR tgtFolderPath[MAX_PATH];
  BOOL createDir = FALSE;

  // get APPDATA folder path
  SHGetSpecialFolderPath(NULL, appdataFolderPath, CSIDL_APPDATA, FALSE);
  if (PathCombine(tgtFolderPath, appdataFolderPath, INIDIR) == NULL)
    return FALSE;

  if (!PathFileExists(tgtFolderPath))
  {
    createDir = TRUE;
  }
  else if (!PathIsDirectory(tgtFolderPath))
  {
    createDir = TRUE;
  }
  if (createDir)
  {
    if (!CreateDirectory(tgtFolderPath, NULL))
      return FALSE;
  }

  if (PathCombine(filepath, tgtFolderPath, INIFILENAME) == NULL)
    return FALSE;

  return TRUE;
}

// set config value to ini file
void writeConfigToIniFile(void)
{
  TCHAR filepath[MAX_PATH];
  TCHAR buf[512];

  if (!getIniFilePath(filepath))
    return;

  // create and write ini file
  wsprintf(buf, _T("%d"), waitValue);
  WritePrivateProfileString(SECNAME, _T("wait"), buf, filepath);

  wsprintf(buf, _T("%d"), speedValue);
  WritePrivateProfileString(SECNAME, _T("speed"), buf, filepath);

  wsprintf(buf, _T("%d"), numberValue);
  WritePrivateProfileString(SECNAME, _T("number"), buf, filepath);

  wsprintf(buf, _T("%d"), fps_display);
  WritePrivateProfileString(SECNAME, _T("fps_display"), buf, filepath);
}

void setConfigValueToGlobalWork(void)
{
  waitValue = 15;
  speedValue = 1000;
  numberValue = 1000;
  fps_display = 1;
}

// get config value from ini file
BOOL getConfigFromIniFile(void)
{
  TCHAR filepath[MAX_PATH];
  TCHAR cdir[MAX_PATH];
  TCHAR buf[1024];

  if (!getIniFilePath(filepath))
  {
    // not get ini filepath
    setConfigValueToGlobalWork();
    return FALSE;
  }

  if (!PathFileExists(filepath))
  {
    // not found ini file. create.
    setConfigValueToGlobalWork();
    writeConfigToIniFile();
  }

  if (!PathFileExists(filepath))
    return FALSE;

  // read ini file
  waitValue = GetPrivateProfileInt(SECNAME, _T("wait"), -1, filepath);
  speedValue = GetPrivateProfileInt(SECNAME, _T("speed"), -1, filepath);
  numberValue = GetPrivateProfileInt(SECNAME, _T("number"), -1, filepath);
  fps_display = GetPrivateProfileInt(SECNAME, _T("fps_display"), -1, filepath);

  return TRUE;
}

// clamp value
int clamp(int v, int minv, int maxv)
{
  if (v < minv)
    return minv;
  if (v > maxv)
    return maxv;
  return v;
}

// set value to config dialog
void setValueOnDialog(HWND hDlg, int waitValue, int speedValue, int numberValue, int fps_display)
{
  // set EDITTEXT
  {
    TCHAR buf[512];

    wsprintf(buf, _T("%d"), waitValue);
    SetDlgItemText(hDlg, IDC_WAITVALUE, buf);

    wsprintf(buf, _T("%d"), speedValue);
    SetDlgItemText(hDlg, IDC_SPEED, buf);

    wsprintf(buf, _T("%d"), numberValue);
    SetDlgItemText(hDlg, IDC_NUMBER, buf);
  }

  // set CHECKBOX (fps display)
  {
    HWND aCheck;
    aCheck = GetDlgItem(hDlg, IDC_FPSDISPLAY);
    SendMessage(aCheck, BM_SETCHECK, ((fps_display) ? BST_CHECKED : BST_UNCHECKED), 0);
  }
}

// get value from config dialog
void getValueFromDialog(HWND hDlg)
{
  // get EDITTEXT
  {
    TCHAR buf[2048];
    int n;

    // get wait value
    GetDlgItemText(hDlg, IDC_WAITVALUE, buf, sizeof(buf) / sizeof(TCHAR));
    n = StrToInt(buf);
    waitValue = clamp(n, 5, 200);

    // get speed value
    GetDlgItemText(hDlg, IDC_SPEED, buf, sizeof(buf) / sizeof(TCHAR));
    n = StrToInt(buf);
    speedValue = clamp(n, 100, 4000);

    // get number value
    GetDlgItemText(hDlg, IDC_NUMBER, buf, sizeof(buf) / sizeof(TCHAR));
    n = StrToInt(buf);
    numberValue = clamp(n, 10, OBJ_MAX);
  }

  // get CHECKBOX (fps display)
  fps_display = (IsDlgButtonChecked(hDlg, IDC_FPSDISPLAY) == BST_CHECKED) ? 1 : 0;
}
