#pragma once

#include <windows.h>

// Returns the focused window. Falls back to the shell window when there is no foreground window.
HWND getFocusedWindow();
