//========================================================================
// GLFW - An OpenGL library
// Platform:    Win32/WGL
// API version: 3.0
// WWW:         http://www.glfw.org/
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <stdlib.h>
#include <malloc.h>
#include <windowsx.h>


// Updates the cursor clip rect
//
static void updateClipRect(_GLFWwindow* window)
{
    RECT clipRect;
    GetClientRect(window->win32.handle, &clipRect);
    ClientToScreen(window->win32.handle, (POINT*) &clipRect.left);
    ClientToScreen(window->win32.handle, (POINT*) &clipRect.right);
    ClipCursor(&clipRect);
}

// Hide mouse cursor
//
static void hideCursor(_GLFWwindow* window)
{
    UNREFERENCED_PARAMETER(window);
}

// Capture mouse cursor
//
static void captureCursor(_GLFWwindow* window)
{
    ShowCursor(FALSE);
    updateClipRect(window);
    SetCapture(window->win32.handle);
}

// Show mouse cursor
//
static void showCursor(_GLFWwindow* window)
{
    UNREFERENCED_PARAMETER(window);

    ReleaseCapture();
    ClipCursor(NULL);
    ShowCursor(TRUE);
}

// Translates a Windows key to the corresponding GLFW key
//
static int translateKey(WPARAM wParam, LPARAM lParam)
{
    MSG next_msg;
    DWORD msg_time;
    DWORD scan_code;

    // Check for numeric keypad keys
    // NOTE: This way we always force "NumLock = ON", which is intentional since
    // the returned key code should correspond to a physical location.
    int hiFlags = HIWORD(lParam);
    if (!(hiFlags & 0x100))
    {
        switch (MapVirtualKey(hiFlags & 0xFF, 1))
        {
            case VK_INSERT:   return GLFW_KEY_KP_0;
            case VK_END:      return GLFW_KEY_KP_1;
            case VK_DOWN:     return GLFW_KEY_KP_2;
            case VK_NEXT:     return GLFW_KEY_KP_3;
            case VK_LEFT:     return GLFW_KEY_KP_4;
            case VK_CLEAR:    return GLFW_KEY_KP_5;
            case VK_RIGHT:    return GLFW_KEY_KP_6;
            case VK_HOME:     return GLFW_KEY_KP_7;
            case VK_UP:       return GLFW_KEY_KP_8;
            case VK_PRIOR:    return GLFW_KEY_KP_9;
            case VK_DIVIDE:   return GLFW_KEY_KP_DIVIDE;
            case VK_MULTIPLY: return GLFW_KEY_KP_MULTIPLY;
            case VK_SUBTRACT: return GLFW_KEY_KP_SUBTRACT;
            case VK_ADD:      return GLFW_KEY_KP_ADD;
            case VK_DELETE:   return GLFW_KEY_KP_DECIMAL;
            default:          break;
        }
    }

    // Check which key was pressed or released
    switch (wParam)
    {
        // The SHIFT keys require special handling
        case VK_SHIFT:
        {
            // Compare scan code for this key with that of VK_RSHIFT in
            // order to determine which shift key was pressed (left or
            // right)
            scan_code = MapVirtualKey(VK_RSHIFT, 0);
            if (((lParam & 0x01ff0000) >> 16) == scan_code)
                return GLFW_KEY_RIGHT_SHIFT;

            return GLFW_KEY_LEFT_SHIFT;
        }

        // The CTRL keys require special handling
        case VK_CONTROL:
        {
            // Is this an extended key (i.e. right key)?
            if (lParam & 0x01000000)
                return GLFW_KEY_RIGHT_CONTROL;

            // Here is a trick: "Alt Gr" sends LCTRL, then RALT. We only
            // want the RALT message, so we try to see if the next message
            // is a RALT message. In that case, this is a false LCTRL!
            msg_time = GetMessageTime();
            if (PeekMessage(&next_msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (next_msg.message == WM_KEYDOWN ||
                    next_msg.message == WM_SYSKEYDOWN)
                {
                    if (next_msg.wParam == VK_MENU &&
                        (next_msg.lParam & 0x01000000) &&
                        next_msg.time == msg_time)
                    {
                        // Next message is a RALT down message, which
                        // means that this is NOT a proper LCTRL message!
                        return -1;
                    }
                }
            }

            return GLFW_KEY_LEFT_CONTROL;
        }

        // The ALT keys require special handling
        case VK_MENU:
        {
            // Is this an extended key (i.e. right key)?
            if (lParam & 0x01000000)
                return GLFW_KEY_RIGHT_ALT;

            return GLFW_KEY_LEFT_ALT;
        }

        // The ENTER keys require special handling
        case VK_RETURN:
        {
            // Is this an extended key (i.e. right key)?
            if (lParam & 0x01000000)
                return GLFW_KEY_KP_ENTER;

            return GLFW_KEY_ENTER;
        }

        // Funcion keys (non-printable keys)
        case VK_ESCAPE:        return GLFW_KEY_ESCAPE;
        case VK_TAB:           return GLFW_KEY_TAB;
        case VK_BACK:          return GLFW_KEY_BACKSPACE;
        case VK_HOME:          return GLFW_KEY_HOME;
        case VK_END:           return GLFW_KEY_END;
        case VK_PRIOR:         return GLFW_KEY_PAGE_UP;
        case VK_NEXT:          return GLFW_KEY_PAGE_DOWN;
        case VK_INSERT:        return GLFW_KEY_INSERT;
        case VK_DELETE:        return GLFW_KEY_DELETE;
        case VK_LEFT:          return GLFW_KEY_LEFT;
        case VK_UP:            return GLFW_KEY_UP;
        case VK_RIGHT:         return GLFW_KEY_RIGHT;
        case VK_DOWN:          return GLFW_KEY_DOWN;
        case VK_F1:            return GLFW_KEY_F1;
        case VK_F2:            return GLFW_KEY_F2;
        case VK_F3:            return GLFW_KEY_F3;
        case VK_F4:            return GLFW_KEY_F4;
        case VK_F5:            return GLFW_KEY_F5;
        case VK_F6:            return GLFW_KEY_F6;
        case VK_F7:            return GLFW_KEY_F7;
        case VK_F8:            return GLFW_KEY_F8;
        case VK_F9:            return GLFW_KEY_F9;
        case VK_F10:           return GLFW_KEY_F10;
        case VK_F11:           return GLFW_KEY_F11;
        case VK_F12:           return GLFW_KEY_F12;
        case VK_F13:           return GLFW_KEY_F13;
        case VK_F14:           return GLFW_KEY_F14;
        case VK_F15:           return GLFW_KEY_F15;
        case VK_F16:           return GLFW_KEY_F16;
        case VK_F17:           return GLFW_KEY_F17;
        case VK_F18:           return GLFW_KEY_F18;
        case VK_F19:           return GLFW_KEY_F19;
        case VK_F20:           return GLFW_KEY_F20;
        case VK_F21:           return GLFW_KEY_F21;
        case VK_F22:           return GLFW_KEY_F22;
        case VK_F23:           return GLFW_KEY_F23;
        case VK_F24:           return GLFW_KEY_F24;
        case VK_NUMLOCK:       return GLFW_KEY_NUM_LOCK;
        case VK_CAPITAL:       return GLFW_KEY_CAPS_LOCK;
        case VK_SCROLL:        return GLFW_KEY_SCROLL_LOCK;
        case VK_PAUSE:         return GLFW_KEY_PAUSE;
        case VK_LWIN:          return GLFW_KEY_LEFT_SUPER;
        case VK_RWIN:          return GLFW_KEY_RIGHT_SUPER;
        case VK_APPS:          return GLFW_KEY_MENU;

        // Numeric keypad
        case VK_NUMPAD0:       return GLFW_KEY_KP_0;
        case VK_NUMPAD1:       return GLFW_KEY_KP_1;
        case VK_NUMPAD2:       return GLFW_KEY_KP_2;
        case VK_NUMPAD3:       return GLFW_KEY_KP_3;
        case VK_NUMPAD4:       return GLFW_KEY_KP_4;
        case VK_NUMPAD5:       return GLFW_KEY_KP_5;
        case VK_NUMPAD6:       return GLFW_KEY_KP_6;
        case VK_NUMPAD7:       return GLFW_KEY_KP_7;
        case VK_NUMPAD8:       return GLFW_KEY_KP_8;
        case VK_NUMPAD9:       return GLFW_KEY_KP_9;
        case VK_DIVIDE:        return GLFW_KEY_KP_DIVIDE;
        case VK_MULTIPLY:      return GLFW_KEY_KP_MULTIPLY;
        case VK_SUBTRACT:      return GLFW_KEY_KP_SUBTRACT;
        case VK_ADD:           return GLFW_KEY_KP_ADD;
        case VK_DECIMAL:       return GLFW_KEY_KP_DECIMAL;

        // Printable keys are mapped according to US layout
        case VK_SPACE:         return GLFW_KEY_SPACE;
        case 0x30:             return GLFW_KEY_0;
        case 0x31:             return GLFW_KEY_1;
        case 0x32:             return GLFW_KEY_2;
        case 0x33:             return GLFW_KEY_3;
        case 0x34:             return GLFW_KEY_4;
        case 0x35:             return GLFW_KEY_5;
        case 0x36:             return GLFW_KEY_6;
        case 0x37:             return GLFW_KEY_7;
        case 0x38:             return GLFW_KEY_8;
        case 0x39:             return GLFW_KEY_9;
        case 0x41:             return GLFW_KEY_A;
        case 0x42:             return GLFW_KEY_B;
        case 0x43:             return GLFW_KEY_C;
        case 0x44:             return GLFW_KEY_D;
        case 0x45:             return GLFW_KEY_E;
        case 0x46:             return GLFW_KEY_F;
        case 0x47:             return GLFW_KEY_G;
        case 0x48:             return GLFW_KEY_H;
        case 0x49:             return GLFW_KEY_I;
        case 0x4A:             return GLFW_KEY_J;
        case 0x4B:             return GLFW_KEY_K;
        case 0x4C:             return GLFW_KEY_L;
        case 0x4D:             return GLFW_KEY_M;
        case 0x4E:             return GLFW_KEY_N;
        case 0x4F:             return GLFW_KEY_O;
        case 0x50:             return GLFW_KEY_P;
        case 0x51:             return GLFW_KEY_Q;
        case 0x52:             return GLFW_KEY_R;
        case 0x53:             return GLFW_KEY_S;
        case 0x54:             return GLFW_KEY_T;
        case 0x55:             return GLFW_KEY_U;
        case 0x56:             return GLFW_KEY_V;
        case 0x57:             return GLFW_KEY_W;
        case 0x58:             return GLFW_KEY_X;
        case 0x59:             return GLFW_KEY_Y;
        case 0x5A:             return GLFW_KEY_Z;
        case 0xBD:             return GLFW_KEY_MINUS;
        case 0xBB:             return GLFW_KEY_EQUAL;
        case 0xDB:             return GLFW_KEY_LEFT_BRACKET;
        case 0xDD:             return GLFW_KEY_RIGHT_BRACKET;
        case 0xDC:             return GLFW_KEY_BACKSLASH;
        case 0xBA:             return GLFW_KEY_SEMICOLON;
        case 0xDE:             return GLFW_KEY_APOSTROPHE;
        case 0xC0:             return GLFW_KEY_GRAVE_ACCENT;
        case 0xBC:             return GLFW_KEY_COMMA;
        case 0xBE:             return GLFW_KEY_PERIOD;
        case 0xBF:             return GLFW_KEY_SLASH;
        case 0xDF:             return GLFW_KEY_WORLD_1;
        case 0xE2:             return GLFW_KEY_WORLD_2;
        default:               break;
    }

    // No matching translation was found, so return -1
    return -1;
}

// Window callback function (handles window events)
//
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    _GLFWwindow* window = (_GLFWwindow*) GetWindowLongPtr(hWnd, 0);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
            SetWindowLongPtr(hWnd, 0, (LONG_PTR) cs->lpCreateParams);
            break;
        }

        case WM_ACTIVATE:
        {
            // Window was (de)focused and/or (de)iconified

            BOOL focused = LOWORD(wParam) != WA_INACTIVE;
            BOOL iconified = HIWORD(wParam) ? TRUE : FALSE;

            if (focused && iconified)
            {
                // This is a workaround for window iconification using the
                // taskbar leading to windows being told they're focused and
                // iconified and then never told they're defocused
                focused = FALSE;
            }

            if (!focused && _glfw.focusedWindow == window)
            {
                // The window was defocused (or iconified, see above)

                if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                    showCursor(window);

                if (window->monitor)
                {
                    if (!iconified)
                    {
                        // Iconify the (on top, borderless, oddly positioned)
                        // window or the user will be annoyed
                        _glfwPlatformIconifyWindow(window);
                    }

                    _glfwRestoreVideoMode(window->monitor);
                }
            }
            else if (focused && _glfw.focusedWindow != window)
            {
                // The window was focused

                if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                    captureCursor(window);

                if (window->monitor)
                    _glfwSetVideoMode(window->monitor, &window->videoMode);
            }

            _glfwInputWindowFocus(window, focused);
            _glfwInputWindowIconify(window, iconified);
            return 0;
        }

        case WM_SHOWWINDOW:
        {
            _glfwInputWindowVisibility(window, wParam ? GL_TRUE : GL_FALSE);
            break;
        }

        case WM_SYSCOMMAND:
        {
            switch (wParam & 0xfff0)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                {
                    if (window->monitor)
                    {
                        // We are running in fullscreen mode, so disallow
                        // screen saver and screen blanking
                        return 0;
                    }
                    else
                        break;
                }

                // User trying to access application menu using ALT?
                case SC_KEYMENU:
                    return 0;
            }
            break;
        }

        case WM_CLOSE:
        {
            _glfwInputWindowCloseRequest(window);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            _glfwInputKey(window, translateKey(wParam, lParam), GLFW_PRESS);
            break;
        }

        case WM_CHAR:
        {
            _glfwInputChar(window, wParam);
            return 0;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            // Special trick: release both shift keys on SHIFT up event
            if (wParam == VK_SHIFT)
            {
                _glfwInputKey(window, GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE);
                _glfwInputKey(window, GLFW_KEY_RIGHT_SHIFT, GLFW_RELEASE);
            }
            else
                _glfwInputKey(window, translateKey(wParam, lParam), GLFW_RELEASE);

            break;
        }

        case WM_LBUTTONDOWN:
        {
            SetCapture(hWnd);
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
            return 0;
        }

        case WM_RBUTTONDOWN:
        {
            SetCapture(hWnd);
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
            return 0;
        }

        case WM_MBUTTONDOWN:
        {
            SetCapture(hWnd);
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS);
            return 0;
        }

        case WM_XBUTTONDOWN:
        {
            if (HIWORD(wParam) == XBUTTON1)
            {
                SetCapture(hWnd);
                _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_4, GLFW_PRESS);
            }
            else if (HIWORD(wParam) == XBUTTON2)
            {
                SetCapture(hWnd);
                _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_5, GLFW_PRESS);
            }

            return 1;
        }

        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
            return 0;
        }

        case WM_RBUTTONUP:
        {
            ReleaseCapture();
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
            return 0;
        }

        case WM_MBUTTONUP:
        {
            ReleaseCapture();
            _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_RELEASE);
            return 0;
        }

        case WM_XBUTTONUP:
        {
            if (HIWORD(wParam) == XBUTTON1)
            {
                ReleaseCapture();
                _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_4, GLFW_RELEASE);
            }
            else if (HIWORD(wParam) == XBUTTON2)
            {
                ReleaseCapture();
                _glfwInputMouseClick(window, GLFW_MOUSE_BUTTON_5, GLFW_RELEASE);
            }

            return 1;
        }

        case WM_MOUSEMOVE:
        {
            const int newCursorX = GET_X_LPARAM(lParam);
            const int newCursorY = GET_Y_LPARAM(lParam);

            if (newCursorX != window->win32.oldCursorX ||
                newCursorY != window->win32.oldCursorY)
            {
                int x, y;

                if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                {
                    if (_glfw.focusedWindow != window)
                        return 0;

                    x = newCursorX - window->win32.oldCursorX;
                    y = newCursorY - window->win32.oldCursorY;
                }
                else
                {
                    x = newCursorX;
                    y = newCursorY;
                }

                window->win32.oldCursorX = newCursorX;
                window->win32.oldCursorY = newCursorY;
                window->win32.cursorCentered = GL_FALSE;

                _glfwInputCursorMotion(window, x, y);
            }

            if (!window->win32.cursorInside)
            {
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(tme));
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = window->win32.handle;
                TrackMouseEvent(&tme);

                window->win32.cursorInside = GL_TRUE;
                _glfwInputCursorEnter(window, GL_TRUE);
            }

            return 0;
        }

        case WM_MOUSELEAVE:
        {
            window->win32.cursorInside = GL_FALSE;
            _glfwInputCursorEnter(window, GL_FALSE);
            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            _glfwInputScroll(window, 0.0, (SHORT) HIWORD(wParam) / (double) WHEEL_DELTA);
            return 0;
        }

        case WM_MOUSEHWHEEL:
        {
            // This message is only sent on Windows Vista and later
            _glfwInputScroll(window, (SHORT) HIWORD(wParam) / (double) WHEEL_DELTA, 0.0);
            return 0;
        }

        case WM_SIZE:
        {
            if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                updateClipRect(window);

            _glfwInputWindowSize(window, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_MOVE:
        {
            if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                updateClipRect(window);

            _glfwInputWindowPos(window, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_PAINT:
        {
            _glfwInputWindowDamage(window);
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (DBT_DEVNODES_CHANGED == wParam)
            {
                _glfwInputMonitorChange();
                return TRUE;
            }
            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Translate client window size to full window size (including window borders)
//
static void getFullWindowSize(_GLFWwindow* window,
                              int clientWidth, int clientHeight,
                              int* fullWidth, int* fullHeight)
{
    RECT rect = { 0, 0, clientWidth, clientHeight };

    // Adjust according to window styles
    AdjustWindowRectEx(&rect, window->win32.dwStyle, FALSE, window->win32.dwExStyle);

    // Calculate width and height of full window
    *fullWidth = rect.right - rect.left;
    *fullHeight = rect.bottom - rect.top;
}

// Registers the GLFW window class
//
static ATOM registerWindowClass(void)
{
    WNDCLASS wc;
    ATOM classAtom;

    // Set window class parameters
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Redraw on...
    wc.lpfnWndProc   = (WNDPROC) windowProc;          // Message handler
    wc.cbClsExtra    = 0;                             // No extra class data
    wc.cbWndExtra    = sizeof(void*) + sizeof(int);   // Make room for one pointer
    wc.hInstance     = GetModuleHandle(NULL);         // Set instance
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);   // Load arrow pointer
    wc.hbrBackground = NULL;                          // No background
    wc.lpszMenuName  = NULL;                          // No menu
    wc.lpszClassName = _GLFW_WNDCLASSNAME;            // Set class name

    // Load user-provided icon if available
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), L"GLFW_ICON");
    if (!wc.hIcon)
    {
        // User-provided icon not found; load default icon
        wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    }

    classAtom = RegisterClass(&wc);
    if (!classAtom)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Win32: Failed to register window class");
        return 0;
    }

    return classAtom;
}

// Creates the GLFW window and rendering context
//
static int createWindow(_GLFWwindow* window,
                        const _GLFWwndconfig* wndconfig,
                        const _GLFWfbconfig* fbconfig)
{
    int xpos, ypos, fullWidth, fullHeight;
    POINT cursorPos;
    WCHAR* wideTitle;

    window->win32.dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    window->win32.dwExStyle = WS_EX_APPWINDOW;

    if (window->monitor)
    {
        window->win32.dwStyle |= WS_POPUP;

        _glfwPlatformGetMonitorPos(wndconfig->monitor, &xpos, &ypos);

        fullWidth  = wndconfig->width;
        fullHeight = wndconfig->height;
    }
    else
    {
        window->win32.dwStyle |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        if (wndconfig->resizable)
        {
            window->win32.dwStyle |= WS_MAXIMIZEBOX | WS_SIZEBOX;
            window->win32.dwExStyle |= WS_EX_WINDOWEDGE;
        }

        xpos = CW_USEDEFAULT;
        ypos = CW_USEDEFAULT;

        getFullWindowSize(window,
                        wndconfig->width, wndconfig->height,
                        &fullWidth, &fullHeight);
    }

    wideTitle = _glfwCreateWideStringFromUTF8(wndconfig->title);
    if (!wideTitle)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Win32: Failed to convert title to wide string");
        return GL_FALSE;
    }

    window->win32.handle = CreateWindowEx(window->win32.dwExStyle,
                                          _GLFW_WNDCLASSNAME,
                                          wideTitle,
                                          window->win32.dwStyle,
                                          xpos, ypos,
                                          fullWidth, fullHeight,
                                          NULL, // No parent window
                                          NULL, // No window menu
                                          GetModuleHandle(NULL),
                                          window); // Pass GLFW window to WM_CREATE

    free(wideTitle);

    if (!window->win32.handle)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Failed to create window");
        return GL_FALSE;
    }

    // Initialize cursor position data
    GetCursorPos(&cursorPos);
    ScreenToClient(window->win32.handle, &cursorPos);
    window->win32.oldCursorX = window->cursorPosX = cursorPos.x;
    window->win32.oldCursorY = window->cursorPosY = cursorPos.y;

    if (!_glfwCreateContext(window, wndconfig, fbconfig))
        return GL_FALSE;

    return GL_TRUE;
}

// Destroys the GLFW window and rendering context
//
static void destroyWindow(_GLFWwindow* window)
{
    _glfwDestroyContext(window);

    if (window->win32.handle)
    {
        DestroyWindow(window->win32.handle);
        window->win32.handle = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformCreateWindow(_GLFWwindow* window,
                              const _GLFWwndconfig* wndconfig,
                              const _GLFWfbconfig* fbconfig)
{
    int status;

    if (!_glfw.win32.classAtom)
    {
        _glfw.win32.classAtom = registerWindowClass();
        if (!_glfw.win32.classAtom)
            return GL_FALSE;
    }

    if (window->monitor)
    {
        if (!_glfwSetVideoMode(window->monitor, &window->videoMode))
            return GL_FALSE;
    }

    if (!createWindow(window, wndconfig, fbconfig))
        return GL_FALSE;

    status = _glfwAnalyzeContext(window, wndconfig, fbconfig);

    if (status == _GLFW_RECREATION_IMPOSSIBLE)
        return GL_FALSE;

    if (status == _GLFW_RECREATION_REQUIRED)
    {
        // Some window hints require us to re-create the context using WGL
        // extensions retrieved through the current context, as we cannot check
        // for WGL extensions or retrieve WGL entry points before we have a
        // current context (actually until we have implicitly loaded the ICD)

        // Yes, this is strange, and yes, this is the proper way on Win32

        // As Windows only allows you to set the pixel format once for a
        // window, we need to destroy the current window and create a new one
        // to be able to use the new pixel format

        // Technically, it may be possible to keep the old window around if
        // we're just creating an OpenGL 3.0+ context with the same pixel
        // format, but it's not worth the added code complexity

        // First we clear the current context (the one we just created)
        // This is usually done by glfwDestroyWindow, but as we're not doing
        // full window destruction, it's duplicated here
        _glfwPlatformMakeContextCurrent(NULL);

        // Next destroy the Win32 window and WGL context (without resetting or
        // destroying the GLFW window object)
        destroyWindow(window);

        // ...and then create them again, this time with better APIs
        if (!createWindow(window, wndconfig, fbconfig))
            return GL_FALSE;
    }

    if (window->monitor)
    {
        // Place the window above all topmost windows
        _glfwPlatformShowWindow(window);
        SetWindowPos(window->win32.handle, HWND_TOPMOST, 0,0,0,0,
                     SWP_NOMOVE | SWP_NOSIZE);
    }

    return GL_TRUE;
}

void _glfwPlatformDestroyWindow(_GLFWwindow* window)
{
    destroyWindow(window);

    if (window->monitor)
        _glfwRestoreVideoMode(window->monitor);
}

void _glfwPlatformSetWindowTitle(_GLFWwindow* window, const char* title)
{
    WCHAR* wideTitle = _glfwCreateWideStringFromUTF8(title);
    if (!wideTitle)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Win32: Failed to convert title to wide string");
        return;
    }

    SetWindowText(window->win32.handle, wideTitle);

    free(wideTitle);
}

void _glfwPlatformGetWindowPos(_GLFWwindow* window, int* xpos, int* ypos)
{
    POINT pos = { 0, 0 };
    ClientToScreen(window->win32.handle, &pos);

    if (xpos)
        *xpos = pos.x;
    if (ypos)
        *ypos = pos.y;
}

void _glfwPlatformSetWindowPos(_GLFWwindow* window, int xpos, int ypos)
{
    RECT rect = { xpos, ypos, xpos, ypos };
    AdjustWindowRectEx(&rect, window->win32.dwStyle,
                       FALSE, window->win32.dwExStyle);
    SetWindowPos(window->win32.handle, NULL, rect.left, rect.top, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void _glfwPlatformGetWindowSize(_GLFWwindow* window, int* width, int* height)
{
    RECT area;
    GetClientRect(window->win32.handle, &area);

    if (width)
        *width = area.right;
    if (height)
        *height = area.bottom;
}

void _glfwPlatformSetWindowSize(_GLFWwindow* window, int width, int height)
{
    GLboolean sizeChanged = GL_FALSE;

    if (window->monitor)
    {
        GLFWvidmode mode;
        _glfwPlatformGetVideoMode(window->monitor, &mode);

        if (width > mode.width || height > mode.height)
        {
            // The new video mode is larger than the current one, so we resize
            // the window before switch modes to avoid exposing whatever is
            // underneath

            SetWindowPos(window->win32.handle, HWND_TOP, 0, 0, width, height,
                         SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
            sizeChanged = GL_TRUE;
        }

        // TODO: Change video mode
    }
    else
    {
        // If we are in windowed mode, adjust the window size to
        // compensate for window decorations
        getFullWindowSize(window, width, height, &width, &height);
    }

    // Set window size (if we haven't already)
    if (!sizeChanged)
    {
        SetWindowPos(window->win32.handle, HWND_TOP, 0, 0, width, height,
                     SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    }
}



//========================================================================
// Creates a Windows icon from a GLFWimage
//========================================================================

static HICON createIcon(GLFWimage* image)
{
    BITMAPV5HEADER header;
    HDC hdc;
    unsigned char* BGRAData;
    HBITMAP bitmap, mask;
    ICONINFO iconinfo;
    HICON icon;
    unsigned char* dibData;
    int i;
    
    // fill in BITMAPV5HEADER to pass to CreateDIBSection
    header.bV5Size = sizeof(header);
    header.bV5Width = image->width;
    header.bV5Height = image->height;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask =   0x00ff0000;
    header.bV5GreenMask = 0x0000ff00;
    header.bV5BlueMask =  0x000000ff;
    header.bV5AlphaMask = 0xff000000;
    
    // create a HBITMAP that we can write to
    hdc = GetDC(NULL);
    bitmap = CreateDIBSection(hdc, (BITMAPINFO*) &header, DIB_RGB_COLORS, (void**) &dibData, NULL, 0);
    ReleaseDC(NULL, hdc);
    
    // first we need to convert RGBA to BGRA (yay Windows!)
    // we also need to convert lines, because Windows wants bottom-to-top RGBA
    BGRAData = malloc(image->width * image->height * 4);
    if (!BGRAData)
    {
        _glfwInputError(GLFW_OUT_OF_MEMORY, NULL);
        return NULL;
    }

    for (i = 0; i < image->width * image->height; i++) {
        unsigned char *dst = BGRAData + 4 * i;
        unsigned char *src = image->data + 4 * i;

        dst[0] = src[2]; // copy blue channel
        dst[1] = src[1]; // copy green channel
        dst[2] = src[0]; // copy red channel
        dst[3] = src[3]; // copy alpha channel
    }
    
    // copy the BGRA data into dibData
    memcpy(dibData, BGRAData, image->width * image->height * 4);
    
    // free the BGRA data
    free(BGRAData);
    
    // create a mask that we don't use (but needed for iconinfo)
    mask = CreateBitmap(image->width, image->height, 1, 1, NULL);
    
    iconinfo.fIcon = TRUE;
    iconinfo.hbmMask = mask;
    iconinfo.hbmColor = bitmap;
    
    // create our icon
    icon = CreateIconIndirect(&iconinfo);
    
    // clean up
    DeleteObject(mask);
    DeleteObject(bitmap);
    
    return icon;
}

//========================================================================
// Chooses the best fitting image given the images and desired size
//========================================================================

static GLFWimage* bestFit(GLFWimage *icons, const int numicons, const int targetWidth, const int targetHeight)
{
    GLFWimage *curIcon = icons;
    GLFWimage *bestIcon = curIcon;
    const double targetRatio = (double) targetWidth / targetHeight;

    while (curIcon < icons + numicons)
    {
        // always use exact match
        if (curIcon->width == targetWidth && curIcon->height == targetHeight)
        {
            return curIcon;
        }
        
        // at least wide or high enough, ratio preferably as close as possible
        if (curIcon->width >= targetWidth || curIcon->height >= targetHeight)
        {
            const double curRatio = (double) curIcon->width / curIcon->height;
            const double bestRatio = (double) bestIcon->width / bestIcon->height;
            double curDelta = targetRatio - curRatio;
            double bestDelta = targetRatio - bestRatio;

            if (curDelta < 0)
            {
                curDelta = -curDelta;
            }

            if (bestDelta < 0)
            {
                bestDelta = -bestDelta;
            }

            // if our ratio is closer OR if the best icon so far isn't large
            // enough we'll become the new best icon
            if ((curDelta < bestDelta)
                || (bestIcon->width < targetWidth && bestIcon->height < targetHeight))
            {
                bestIcon = curIcon;
            }
        }

        // maybe nothing is wide or high enough, if that's the case
        // we'll get the largest thing available (in area)
        else if (bestIcon->width < targetWidth && bestIcon->height < targetHeight)
        {
            if (curIcon->width * curIcon->height > bestIcon->width * bestIcon->height)
            {
                bestIcon = curIcon;
            }
        }

        ++curIcon;
    }

    return bestIcon;
}

//========================================================================
// Set the window icon(s)
//========================================================================

void _glfwPlatformSetWindowIcons(_GLFWwindow* window, GLFWimage *icons, int numicons)
{
    GLFWimage* normalicon;
    GLFWimage* smallicon;

    normalicon = bestFit(icons, numicons, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    smallicon = bestFit(icons, numicons, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

    SendMessage(window->win32.handle, WM_SETICON, ICON_BIG, (LPARAM) createIcon(normalicon));
    SendMessage(window->win32.handle, WM_SETICON, ICON_SMALL, (LPARAM) createIcon(smallicon));
}

void _glfwPlatformIconifyWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_MINIMIZE);
}

void _glfwPlatformRestoreWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_RESTORE);
}

void _glfwPlatformShowWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_SHOWNORMAL);
    BringWindowToTop(window->win32.handle);
    SetForegroundWindow(window->win32.handle);
    SetFocus(window->win32.handle);
}

void _glfwPlatformHideWindow(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_HIDE);
}

void _glfwPlatformPollEvents(void)
{
    MSG msg;
    _GLFWwindow* window;

    window = _glfw.focusedWindow;
    if (window)
    {
        int width, height;
        _glfwPlatformGetWindowSize(window, &width, &height);
        window->win32.cursorCentered = GL_FALSE;
        window->win32.oldCursorX = width / 2;
        window->win32.oldCursorY = height / 2;
    }

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // Treat WM_QUIT as a close on all windows

            window = _glfw.windowListHead;
            while (window)
            {
                _glfwInputWindowCloseRequest(window);
                window = window->next;
            }
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    window = _glfw.focusedWindow;
    if (window)
    {
        // LSHIFT/RSHIFT fixup (keys tend to "stick" without this fix)
        // This is the only async event handling in GLFW, but it solves some
        // nasty problems
        {
            int lshift_down, rshift_down;

            // Get current state of left and right shift keys
            lshift_down = (GetAsyncKeyState(VK_LSHIFT) >> 15) & 1;
            rshift_down = (GetAsyncKeyState(VK_RSHIFT) >> 15) & 1;

            // See if this differs from our belief of what has happened
            // (we only have to check for lost key up events)
            if (!lshift_down && window->key[GLFW_KEY_LEFT_SHIFT] == 1)
                _glfwInputKey(window, GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE);

            if (!rshift_down && window->key[GLFW_KEY_RIGHT_SHIFT] == 1)
                _glfwInputKey(window, GLFW_KEY_RIGHT_SHIFT, GLFW_RELEASE);
        }

        // Did the cursor move in an focused window that has captured the cursor
        if (window->cursorMode == GLFW_CURSOR_CAPTURED &&
            !window->win32.cursorCentered)
        {
            int width, height;
            _glfwPlatformGetWindowSize(window, &width, &height);
            _glfwPlatformSetCursorPos(window, width / 2, height / 2);
            window->win32.cursorCentered = GL_TRUE;
        }
    }
}

void _glfwPlatformWaitEvents(void)
{
    WaitMessage();

    _glfwPlatformPollEvents();
}

void _glfwPlatformSetCursorPos(_GLFWwindow* window, int xpos, int ypos)
{
    POINT pos = { xpos, ypos };
    ClientToScreen(window->win32.handle, &pos);
    SetCursorPos(pos.x, pos.y);
}

void _glfwPlatformSetCursorMode(_GLFWwindow* window, int mode)
{
    switch (mode)
    {
        case GLFW_CURSOR_NORMAL:
            showCursor(window);
            break;
        case GLFW_CURSOR_HIDDEN:
            hideCursor(window);
            break;
        case GLFW_CURSOR_CAPTURED:
            captureCursor(window);
            break;
    }
}

//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI HWND glfwGetWin32Window(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return window->win32.handle;
}

