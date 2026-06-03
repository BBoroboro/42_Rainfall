# Rainfall

## Overview

Rainfall is a reverse engineering and binary exploitation training project focused on analyzing vulnerable ELF binaries in a Linux environment.
The objective is to identify security vulnerabilities, understand program behavior at assembly level, and develop working exploits to gain control over execution flow.

This project is part of a structured learning path in system-level security and exploitation techniques.

---

## Objectives

* Perform static and dynamic analysis of ELF binaries
* Identify memory corruption vulnerabilities
* Exploit typical low-level vulnerabilities such as:

  * Buffer overflows
  * Format string vulnerabilities
  * Basic control-flow hijacking
* Develop reliable exploitation strategies
* Gain familiarity with Linux debugging and reverse engineering tools

---

## Methodology

Each binary is approached using a structured reverse engineering workflow:

1. **Static analysis**

   * Identify program structure using tools like Ghidra
   * Analyze functions, strings, and control flow

2. **Dynamic analysis**

   * Execute binaries under controlled conditions
   * Use debuggers (GDB + pwndbg) to inspect memory and registers

3. **Vulnerability identification**

   * Locate unsafe memory operations
   * Identify potential control-flow or data corruption vectors

4. **Exploit development**

   * Build proof-of-concept exploits
   * Use techniques such as:

     * Stack overflow exploitation
     * Shellcode injection (when applicable)
     * Return address manipulation

---

## Tools Used

* GDB / pwndbg
* Ghidra
* objdump / readelf
* strace / ltrace
* Linux (Kali / Debian environments)

---

## Key Concepts Covered

* Stack memory layout
* Calling conventions (x86 / x64)
* Function prologues and epilogues
* Buffer overflow exploitation
* Basic shellcode execution
* Debugging and runtime analysis

---

## Learning Outcome

Through this project, I developed a practical understanding of:

* How memory corruption vulnerabilities occur in real binaries
* How execution flow can be manipulated at low level
* How to transition from static analysis to working exploit
* How debugging tools are used in real exploitation scenarios

---

## To connect

CMD: ssh level0@192.168.186.132 -p 4242
PASS: level0