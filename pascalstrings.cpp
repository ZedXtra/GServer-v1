#include <string.h>
#include <ctype.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <stdio.h>
#include <unistd.h>
#include <ostream>
#include "pascalstrings.h"

static unsigned char dummy = 0; 
static char* dummystr = ""; 
int stringc = 0; 

//extern const char fileseparator;

JString::JString() 
  : buffer( NULL ) 
  , len( 0 )
  , tempstart( 0 )
{
  stringc++;
}

JString::JString( const JString& str )
  : buffer( NULL )
  , len( 0 )
  , tempstart( 0 )
{
  stringc++;
  operator <<(str);
}

JString::JString( int value )
  : buffer( NULL )
  , len( 0 )
  , tempstart( 0 )
{
  stringc++;
  operator <<(value);
}

JString::JString( const char* arr )
  : buffer( NULL )
  , len( 0 )
  , tempstart( 0 )
{
  stringc++;
  operator <<(arr);
}

JString::JString( char c )
  : buffer( NULL )
  , len( 0 )
  , tempstart( 0 )
{
  stringc++;
  operator <<(c);
}

JString::~JString() { 
  clear(); 
  stringc--; 
};  


void JString::clear() {
  if (buffer) {
    free(buffer);
    buffer = NULL;
  }
  len = 0;
  tempstart = 0;
} 
 
int JString::length() const{
  return len-tempstart; 
} 
 
const char* JString::text() const{
  if (buffer)
    return (char*)(buffer+tempstart);
  else
    return dummystr;
}
 
int JString::indexOf(char check) const { 
  int i; 
  for (i=tempstart; i<len; i++)
    if (check==buffer[i]) return i-tempstart; 
  return -1; 
} 
 
int JString::indexOf(const JString& str) const { 
  int i,j,sublen; 
  unsigned char firstchar; 
  bool ident;

  sublen = Length(str);
  if (sublen<=0) return -1;
  firstchar = str[1];
  for (i = tempstart; i<=len-sublen; i++) {
    if (buffer[i]==firstchar) {
      ident = true;
      for (j = 1; j<sublen; j++) if (buffer[i+j]!=str[1+j]) {
        ident = false;
        break;
      }
      if (ident) return i-tempstart;
    }
  }
  return -1;
}


char JString::charAt(int i) const{
  if (i>=0 && i<length())
    return buffer[i+tempstart];
  else
    return 0;
}

JString JString::subJString(int begin) const{
  return subJString(begin, length());
}

JString JString::subJString(int begin, int end) const{
  if (begin<0) begin = 0;
  if (end>length()) end = length();
  JString data;

  int lent = end-begin;
  if (lent>0)
    data.addbuffer((char*)(buffer+tempstart+begin),lent);

  return data;
}

void JString::setTempStart(int index) {
  if (index<1) index = 1;
  if (index>length()+1) index = length()+1;
  tempstart += index-1;
}

void JString::resetTempStart() {
  tempstart = 0;
}


void JString::addbuffer(const char* text, int lent){
  if (!text || lent<=0 || lent>1000000) return;

  if (Assigned(buffer))
    buffer = (unsigned char*)realloc(buffer,len+lent+1);
  else
    buffer = (unsigned char*)malloc(len+lent+1);

  memcpy(buffer+len,text,lent);
  buffer[len+lent] = 0;

  len += lent;
}

JString& JString::operator <<(const JString& str){
  if (str.length()>0) addbuffer((char*)str.text(), str.length());
  return *this;
}

JString& JString::operator <<(const char* text){
  addbuffer(text, strlen(text));
  return *this;
}

JString& JString::operator <<(int value){
  char buf[20];
  sprintf(buf, "%d", value);
  addbuffer(buf, strlen(buf));
  return *this;
}

JString& JString::operator <<(unsigned int value){ 
  char buf[20]; 
  sprintf(buf, "%u", value); 
  addbuffer(buf, strlen(buf)); 
  return *this; 
} 

JString& JString::operator <<(double value){ 
  char buf[20]; 
  sprintf(buf, "%g", value); 
  addbuffer(buf, strlen(buf)); 
  return *this; 
} 


JString& JString::operator <<(char c){
  char buf[1] = {c};
  addbuffer(buf, 1);
  return *this;
}

unsigned char& JString::operator [](int index) {
  if (index>=1 && index<=length()) return buffer[tempstart+index-1];
  else return dummy;
}

unsigned char JString::operator [](int index) const {
  if (index>=1 && index<=length()) return buffer[tempstart+index-1];
  else return dummy;
}

bool compstr(const char* s1, const char* s2){
  if (s1 && s2) {
    return (strcmp(s1,s2)==0);
  } else if (!s1 && !s2)
    return true;
  else
    return false;
}

bool operator ==(const JString& s1, const JString& s2){
  return compstr(s1.text(), s2.text());
}

bool operator ==(const JString& s1, const char* s2){
  return compstr(s1.text(), s2);
}

bool operator ==(const char* s1, const JString& s2){
  return compstr(s1, s2.text());
}

bool operator !=(const JString& s1, const JString& s2){
  return !compstr(s1.text(), s2.text());
}

bool operator !=(const JString& s1, const char* s2){
  return !compstr(s1.text(), s2);
}

bool operator !=(const char* s1, const JString& s2){
  return !compstr(s1, s2.text());
}

bool operator >(const JString& s1, const JString& s2){
  if (s1.length()>s2.length()) return true;
  if (s1.length()<s2.length()) return false;

  int i;
  for (i = 0; i<s1.length(); i++)
    if (s1.charAt(i)>s2.charAt(i))
      return true;
    else if (s1.charAt(i)<s2.charAt(i))
      return false;

  return false;
}

JString operator +( const JString& str1, const JString& str2 ) {
  JString result( str1 );
  result << str2;
  return result;
}

JString operator +( const JString& str1, const char* str2 ) {
  JString result( str1 );
  result << str2;
  return result;
}

JString operator +( const JString& str1, int value ) {
  JString result( str1 );
  result << value;
  return result;
}

JString operator +( const JString& str1, unsigned int value ) { 
  JString result( str1 ); 
  result << value; 
  return result; 
} 

JString operator +( const JString& str1, double value ) { 
  JString result( str1 ); 
  result << value; 
  return result; 
} 

JString operator +( const JString& str1, char c ) {
  JString result( str1 );
  result << c;
  return result;
}

std::ostream& operator <<(std::ostream& out, const JString& str2) {
  return out << str2.text();
}

//--- new functions ---

int LastDelimiter(const JString& str1, const JString& str2) {
  int i, j;
  for (i = Length(str2); i>=1; i--) for (j = 1; j<=Length(str1); j++)
    if (str2[i]==str1[j]) return i;
  return 0;
}

JString LowerCase(const JString& str) {
  JString newstr(str);
  int len = newstr.length();
  for (int i=0; i<len; i++) newstr.buffer[i] = tolower(newstr.buffer[i]);
  return newstr;
}

JString Trim(const JString& str) {
//  return TrimLeft(TrimRight(str));

  // left side
  int i = 1;
  while (i<=str.length() && (str[i] & 0x7f)==32) i++;
  // right side
  int j = str.length();
  while (j>0 && (str[j] & 0x7f)==32) j--;

  return str.subJString(i-1,j);
}

JString TrimLeft(const JString& str) {
  int i = 1;
  while (i<=str.length() && (str[i] & 0x7f)==32) i++;
  return str.subJString(i-1,str.length());
}

JString TrimRight(const JString& str) {
  int i = str.length();
  while (i>0 && (str[i] & 0x7f)==32) i--;
  return str.subJString(0,i);
}

int strtoint(const JString& str) {
  if (Length(str)>0) {
    int res = atol(str.text());
    if (res==0 && str!="0") res = -1;
    return res;
  } else  return -1;
}

double strtofloat(const JString& str) {
  if (Length(str)>0) {
    double res = atof(str.text());
    if (res==0 && str!="0") res = -1;
    return res;
  } else return 0;
}

JString floattostr(double value) {
  JString data = (int)value;
  int i = (int)((value*10) - ((int)value*10));
  if (i>0) data << "." << i;
  return data;
}

JString inttostr(int value) {
  return (int)value;
}

/*
JString LowerCaseFilename(JString level) {
  int i;
  for (i=Length(level); i>=1; i--) {
    if (level[i] == fileseparator) return Copy(level, i+1, Length(level)-2);
  }
  return Trim(LowerCase(level));
}
*/

JString LowerCaseFilename(JString level) {
  int i;

  i = LastDelimiter(JString()+"/\\",level);
  if (i>0) level = Copy(level,i+1,Length(level)-i);
  return Trim(LowerCase(level));
}

bool FileExists(JString filename) {
  if (Length(filename)<=0) return false;

  struct stat filestat;
  if (stat(filename.text(),&filestat)==0 && S_ISREG(filestat.st_mode)) return true;
  return false;

//  ifstream in(filename.text());
//  if (in) {
//    in.close();
//    return true;
//  }
//  return false;

//  return (access(filename.text(),4)!=-1);
}

/*
JString ExtractFilepath(const JString& str) {
  int i;
  for (i=Length(str); i>=1; i--) {
    if (str[i] == fileseparator) return Copy(str, 1, i - 1);
  }
  return JString()+"";
}
*/

JString ExtractFilepath(const JString& str) {
  int i = LastDelimiter(JString()+"/\\",str);
  if (i>=0) return Copy(str,1,i);
  else return JString()+str;
}

bool iscorrect(const JString& str) {
  for (int i=1; i<=Length(str); i++)
    if (str[i]<20) return false;
  return true;
}

JString escaped39(const JString& str) {
  JString res;
  int len = Length(str);
  for (int i=1; i<=len; i++) {
    unsigned char c = str[i];

    if (c=='\\')
      res << "\\\\";
    else if (c=='\'')
      res << "\\'";
    else if (c>=30)
      res << (char)c;
  }
/*
  JString org = str;
  JString res = "";
  while (true) {
    int i = Pos('\\',org);
    if (i>0) {
      res << Copy(org,1,i-1) << "\\\\";
      org = Copy(org,i+1,Length(org)-i);
    } else {
      res << org;
      break;
    }
  }
  org = res;
  res = "";
  while (true) {
    int i = Pos('\'',org);
    if (i>0) {
      res << Copy(org,1,i-1) << "\\'";
      org = Copy(org,i+1,Length(org)-i);
    } else {
      res << org;
      break;
    }
  }*/
  return res;
}





