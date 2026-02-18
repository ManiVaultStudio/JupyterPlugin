# FAQ

## Compilation errors

### `returned non-zero exit status 9009`

Compilation of the python wheels `mvstudio_kernel` and `mvstudio_data` might fail with an error (on Windows with MSVC):
```
1>------ Build started: Project: mvstudio_kernel, Configuration: Debug x64 ------
1>
1>
1>Command '['C:\\Users\\default\\AppData\\Local\\Microsoft\\WindowsApps\\python.EXE', '-EsSc', 'import sys; print(sys.executable)']' returned non-zero exit status 9009.
1>The system cannot find the batch label specified - VCEnd
```

The error `returned non-zero exit status 9009` is the Windows internal code for "File Not Found".

Your build is trying to use this Python executable: `C:\Users\default\AppData\Local\Microsoft\WindowsApps\python.EXE`

This is not a real Python executable. It is a zero-byte "execution alias" (shim) that Windows 10/11 puts in your path to redirect you to the Microsoft Store to install Python. Even if Python is installed, these shims are notoriously unstable in automated build environments (like CMake/Visual Studio) because they don't behave like standard binaries.

This prevents CMake from ever finding that broken path again:

1. Open the system settings page "Manage App Execution Aliases".
2. Scroll down and find App Installer / Python. Turn OFF the switches for python.exe and python3.exe.
