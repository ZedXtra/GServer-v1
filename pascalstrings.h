#ifndef PASCALSTRINGS_H 
#define PASCALSTRINGS_H 
 
#define true (0==0) 
#define false (0!=0) 
 
#define Length(a) (a).length() 
#define Pos(a,b) ((b).indexOf(a)+1) 
#define Copy(a,b,c) (a).subJString(b-1,b+c-1) 
#define Assigned(a) (a) 
 
#include <fstream> 

#include <iostream>
 
extern int stringc;

class JString {
  int tempstart;

 public:
  unsigned char *buffer;
  int len;

  JString();
  JString(const JString& str); 
  JString(const char* text); 
  JString(int value); 
  JString(char c); 
  ~JString(); 
 
  void addbuffer(const char*, int lent); 
  void clear(); 
  int length() const; 
  const char* text() const; 
  char charAt(int pos) const; 
  int indexOf(char check) const ; 
  int indexOf(const JString& str) const; 
  JString subJString(int begin) const; 
  JString subJString(int begin, int end) const;
  void setTempStart(int index);
  void resetTempStart();

  JString& operator =(const JString& str) { clear(); return operator <<(str); }; 
  JString& operator <<(const JString& str); 
  JString& operator <<(const char* text); 
  JString& operator <<(int value); 
  JString& operator <<(unsigned int value);  
  JString& operator <<(double value);  
  JString& operator <<(char c); 
  unsigned char& operator [](int index); 
  unsigned char operator[](int index) const; 
 
  friend bool operator ==(const JString& str1, const JString& str2); 
  friend bool operator ==(const JString& str1, const char* str2); 
  friend bool operator ==(const char* str1, const JString& str2);
  friend bool operator !=(const JString& str1, const JString& str2);
  friend bool operator !=(const JString& str1, const char* str2);
  friend bool operator !=(const char* str1, const JString& str2);
  friend bool operator >(const JString& str1, const JString& str2);

  friend JString operator +(const JString& str1,const JString& str2);
  friend JString operator +(const JString& str1,const char* str2);
  friend JString operator +(const JString& str1,int value);
  friend JString operator +(const JString& str1,unsigned int value); 
  friend JString operator +(const JString& str1,double value); 
  friend JString operator +(const JString& str1,char c);
};

std::ostream& operator <<(std::ostream& out, const JString& str2);


int LastDelimiter(const JString& str1, const JString& str2);
JString LowerCase(const JString& str);
JString Trim(const JString& str);
JString TrimLeft(const JString& str);
JString TrimRight(const JString& str);
int strtoint(const JString& str);
double strtofloat(const JString& str);
JString floattostr(double value);
JString inttostr(int value);
JString LowerCaseFilename(JString level);
bool FileExists(JString filename);
JString ExtractFilepath(const JString& str);
//bool Assigned(const void* p);
bool iscorrect(const JString& str);
JString escaped39(const JString& str);


#endif // PASCALJStringS_H

