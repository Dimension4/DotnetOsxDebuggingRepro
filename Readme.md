This project is a repro case for .net core debugging failure on macOS when attaching to [SketchUp (Pro)](https://www.sketchup.com) on an Apple M1 chip based macOS. We don't know if Intel chips (or rather x64 macOS) is affected by this as well.

# About SketchUp and this issue

This issue focuses on SketchUp 2021 and 2022. 2021 is still a x64 executable, meaning it relies no Rosetta, while 2022 is arm64. SketchUp itself doesn't use any dotnet, it's a native executable. Plugins are usually written in Ruby, however in this case we use a Ruby C extension (natively supported by Ruby). This Ruby C extension then boots the core clr, loads our managed assembly, and calls the `PluginEntry` method.

We ship our own .net 6 runtimes so I included them in this repo (in case it makes a difference when using shared runtimes).

```
SketchUp 2021/2022
↓
Ruby 2.7.2 Runtime
↓
MyPlugin.rb (minimal shim that loads the C extension)
↓
NativeHost (Ruby C extension, see /native folder)
↓
DotnetOsxDebuggingRepro.dll (managed, see /dotnet folder)
```

# Requirements

- CMake 3.20+ (arbitrarily selected, you may be able to skew this down in the CMakeLists.txt)
- dotnet cli
- ninja (unless you overwrite the generator in the presets)
- C++17 compiler
- powershell core (pwsh; to register the plugin in SketchUp)
- SketchUp 2021/2022

# How to build

You can either use VS Code with the CMake and C# extensions or the command line:

## macOS

```
dotnet build ./dotnet
cmake ./native --preset osx-universal-debug
cmake --build ./native/out/build/osx-universal-debug/
```

## Windows

Make sure you're using the x64 Developer Command Line (or Power Shell).

```
dotnet build ./dotnet
cmake ./native --preset win-x64-debug
cmake --build ./native/out/build/win-x64-debug/
```

# How to register the plugin

In pwsh: `.\dotnet\Registration\register.ps1`

You only need to run this once before launching SketchUp.

# How to reproduce

1. Build everything
2. Register the plugin
3. Set a breakpoint anywhere in the C# code (`EntryPoint.cs`)
4. Start SketchUp 2021 or 2022 (you'll be greeted by a welcome window. At this point no plugins have been loaded)
   1. on macOS, you probably want to start the application from the terminal so you can see stdout. pwsh: `& '/Applications/SketchUp 2021/SketchUp.app/Contents/MacOS/SketchUp'`.
5. Make sure you're in the `Files` tab
6. Attach the dotnet debugger to the SketchUp process (you can use VS Code's debug pane)
7. Click on any of the images below the `Create new model` label (this will open the main window and load all plugins)

On Windows this should work perfectly fine, however on macOS SketchUp will freeze before the main window shows and the debugger will error out with `CORDBG_E_MISSING_DEBUGGER_EXPORTS` before it attaches successfully (you'll have to kill the process with `pkill -SIGKILL SketchUp.*`).

When repeating the steps above without attaching a debugger, you can see that the runtime starts and executes fine. We get the expected console output on both Windows x64 and macOS arm64.