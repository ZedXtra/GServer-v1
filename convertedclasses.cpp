#include <unistd.h>  
#include <stdlib.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netdb.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <signal.h>  
#include <stdio.h>  
#include <fcntl.h>  
#include <fstream>  
#include <math.h>  
#include <errno.h>  
#include <string.h>

#include "convertedclasses.h"
  
static void* dummy = 0;
int clist = 0;  
int cstringlist = 0;  
int cstream = 0;  
int gotbytes = 0;
int sentbytes = 0;

void TList::Add(void* p) {
  if (!Assigned(p)) return;

  buf = (void**)realloc(buf,(count+1)*sizeof(void*));
  buf[count] = p;

  count++;
}

void TList::Insert(int index, void* p) {
  if (!Assigned(p) || index<0 || index>count) return;

  buf = (void**)realloc(buf,(count+1)*sizeof(void*));
  if (count>index)
    memmove((void**)(buf+index+1),(void**)(buf+index),(count-index)*sizeof(void*));
  buf[index] = p;

  count++;
}

void TList::Remove(void* p) {
  while (true) {
    int i = indexOf(p);
    if (i<0) break;
    Delete(i);
  }
}

void TList::Delete(int index) {
  if (index<0 || index>=count) return;

  if (count>1) {
    if (count>index+1)
      memmove((void**)(buf+index),(void**)(buf+index+1),(count-index-1)*sizeof(void*));
    buf = (void**)realloc(buf,(count-1)*sizeof(void*));
    count--; 
  } else
    Clear();
}

int TList::indexOf(const void* p) {
  for (int i=0; i<count; i++) if (buf[i]==p) return i;
  return -1;
}

void TList::pack() {
  int i = 0;
  while (i<count) if (!Assigned((void*)buf[i])) Delete(i); else i++;
}

void TList::Clear() {
  if (buf) {
    free(buf);
    buf = NULL;
  }
  count = 0;
}

void TList::Assign(TList* list) {
  Clear();
  if (!Assigned(list) || list->count<=0) return;

  buf = (void**)malloc(list->count*sizeof(void*));
  memcpy(buf,list->buf,list->count*sizeof(void*));

  count = list->count;
}

void*& TList::operator [](int index) {
  if (index>=0 && index<count) return buf[index];
  else return dummy;
}

//--- TJStringList

void TJStringList::SetPlainMem() {
  if (count==0) {
    plainmem = true;
    buffer = NULL;
    totalsize = 0;
  }
}

void TJStringList::Add(const JString& str) {
  if (plainmem) {

    unsigned int len = str.length();
    unsigned int entrylen = len + sizeof(int);

    buffer = (char*)realloc(buffer,totalsize + entrylen);
    char* endoftext = buffer + totalsize - (unsigned int)(count*sizeof(int));
    if (count>0)
      memmove(endoftext+entrylen,endoftext,count*sizeof(int));

    memcpy(endoftext,str.text(),len);
    *((int*)(endoftext+len)) = (unsigned int)(endoftext - buffer);
    totalsize += entrylen;

  } else {

    buf = (JString**)realloc(buf,(count+1)*sizeof(JString*));
    buf[count] = new JString(str);
  }

  count++;
}

void TJStringList::Delete(int index) {
  if (index<0 || index>=count) return;

  if (count>1) {

    if (plainmem) {

      int* p = (int*)(buffer + totalsize - (unsigned int)((index+1)*sizeof(int)));
      char* text = buffer + *p;
      char* endoftext = buffer + totalsize - (unsigned int)(count*sizeof(int));
      unsigned int len = 0;
      if (index==count-1)
        len = endoftext-text;
      else
        len = *(p-1) - *p;
      unsigned int entrylen = len + sizeof(int);

      if (count>index+1) {
        int* p2 = p;
        for (int i=index+1; i<count; i++) *(--p2) -= len;
        memmove(text,text+len,(endoftext-(text+len))+(unsigned int)((count-(index+1))*sizeof(int)));
          // text + pointers before p
      }
      if (index>0)
        memmove((char*)p-len,(char*)(p+1),index*sizeof(int)); // pointers after p

      buffer = (char*)realloc(buffer,totalsize - entrylen);
      totalsize -= entrylen;

    } else {
      delete(buf[index]);
      if (count>index+1)
        memmove((JString**)(buf+index),(JString**)(buf+index+1),(count-index-1)*sizeof(JString*));
      buf = (JString**)realloc(buf,(count-1)*sizeof(JString*));
    }

    count--;

  } else
    Clear();
}

void TJStringList::Remove(const JString& str) {
  while (true) {
    int i = indexOf(str);
    if (i<0) break;
    Delete(i);
  }
}

int TJStringList::indexOf(const JString& str) {
  if (count>0) {
    if (plainmem) {

      int* p = (int*)(buffer + totalsize);
      int slen = str.length();
      for (int i=0; i<count; i++) {
        p--;

        char* text = buffer + *p;
        unsigned int len = 0;
        if (i==count-1)
          len = totalsize - count*sizeof(int) - *p;
        else
          len = *(p-1) - *p;
        if (len==slen && (len==0 || strncmp(str.text(),text,len)==0))
          return i;
      }

    } else
      for (int i=0; i<count; i++) if (*buf[i]==str) return i;
  }
  return -1;
}

void TJStringList::Clear() {
  if (plainmem) {
    if (buffer) {
      free(buffer);
      buffer = NULL;
    }
    totalsize = 0;
  } else {
    if (buf) {
      for (int i=0; i<count; i++) delete(buf[i]);
      free(buf);
      buf = NULL;
    }
  }
  count = 0;
}

void TJStringList::Assign(TJStringList* list) {
  Clear();
  if (!Assigned(list) || list->count<=0) return;

  if (plainmem) {
    if (list->plainmem) {
      totalsize = list->totalsize;
      buffer = (char*)malloc(totalsize);
      memcpy(buffer,list->buffer,totalsize);
    } else 
      for (int i=0; i<list->count; i++) Add((*list)[i]);
  } else {
    buf = (JString**)malloc(list->count*sizeof(JString*));
    for (int i=0; i<list->count; i++) buf[i] = new JString((*list)[i]);
  }
  count = list->count;
}

void TJStringList::SetCommaText(JString str) {
  Clear();
  while (str.length()>0) {
    int i = str.indexOf(',');
    if (i<0) break;
    Add(Trim(str.subJString(0,i)));
    str = Trim(str.subJString(i+1));
  }
  if (str.length()>0) Add(Trim(str));
}

JString TJStringList::GetCommaText() {
  JString res;
  for (int i=0; i<count; i++) {
    if (i>0) res << ",";
    if (plainmem)
      res << escaped39((*this)[i]);
    else
      res << escaped39(*buf[i]);
    if (Length(res)>60000) break;
  }
  if (Length(res)>65000) res = "";
  return res;
} 

JString TJStringList::operator [](int index) {
  if (index>=0 && index<count) {
    if (plainmem) {

      int* p = (int*)(buffer + totalsize - (unsigned int)((index+1)*sizeof(int)));
      char* text = buffer + *p;
      char* endoftext = buffer + totalsize - (unsigned int)(count*sizeof(int));
      unsigned int len = 0;
      if (index==count-1)
        len = endoftext-text;
      else
        len = *(p-1) - *p;

      JString str;
      str.addbuffer(text,len);
      return str;

    } else
      return JString(*buf[index]);
  } else
    return JString();
}

JString TJStringList::getValue(JString str) {
  str << "=";
  if (plainmem) {
    for (int i=0; i<count; i++) {
      JString cstr = (*this)[i];
      if (cstr.indexOf(str)==0)
        return cstr.subJString(Length(str));
    }
  } else {
    for (int i=0; i<count; i++)
      if (buf[i]->indexOf(str)==0)
        return buf[i]->subJString(Length(str));
  }
  return JString();
}

void TJStringList::LoadFromFile(const JString& filename) {
  Clear();
  if (FileExists(filename)) {
    TStream* data = new TStream();
    data->LoadFromFile(filename);

    while (true) {
      JString str = data->readLine();
      if (Length(str)<1 && data->Position>=data->Size-1) break;
      if (Length(str)<1) continue;
      Add(str);
    }
    delete(data);
  }
}

void TJStringList::SaveToFile(JString filename) {
  if (Length(filename)<=0) return;
  if (FileExists(filename)) unlink(filename.text());

  std::ofstream out(filename.text(), std::ios::out | std::ios::binary);
  char endc = '\n';
  if (out) {
    for (int i=0; i<count; i++) {
      if (plainmem) {
        JString cstr = (*this)[i];
        out.write(cstr.text(),cstr.length());
      } else
        out.write(buf[i]->text(),buf[i]->length());
      if (i<count-1) out.put(endc);
    }
    out.close();
  }
}

//--- TStream ---

int TStream::Read(void* Buffer, int Count) {
  if (Count<0 || Length(buf)<=0) return 0;
  if ((Size-Position)<Count) Count = Size-Position;
  memcpy(Buffer,(char*)(buf.text()+Position),Count);
  Position += Count;
  return Count;
}

char filesbuf[200000];

void TStream::LoadFromFile(JString filename) {
  buf = "";
  Size = 0;
  Position = 0;
  if (!FileExists(filename)) return;

  std::ifstream in(filename.text(), std::ios::in | std::ios::binary);
  if (in) {
    in.seekg(0,std::ios::end);
    long insize = in.tellg();
    in.seekg(0,std::ios::beg);
    JString lfile = LowerCaseFilename(filename);

    if (lfile=="config/weapons.txt" || insize<64000) {
      buf.buffer = (unsigned char*)malloc(insize+1);
      in.read((char*)buf.buffer,insize);
      buf.buffer[insize] = 0;
      buf.len = insize;
      Size = insize;
    }

    in.close();
  }
}

void TStream::SaveToFile(JString filename) {
  if (Length(filename)<=0) return;
  if (FileExists(filename)) unlink(filename.text());

  std::ofstream out(filename.text(), std::ios::out | std::ios::binary);
  if (out) {
    out.write(buf.text(),buf.length());
    out.close();
  }
}

JString TStream::readLine() {
  JString str;

  if (Size<=0 || Position>=Size || Length(buf)<=0)
      return str;

/*
  char* ptr = (char*)buf.text() + Position;
  char* ptr2 = strchr(ptr,'\n');
  if (ptr2) {
    Position += ptr2-ptr+1;
    if (ptr2>ptr && *(ptr2-1)=='\r') ptr2--;
    str.addbuffer(ptr,ptr2-ptr);
  } else {
    str.addbuffer(ptr,Size-Position);
    Position = Size;
  }
*/
  char* ptr = (char*)buf.text() + (unsigned int)Position;
  char* ptr2 = ptr;
  int i = Position;
  while (i<Size && *ptr2!='\n') { i++,ptr2++; }
  if (i>Position && *(ptr2-1)=='\r') ptr2--;
  str.addbuffer(ptr,ptr2-ptr);
  Position = i+1;
  if (Position>Size) Position = Size;

  return str;
}

//--- TSocket ---


void TSocket::Connect(const JString& server, int port, bool blocked) {
  sockaddr_in addr;
  long ipaddr;
  hostent* host;
  block = blocked;

  if (Length(server)<=0 || (sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1) {
    sock = 0;
    error = TSOCKET_ERRCREATE;
    return;
  }
  ipaddr = inet_addr(server.text());
  if (ipaddr==-1) {
    host = gethostbyname(server.text());
    if (!host) {
      Close();
      error = TSOCKET_ERRCREATE;
      return;
    }
    ipaddr = ((long*)host->h_addr_list)[0];
    delete(host);
  }
  ip = ipaddr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ipaddr;
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))==-1) {
    Close();
    error = TSOCKET_ERRCONNECT;
    return;
  }
  if (!block)
    SetNonBlocking();
}

void TSocket::Bind(int port, bool block, bool udp) {  
  sockaddr_in addr;   
  this->block = block;  
  this->udp = udp;  
   
  if (!udp) sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  
  else sock = socket(AF_INET,SOCK_DGRAM,0);  
  
  if (sock==-1) {   
    sock = 0;  
    error = TSOCKET_ERRCREATE;  
    return;  
  }   
  
  int x = 1;  
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&x,sizeof(x));  
  setsockopt(sock,IPPROTO_TCP,1,(char*)&x,sizeof(x)); // naggle-algorithm delay off

  addr.sin_family = AF_INET;   
  addr.sin_port = htons(port);   
  addr.sin_addr.s_addr = INADDR_ANY;   
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))==-1) {   
    Close();  
    error = TSOCKET_ERRBIND;  
    return;   
  }  
  if (!udp) listen(sock,10);  
  if (!block)   
    SetNonBlocking();  
}  
  
JString TSocket::Read() {  
  if (!sock) return JString();  
  
  JString buffer;  
  while (true) {  
    if (!block || buffer.length()>0) {  
      fd_set set;  
      FD_ZERO(&set);  
      FD_SET(sock,&set);  
      struct timeval tm;  
      tm.tv_sec = 0;  
      tm.tv_usec = 0;  
      select(sock+1,&set,NULL,NULL,&tm);  
      if (!FD_ISSET(sock,&set)) return buffer;   
    }  

#ifdef OS_WIN32  
    int addr_len;
#else
    unsigned int addr_len;
#endif

    addr_len = sizeof(struct sockaddr);  
    int bytesread;  
    if (udp) bytesread = recvfrom(sock,filesbuf,8192,0,(struct sockaddr *)&udpsender,&addr_len);  
    else bytesread = recv(sock,filesbuf,8192,0);  
  
    if (bytesread>0) {  
      buffer.addbuffer(filesbuf,bytesread);  
      gotbytes += bytesread;
    } else if (bytesread==-1) {  
      if (errno!=EWOULDBLOCK && errno!=EMSGSIZE && errno!=EINTR) Close();  
      if (!sock || !block) return buffer;  
    } else if (bytesread==0) {  
      Close();
      return buffer;
    }
  }
}

int TSocket::Send(const JString& str) {
  if (!sock || Length(str)<=0) return 0;
  int bytessent = -1;

  if (!block) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock,&set);
    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = 0;
    select(sock+1,NULL,&set,NULL,&tm);
    if (!FD_ISSET(sock,&set)) return bytessent;
  }

  if (udp) bytessent = sendto(sock,str.text(),str.length(),0,
    (struct sockaddr *)&udpsender,sizeof(struct sockaddr));
  else bytessent = send(sock,str.text(),str.length(),0);
  if (bytessent>0) sentbytes += bytessent;
  
  if (bytessent==0) Close();  
  return bytessent;  
}  
   
TSocket* TSocket::Accept() {  
  if (sock) {  
    sockaddr_in addr;  

#ifdef OS_WIN32  
    int addr_len;
#else
    unsigned int addr_len;
#endif

    addr_len = sizeof(addr);  
    int newsock = accept(sock,(struct sockaddr*)&addr,&addr_len);  
    if (newsock!=-1) {  
      int x = 1;   
      setsockopt(newsock,IPPROTO_TCP,1,(char*)&x,sizeof(x)); // nagle-algorithm delay off  

      TSocket* newsocket = new TSocket();
      newsocket->sock = newsock;
#ifdef SunOS  
      newsocket->ip = addr.sin_addr.S_un.S_addr;  
#else  
      newsocket->ip = addr.sin_addr.s_addr;  
#endif  
      newsocket->block = block;  
      if (!block)   
        newsocket->SetNonBlocking();  
      return newsocket;  
    }  
  }  
  return NULL;  
}  
  
void TSocket::Close() {  
  if (!sock) return;  
  close(sock);  
  sock = 0;  
  error = TSOCKET_CLOSED;   
}  
  
void TSocket::SetNonBlocking() {  
  block = false;  
//  unsigned long nonblock = 1;  
//  ioctl(sock,FIONBIO,&nonblock);   
  fcntl(sock, F_SETFL, O_NONBLOCK);   
}  
  
JString TSocket::GetIP() {  
  in_addr addr;  
#ifdef SunOS  
  addr.S_un.S_addr = ip;  
#else  
  addr.s_addr = ip;  
#endif  
  return inet_ntoa(addr);  
}  
  
