@echo on
@setlocal enabledelayedexpansion

@SET PRJNAME=esz7-term
@SET BUILDTYPE=%1
@IF "%BUILDTYPE%" EQU "" (
    @SET BUILDTYPE=Debug
)
@SET BUILDDIR=build-%BUILDTYPE%
@SET EXEFILE=%BUILDDIR%\esz7-term

@IF NOT EXIST "CMakeLists.txt" (
    @echo.
    @echo ERROR: not found CMakeLists.txt - directory not for building
    @echo.
    @pause
    @exit /b 1
)

@IF EXIST "%BUILDDIR%\" (
    @echo Remove current build directory - %BUILDDIR% ...
    @rd /Q /S %BUILDDIR%
    @if "%ERRORLEVEL%" NEQ "0" (
        @echo ERROR: Can't remove direcory %BUILDDIR%
        @echo.
        @pause
        @exit /b 1
    )
)

@echo Create build directory         - %BUILDDIR% ...
@mkdir %BUILDDIR%
@IF NOT EXIST "%BUILDDIR%\" (
    @echo ERROR: directory not found - %BUILDDIR%
    @echo.
    @pause
    @exit /b 1
)

cmake -G "MinGW Makefiles" -S . -B %BUILDDIR% -DCMAKE_BUILD_TYPE=%BUILDTYPE%
@if "%ERRORLEVEL%" NEQ "0" (
    @echo.
    @echo ERROR: Can't create build configuration
    @echo.
    @pause
    @exit /b 1
)

cmake --build %BUILDDIR% -j26 --config %BUILDTYPE%
@if "%ERRORLEVEL%" NEQ "0" (
    @echo.
    @echo ERROR: build failed
    @echo.
    @pause
    @exit /b 1
)

@IF EXIST "%EXEFILE%" (
    @rd /Q /S dist-%BUILDTYPE%\
    cmake --build %BUILDDIR% --target install
    @if "%ERRORLEVEL%" NEQ "0" (
        @echo.
        @echo ERROR: Failed to create production package
        @echo.
        @pause
        @exit /b 1
    )
)

@IF NOT EXIST "dist-%BUILDTYPE%\" (
    @echo.
    @echo ERROR: production package not found - "dist-%BUILDTYPE%" ?
    @echo.
    @pause
    @exit /b 1
)

@dir dist-%BUILDTYPE%\

@rem -----------------------------------------------
@echo.
@echo The producation package built successfully
@echo.

@cd ..
@exit /b 0
