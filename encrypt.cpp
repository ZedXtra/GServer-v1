#define _XOPEN_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
 

char * encrypt(char *str) {
  static char *c=
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
  char s[2];
  s[0]= c[rand() % (int)strlen(c)],
  s[1]= c[rand() % (int)strlen(c)];
  return crypt(str,s);
}   

main(int argc,char **argv)
{
  //Recoded so it does not crash when no argument is given.
  // - Mister Bubbles
  if (argv[1] != NULL)
    printf("%s\n",encrypt(argv[1]));
  else
    printf("Usage: encrypt [text]\n");
}
