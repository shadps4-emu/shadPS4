<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

 # Debugging and reporting issues about shadPS4 and games

This document covers information about debugging, troubleshooting and reporting developer-side issues related to shadPS4 and games.

## Setup

This section will guide you through setting up tools for debugging the emulator. This list will likely expand as more tools and platforms receive consistent setups.

<details>
<summary>Linux</summary>

RenderDoc doesn't work with Wayland, so to use it you have to run the emulator with `SDL_VIDEODRIVER=x11` set.

</details>

<details>
<summary>Windows and Visual Studio</summary>

Make sure you have the project set up for building on Windows with Visual Studio and CMake: [Build shadPS4 for Windows
](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-windows.md)

1. Open the project folder in Visual Studio **as a folder**. _Do not run `cmake ..` or other commands that set up the project._
   
2. In the Solution Explorer, click the **Switch between solutions and available views** button.\
   ![image](https://github.com/user-attachments/assets/4e2be2b1-ba5a-4451-9ab2-f4ecf246213d)

3. Double-click on **CMake Targets View**.\
  ![image](https://github.com/user-attachments/assets/5ce7cf90-cd61-4cfa-bef5-645909827290)

4. Under **shadPS4 Project**, right-click on the **shadps4 (executable)** solution and click **Set as Startup Item**. This will let you start and debug shadPS4 using the VS debug buttons, as well as the default F5 shortcut.\
   ![image](https://github.com/user-attachments/assets/34c7c047-28a3-499f-be8f-df781134d104)

5. Right-click the **shadps4 (executable)** solution once more and click **Add debug configuration**.

6. Add an `"args: []"` section into the first `configurations` entry.\
   List your game path as an argument, as if you were launching the non-GUI emulator from the command line.
   ![image](https://github.com/user-attachments/assets/8c7c3e69-f38f-4d6b-bdfd-4f1c41c50be7)

7. Set the appropriate CMake configuration for debugging or testing.
   - For debugging the emulator and games within it, select `x64-Clang-Debug`.
   - For testing the emulator with compiler optimizations as a release build, it is recommended to select `x64-Clang-RelWithDebInfo`,
     as debug symbols will still be generated in case you encounter release configuration-exclusive bugs/errors.
   ![image](https://github.com/user-attachments/assets/0d975f7a-7bea-4f89-87ef-5d685bea4381)

Launch and debug the emulator through **Debug > Start Debugging** (F5 by default), or **Debug > Start Without Debugging** (Ctrl+F5 by default) when testing games for performance.

</details>

## Configuration

You can configure the emulator by editing the `config.toml` file found in the `user` folder created after starting the application.

<details>
   <summary>Some configuration entries worth changing</summary>

- `[General]`
  
  - `logType`: Configures logging synchronization (`sync`/`async`)
    - By default, the emulator logs messages asynchronously for better performance. Some log messages may end up being received out-of-order.
    - It can be beneficial to set this to `sync` in order for the log to accurately maintain message order, at the cost of performance.
    - When communicating about issues with games and the log messages aren't clear due to potentially confusing order, set this to `sync` and send that log as well.
  - `logFilter`: Sets the logging category for various logging classes.
    - Format: `<class>:<level> ...`
    - Multiple classes can be set by separating them with a space. (example: `Render:Warning Debug:Critical Lib.Pad:Error`)
    - Sub-classes can be specified in the same format as seen in the console/log (such as `Core.Linker`).  
    - All classes and sub-classes can be set by specifying a `*` symbol. (example: `Kernel.*:Critical`)
    - Valid log levels: `Trace, Debug, Info, Warning, Error, Critical` - in this order, setting a level silences all levels preceding it and logs every level after it.
    - Examples:
      - If the log is being spammed with messages coming from Lib.Pad, you can use `Lib.Pad:Critical` to only log critical-level messages.
      - If you'd like to mute everything, but still want to receive messages from Vulkan rendering: `*:Critical Render.Vulkan:Info`
  - `isIdenticalLogGrouped`: Group same logs in one line with a counter (`true`/`false`)
    - By default, the emulator will not rewrite the same line, and instead add a counter.

   - `Fullscreen`: Display the game in a full screen borderless window.
     
- `[GPU]`
  - `dumpShaders`: Dump shaders that are loaded by the emulator. Dump path: `../user/shader/dumps`
  - `nullGpu`: Disables rendering.
  - `screenWidth` and `screenHeight`: Configures the game window width and height.
    
- `[Vulkan]`
   - `validation`-related settings: Use when debugging Vulkan.
   - `rdocEnable`: Automatically hook RenderDoc when installed. Useful for debugging shaders and game rendering.
   - `rdocMarkersEnable`: Enable automatic RenderDoc event annotation
     
- `[LLE]`
   - `libc`: Use LLE with `libc`.
   
</details>

## Quick analysis

This section will provide some preliminary steps to take and tips on what to do when you encounter scenarios that require debugging.

<details open>
<summary>When a game crashes and breaks in the debugger</summary>

1. Analyze the log
   - A console will open by default when you launch the emulator. It shows the same log messages that go into the log file found at `<emulator executable>/user/log/shad_log.txt`.

   - It is recommended that you start analyzing the log bottom-up first:
     - Are there any critical or error-level messages at the end of the log that would point to a reason for the game crashing?
     - Do any of the last few messages contain information about the game loading files?
     - Did the game window draw anything on-screen?
    
   - Continue analyzing the log from the start to see other errors (such as with initialization, memory mapping, linker errors etc.)

2. Analyze the stack trace
   - When the emulator is launched through a debugger, it will **break** when an exception or violation is encountered.\
     _(**breaking** in this context means pausing execution of the program before it continues or stops altogether.
         Breaks can be intentional as well - these are set with various kinds of **breakpoints**.)_

   - Default setups of most debuggers include a **Stack trace** window/panel that lists the functions the program has called before breaking.

   - The stack trace entries can be navigated to and will show the relevant function, as well as switch to the state that the program was in at the time of execution.\
     Use the **Locals** and **Watch** windows to investigate variables and other code in these contexts.

 3. Identify the reason for the crash
    - **Logs aren't always accurate in determining the reason for a crash.**\
      Some log entries are reported as errors but may not be fatal for the execution to stop. `Critical` entries are most likely to be the cause for crashes.

    - Pinpoint the area of the emulator where the crash occured\
      If the stack trace ends with functions that are relevant to rendering, it is safe to assume that the issue is with **rendering**.\
      Similarly, if a crash is in a library responsible for playing videos, your issue can be narrowed down to the scope of video playback in the emulator.

    - **⚠ Some crashes are intentional**
      - If you identify **Access violations for writing operations** where the function is (or in cases of game libraries, _looks like_ it is) copying memory,
      it most likely is an **intentional exception** meant to catch game data being written by the game.
      This is used by the emulator developers to identify procedures that have to do with game data changing.
      - Debugging tools usually include an option to not break on certain types of exceptions. **Exclude access violations and other intentional exceptions when debugging to skip these exceptions.**
      - You can also identify such cases if the game works in Release builds of the emulator. These intentional exceptions are development-time only.
      - Attempt to **Continue** and observe whether the stack trace and/or variables and registers change when you encounter exceptions.

</details>

## Reporting and communicating about issues

When communicating with the project about game-specific issues, specify an **uniquely identifable game name** along with its `CUSA-xxxxx` code that is specific to the region/variant of the game you're testing.\
The version number is also important to add at least in the description, especially if you can verify that the game behaves differently across versions.\
Accurately identifying games will help other developers that own that game recognize your issue by its title and jump in to help test and debug it.

- Examples of good naming schemes:
  - Amplitude (2016) `CUSA02480`
  - Rock Band 4 (`CUSA02084`) v1.0
  - inFamous: Second Son \[`CUSA-00004`\]
- Examples of unideal naming schemes:
  - _The Witness_
  - _GTA 5_
  - _Watch Dogs_
   
- If your issue is small or you aren't sure whether you have properly identified something, [join the Discord server](https://discord.gg/MyZRaBngxA) and use the #development channel
  to concisely explain the issue, as well as any findings you currently have.

- It is recommended that you check the [game compatibility issue tracker](https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues) and post very short summaries of progress changes there,
  (such as the game now booting into the menu or getting in-game) for organizational and status update purposes.
  
- ⚠ **Do not post theoretical, unproven game-specific issues in the emulator issue tracker that you cannot verify and locate in the emulator source code as being a bug.**\
    Do, however, add information about the game you experienced the issue in, so that it can be tested in a reproducible environment.
  - Good example: "_Crash in `Shader::Gcn::CFG::EmitBlocks()`, out of bounds list access_" -> _issue description shares stack trace, points to code in the repository and provides relevant information_
  - Bad example: "_Amplitude crashes on boot, access violation_" -> _issue description reiterates title, focuses on the game instead of the emulator and refuses to elaborate_
