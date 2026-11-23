# PinTop (Native C++ / Win32)

PinTop is a small native Windows utility for keeping desktop windows always on top. It uses Win32 APIs and does not require .NET, Electron, Qt, Visual Studio, or a local compiler on the target PC.

## Controls

- **Window system menu:** right-click a window's title bar (or press `Alt + Space`) and choose **Always on Top**. The menu item is checked while that window is topmost. This is the Linux-style control requested.
- **Middle-click:** middle-click a window's title bar to toggle pinning.
- **Keyboard:** `Win + Space` toggles the foreground window if Windows permits registration. Windows reserves this shortcut for input-language switching on many systems, so it may be unavailable.
- **System tray:** right-click PinTop's tray icon to control startup or exit.

The system-menu integration uses a small companion `PinTopHook.dll` to add the command to other applications' native system menus. Keep `PinTop.exe` and `PinTopHook.dll` in the same directory. Both are included in the GitHub Actions artifact.

## Build with GitHub Actions (no local build tools)

1. Create an empty GitHub repository.
2. Upload the contents of this project, including `.github/workflows/build-windows.yml`.
3. Open **Actions** → **Build PinTop (Windows x64)** → **Run workflow**.
4. When the workflow completes, download the `PinTop-windows-x64` artifact.
5. Extract it and run `PinTop.exe`. Do not separate `PinTopHook.dll` from the EXE.

Pushing a commit to `main` or `master` also triggers a build.

## Startup

On first launch, PinTop adds itself to the current user's `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` registry key. No administrator privileges are normally required. Startup can be disabled or re-enabled from the tray menu.

## Notes and limitations

- The system-menu hook is built for **64-bit Windows applications**. It will not add the item to 32-bit applications in this x64 build.
- Windows may prevent interaction with elevated applications when PinTop is not elevated. Run PinTop at the same integrity level as the target app if necessary.
- Some applications replace or customize their native system menus; the command may not appear in those menus.
- `Win + Space` is reserved by Windows on many systems. Middle-click and the system-menu item are alternatives.
- The topmost state belongs to Windows and remains after PinTop exits; PinTop must be running to toggle it using these controls.
