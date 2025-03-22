# Tile Window Manager

A Windows tiling window manager using win32 api. Basically a scratch project. Wont Recommend it for Regular use. 
A Windows tiling window manager using the Win32 API. This is an experimental project and not recommended for daily use.

Inspired by [LightWM](https://github.com/nir9/lightwm) - a lightweight tiling window manager for Windows written in C.


## Building the Project

1. First ensure no instances of the application are running:

   ```bash
   taskkill /F /IM windows_dll.dll
   taskkill /F /IM main.exe
   ```

2. Compile the DLL containing the shell hooks:

   ```bash
   cl /LD /EHsc windows_dll.cpp /Fe:"windows_dll.dll" /link user32.lib kernel32.lib
   ```

3. Then compile the main application:
   ```bash
   cl /EHsc main.cpp /Fe:"main.exe" /link user32.lib shell32.lib kernel32.lib windows_dll.lib
   ```

Note: Always run these commands in an Administrator Developer Command Prompt and ensure you're in the correct directory.

## Running the Window Manager

Start the tile window manager:

```bash
.\main.exe
```

## Keyboard Shortcuts

- **WIN+Shift+1**: Switch to Grid layout
- **WIN+Shift+2**: Switch to Horizontal layout
- **WIN+Shift+3**: Switch to Vertical layout
- **WIN+Shift+4**: Switch to Main+Deck layout
- **WIN+Shift+T**: Force retiling of all windows

## How It Works

The application sets a Windows Shell Hook to monitor window creation and activation events.
When these events occur, it collects all visible windows and arranges them according to the selected layout.

## Customization

You can modify the `TilingConfig` struct in `windows_dll.cpp` to adjust:

- Padding between windows
- Default layout
- Main window ratio (for Main+Deck layout)
- Other tiling parameters


