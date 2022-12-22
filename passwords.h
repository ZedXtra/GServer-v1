#ifndef PASSWORDS_H
#define PASSWORDS_H

#include "pascalstrings.h"

JString encryptpassword(const JString& str);
bool comparepassword(const JString& pass, const JString& teststr);

#endif // PASSWORDS_H
