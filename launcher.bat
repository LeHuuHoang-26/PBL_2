@echo off
setlocal
cd /d "%~dp0"

:: Thiet lap moi truong thu vien Qt6 va MinGW
set "PATH=C:\msys64\ucrt64\bin;%PATH%"

echo ========================================================
echo        KHOI DONG HE THONG QUAN LY PHONG KHAM AN TAM      
echo ========================================================
echo.

if exist "bin\MngCareBook.exe" (
    echo Dang khoi chay ung dung tu bin\MngCareBook.exe...
    start "" "bin\MngCareBook.exe"
    exit /b 0
)

if exist "MngCareBook.exe" (
    echo Dang khoi chay ung dung tu MngCareBook.exe...
    start "" "MngCareBook.exe"
    exit /b 0
)

if exist "release\AnTamClinic.exe" (
    echo Dang khoi chay ung dung tu release\AnTamClinic.exe...
    start "" "release\AnTamClinic.exe"
    exit /b 0
)

if exist "build-qt\release\AnTamClinic.exe" (
    echo Dang khoi chay ung dung tu build-qt\release\AnTamClinic.exe...
    start "" "build-qt\release\AnTamClinic.exe"
    exit /b 0
)

echo [LOI] Khong tim thay file thuc thi!
echo He thong se tien hanh bien dich tu dong...
mingw32-make -j4
if %ERRORLEVEL% equ 0 (
    if exist "bin\MngCareBook.exe" (
        start "" "bin\MngCareBook.exe"
        exit /b 0
    )
    if exist "release\AnTamClinic.exe" (
        start "" "release\AnTamClinic.exe"
        exit /b 0
    )
)

echo.
echo [LOI] Khong the khoi chay ung dung. Vui long kiem tra lai trinh bien dich!
pause
exit /b 1
