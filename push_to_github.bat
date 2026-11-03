@echo off
echo ===================================================
echo  Pushing FUT Analytics Football Simulator to GitHub
echo  Repository: https://github.com/Fash888/football_sim
echo ===================================================
set "PATH=C:\Users\USER-PC\AppData\Local\hermes\tools\git-2.53.0+3-win32-x64\cmd;%PATH%"
cd /d "%~dp0"
git push -u origin main
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===================================================
    echo  SUCCESS: Code pushed to https://github.com/Fash888/football_sim
    echo ===================================================
) else (
    echo.
    echo ===================================================
    echo  Push encountered an authentication step.
    echo  If prompted, complete browser sign-in or enter your GitHub Personal Access Token.
    echo ===================================================
)
pause
