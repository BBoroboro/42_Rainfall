## Code in ASM

```
(gdb) disas main
Dump of assembler code for function main:
   0x0804847c <+0>:     push   %ebp
   0x0804847d <+1>:     mov    %esp,%ebp
   0x0804847f <+3>:     and    $0xfffffff0,%esp
   0x08048482 <+6>:     sub    $0x20,%esp
   0x08048485 <+9>:     movl   $0x40,(%esp)
   0x0804848c <+16>:    call   0x8048350 <malloc@plt>
   0x08048491 <+21>:    mov    %eax,0x1c(%esp)
   0x08048495 <+25>:    movl   $0x4,(%esp)
   0x0804849c <+32>:    call   0x8048350 <malloc@plt>
   0x080484a1 <+37>:    mov    %eax,0x18(%esp)
   0x080484a5 <+41>:    mov    $0x8048468,%edx
   0x080484aa <+46>:    mov    0x18(%esp),%eax
   0x080484ae <+50>:    mov    %edx,(%eax)
   0x080484b0 <+52>:    mov    0xc(%ebp),%eax
   0x080484b3 <+55>:    add    $0x4,%eax
   0x080484b6 <+58>:    mov    (%eax),%eax
   0x080484b8 <+60>:    mov    %eax,%edx
   0x080484ba <+62>:    mov    0x1c(%esp),%eax
   0x080484be <+66>:    mov    %edx,0x4(%esp)
   0x080484c2 <+70>:    mov    %eax,(%esp)
   0x080484c5 <+73>:    call   0x8048340 <strcpy@plt>
   0x080484ca <+78>:    mov    0x18(%esp),%eax
   0x080484ce <+82>:    mov    (%eax),%eax
   0x080484d0 <+84>:    call   *%eax
   0x080484d2 <+86>:    leave  
   0x080484d3 <+87>:    ret    
End of assembler dump.
```
```bash
   level6@RainFall:~$ objdump -D ./level6 
   [...]
   08048454 <n>:
   8048454:       55                      push   %ebp
   8048455:       89 e5                   mov    %esp,%ebp
   8048457:       83 ec 18                sub    $0x18,%esp
   804845a:       c7 04 24 b0 85 04 08    movl   $0x80485b0,(%esp)
   8048461:       e8 0a ff ff ff          call   8048370 <system@plt>
   8048466:       c9                      leave  
   8048467:       c3                      ret    

   08048468 <m>:
   8048468:       55                      push   %ebp
   8048469:       89 e5                   mov    %esp,%ebp
   804846b:       83 ec 18                sub    $0x18,%esp
   804846e:       c7 04 24 d1 85 04 08    movl   $0x80485d1,(%esp)
   8048475:       e8 e6 fe ff ff          call   8048360 <puts@plt>
   804847a:       c9                      leave  
   804847b:       c3                      ret    
   [...]
```