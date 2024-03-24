set OUTPUT_PATH=bin
set OUTPUT_EXE=esnpad.exe

rmdir /s /q %OUTPUT_PATH%
mkdir %OUTPUT_PATH%
rc.exe /fo %OUTPUT_PATH%/resources.res resources.rc

cl.exe main.c file.c ^
/DUNICODE /D_UNICODE /WX /EHsc /Zi ^
/Fe%OUTPUT_PATH%\%OUTPUT_EXE% /Fo%OUTPUT_PATH%\ /Fd%OUTPUT_PATH%\vc140.pdb ^
/link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib comdlg32.lib %OUTPUT_PATH%\resources.res

del %OUTPUT_PATH%\*.obj
del %OUTPUT_PATH%\*.ilk
del %OUTPUT_PATH%\*.res
