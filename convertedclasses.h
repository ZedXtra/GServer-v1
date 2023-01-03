#ifndef CONVERTEDCLASSES_H 
#define CONVERTEDCLASSES_H 
 
extern int cstream; 
extern int clist; 
extern int cstringlist; 
extern int gotbytes;
extern int sentbytes;
 
#include <netinet/in.h> 
#include "pascalstrings.h" 

extern char filesbuf[];

class TInteger {
 public:
  int value;
  TInteger(int val) { value = val; };
};

class TList {
 public:
  int count;
  void** buf;

  TList() { clist++; buf = NULL; count = 0; };
  ~TList() { Clear(); clist--;  };
 
  void Add(void* p); 
  void Insert(int index, void* p); 
  void Remove(void* p); 
  void Delete(int index); 
  int indexOf(const void* p); 
  void pack(); 
  void Clear(); 
  void Assign(TList* list);

  void*& operator[](int index); 
}; 
 
class TJStringList {
 private:
  JString** buf;
 public:
  int count;

  bool plainmem;
  char* buffer;
  unsigned int totalsize;

  TJStringList() { cstringlist++; buf = NULL; count = 0; plainmem = false; };
  ~TJStringList() { cstringlist--; Clear(); };

  void Add(const JString& str);
  void Delete(int index);
  void Remove(const JString& str);
  int indexOf(const JString& str);
  void Clear();
  void Assign(TJStringList* list);
  void SetCommaText(JString str);
  JString GetCommaText();
  JString getValue(JString str);
  void LoadFromFile(const JString& filename);
  void AddLine(const JString& addline);
  void SaveToFile(JString filename);
  void SetPlainMem();

  JString operator[](int index); 
}; 

class TStream {
 public:
  int Position;
  int Size;
  JString buf;

  TStream() { cstream++; Position = 0; Size = 0; };
  ~TStream() { cstream--; }
  void Write(JString str) { buf << str; Size = buf.length(); Position = Size; };
  int Read(void* Buffer, int Count); 
  JString readLine(); 
  void LoadFromFile(JString filename);
  void SaveToFile(JString filename);
};


// Socket errors 
enum { 
  TSOCKET_NOERROR, 
  TSOCKET_ERRCREATE, 
  TSOCKET_ERRCONNECT, 
  TSOCKET_ERRBIND, 
  TSOCKET_CLOSED
};

class TSocket {
 private:
  bool udp;

 public: 
  int sock; 
  int error; 
  bool block; 
  sockaddr_in udpsender; 
  unsigned int ip; 
 
  TSocket() { sock = error = ip = 0; block = udp = false; } 
  ~TSocket() { if (sock) close(sock); sock = 0; } 
 
  void Connect(const JString& server, int port, bool blocked); 
  void Bind(int port, bool block, bool udp); 
 
  JString Read(); 
  int Send(const JString& str);  
  TSocket* Accept(); 
  void Close(); 
  void SetNonBlocking(); 
  JString GetIP(); 
}; 
 
 
 
#endif // CONVERTEDCLASSES_H

