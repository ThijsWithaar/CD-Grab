@set SETUP_MSVC_2019="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not exist %SETUP_MSVC_2019% (
	@set SETUP_MSVC_2019="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)
SET VCPKG_ROOT=C:\build\vcpkg

@if "%VCPKG_BINARY_SOURCES%x" == "x" (
	SET VCPKG_BINARY_SOURCES=clear;files,%VCPKG_ROOT%\..\vcpkg.cache,readwrite
)

call %SETUP_MSVC_2019%
cmake -G "Visual Studio 16 2019" -A x64 -T "v142" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -H. -Bbuild.sln

@rem cmake -G Ninja -A x64 -DCMAKE_CXX_COMPILER="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\Llvm\bin" -DCMAKE_TOOLCHAIN_FILE=C:\build\vcpkg\scripts\buildsystems\vcpkg.cmake -H. -Bbuild.llvm

@rem cmake -G Ninja -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/Llvm/bin/clang-cl.exe" -DCMAKE_TOOLCHAIN_FILE=C:\build\vcpkg\scripts\buildsystems\vcpkg.cmake -H. -Bbuild.llvm

@rem cmake --C  -H. -Bbuild.llvm

pause
