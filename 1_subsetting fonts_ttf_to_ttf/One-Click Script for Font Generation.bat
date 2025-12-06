@echo off
echo ========================================================
echo      GENERATING TRADITIONAL CHINESE SUBSET FONT
echo ========================================================
echo.
echo Input Font: TaipeiSansTCBeta-Regular.ttf
echo Character List: tw_edu_common_4808_chars.txt
echo.

:: Check if fonttools is installed
python -c "import fontTools" 2>NUL
if %errorlevel% neq 0 (
    echo [ERROR] fonttools library not found!
    echo Please run: pip install fonttools
    pause
    exit /b
)

echo Processing... Please wait...
python -m fontTools.subset "TaipeiSansTCBeta-Regular.ttf" --text-file="tw_edu_common_4808_chars.txt" --unicodes="U+0000-007F,U+3000-303F" --layout-features="*" --output-file="TaipeiSansTC-Subset.ttf"

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Created 'TaipeiSansTC-Subset.ttf'
    echo Size:
    for %%I in ("TaipeiSansTC-Subset.ttf") do echo %%~zI bytes
    echo.
    echo Next Steps:
    echo 1. Rename this file to 'font.ttf'
    echo 2. Copy it to your PlatformIO 'data' folder.
    echo 3. Run 'Upload Filesystem Image'.
) else (
    echo.
    echo [FAILED] Something went wrong. Check filenames.
)

pause