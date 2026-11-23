# PinTop (Native C++)

A small, native Win32 system-tray utility for keeping desktop windows always on top. It does not use .NET, Electron, Qt, or a background service.

## Features

- `Win + Space` toggles the foreground window's topmost state when Windows permits that global hotkey.
- Middle-click a window's title bar to toggle its topmost state.
- System-tray balloon notifications confirm pin/unpin actions.
- Automatically adds itself to the current user's `Run` registry key on first launch (no administrator rights normally required).
- Tray menu lets you disable/re-enable startup or exit.
- Uses the native `SetWindowPos(HWND_TOPMOST/HWND_NOTOPMOST)` API.

## Build without installing anything locally (recommended)

This project includes a GitHub Actions workflow at `.github/workflows/build-windows.yml`. GitHub's hosted Windows runner compiles the application, so you do not need Visual Studio, Build Tools, CMake, a compiler, or the .NET SDK installed on your PC.

1. Sign in to GitHub and create a **new, empty repository** (for example, `pintop`).
2. Extract this ZIP and upload the *contents* of the `PinTop-CPP` folder to the repository. Make sure `.github/workflows/build-windows.yml` is included. GitHub's web uploader may hide dot-folders; if so, use GitHub Desktop or the `git` command line to upload the folder.
3. Open the repository's **Actions** tab.
4. Select **Build PinTop (Windows x64)**.
5. Click **Run workflow**. Alternatively, pushing a commit to `main` or `master` automatically starts a build.
6. When the run completes, open it and download the `PinTop-windows-x64` artifact.
7. Extract the downloaded ZIP and run `PinTop.exe`.

The artifact is retained by GitHub for 30 days. You can rerun the workflow whenever you want a fresh build. The workflow builds on GitHub's Windows x64 runner using MSVC and statically links the MSVC runtime (`/MT`), so the target PC does not normally need the Visual C++ redistributable.

## Important Windows shortcut limitation

Windows commonly reserves `Win + Space` for switching input languages. `RegisterHotKey` may reject it. PinTop reports this in a tray notification; middle-clicking a title bar remains available. The app does not attempt to override Windows' reserved shortcut.

## Optional local build

If you already have a C++ compiler and CMake installed, you can build locally:

```powershell
cmake -S . -B out -A x64
cmake --build out --config Release
```

Or run `build.cmd` from an **x64 Native Tools Command Prompt for Visual Studio**. Neither is required when using the GitHub Actions workflow.

## Use

- Run `PinTop.exe`; it appears in the notification area/system tray.
- Press `Win + Space` to toggle the active window if the hotkey registered successfully.
- Or middle-click the title bar of the window you want to pin.
- Right-click the tray icon to toggle startup or exit.

Startup is enabled on first launch. It is stored under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, so it applies only to the current Windows user and does not require elevation. The tray menu can disable or re-enable it.

## Notes

- Topmost state is managed by Windows for the target window. PinTop does not need to remain open for a window to retain its topmost flag, but the keyboard/mouse toggles and tray controls require PinTop to be running.
- Some elevated, protected, or unusual application windows may reject interaction from a non-elevated PinTop instance.
- The middle-click gesture may conflict with applications that assign their own middle-click title-bar behavior.
- Closing PinTop does not automatically unpin windows; Windows retains each window's current topmost flag until that window closes or another app changes it.
