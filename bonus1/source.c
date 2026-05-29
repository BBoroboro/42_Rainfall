int main(int ac,char **av)

{
  int exit;
  char local_3c [40];
  int local_14;

    // must be param 1 not 2 here i think there is a mistake
//   local_14 = atoi((char *)(param_2 + 4)); // copy first 4 bytes of param 2 to local_14
local_14 = atoi(av[1]);
  if (local_14 < 10) {
    memcpy(local_3c,(av[2]),local_14 * 4); // dest = loc3 then in loc3 we have 
    if (local_14 == 0x574f4c46) {
      execl("/bin/sh","sh",0);
    }
    exit = 0;
  }
  else {
    exit = 1;
  }
  return exit;
}

