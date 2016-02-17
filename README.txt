# klogski
Small and simple Key Logger for windows

Installs hook with SetWindowsHookEx and WH_KEYBOARD_LL which then 
doesn't require an external DLL.

Once running, you can terminate it with combination of CTRL+ALT+F12
Everything typed is logged to "klog.txt" in same directory.

example of information logged

==================================================================
  [ Log starting on 17 Feb 2016 at 04:23:52
  [ Window Text: Administrator: Visual Studio Command Prompt (2010)
  [ User: John-PC\admin Process: cmd.exe
[Down]
[Down]
[Down][Up][Up][Up][Up][Down][Backspace][Backspace][Backspace]netstat -ano
[Ctrl][Alt][Alt][Alt][Alt][F12]
  [ Hotkey entered

  [ Log ending on 17 Feb 2016 at 04:24:04
==================================================================

To compile this, you can use msvc or gcc

  gcc klogski.c -lshlwapi -oklogski
  cl klogski.c 
  
To create a smaller exe file around 5kb, compile with clib.c but this requires longer command

  cl klogski.c /nologo /c /O2 /Os /GS- /Oi-
  cl clib.c /nologo /c /O2 /Os /GS- /Oi-
  link /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /MERGE:.rdata=.text klogski.obj clib.obj Shlwapi.lib user32.lib kernel32.lib advapi32.lib
  
  author: odzhan
  