#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>

// Use a vector for more flexibility with window collection
std::vector<HWND> g_windows;

// Enumeration for layout modes
enum TilingLayout {
  LAYOUT_GRID,         // Traditional grid layout
  LAYOUT_HORIZONTAL,   // Windows side by side
  LAYOUT_VERTICAL,     // Windows stacked vertically
  LAYOUT_MAIN_AND_DECK // One large window with others stacked
};

// Configuration settings for tiling
struct TilingConfig {
  int padding = 4;                   // Padding between windows
  bool skipTaskbar = true;           // Skip the taskbar when tiling
  TilingLayout layout = LAYOUT_GRID; // Default layout
  float mainRatio = 0.6f; // Ratio for main window in main+deck layout
  int borderSize = 2;     // Border size for windows
} g_config;

// Add this function declaration before it's used
void SetTileWindow(HWND hwnd, int x, int y, int width, int height) {
  if (!IsWindow(hwnd))
    return;

  // Restore if maximized
  if (IsZoomed(hwnd)) {
    ShowWindow(hwnd, SW_RESTORE);
  }

  // Remove maximize box
  LONG style = GetWindowLong(hwnd, GWL_STYLE);
  style &= ~WS_MAXIMIZEBOX;
  SetWindowLong(hwnd, GWL_STYLE, style);

  // Move and resize
  MoveWindow(hwnd, x, y, width, height, TRUE);

  // Ensure visible and on top
  ShowWindow(hwnd, SW_SHOW);
  BringWindowToTop(hwnd);
}

// Log function to debug shell hook events
void LogShellEvent(int code, WPARAM wParam, LPARAM lParam) {
  std::string eventName;

  switch (code) {
  case HSHELL_WINDOWCREATED:
    eventName = "HSHELL_WINDOWCREATED";
    break;
  case HSHELL_WINDOWDESTROYED:
    eventName = "HSHELL_WINDOWDESTROYED";
    break;
  case HSHELL_ACTIVATESHELLWINDOW:
    eventName = "HSHELL_ACTIVATESHELLWINDOW";
    break;
  case HSHELL_WINDOWACTIVATED:
    eventName = "HSHELL_WINDOWACTIVATED";
    break;
  case HSHELL_GETMINRECT:
    eventName = "HSHELL_GETMINRECT";
    break;
  case HSHELL_REDRAW:
    eventName = "HSHELL_REDRAW";
    break;
  case HSHELL_TASKMAN:
    eventName = "HSHELL_TASKMAN";
    break;
  case HSHELL_LANGUAGE:
    eventName = "HSHELL_LANGUAGE";
    break;
  case HSHELL_SYSMENU:
    eventName = "HSHELL_SYSMENU";
    break;
  case HSHELL_ENDTASK:
    eventName = "HSHELL_ENDTASK";
    break;
  case HSHELL_ACCESSIBILITYSTATE:
    eventName = "HSHELL_ACCESSIBILITYSTATE";
    break;
  case HSHELL_APPCOMMAND:
    eventName = "HSHELL_APPCOMMAND";
    break;
  case HSHELL_RUDEAPPACTIVATED:
    eventName = "HSHELL_RUDEAPPACTIVATED";
    break;
  case HSHELL_FLASH:
    eventName = "HSHELL_FLASH";
    break;
  default:
    eventName = "UNKNOWN";
  }

  char windowTitle[256] = {0};
  if (code == HSHELL_WINDOWCREATED || code == HSHELL_WINDOWDESTROYED ||
      code == HSHELL_WINDOWACTIVATED || code == HSHELL_RUDEAPPACTIVATED) {
    GetWindowTextA((HWND)wParam, windowTitle, sizeof(windowTitle));
  }

  printf("ShellProc event: %s (code=%d, wParam=0x%llX, window='%s')\n",
         eventName.c_str(), code, (unsigned long long)wParam, windowTitle);
  fflush(stdout);
}

// Check if a window should be included in tiling
bool ShouldTileWindow(HWND hwnd) {
  // Skip invisible or minimized windows
  if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
    return false;
  }

  char className[256] = {0};
  GetClassNameA(hwnd, className, sizeof(className));

  // Only include standard app windows
  LONG style = GetWindowLong(hwnd, GWL_STYLE);
  LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

  bool isPopup = (style & WS_POPUP) || (exStyle & WS_EX_TOOLWINDOW);
  bool isAppWindow = (style & WS_OVERLAPPEDWINDOW) || (style & WS_CAPTION);

  if (isPopup || !isAppWindow) {
    return false;
  }

  // Skip certain system windows
  const char *skipClasses[] = {
      "Shell_TrayWnd", "DV2ControlHost", "Windows.UI.Core.CoreWindow",
      "Progman",       "SysShadow",      "Button"};

  for (const char *cls : skipClasses) {
    if (strcmp(className, cls) == 0) {
      return false;
    }
  }

  return true;
}

// Add try-catch blocks around vector operations
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  try {
    if (ShouldTileWindow(hwnd)) {
      char title[256];
      GetWindowTextA(hwnd, title, sizeof(title));
      g_windows.push_back(hwnd);
      printf("  Found window: '%s'\n", title);
      fflush(stdout);
    }
  } catch (const std::exception &e) {
    printf("Error in EnumWindowsProc: %s\n", e.what());
    fflush(stdout);
  }
  return TRUE;
}

// Tile windows in a horizontal layout (windows side by side)
void TileWindowsHorizontal() {
  if (g_windows.empty()) {
    return;
  }

  // Get work area dimensions
  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

  int screenWidth = workArea.right - workArea.left;
  int screenHeight = workArea.bottom - workArea.top;
  int numWindows = (int)g_windows.size();

  int windowWidth = (screenWidth / numWindows) - (g_config.padding * 2);

  // Position each window side by side
  for (int i = 0; i < numWindows; i++) {
    int x = workArea.left + (i * (windowWidth + g_config.padding * 2)) +
            g_config.padding;
    int y = workArea.top + g_config.padding;

    HWND hwnd = g_windows[i];

    // Ensure the window is not maximized
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE) {
      ShowWindow(hwnd, SW_RESTORE);
    }

    SetWindowPos(hwnd, NULL, x, y, windowWidth,
                 screenHeight - (g_config.padding * 2),
                 SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

// Tile windows in a vertical layout (windows stacked)
void TileWindowsVertical() {
  if (g_windows.empty()) {
    return;
  }

  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

  int screenWidth = workArea.right - workArea.left;
  int screenHeight = workArea.bottom - workArea.top;
  int numWindows = (int)g_windows.size();

  int windowHeight = (screenHeight / numWindows) - (g_config.padding * 2);

  for (int i = 0; i < numWindows; i++) {
    int x = workArea.left + g_config.padding;
    int y = workArea.top + (i * (windowHeight + g_config.padding * 2)) +
            g_config.padding;

    HWND hwnd = g_windows[i];
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE) {
      ShowWindow(hwnd, SW_RESTORE);
    }

    SetWindowPos(hwnd, NULL, x, y, screenWidth - (g_config.padding * 2),
                 windowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

// Tile windows with main window and rest stacked (like monocle layout in some
// WMs)
void TileWindowsMainAndDeck() {
  if (g_windows.empty()) {
    return;
  }

  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

  int screenWidth = workArea.right - workArea.left;
  int screenHeight = workArea.bottom - workArea.top;
  int numWindows = (int)g_windows.size();

  if (numWindows == 1) {
    HWND hwnd = g_windows[0];
    int x = workArea.left + g_config.padding;
    int y = workArea.top + g_config.padding;

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE) {
      ShowWindow(hwnd, SW_RESTORE);
    }

    SetWindowPos(hwnd, NULL, x, y, screenWidth - (g_config.padding * 2),
                 screenHeight - (g_config.padding * 2),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    return;
  }

  HWND mainWindow = g_windows[0];
  if (GetWindowLong(mainWindow, GWL_STYLE) & WS_MAXIMIZE) {
    ShowWindow(mainWindow, SW_RESTORE);
  }

  int mainWidth =
      (int)(screenWidth * g_config.mainRatio) - (g_config.padding * 2);
  int secondaryWidth = screenWidth - mainWidth - (g_config.padding * 3);
  int secondaryHeight =
      (screenHeight / (numWindows - 1)) - (g_config.padding * 2);

  SetWindowPos(mainWindow, NULL, workArea.left + g_config.padding,
               workArea.top + g_config.padding, mainWidth,
               screenHeight - (g_config.padding * 2),
               SWP_NOZORDER | SWP_NOACTIVATE);

  for (int i = 1; i < numWindows; i++) {
    HWND hwnd = g_windows[i];
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE) {
      ShowWindow(hwnd, SW_RESTORE);
    }

    int x = workArea.left + mainWidth + (g_config.padding * 2);
    int y = workArea.top +
            ((i - 1) * (secondaryHeight + g_config.padding * 2)) +
            g_config.padding;

    SetWindowPos(hwnd, NULL, x, y, secondaryWidth, secondaryHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

// Helper to get window Z-order (add this before TileWindowsGrid)
int GetWindowZOrder(HWND hwnd) {
  int z = 0;
  for (HWND h = hwnd; h != NULL; h = GetWindow(h, GW_HWNDPREV)) {
    z++;
  }
  return z;
}

// Tile windows in a grid layout - improved version
void TileWindowsGrid() {
  if (g_windows.empty()) {
    printf("No windows to tile\n");
    fflush(stdout);
    return;
  }

  printf("\nStarting grid tiling with %d windows\n", (int)g_windows.size());

  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

  int screenWidth = workArea.right - workArea.left;
  int screenHeight = workArea.bottom - workArea.top;
  int numWindows = (int)g_windows.size();

  if (numWindows == 1) {
    HWND hwnd = g_windows[0];
    int x = workArea.left + g_config.padding;
    int y = workArea.top + g_config.padding;

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE) {
      ShowWindow(hwnd, SW_RESTORE);
    }

    SetWindowPos(hwnd, NULL, x, y, screenWidth - (g_config.padding * 2),
                 screenHeight - (g_config.padding * 2),
                 SWP_NOZORDER | SWP_NOACTIVATE);
    return;
  }

  // Sort windows by z-order with error handling
  try {
    std::sort(g_windows.begin(), g_windows.end(), [](HWND a, HWND b) {
      return GetWindowZOrder(a) < GetWindowZOrder(b);
    });
  } catch (const std::exception &e) {
    printf("Error sorting windows: %s\n", e.what());
  }

  // Calculate grid layout
  int rows = (int)ceil(sqrt(numWindows));
  int cols = (numWindows + rows - 1) / rows;

  int cellWidth = (screenWidth / cols) - (g_config.padding * 2);
  int cellHeight = (screenHeight / rows) - (g_config.padding * 2);

  printf("Tiling with grid: %d rows x %d columns\n", rows, cols);
  fflush(stdout);

  for (int i = 0; i < numWindows; i++) {
    int row = i / cols;
    int col = i % cols;

    int x = workArea.left + (col * (cellWidth + g_config.padding * 2));
    int y = workArea.top + (row * (cellHeight + g_config.padding * 2));

    HWND hwnd = g_windows[i];
    char title[256] = {0};
    GetWindowTextA(hwnd, title, sizeof(title));

    printf("Tiling window '%s' to position (%d,%d) size (%d,%d)\n", title, x, y,
           cellWidth, cellHeight);

    SetTileWindow(hwnd, x, y, cellWidth, cellHeight);
  }
}

void ApplyTiling() {
  if (g_windows.empty()) {
    printf("No windows to tile\n");
    fflush(stdout);
    return;
  }

  switch (g_config.layout) {
  case LAYOUT_HORIZONTAL:
    printf("Applying horizontal layout\n");
    TileWindowsHorizontal();
    break;
  case LAYOUT_VERTICAL:
    printf("Applying vertical layout\n");
    TileWindowsVertical();
    break;
  case LAYOUT_MAIN_AND_DECK:
    printf("Applying main+deck layout\n");
    TileWindowsMainAndDeck();
    break;
  case LAYOUT_GRID:
  default:
    printf("Applying grid layout\n");
    TileWindowsGrid();
    break;
  }
}

extern "C" __declspec(dllexport) LRESULT CALLBACK ShellProc(int code,
                                                            WPARAM wParam,
                                                            LPARAM lParam) {
  if (code < 0)
    return CallNextHookEx(NULL, code, wParam, lParam);

  try {
    printf("\n--- Shell Hook Event ---\n");
    LogShellEvent(code, wParam, lParam);

    // Force retile on these events
    if (code == HSHELL_WINDOWCREATED || code == HSHELL_WINDOWDESTROYED ||
        code == HSHELL_REDRAW || code == HSHELL_WINDOWACTIVATED) {

      Sleep(100); // Small delay to let window settle
      g_windows.clear();
      EnumWindows(EnumWindowsProc, 0);

      if (!g_windows.empty()) {
        printf("Retiling %d windows\n", (int)g_windows.size());
        ApplyTiling();
      }
    }
  } catch (const std::exception &e) {
    printf("Error in ShellProc: %s\n", e.what());
  }

  return CallNextHookEx(NULL, code, wParam, lParam);
}

// Change SetLayout to SetTilingLayout to avoid conflict with Windows API
extern "C" __declspec(dllexport) void SetTilingLayout(int layoutId) {
  switch (layoutId) {
  case 1:
    g_config.layout = LAYOUT_HORIZONTAL;
    break;
  case 2:
    g_config.layout = LAYOUT_VERTICAL;
    break;
  case 3:
    g_config.layout = LAYOUT_MAIN_AND_DECK;
    break;
  default:
    g_config.layout = LAYOUT_GRID;
    break;
  }
  printf("Layout changed to %d\n", layoutId);
  fflush(stdout);
}

extern "C" __declspec(dllexport) void ForceTile() {
  try {
    printf("Forced tiling requested\n");
    fflush(stdout);

    g_windows.clear();
    EnumWindows(EnumWindowsProc, 0);
    ApplyTiling();
  } catch (const std::exception &e) {
    printf("Error in ForceTile: %s\n", e.what());
    fflush(stdout);
  }
}
