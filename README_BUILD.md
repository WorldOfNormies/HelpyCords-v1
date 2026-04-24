# HelpyCords — Build Guide

## Prerequisites (must be installed)

1. **Visual Studio 2022** (Community is free)
   - Download: https://visualstudio.microsoft.com/vs/community/
   - During install, select: **"Desktop development with C++"** workload
   - This is NOT VS Code — this is the full Visual Studio IDE (needed for the MSVC compiler)

2. **CMake** (you have this already)

3. **VS Code** with these extensions:
   - C/C++ (by Microsoft)
   - CMake Tools (by Microsoft)

4. **JUCE** folder cloned inside your project folder
   - Your folder structure must be:
     ```
     VST/
     ├── JUCE/               ← clone from https://github.com/juce-framework/JUCE
     ├── CMakeLists.txt
     ├── HelpyCordsPlugin.h
     ├── HelpyCordsPlugin.cpp
     ├── HelpyCordsEditor.h
     ├── HelpyCordsEditor.cpp
     └── .vscode/
         ├── settings.json
         ├── tasks.json
         └── c_cpp_properties.json
     ```

---

## Why MinGW DOES NOT work

Your log shows CMake was using `e:\mingw64\bin\g++.exe`. 
**JUCE VST3 on Windows requires MSVC** (Visual Studio's compiler).
MinGW is missing the Windows SDK headers JUCE needs. Always use MSVC.

---

## Step-by-Step Build

### Step 1 — Copy files into your project folder

Copy all files from this folder into:
```
C:\Users\World Of Normies\Desktop\VST\
```
(replace existing files when prompted)

### Step 2 — Remove old CMake cache

Delete this folder if it exists:
```
C:\Users\World Of Normies\Desktop\VST\build\
```

### Step 3 — Open in VS Code

```
File → Open Folder → C:\Users\World Of Normies\Desktop\VST
```

### Step 4 — Select the MSVC kit

When VS Code opens, it may ask "Select a Kit". Choose:
```
Visual Studio Community 2022 Release - amd64
```

If it doesn't ask, press `Ctrl+Shift+P` and type:
```
CMake: Select a Kit
```
Then choose the Visual Studio 2022 option (NOT GCC, NOT MinGW).

### Step 5 — Configure

Press `Ctrl+Shift+P` and type:
```
CMake: Configure
```
Wait for it to finish. You should see "Build files written to: .../build"

### Step 6 — Build

Press `Ctrl+Shift+B`  (or `Ctrl+Shift+P` → "Tasks: Run Build Task")

The VST3 file will be built to:
```
build\HelpyCords_artefacts\Release\VST3\HelpyCords.vst3
```

---

## Install the VST3

Copy `HelpyCords.vst3` to:
```
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in your DAW.

---

## Troubleshooting

**"Could not find a package configuration file provided by JUCE"**
→ Make sure the JUCE folder is inside your project folder, not somewhere else.

**"cl.exe not found"**
→ Visual Studio 2022 with C++ workload is not installed, or VS Code can't find it.
  Run: `Developer Command Prompt for VS 2022` and try building from there.

**Kit shows GCC or MinGW**
→ Manually select the Visual Studio kit via `CMake: Select a Kit`

**JUCE version mismatch warnings**
→ Make sure your JUCE folder is up to date: `git pull` inside the JUCE folder.
