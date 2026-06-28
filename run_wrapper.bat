@echo off
if not exist netbar_billing.exe (
    echo 正在编译 netbar_billing.c...
    gcc -g netbar_billing.c -o netbar_billing.exe
    if errorlevel 1 (
        echo 编译失败，请检查 gcc 是否在 PATH 中，或使用 MinGW 编译。
        pause
        exit /b 1
    )
)
python wrapper_server.py
