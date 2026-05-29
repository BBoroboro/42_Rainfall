# LEVEL00 - Authentification Bypass via Integer Comparision

> Vulnerability class: Logic Flaw - Weak Authentification
> Binary: ELF 32-bits, statically linked, not stripped
> Tools used: GDB, file, shell

---

## Overview

The program expects a specific numeric argument. by reverse engineering the comparision logic in `main`, we identify the expected value and bypass authentification to spawn a privileged shell.

---

## 1. Reconnaissance

```bash
   $ file level0 
   level0: setuid ELF 32-bit LSB executable, Intel 80386, version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.24, BuildID[sha1]=0x85cf4024dbe79c7ccf4f30e7c601a356ce04f412, not stripped
```

Key observations:

- setuid binary: exploiting it excalates privileges
- not stripped: symbol names are preserved, making static analysis easier
- statically linked: no external library dependencies to worry about

---

## 2. Dynamic Analysis

Running the binary reveals it requires an argument:
```bash
   $ ./level0 
   Segmentation fault (core dumped)
   $ ./level0 eded
   No !
```

The program crashes without input, and rejects arbitrary strings. This behaviour suggests the program validates a specific value.

---

## 3. Static Analysis (GDB)

Disassembling `main` function reveals the validation logic:
```asm
   0x08048ed4 <+20>:    call   0x8049710 <atoi>       ; convert argv[1] to integer
   0x08048ed9 <+25>:    cmp    $0x1a7,%eax            ; compare with hardcoded value
   0x08048ede <+30>:    jne    0x8048f58 <main+152>   ; jump to exit if not equal
```

The program:
1. Calls `atoi()` on the first argument
2. Compares the result against the hardcoded constant `0x1a7`
3. Jumps to the end of `main` if they differ

If the comparision succeeds, execution reaches a call to `execve()`:
```asm
   0x08048f4a <+138>:   movl   $0x80c5348,(%esp)
   0x08048f51 <+145>:   call   0x8054640 <execv>
```

Inspecting the string at `0x80c5348` confirms the target:
```bash
   (gdb) x/s 0x80c5348
   0x80c5348:       "/bin/sh"
```

---

## 4. Vulnerability

Root cause: The program uses a single hardcoded integer comparison as its entire authentication mechanism. There is no cryptographic check, no environment validation, and no obfuscation. The secret value is embedded in plaintext in the binary.

Impact: Any user who reads the binary (via GDB or `strings`) can immediately retrieve the password and gain a shell running as the next user.

---

## 5. Exploitation

`0x1a7` = **423** in decimal.
```bash
   level0@RainFall:~$ ./level0 423
   $ whoami
   level1
   $ cat /home/user/level1/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

---

## 6. Key Takeaways

| Concept | Detail |
|---|---|
| `atoi()` misuse | Numeric input is trivially reversible from the binary |
| Hardcoded secrets | Constants in binaries are always recoverable via static analysis |
| setuid attack surface | Logic flaws in setuid binaries directly lead to privilege escalation |








================ V1 

## Enumeration

By enumerating the home directory we found an ELF 32-bits binary:
```txt
   $ file level0 
   level0: setuid ELF 32-bit LSB executable, Intel 80386, version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.24, BuildID[sha1]=0x85cf4024dbe79c7ccf4f30e7c601a356ce04f412, not stripped
```

## Investigation

This programs takes an input and displays the following messages, if not it segfaults:
```bash
   $ ./level0 
   Segmentation fault (core dumped)
   $ ./level0 eded
   No !
```

It seems like the program is requiring a specific input.

For further inspection, we disassemble the main function:
```
   0x08048ed1 <+17>:    mov    %eax,(%esp)
   0x08048ed4 <+20>:    call   0x8049710 <atoi>
   0x08048ed9 <+25>:    cmp    $0x1a7,%eax
   0x08048ede <+30>:    jne    0x8048f58 <main+152>
```

This program calls atoi() on our input, compares the result with the value `0x1a7` and jump if the comparision is not equal.
If `0x1a7` is not equal to EAX the program jumps to the end of the main.

If we avoid this jump, the program continues until this call to execve():
```
   0x08048f46 <+134>:   mov    %eax,0x4(%esp)
   0x08048f4a <+138>:   movl   $0x80c5348,(%esp)
   0x08048f51 <+145>:   call   0x8054640 <execv>
```
And executes a shell `/bin/sh`:
```
   (gdb) x/s 0x80c5348
   0x80c5348:       "/bin/sh"
```

## Vulnerability 

This program presents an authentication bypass via weak integer comparison (logic flaw).

## Exploitation

The value `0x1a7` is equal to 423 in decimal.

We provide `423` as input to our program to get a shell with elevated privilege:
```bash
   level0@RainFall:~$ ./level0 423
   $ whoami
   level1
```

Finally, we use the command given in the subject to find the password to level1:
```bash
   $ cat /home/user/level1/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   $ exit
   level0@RainFall:~$ su level1
   Password:
```


