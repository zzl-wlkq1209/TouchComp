# TouchComp

TouchComp is a Visual Studio / Qt project for haptic-device compensation validation.

## Recommended Workflow

Use Git to synchronize source code between the lab workstation and the laptop.
Build artifacts are generated locally on each machine and should not be committed.

```powershell
git pull
.\scripts\build-release.ps1
```

## Dependency Variables

Debug and Release use system environment variables for machine-specific
dependency locations. Configure these variables on each computer:

```powershell
OPENHAPTICS_DIR=D:\Program Files\OpenHaptics\Developer\3.5.0
EIGEN_DIR=D:\Program Files\eigen-3.3.7
BOOST_ROOT=D:\Program Files\boost_1_75_0
UR_RTDE_DIR=D:\Program Files\ur_rtde
QT_DIR=D:\Qt\Qt5.15.2\5.15.2\msvc2019_64
```

`EIGEN_DIR` should point to the folder that directly contains the `Eigen`
directory. `QT_DIR` should point to the Qt kit directory that contains
`include`, `lib`, and `bin`.

## Opening On Another Computer

Install Visual Studio 2019 (or Build Tools) with the C++ desktop workload,
Qt VS Tools, and the Qt 5.15.2 MSVC 2019 x64 kit. Set `QT_DIR` to that kit's
directory before opening `TouchComp.sln`. The project uses the Visual Studio
2019 `v142` toolset and reads the Qt path from `QT_DIR`; it does not depend on
the original workstation's absolute Qt installation path.

The project also uses the checked-in project libraries under:

```text
TouchComp/lib/x64/Release/
```

Qt is still resolved through the Visual Studio Qt extension using the Qt version
name in `TouchComp/TouchComp.vcxproj`, and the Qt include directory is also kept in
the Release include path through `QT_DIR`:

```text
5.15.2_msvc2019_64
```

Make sure both machines have a matching Qt version registered in Qt VS Tools.

## Notes

- Debug and Release both use the same portable dependency variables.
- Device IP addresses are still fixed in source code by design.
