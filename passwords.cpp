#define _XOPEN_SOURCE
#include <stdlib.h>
#include <unistd.h>

#include "passwords.h"

char passc[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/";

JString encryptpassword(const JString& str) {
  return str;
/*
  char s[2];
  s[0]= passc[rand() % 64],
  s[1]= passc[rand() % 64];
  return crypt(Copy(str,1,8).text(),s);
*/
}

bool comparepassword(const JString& pass, const JString& teststr) {
  if (Length(pass)<2) return false;
  return (pass==teststr);
/*
  return (pass==crypt(Copy(teststr,1,8).text(),Copy(pass,1,2).text()));
*/
}
