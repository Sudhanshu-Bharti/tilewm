#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

HHOOK hookHandle;
HWND hwndMain;

// For communicating with the DLL
extern "C" __declspec(dllimport) void SetTilingLayout(int layoutId);
extern "C" __declspec(dllimport) void ForceTile();

// Function to clean up the hook - imp
void cleanup() {
  if (hookHandle) {
    UnhookWindowsHookEx(hookHandle);
    hookHandle = NULL;
  }
}

// Keyboard hook procedure to handle shortcuts
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode < 0)
    return CallNextHookEx(NULL, nCode, wParam, lParam);

  if (wParam == WM_KEYDOWN) {
    KBDLLHOOKSTRUCT *kbStruct = (KBDLLHOOKSTRUCT *)lParam;

    // Check modifiers
    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool winPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0;

    if (altPressed && shiftPressed) {
      printf("Hotkey detected: Alt+Shift+%c\n", (char)kbStruct->vkCode);

      switch (kbStruct->vkCode) {
      case '1':
        wcout << L"Switching to Grid layout" << endl;
        SetTilingLayout(0);
        ForceTile();
        return 1;
      case '2':
        wcout << L"Switching to Horizontal layout" << endl;
        SetTilingLayout(1);
        ForceTile();
        return 1;
      case '3':
        wcout << L"Switching to Vertical layout" << endl;
        SetTilingLayout(2);
        ForceTile();
        return 1;
      case '4':
        wcout << L"Switching to Main+Deck layout" << endl;
        SetTilingLayout(3);
        ForceTile();
        return 1;
      case 'T':
        wcout << L"Force retiling windows" << endl;
        ForceTile();
        return 1;
      default:
        break;
      }
    }

    if (winPressed) {
      switch (kbStruct->vkCode) {
      case '1':
      case '2':
      case '3':
      case '4':
        printf("Windows+%c pressed\n", (char)kbStruct->vkCode);
        SetTilingLayout(kbStruct->vkCode - '1');
        ForceTile();
        return 1;
      case 'T':
        printf("Windows+T pressed - force retile\n");
        ForceTile();
        return 1;
      }
    }
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Window Procedure to handle shell messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_DESTROY:
    cleanup();
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main() {
  // Load the DLL that contains the hook procedure
  HMODULE windows_dll = LoadLibraryW(L"windows_dll.dll");
  if (!windows_dll) {
    wcout << L"Failed to load DLL, error code: " << GetLastError() << endl;
    return 1;
  }

  // recasting
  HOOKPROC shell_proc =
      reinterpret_cast<HOOKPROC>(GetProcAddress(windows_dll, "ShellProc"));

  // If that fails, try with decoration because of name mangling
  if (!shell_proc) {
    shell_proc = reinterpret_cast<HOOKPROC>(
        GetProcAddress(windows_dll, "_ShellProc@12"));
    if (!shell_proc) {
      wcout << L"Failed to get function address with both methods, error code: "
            << GetLastError() << endl;
      FreeLibrary(windows_dll);
      return 1;
    }
    wcout << L"Found ShellProc with decorated name '_ShellProc@12'" << endl;
  } else {
    wcout << L"Found ShellProc with undecorated name 'ShellProc'" << endl;
  }

  // Create a hidden window to receive shell messages
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = L"HiddenShellWindow";
  RegisterClassW(&wc);

  hwndMain = CreateWindowW(L"HiddenShellWindow", L"Hidden Shell Hook", 0, 0, 0,
                           0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
  if (!hwndMain) {
    wcout << L"Failed to create window, error code: " << GetLastError() << endl;
    FreeLibrary(windows_dll);
    return 1;
  }

  // Register for shell hook notifications
  RegisterShellHookWindow(hwndMain);

  // Set the WH_SHELL hook
  hookHandle = SetWindowsHookExW(WH_SHELL, shell_proc, windows_dll, 0);
  if (!hookHandle) {
    wcout << L"Failed to set hook, error code: " << GetLastError() << endl;
    FreeLibrary(windows_dll);
    return 1;
  }

  // Set up keyboard hook for hotkeys
  HHOOK keyboardHook =
      SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
  if (!keyboardHook) {
    wcout << L"Failed to set keyboard hook, error code: " << GetLastError()
          << endl;
  }

  wcout << L"Shell Hook successfully set!" << endl;
  wcout << L"Tile Window Manager is now running. Press Ctrl+C to exit." << endl;
  wcout << L"Keyboard shortcuts:" << endl;
  wcout << L"  Alt+Shift+1: Grid layout" << endl;
  wcout << L"  Alt+Shift+2: Horizontal layout" << endl;
  wcout << L"  Alt+Shift+3: Vertical layout" << endl;
  wcout << L"  Alt+Shift+4: Main+Deck layout" << endl;
  wcout << L"  Alt+Shift+T: Force retile" << endl;

  // Message loop to process shell events
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  // Cleanup
  cleanup();
  if (keyboardHook) {
    UnhookWindowsHookEx(keyboardHook);
  }
  FreeLibrary(windows_dll);
  return 0;
}