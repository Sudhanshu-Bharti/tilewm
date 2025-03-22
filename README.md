# Tile Window Manager

A Windows tiling window manager similar to Komorebi that automatically arranges windows in various layouts.

## Building the Project

1. First ensure no instances of the application are running:

   ```bash
   taskkill /F /IM windows_dll.dll
   taskkill /F /IM main.exe
   ```

2. Clean existing files:

   ```bash
   del windows_dll.dll windows_dll.lib windows_dll.obj main.exe main.obj
   ```

3. Compile the DLL containing the shell hooks:

   ```bash
   cl /LD /EHsc windows_dll.cpp /Fe:"windows_dll.dll" /link user32.lib kernel32.lib
   ```

4. Then compile the main application:
   ```bash
   cl /EHsc main.cpp /Fe:"main.exe" /link user32.lib shell32.lib kernel32.lib windows_dll.lib
   ```

Note: Always run these commands in an Administrator Developer Command Prompt and ensure you're in the correct directory.

## Running the Window Manager

Start the tile window manager:

```bash
.\main.exe
```

## Features

- Automatically tiles windows when new windows are created/activated
- Multiple tiling layouts:
  - Grid: Windows arranged in a grid pattern (default)
  - Horizontal: Windows arranged side by side
  - Vertical: Windows stacked on top of each other
  - Main+Deck: One large window with others stacked to the side
- Keyboard shortcuts for changing layouts
- Smart window filtering to avoid tiling system windows
- Adds configurable padding between windows

## Keyboard Shortcuts

- **Alt+Shift+1**: Switch to Grid layout
- **Alt+Shift+2**: Switch to Horizontal layout
- **Alt+Shift+3**: Switch to Vertical layout
- **Alt+Shift+4**: Switch to Main+Deck layout
- **Alt+Shift+T**: Force retiling of all windows

## How It Works

The application sets a Windows Shell Hook to monitor window creation and activation events.
When these events occur, it collects all visible windows and arranges them according to the selected layout.

## Customization

You can modify the `TilingConfig` struct in `windows_dll.cpp` to adjust:

- Padding between windows
- Default layout
- Main window ratio (for Main+Deck layout)
- Other tiling parameters

## Troubleshooting

If you encounter issues:

- Make sure both the DLL and EXE are in the same directory
- Run the application with administrator privileges
- Check that you have the required Visual C++ Runtime
- If windows aren't being tiled, try pressing Alt+Shift+T to force retiling
