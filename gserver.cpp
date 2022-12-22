#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/param.h>
#include <locale.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "cfg.h"
#include "gserver.h"
#include "passwords.h"

// ADDED: To detect windows or linux OS
#include "os_detect.h"

// Advanced commands
//#define ALLOW_SHUTDOWN   // Allow any RC admin to shutdown server

int cplayer = 0;
int cplayermodify = 0;
int caccounts = 0;
//Guest accounts will be used when a player connects with an incorrect username. The account will be reserved to the player's IP address while the server is running. Restarting the server will clear IP reservation.
JString guestaccounts[30] = {"Guest0", "Guest1", "Guest2", "Guest3", "Guest4", "Guest5", "Guest6", "Guest7", "Guest8", "Guest9",
					  "ExtraGuest0", "ExtraGuest1", "ExtraGuest2", "ExtraGuest3", "ExtraGuest4", "ExtraGuest5", "ExtraGuest6", "ExtraGuest7", "ExtraGuest8", "ExtraGuest9",
					  "UnexpectedGuest0", "UnexpectedGuest1", "UnexpectedGuest2", "UnexpectedGuest3", "UnexpectedGuest4", "UnexpectedGuest5", "UnexpectedGuest6", "UnexpectedGuest7", "UnexpectedGuest8", "UnexpectedGuest0"};
int guestips[30] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};					  
					  
void RefindAllSubdirs(JString path);
void signalhandler(int sig);
unsigned int starttime_global;

TServerFrame* ServerFrame;
TList* weapons = new TList();

JString applicationdir;
JString curdir;
unsigned int curtime = 0;
int respawntime = 10;
int apincrementtime = 1200;
#ifdef NEWWORLD
  JString startlevel = JString("tespand2.nw");
  double startx = 30.5;
  double starty = 50;
#else
  JString startlevel = JString("unstickme.graal");
  double startx = 29.5;
  double starty = 30;
#endif

#ifdef OS_WIN32
  const char fileseparator = '\\';
#else
  const char fileseparator = '/';
#endif
extern void UpdateStats(JString ouser,JString tuser,JString online,JString muser);
int maxnoaction = 2147483647; // 1200; in secs, time without actions till the player will be disconnected
int maxnodata = 600; // 300; in secs, time without verification data till the player will be disconnected
int changenicktimeout = 10;
int changecolorstimeout = 10;
JString startmesfile = JString()+"startmessage.rtf";
JString startmessagespecial = "";
bool allowoutjail = true;
int matiadaynightcounter = 30;
int matiahalfday = 1800;
//int maxplayersonserver = 0;

TJStringList* itemnames = new TJStringList();

TJStringList* subdirs = new TJStringList();

int npcidbufsize = 64000;
unsigned char* npcidused = new unsigned char[npcidbufsize];
int playeridbufsize = 4096;
unsigned char* playeridused = new unsigned char[playeridbufsize];

bool sendtoothers[] = {
  false,false,true ,false,false,
#ifdef NEWWORLD
  false,false,false,false,true ,
#else
  false,false,false,true ,true ,
#endif
  true ,false,false,true ,false,
  false,false,false,false,false,
  false,true ,false,false,true ,
  false,false,false,false,false,
  true ,true ,true ,true ,false,
  };
bool sendtoadmins[] = {
  false,false,false,false,false,
  false,false,false,false,false,
  false,true ,false,false,false,
  false,false,false,true ,false,
  true ,false,false,false,false,
  false,false,false,false,false,
  true ,false,true ,false,true ,
  };
bool sendinit[] = {
  false,true ,true ,true ,true ,
#ifdef NEWWORLD
  true ,true ,true ,false,true ,
#else
  true ,true ,true ,true ,true ,
#endif
  true ,true ,false,true ,true ,
  true ,true ,false,true ,false,
  false,true ,true ,true ,false,
  false,true ,false,false,false,
  false,false,true ,false,false,
  };
bool sendothersinit[] = {
  true ,false,false,false,false,
#ifdef NEWWORLD
  false,false,false,false,true ,
#else
  false,false,false,true ,true ,
#endif
  false,true ,false,true ,false,
  true ,true ,false,true ,false,
  true ,false,false,false,false,
  false,false,false,false,false,
  true ,true ,false,true ,true ,
  };
bool canlevel1[] = {
  false,true ,true ,true ,true ,
#ifdef NEWWORLD
  true ,true ,true ,false,true ,
#else
  true ,true ,true ,true ,true ,
#endif
  true ,true ,false,true ,false,
  true ,true ,false,true ,false,
  true ,false,false,false,false,
  false,true ,false,false,false,
  false,false,true ,false,false,
  };
bool propsapply[] = {
  false,true ,true ,true ,true ,
#ifdef NEWWORLD
  true ,true ,true ,false,true ,
#else
  true ,true ,true ,true ,true ,
#endif
  true ,true ,false,true ,false,
  false,false,false,true ,false,
  false,true ,false,false,false,
  false,true ,false,false,false,
  false,false,true ,false,false,
};
bool sendrcprops[] = {
  false,true ,true ,true ,true ,
#ifdef NEWWORLD
  true ,true ,true ,false,true ,
#else
  true ,true ,true ,true ,true ,
#endif
  true ,true ,false,true ,false,
  true ,true ,false,true ,false,
  true ,true ,true ,false,false,
  true ,true ,true ,true ,true ,
  true ,false,true ,false,false,
  };


  
JString RemoveLineEnds(JString str) {  
  int i;
  
  // Replace all line ends with "§", which makes  
  // the resulting string easier to handle  
  while (true) {  
    i = Pos((char)13,str);  
    if (i==0) break;  
    str = Copy(str,1,i-1) + " " + Copy(str,i+1,Length(str)-i);  
  }  
  while (true) {  
    i = Pos((char)10,str);  
    if (i==0) break;  
    str = TrimRight(Copy(str,1,i-1))+"§"+Copy(str,i+1,Length(str)-i);  
  }  
  return str;  
}  
  
JString AddLineEnds(JString str) {  
  int i;  
  
  // Replace all "§" with line ends  
  while (true) {  
    i = Pos("§",str);  
    if (i==0) break;  
    str = Copy(str,1,i-1)+(char)(10)+Copy(str,i+1,Length(str)-i);  
  }  
  return str;
}  
  
JString AddFileSeparators(JString str) {  
  int i;  
  
  // Replace all "§" with file path separators  
  while (true) {  
    i = Pos("§",str);  
    if (i==0) break;  
    str = Copy(str,1,i-1)+fileseparator+Copy(str,i+1,Length(str)-i);  
  }  
  return str;  
}  
  
JString GetLevelFile(JString filename) {  
  int i;
  filename = LowerCaseFilename(filename);  
  if (Length(filename)<1) return JString();  
  
  // Just check if the file exists in the  
  // levels folder (or in subfolders if filename  
  // contains "§" characters which represents file path  
  // separators)  
  JString exactpath = curdir + AddFileSeparators(filename);  
  if (FileExists(exactpath)) return exactpath;  

  for (i=0; i<subdirs->count; i++) {
    JString exactpath = (*subdirs)[i] + fileseparator + filename;
    if (FileExists(exactpath)) {
      return exactpath;
    }
  }

  return JString();  
}
  
bool LevelFileExists(const JString& filename) {
  return (Length(GetLevelFile(filename))>0);
}  
  
JString RemoveExtension(JString gifname) {  
  int i;  
  
  gifname = LowerCaseFilename(gifname);  
  i = Pos('.',gifname);  
  if (i>0) gifname = Copy(gifname,1,i-1);  
  return gifname;  
}  
  
JString RemoveGifExtension(JString gifname) {  
  int i;  
  
  gifname = LowerCaseFilename(gifname);  
  i = Pos(".gif",gifname);  
  if (i>0) gifname = Copy(gifname,1,i-1);  
  return gifname;  
}  
  
int getGifNumber(JString gifname, const JString& leading) {  
  int len;  

  gifname = RemoveGifExtension(gifname);  
  len = Length(leading);  
  if ((len>=Length(gifname)) || (Pos(leading,gifname) != 1)) return -1;  
  gifname = Copy(gifname,len+1,Length(gifname)-len);  
  return strtoint(gifname);  
}  
  
unsigned int GetUTCFileModTime(JString filename) {  
  if (Length(filename)<=0) return 0;  
  
  struct stat filestat;  
  if (stat(filename.text(),&filestat)!=-1) return filestat.st_mtime;  
  return 0;  
}  
  
  
//--- TServerFrame ---  
  
void TServerFrame::startServer(const JString& port) {  
  levels = new TList();  
  players = new TList();  
  
  unsigned short portnum = (unsigned short)strtoint(port);  
  sock = new TSocket();  
  sock->Bind(portnum,false,false);  
  if (sock->error==TSOCKET_ERRCREATE) {
    std::cout << "socket exception: cannot create socket" << std::endl;  
    exit(0);  
  }  
  if (sock->error==TSOCKET_ERRBIND) {  
    std::cout << "Port address " << portnum << " is currently used by another programm. Try later!" << std::endl;  
    exit(0);  
  }  
  std::cout << "Server started. Port: " << portnum << std::endl;   
  toerrorlog("Server started.");  
  
  responsesock = new TSocket();  
  responsesock->Bind(14899,false,true);  
  if (!sock->sock) std::cout << "Couldn't bind the response socket" << std::endl;  
  else std::cout << "Response socket established at port 14899" << std::endl;  
  
  curdir = getDir()+"levels"+fileseparator;  
  savepropscounter = 60;  
  starttime = time(NULL);
  starttime_global = starttime;
//  ConvertAccounts();
  LoadWeapons("weapons.txt");

  LoadStartMessage();  
  serverflags = new TJStringList();
  serverflags->SetPlainMem();
  serverflags->LoadFromFile(getDir()+"data/serverflags.txt");  
  for (int i=serverflags->count-1; i>=0; i--)
    if (!iscorrect((*serverflags)[i])) serverflags->Delete(i);

/*  TJStringList* list = new TJStringList();
  list->LoadFromFile(getDir()+"data/maxplayers.txt");
  if (list->count>=1) maxplayersonserver = atoi((*list)[0].text());
  delete(list);*/
}  
  
void TServerFrame::LoadStartMessage() {  
  startmes = "";  
  JString filename = getDir()+startmesfile;  
  if (FileExists(filename)) {  
    TStream* stream = new TStream();  
    stream->LoadFromFile(filename);  
    startmes = RemoveLineEnds(stream->buf);  
    delete(stream);  
  }  
}  
  
TServerFrame::~TServerFrame() {  
  int i;  
  TServerPlayer* player;  
  
//  SaveHTMLStats(true);  
  for (i = 0; i<players->count; i++) {  
    player = (TServerPlayer*)(*players)[i];  
    if (Assigned(player)) {  
      player->SaveMyAccount();  
      player->SendData(DISMESSAGE,JString()+"This server has been shutdown! Oh no! Will we be back? Maybe! Check the Discord!");
      player->SendOutgoing();  
      player->sock->Close();  
    }  
  }  
  delete(sock);  
  delete(responsesock);  
}  
  
void TServerFrame::tolog(const JString& str) {  
  JString filename = getDir()+"logs/rclog.txt";  
  std::ofstream out(filename.text(),std::ios::out | std::ios::app);  
  if (out) {  
    time_t t = time(NULL);  
    out << "Time: " << ctime(&t);  
    out << str << std::endl;  
    out.close();  
  }  
}  
  
void TServerFrame::toerrorlog(JString str) {  
  time_t t = time(NULL);  
  str = JString() + "Time: " + ctime(&t) + str + "\n";  
  std::cout << str;  
  
  JString filename = getDir()+"logs/errorlog.txt";
  std::ofstream out(filename.text(),std::ios::out | std::ios::app);  
  if (out) {  
    out << str;  
    out.close();  
  }  
}  
  
JString TServerFrame::getDir() {  
  return applicationdir;   
}   
   
int TServerFrame::GetFreeNPCID() {  
   
  for (int i=0; i<npcidbufsize; i++) if (npcidused[i] != 255)  
      for (int j=0; j<8; j++) if (!(npcidused[i] & (1<<j))) {  
    npcidused[i] += (1<<j);  
    return (i<<3)+j+1;    
  }  
  return 0;  
}  
  
void TServerFrame::DeleteNPCID(int id) {  
  if (id>=1 && id<=npcidbufsize*8) {  
    id--;  
    int i = (id>>3), j = (id & 0x7);  
    if (npcidused[i] & (1<<j))
      npcidused[i] -= (1<<j);  
  }  
}  
  
int TServerFrame::GetFreePlayerID() {  
  
  for (int i=0; i<playeridbufsize; i++) if (playeridused[i] != 255)  
      for (int j=0; j<8; j++) if (!(playeridused[i] & (1<<j))) {  
    playeridused[i] += (1<<j);  
    return (i<<3)+j+1;  
  }  
  return 0;  
}  
  
void TServerFrame::DeletePlayerID(int id) {  
  if (id>=1 && id<=playeridbufsize*8) {  
    id--;  
    int i = (id>>3), j = (id & 0x7);  
    if (playeridused[i] & (1<<j))  
      playeridused[i] -= (1<<j);  
  }  
}  
  
void TServerFrame::CreatePlayer(TSocket* clientsock) {  
  int nextid;  
  TServerPlayer* player;
  bool isipbanned;  
  
  isipbanned = IsIPInList(clientsock,"",getDir()+"data/ipbanned.txt");  
  nextid = 0; 
  if (!isipbanned && players->count<75)  
    nextid = GetFreePlayerID();  
  
  player = new TServerPlayer(clientsock,nextid);  
  players->Add(player);  
  player->initialized = true;  
  if (isipbanned) {  
    player->id = 1;
    player->SendData(DISMESSAGE,JString()+"Your IP is on the ban list! What have you done? Check the Discord!");  
    player->id = 0;
    player->deleteme = true;  
  } else if (nextid==0) {  
    player->id = 1;  
    player->SendData(DISMESSAGE,JString()+"There are too many players on this server! How?");  
    player->id = 0;  
    player->deleteme = true;  
  } else  
    player->SendData(UNLIMITEDSIG,JString()+(char)(73+32)); // server signature  
  player->initialized = false;  
}  
  
TServerLevel* TServerFrame::LoadLevel(JString filename) {
  int i;  
  TServerLevel* level;  
  if (!LevelFileExists(filename)) return NULL;  
  
  filename = LowerCaseFilename(filename);  
  for (i = 0; i<levels->count; i++) {  
    level = (TServerLevel*)(*levels)[i];  
    if (level->plainfile==filename)  
      return level;  
  }  
  level = new TServerLevel(filename);  
  if (Length(level->filename)>0) {  
    levels->Add(level);  
    return level;  
  } else  
    delete(level);  
  return NULL;  
}  
  
void TServerFrame::DeletePlayer(TServerPlayer* player) {  
  if (!Assigned(player)) return;
  
  DeletePlayerID(player->id);  
  int i = players->indexOf(player);  
  if (Assigned(player) && (i>=0)) {  
    players->Delete(i);  
    player->SaveMyAccount();  
  
    if (Assigned(player->carrynpc)) {  
      TServerNPC* npc = player->carrynpc;  
      npc->x = player->x+0.5;  
      npc->y = player->y+1;  
      JString str;  
      str << (char)(NPCX+32) << npc->GetProperty(NPCX) << (char)(NPCY+32) << npc->GetProperty(NPCY);  
      npc->SetProperties(str,false);  
      if (!Assigned(npc->level)) DeleteNPC(npc);  
    }  
    if (Assigned(player->level)) {  
      if ((player->level->players->count>=2) && ((*player->level->players)[0]==player))  
        ((TServerPlayer*)(*player->level->players)[1])->SendData(ISLEADER,JString());  
      player->level->players->Remove(player);  
    }  
    for (i = 0; i<levels->count; i++) {  
      TServerLevel* level = (TServerLevel*)(*levels)[i];
      level->RemoveFromList(player);
    #ifdef SERVERSIDENPCS
      for (int j=0; j<level->npcs->count; j++) {
        TServerSideNPC* npc = (TServerSideNPC*)(*level->npcs)[j];
        npc->removeActions(player);
      }
    #endif
    }  
    for (i=destroyedlevels->count-1; i>=0; i--) {
      TDestroyedLevel* dlevel = (TDestroyedLevel*)(*destroyedlevels)[i];
      dlevel->RemoveFromList(player);
      if (!Assigned(dlevel->playerleaved) || dlevel->playerleaved->count<=0) {
        destroyedlevels->Delete(i);
        delete(dlevel);
      }
    }
    if (player->id>0 && player->initialized)  
        for (i = 0; i<players->count; i++) if (Assigned((*players)[i])) {  
      TServerPlayer* player2 = (TServerPlayer*)(*players)[i];  
      if (player2->loggedin()) {  
        if (player2->isrc) { 
          player2->SendData(SDELPLAYER,player->GetCodedID());
          if (player->isrc)
            player2->SendData(DRCLOG,"RC disconnected: "+player->accountname);
        } else
          player2->SendData(OTHERPLPROPS,player->GetCodedID()+(char)(51+32));  
      }  
    }  
  }  
}

void TServerFrame::RemoveDestroyedLevel(JString filename) {
  filename = LowerCaseFilename(filename);
  for (int j=0; j<destroyedlevels->count; j++) {
    TDestroyedLevel* dlevel = (TDestroyedLevel*)(*destroyedlevels)[j];
    if (Assigned(dlevel) && dlevel->plainfile==filename) {
      destroyedlevels->Delete(j);
      delete(dlevel);
      break;
    }
  }
}

void TServerFrame::UpdateLevel(TServerLevel* level) {
  if (!Assigned(level)) return;

#ifndef NEWWORLD
  if (level->players->count==0) {
    int j = levels->indexOf(level);
    delete(level);
    levels->Delete(j);
  } else {
#endif
    TList* playerlist = new TList();
    int j = 0;
    while (level->players->count > 0) {
      TServerPlayer* player = (TServerPlayer*)(*level->players)[0];
      playerlist->Add(player);
      player->leaveLevel();
      level->players->Remove(player);
    }
    level->Clear();
    level->Load();
    for (j = 0; j<playerlist->count; j++) {
      TServerPlayer* player = (TServerPlayer*)(*playerlist)[j];
      JString props = JString()+player->GetProperty(PLAYERX)+player->GetProperty(PLAYERY)+level->plainfile;
      player->SendData(PLAYERWARPED,props);
      player->enterLevel(level->plainfile,player->x,player->y,JString());

      // if player is dead, then bring him back to life
      if (player->power<=0) {
        player->status &= 0xF7; // clear 'dead' flag
        player->power = 0.5; // give at least a half heart
        props = JString()+(char)(STATUS+32)+player->GetProperty(STATUS)+
          (char)(CURPOWER+32)+player->GetProperty(CURPOWER);
        player->SetProperties(props);
        player->SendData(SPLAYERPROPS,props);
      }
    }
    delete(playerlist);
#ifndef NEWWORLD
  }
#endif
}

void TServerFrame::UpdateLevels(TJStringList* files) {
  if (!Assigned(files)) return;
  int j;
  for (int i = 0; i<files->count; i++) {
    JString filename = (*files)[i];
    if (LowerCase(filename)==startmesfile) {
      LoadStartMessage();
      continue;
    }

    JString npcsname;
    npcsname << getDir() << "levelnpcs/" << LowerCaseFilename(filename) << ".save";
    if (FileExists(npcsname)) unlink(npcsname.text());

    TServerLevel* level = NULL;
    for (j = 0; j<levels->count; j++) {
      TServerLevel* level2 = (TServerLevel*)(*levels)[j];
      if (level2->plainfile==filename) { level = level2; break; }
    }
    if (Assigned(level))
      UpdateLevel(level);
    else
      RemoveDestroyedLevel(filename);
  }
}

#ifdef NEWWORLD

void TServerFrame::UpdateNewLevels() {
  // Check if the level on hard disk is newer than the level in RAM
  for (int j=0; j<levels->count; j++) {
    TServerLevel* level = (TServerLevel*)(*levels)[j];
    if (Assigned(level)) {
      JString filename = GetLevelFile(level->plainfile);
      if (Length(filename)>0) {
        unsigned int modtime = GetUTCFileModTime(filename);
        if (modtime>level->modtime)
          UpdateLevel(level);
      }
    }
  }
}

#endif

void TServerFrame::DeleteNPC(TServerNPC* npc) {
  if (!Assigned(npc)) return;  
  JString data = npc->GetCodedID();  
  for (int i=0; i<players->count; i++) {   
    TServerPlayer* player = (TServerPlayer*)(*players)[i];   
    if (Assigned(player) && player->carrynpc==npc) player->carrynpc = NULL;   
    if (Assigned(player) && !player->isrc && player->loggedin() && 
        Assigned(npc->level) && npc->level->GetLeavingTimeEntry(player)>=0) 
      player->SendData(SDELNPC,data);   
  }   
  if (Assigned(npc->level)) {  
    for (int i=0; i<npc->level->players->count; i++) {
      TServerPlayer* player = (TServerPlayer*)(*npc->level->players)[i];   
      if (Assigned(player)) player->SendData(SDELNPC,data);   
    } 
    npc->level->npcs->Remove(npc);  
    npc->level->orgnpcs->Remove(npc);  
    npc->level = NULL;  
  }  
  for (int i=0; i<levels->count; i++) {  
    TServerLevel* level = (TServerLevel*)(*levels)[i];  
    if (Assigned(level)) {
      level->orgnpcs->Remove(npc);  
      level->npcs->Remove(npc);  
    }  
  }  
  delete(npc);  
}  
  
void TServerFrame::SendAddServerFlag(const JString& flag, TServerPlayer* from) {  
  int i;  
  TServerPlayer* player;  
  
  for (i=0; i<ServerFrame->players->count; i++) {  
    player = (TServerPlayer*)(*ServerFrame->players)[i];  
    if (Assigned(player) && player!=from && !player->isrc && player->loggedin())  
      player->SendData(28,flag);  
  }  
}  
  
void TServerFrame::SendDelServerFlag(const JString& flag, TServerPlayer* from) {  
  int i;  
  TServerPlayer* player;  
  
  for (i=0; i<ServerFrame->players->count; i++) {  
    player = (TServerPlayer*)(*ServerFrame->players)[i];  
    if (Assigned(player) && player!=from && !player->isrc && player->loggedin())  
      player->SendData(31,flag);  
  }  
}  
  
TServerPlayer* TServerFrame::GetPlayerForAccount(const JString& accname) {
  if (Length(accname)<=0) return NULL;

  for (int i=0; i<players->count; i++) {
    TServerPlayer* player = (TServerPlayer*)(*players)[i];
    if (Assigned(player) && !player->isrc && player->accountname==accname)
      return player;
  }
  return NULL;
}

void TServerFrame::SendRCLog(const JString& logtext) {
  if (Length(logtext)<=0) return;

  for (int i=0; i<players->count; i++) {
    TServerPlayer* player = (TServerPlayer*)(*players)[i];
    if (Assigned(player) && player->loggedin() && player->isrc)
      player->SendData(DRCLOG,logtext);
  }
}

void TServerFrame::SendPrivRCLog(TServerPlayer *player, const JString& logtext) {
  if (Length(logtext)<=0) return;
    if (Assigned(player) && player->loggedin() && player->isrc)
    player->SendData(DRCLOG,logtext);
}

void TServerFrame::SendOthersRCLog(TServerPlayer *playerme, const JString& logtext) {
  if (Length(logtext)<=0) return;

  for (int i=0; i<players->count; i++) {
    TServerPlayer* player = (TServerPlayer*)(*players)[i];
    if (Assigned(player) && player->loggedin() && player->isrc && (player != playerme))
      player->SendData(DRCLOG,logtext);
  }
}
  
//--- TServerPlayer ---  
   
TServerPlayer::TServerPlayer(TSocket* clientsock, int id) {  
  deleteme = false;  
  initialized = false;  
  isrc = false;  
  
  cplayer++;  
  sock = clientsock;  
  ver = 0;  
  adminlevel = 0;  
  firstlevel = true;  
  firstdata = true;  
  accountname = "";  
  savewait = 30;  
  nomovementcounter = 0;  
  noactioncounter = 0;  
  nodatacounter = 0;  
  changenickcounter = changenicktimeout;  
  changecolorscounter = 0;  
  outbufferfullcounter = 0;  
  level = NULL;  
  packcount = 0;  
  weaponstosend = new TJStringList();  
  weaponstosend->SetPlainMem();
  imagestosend = new TJStringList();
  imagestosend->SetPlainMem();

  maxpower = 3;  
  power = 3;  
  rubins = 0;  
  darts = 5;  
  bombscount = 10;  
  glovepower = 1;  
  bombpower = 1;
#ifdef NEWWORLD
  ani = "idle";
#else
  swordpower = 1;
  swordgif = "sword1.gif";
#endif
  shieldpower = 1;  
  shieldgif = "shield1.gif";  
  shootgif << (char)(1+32);  
  headgif = "head0.gif";  
  colors[0] = 2;  
  colors[1] = 0;  
  colors[2] = 10;  
  colors[3] = 4;  
  colors[4] = 18;  
#ifdef NEWWORLD  
  colors[5] = 18;  
  colors[6] = 18;  
  colors[7] = 18;  
#endif  
  this->id = id;  
  x = startx;  
  y = starty;  
  spritenum = 2;  
#ifdef SERVERSIDENPCS  
  cursprite = spritenum/4;  
  dir = spritenum%4;  
#endif  
  status = 16;  
  carrysprite = -1;  
  levelname = startlevel;    horsebushes = 0;  
  carrynpc = NULL;  
  apcounter = apincrementtime;  
  magicpoints = 0;  
  kills = 0;
  deaths = 0;   
  onlinesecs = 0;  
  udpport = 0;  
  alignment = 50;  
  additionalflags = 0;
 // bank = 0;  
  
  actionflags = new TJStringList();  
  openedchests = new TJStringList();  
  myweapons = new TJStringList();
  actionflags->SetPlainMem();
  openedchests->SetPlainMem();
  myweapons->SetPlainMem();
}
  
TServerPlayer::~TServerPlayer() {  
  cplayer--;  
  ServerFrame->DeletePlayer(this);  
  delete(actionflags);  
  delete(openedchests);  
  delete(myweapons);  
  delete(sock);  
  delete(weaponstosend);  
  delete(imagestosend);  
}  
  
bool TServerPlayer::loggedin() {  
  return (id>0 && initialized && !deleteme);  
}  
  
  
void TServerPlayer::ClientReceive() {  
  if (!sock || !sock->sock) return;  
  buffer << sock->Read();  
  if (sock->error!=TSOCKET_NOERROR) {  
    deleteme = true;  
    return;  
  }  
  if (Length(buffer)>=128*1024) {  
    JString out = "Client "+accountname+" has sent to much data (input buffer >=128k)";  
    ServerFrame->toerrorlog(out);  
    SendData(DISMESSAGE,JString()+"Your client has sent to much data (>=128k in the input buffer)");  
    deleteme = true;  
    return;  
  }  
  while (buffer.length()>=2) {  
    int len = (((unsigned int)buffer[1]) << 8) + ((unsigned int)buffer[2]);  
    if (len<0) {  
      JString out = "Client "+accountname+" has sent a package with length "+len;  
      ServerFrame->toerrorlog(out);  
      SendData(DISMESSAGE,JString()+"Your client has sent a wrong data package length.");  
      deleteme = true;  
      return;  
    }  
    if (buffer.length()>=2+len) {  
      JString p = DecompressBuf(Copy(buffer,3,len));  
      if (p.length()<=0) {  
        JString out = "Client "+accountname+" has sent an empty package (compressed length: "+len+")";  
        ServerFrame->toerrorlog(out);  
        SendData(DISMESSAGE,JString()+"Your client has sent an empty data package.");  
        deleteme = true;  
        return;  
      }  
      while (true) {  
        int index = Pos((char)(10),p);  
        if ((index==0) || (index>p.length())) break;  
        packcount++;  
        parse(Copy(p,1,index-1));  
        if (deleteme) return;  
        p = Copy(p,index+1,Length(p)-index);  
      }  
      buffer = Copy(buffer,3+len,Length(buffer)-2-len);  
    }  
    else  
      break;  
  }  
}  
  
void TServerPlayer::AddRowToComp() {  
  if (rowout.length()>0) {  
    JString p = CompressBuf(rowout);  
    int len = p.length();  
    compout << (char)((len >> 8) & 0xFF) << (char)(len & 0xFF) << p;  
    rowout.clear();  
  
    bool blocked = false;  
    if (compout.length()>=512*1024) blocked = true;  
  
    if (blocked || outbufferfullcounter>=60) {  
      int complen = compout.length();  
      compout = "";  
      outbufferfullcounter = 0;  
      JString out;  
      if (blocked) {  
        out << "The socket to client " << accountname << " is blocked (>512k to send)";  
        SendData(DISMESSAGE,JString()+"Too much data to sent to you (>512k).");  
      } else {  
        out << "The connection to client " << accountname <<  
          " was blocked for more than one minute (still " <<  
          complen << " bytes left)";  
        SendData(DISMESSAGE,JString()+  
          "The connection to you was blocked for more than one minute (still "+  
          complen+" bytes to sent to you)");  
      }  
      ServerFrame->toerrorlog(out);  
      deleteme = true;  
      return;  
    }  
  }  
}  
  
void TServerPlayer::SendData(int id, const JString& str) {  
  if (!loggedin()) return;  
  rowout << (char)(id+32) << str << (char)(10);  
  if (Length(rowout)>=4*1024) AddRowToComp();  
}  
  
void TServerPlayer::SendOutgoing() {  
  if (sock && sock->sock) {  
    if (rowout.length()>0) AddRowToComp();  
    while (compout.length()>0) {  
      int len = 1024;  
      if (compout.length()<len) len = compout.length();  
      int bytessent = sock->Send(compout.subJString(0,len));  
      if (sock->error!=TSOCKET_NOERROR) {  
        JString out = "Error while sending data to client "+accountname;  
        ServerFrame->toerrorlog(out);  
        deleteme = true;  
        return;  
      }  
      if (bytessent>0) compout = compout.subJString(bytessent);  
      else return;  
    }  
  }  
}  

bool TServerPlayer::isblocked() {
  return (Length(compout)>=64*1024);
}

void TServerPlayer::SendWeaponsAndImages() {
  while (weaponstosend->count>0) {
    if (isblocked()) return;

    bool hassent = false;  
    JString name = LowerCase(Trim((*weaponstosend)[0]));  
    weaponstosend->Delete(0);  
  
    int i = itemnames->indexOf(name);  
    if (i>=0) {  
      SendData(SADDDEFWEAPON,JString((char)(i+32)));  
      hassent = true;  
    }  
    if (!hassent) for (i=0; i<weapons->count; i++) {  
      TServerWeapon* weapon = (TServerWeapon*)(*weapons)[i];  
  
      if (Assigned(weapon) && name==LowerCase(weapon->name) && playerworld==weapon->world) {  
        if (Length(weapon->dataforplayer)>0)  
          SendData(SADDNPCWEAPON,weapon->dataforplayer);  
        else  
          SendData(SADDNPCWEAPON,weapon->fullstr);  
        hassent = true;
        break;  
      }  
    }  
    if (!hassent) for (i=0; i<weapons->count; i++) {  
      TServerWeapon* weapon = (TServerWeapon*)(*weapons)[i];  
      if (Assigned(weapon) && name==LowerCase(weapon->name) && Length(weapon->world)<=0) {  
        SendData(SADDNPCWEAPON,weapon->fullstr);  
        hassent = true;  
        break;  
      }  
    }  
  }  
  while (imagestosend->count>0) {  
    if (isblocked()) return;  
  
    bool hassent = false;  
    JString filename = (*imagestosend)[0];  
    imagestosend->Delete(0);  
    JString shortname = LowerCaseFilename(filename);  
    int i = LastDelimiter("§",shortname);  
    if (i>0) shortname = Copy(shortname,i+1,Length(shortname)-i);      
  
    if (LevelFileExists(filename)) {  
      TStream* stream = new TStream();  
      stream->LoadFromFile(GetLevelFile(filename));  
      int len = stream->Size;  
      if (len<64500) {  
        len = 1+1+Length(shortname)+len+1;  
        SendData(100,JString((char)(((len >> 14) & 0x7F)+32)) <<  
          (char)(((len >> 7) & 0x7F)+32) << (char)((len & 0x7F)+32));  
        SendData(102,JString((char)(Length(shortname)+32)) << shortname << stream->buf);  
        hassent = true;
      }  
      delete(stream);  
    }  
    if (!hassent) SendData(GIFFAILED,shortname);  
  }  
}  

bool TServerPlayer::isadminin(JString world) {
  if (adminworlds=="all") return true;

  if (Length(world)<=0) world << startworld;
  unsigned char sep = '§';
  if (Length(world)>0 && world[Length(world)]==sep)
    world = Copy(world,1,Length(world)-1);
  return (Pos(world,adminworlds)>0);
}

void TServerPlayer::parse(const JString& line) {  
  int id,i,j,k,x, npcid, len;  
  double cx,cy;  
  JString data,oldlevel,oldpos,props,props2,gifname;  
  TServerPlayer* player;  
  TServerAccount* caccount;  
  unsigned int upseconds;

  if (firstdata) {  
    SendAccount(line);  
    return;  
  }  
  
  if (Length(line)<1) return;  
  id = line[1]-32;  
  data = Copy(line,2,Length(line)-1);  
  switch (id) {  
    case LEVELWARP:  
    case MODLEVELWARP:  
      {  
        if (Length(data)<2) return;  
  
        // The player has sent a level warp message;  
        // First check if he has also sent the modification  
        // time which the level file has that is on the player's
        // hard disk  
        // If yes, then give it as parameter to the method enterLevel  
        // (if it is the modifcation time of the current level file,  
        // then the level board & links doesn't need to be sent again)  
        JString modtime;  
        if (id==MODLEVELWARP) {  
          if (Length(data)<7) return;  
          modtime = Copy(data,1,5);  
          data = Copy(data,6,Length(data)-5);  
        }  
  
        if (Assigned(level)) {  
          oldlevel = level->plainfile;  
          // Leave the old level
          double oldx = this->x, oldy = this->y;  
          leaveLevel();  
          JString newfile = Copy(data,3,Length(data)-2);  
  
          // Don't warp the player if he is in jail  
          if (adminlevel>=1 || allowoutjail || !LevelIsJail(oldlevel)) {  
  
            // When we are in a playerworld, then test if we  
            // must add the playerworld path to the new level  
            if (!LevelFileExists(newfile) && Length(playerworld)>0 &&  
                LevelFileExists(playerworld + newfile)) {  
              newfile = playerworld + newfile;  
              SendData(LEVELFAILED,JString());  
              SendData(PLAYERWARPED,Copy(data,1,2) << newfile);  
            }  
  
            // Enter the new level; move to the x/y position specified in  
            // data[1] and data[2] and use the sent modification time as
            // fourth parameter  
            enterLevel(newfile,(data[1]-32)/2,(data[2]-32)/2,  
              modtime);  
          } else  
            level = NULL;  
  
          // If the level loading failed, then move the player back  
          // to the old level  
          if (!Assigned(level)) {  
            SendData(LEVELFAILED,JString());  
            SendData(PLAYERWARPED,GetProperty(PLAYERX) << GetProperty(PLAYERY) << oldlevel);  
            enterLevel(oldlevel,oldx,oldy,JString());  
          }  
        }
        break;  
      }  
    case BOARDMODIFY:  
      {
        if (Assigned(level)) level->AddModify(this,data);
        break;  
      }  
    case PLAYERPROPS:  
      {  
        SetProperties(data);  
        break;  
      }  
    case NPCPROPS:   
      {  
#ifdef SERVERSIDENPCS  
        return;  
#endif  
        if ((Length(data)<3) || !Assigned(level)) return;
  
        if (Pos("police",level->plainfile)==1) return;  
        if (level->plainfile=="level13.graal") return;  
  
        npcid = ((data[1]-32) << 14) + ((data[2]-32) << 7) + (data[3]-32);  
        for (i = 0; i<level->npcs->count; i++) {  
          TServerNPC* npc = (TServerNPC*)(*level->npcs)[i];  
          if (npc->id==npcid) {  
            if (npc->filename!="door.gif") {  
              npc->SetProperties(Copy(data,4,Length(data)-3),false);  
              for (j = 0; j<level->players->count; j++) {  
                player = (TServerPlayer*)(*level->players)[j];  
                if (Assigned(player) && player!=this) player->SendData(SNPCPROPS,data);  
              }
            }  
            break;  
          }  
        }  
        break;  
      }  
    case ADDBOMB:  
      {  
        if (!Assigned(level)) return;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SADDBOMB,data);  
        }  
        break;  
      }  
    case DELBOMB:
      {  
        if (!Assigned(level)) return;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SDELBOMB,data);  
        }  
        break;  
      }  
    case TOALLMSG:  
      {  
        if (Length(data)<1) return;  
        if (Assigned(level) && adminlevel<1 && LevelIsJail(level->plainfile)) return;
        noactioncounter = 0;  
  
        // Send the message to all players and admins  
        props = GetCodedID() + data;  
        for (i = 0; i<ServerFrame->players->count; i++) {  
          player = (TServerPlayer*)(*ServerFrame->players)[i];  
          if (player!=this && player->loggedin() && !player->isblocked() &&
            (player->additionalflags & 4)==0)
              player->SendData(STOALLMSG,props);
        }  
        break;  
      }  
    case ADDHORSE:  
      {  
        if (!Assigned(level)) return;  
        if (Length(data)<3) return;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SADDHORSE,data);
        }  
  
        cx = ((double)(data[1]-32))/2;  
        cy = ((double)(data[2]-32))/2;  
        i = data[3]-32;  
        level->putHorse(cx,cy,Copy(data,4,Length(data)-3),i);  
        break;  
      }  
    case DELHORSE:  
      {  
        if (!Assigned(level)) return;  
        if (Length(data)<2) return;
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SDELHORSE,data);
        }  
  
        cx = ((double)(data[1]-32))/2;  
        cy = ((double)(data[2]-32))/2;  
        level->removeHorse(cx,cy);  
        break;  
      }  
    case ADDARROW:  
      {  
        if (!Assigned(level)) return;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SADDARROW,data);
        }
        break;
      }
    case FIRESPY:
      {  
        if (!Assigned(level)) return;  
        data = GetCodedID() + data;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))
            player->SendData(SFIRESPY,data);
        }  
        break;  
      }  
    case CARRYTHROWN:  
      {  
        if (!Assigned(level)) return;  
        data = GetCodedID() + data;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SCARRYTHROWN,data);  
        }  
        break;  
      }  
    case ADDEXTRA:  
      {  
        if (!Assigned(level) || (Length(data)<3) || level->extras->count>=200) return;
        TServerExtra* extra = new TServerExtra();  
        extra->x = ((double)(data[1]-32))/2;  
        extra->y = ((double)(data[2]-32))/2;  
        extra->itemindex = data[3]-32;  
        extra->counter = 20; // seconds to live  
        level->extras->Add(extra);  
  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SADDEXTRA,data);
        }  
        break;
      }  
    case DELEXTRA:  
    case TAKEEXTRA:  
      {  
        if (!Assigned(level) || Length(data)<2) return;  
        cx = ((double)(data[1]-32))/2;  
        cy = ((double)(data[2]-32))/2;  
        if (id==TAKEEXTRA && Length(data)>=3)  
          if (!level->takeExtra(cx,cy,data[3]-32,this)) return;  
        level->removeExtra(cx,cy);  
  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SDELEXTRA,data);
        }  
        break;
      }  
    case CLAIMPKER:  
      {  
        if ((Length(data)<2) || (alignment<10)) return;  
        j = ((data[1]-32)<<7) + (data[2]-32);  
        for (i=0; i<ServerFrame->players->count; i++) {  
          player = (TServerPlayer*)(*ServerFrame->players)[i];  
          if (Assigned(player) && player->id==j) {  
            if (player->lastip==lastip || !player->loggedin()) break;  
            // Subtract alignment points for player killing  
            if (player->alignment>0) {  
              j = (int)((alignment+10)/20);  
              player->alignment = player->alignment-j;  
              if (player->alignment<0)
                player->alignment = 0;  
              props = (char)(PALIGNMENT+32) + player->GetProperty(PALIGNMENT);  
              player->SendData(SPLAYERPROPS,props);  
              player->SetProperties(props);  
            }  
            player->kills++;  
            break;  
          }  
        }  
        break;  
      }  
    case BADDYPROPS:  
      {  
        if (!Assigned(level) || (Length(data)<1) || !(level->players->indexOf(this)==0)) return;  
        j = data[1]-32;  
        for (i = 1; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          player->SendData(SBADDYPROPS,data);
        }
        for (i = 0; i<level->baddies->count; i++) {
          TServerBaddy* baddy = (TServerBaddy*)(*level->baddies)[i];
          if (baddy->id==j) {
            baddy->SetProperties(Copy(data,2,Length(data)-1));
            if (baddy->deleteme) {
              level->baddies->Delete(i);
              delete(baddy);
            }
            return;
          }
        }  
        break;  
      }
    case HURTBADDY:  
      {  
        if (!Assigned(level) || (level->players->count<2) ||  
          (*level->players)[0]==this) return;  
        ((TServerPlayer*)(*level->players)[0])->SendData(SHURTBADDY,data);  
        break;  
      }  
    case ADDBADDY:  
      {  
        if (!Assigned(level) || (Length(data)<4)) return;  
  
        // don't allow addbaddy in levelxxx.graal
        if (Pos("level",level->plainfile)==1 ||  
            Pos("police",level->plainfile)==1) {  
          JString out = "Client '"+accountname+"' has added a baddy in "+level->plainfile;  
          ServerFrame->toerrorlog(out);  
          SendData(DISMESSAGE,JString()+"Adding baddies is not allowed in this level.");  
          deleteme = true;
          return;  
        }  
  
        // add a baddy with the sent attributes  
        gifname = Copy(data,5,Length(data)-4);  
        TServerBaddy* baddy = level->GetNewBaddy();  
        if (Assigned(baddy)) {  
          baddy->x = ((double)(data[1]-32))/2;  
          baddy->y = ((double)(data[2]-32))/2;  
          baddy->orgx = baddy->x;  
          baddy->orgy = baddy->y;  
          baddy->comptype = data[3]-32;  
          if (baddy->comptype<0) baddy->comptype = 0;  
          if (baddy->comptype>=baddytypes) baddy->comptype = baddytypes-1;
          baddy->mode = baddystartmode[baddy->comptype];  
          baddy->power = data[4]-32;  
          baddy->filename = (*baddygifs)[baddy->comptype];  
          if (Length(gifname)>0) baddy->filename = gifname;  
          baddy->respawn = false;  
  
          // inform all players in the current level that a baddy has been added  
          for (i = 0; i<level->players->count; i++) {  
            player = (TServerPlayer*)(*level->players)[i];  
            props = JString((char)(baddy->id+32)) << baddy->GetPropertyList(player);  
            player->SendData(SBADDYPROPS,props);
          }  
        } else {  
//          // >=50 baddies in this level -> it is messed up
//          JString out = JString()+"Client '"+accountname+
//            "' has added the 51th baddy in "+level->plainfile;
//          ServerFrame->toerrorlog(out);
//          updateCurrentLevel();
        }  
        break;  
      }  
    case SETFLAG:  
      {  
        if (!iscorrect(data)) return; 
        if (Pos("server.",data)==1) {  
          if (ServerFrame->serverflags->indexOf(data)<0) {  
            ServerFrame->serverflags->Add(data);  
            ServerFrame->SendAddServerFlag(data,this);  
          }  
          return;  
        }  
        if (Length(data)>0 && actionflags->indexOf(data)<0) actionflags->Add(data);  
        break;
      }  
    case UNSETFLAG:  
      {  
        if (!iscorrect(data)) return; 
        if (Pos("server.",data)==1) {  
          i = ServerFrame->serverflags->indexOf(data);  
          if (i>=0) {  
            ServerFrame->serverflags->Delete(i);  
            ServerFrame->SendDelServerFlag(data,this);  
          }  
          return;  
        }  
  
        i = actionflags->indexOf(data);  
        if (i>=0) actionflags->Delete(i);  
        break;  
      }  
    case OPENCHEST:  
      {
        if (Assigned(level))  
          openedchests->Add(data+level->plainfile);  
        break;  
      }  
    case ADDNPC:  
      {  
        // only allowed for admins and playerworlds  
        if ((adminlevel>=1 && isadminin(playerworld)) ||
            (Length(playerworld)>0 && Pos("classic",playerworld)!=1))
          parseNewNPC(data);
        break;  
      }  
    case DELNPC:  
      {  
        if ((Length(data)<3) || !Assigned(level)) return;
  
        npcid = ((data[1]-32) << 14) + ((data[2]-32) << 7) + (data[3]-32);  
        for (i = 0; i<level->npcs->count; i++) {  
          TServerNPC* npc = (TServerNPC*)(*level->npcs)[i];  
          if (npc->id==npcid) { ServerFrame->DeleteNPC(npc); break; }  
        }  
        break;  
      }  
    case WANTGIF:  
      {  
        if (Length(data)>0) {  
          gifname = data;  
          if (gifname==RemoveExtension(gifname))  
            gifname << ".gif";  
          if (!LevelFileExists(gifname) && Length(playerworld)>0)  
            gifname = playerworld + gifname;  
          if (LevelFileExists(gifname) && imagestosend->count<200) {  
            imagestosend->Add(gifname);
            return;  
          }  
        }  
        SendData(GIFFAILED,data);  
        break;  
      }  
    case NPCWEAPONIMG:  
      {  
        if (!Assigned(level)) return;  
        data = GetCodedID() + data;  
        for (i=0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))
            player->SendData(SNPCWEAPONIMG,data);
        }  
        break;  
      }  
    case HURTPLAYER:  
      {  
        if (Length(data)<6) return;  
        x = ((data[1]-32)<<7) + (data[2]-32);  
        data = GetCodedID() + Copy(data,3,4);  
        for (i=0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && player->id==x) {  
            player->SendData(SPLAYERHURT,data);  
            break;  
          }  
        }  
        break;  
      }
    case EXPLOSION:  
      {  
        if (!Assigned(level)) return;  
        for (i = 0; i<level->players->count; i++) {  
          player = (TServerPlayer*)(*level->players)[i];  
          if (Assigned(player) && player!=this && !player->isblocked() &&
              (udpport<=0 || player->udpport<=0))  
            player->SendData(SEXPLOSION,data);  
        }  
        break;  
      }  
    case PRIVMESSAGE:  
      {  
        bool injail = !Assigned(level) || LevelIsJail(level->plainfile);
  
        noactioncounter = 0;  
        if (ver>=26 || isrc) {  
          if (Length(data)<2) return;  
          k = ((data[1]-32)<<7) + (data[2]-32);  
          data = Copy(data,3,Length(data)-2);  
        } else {  
          if (Length(data)<1) return;  
          k = data[1]-32;  
          data = Copy(data,2,Length(data)-1);  
        }  
        if (k<0) k = 0;  
        if (Length(data)<=2*k) return;  
        props = GetCodedID()+Copy(data,1+2*k,Length(data)-2*k);  
        if (Length(props)>1024) {  
          props2 = JString() + "Server message:§ Your message was too long.";  
          SendData(SADMINMSG,props2);
          return;
        }  
        for (i=0; i<k; i++) {  
          x = ((data[1+2*i]-32)<<7) + (data[2+2*i]-32);  
          for (j=0; j<ServerFrame->players->count; j++) {  
            player = (TServerPlayer*)(*ServerFrame->players)[j];  
            if (Assigned(player) && player->id==x) {  
              bool dosend = true;  
              if (!isrc && adminlevel<1) {
                if (injail) {
                  if (!player->isrc) {
                    dosend = false;
                    props2 = player->GetCodedID() + "\"Server Message:\","+
                      "\"From jail you can only send PMs to admins (RCs).\"";
                    SendData(SPRIVMESSAGE,props2);
                  }
                } else if (k>1 && (player->additionalflags & 1)>0) {
//                } else if (k>1 && (Length(plainguild)<=0 || plainguild!=player->plainguild)) {
                  dosend = false;
//                  props2 = player->GetCodedID() + "\"Server Message:\","+
//                    "\"The player does not read mass messages.\"";
//                  SendData(SPRIVMESSAGE,props2);
                }
              }
              if (dosend) player->SendData(SPRIVMESSAGE,props);
              break;
            }
          }
        }
        break;
      }
    case DELNPCWEAPON:
      { 
        myweapons->Remove(data); 
        break;  
      }  
    case PACKCOUNT:  
      {  
        nodatacounter = 0;  
        j = ((data[1]-32) << 7) + (data[2]-32);  
        if (j!=packcount || packcount>10000) {  
          JString out = "Client "+accountname+" sent wrong package count / too many messages";  
          out << "\nreal count: " << packcount << ", sent count: " << j;  
          ServerFrame->toerrorlog(out);
          if (j!=packcount) {  
            SendData(DISMESSAGE,JString()+"The server got uncertified data packages from your connection.");  
            deleteme = true;  
          }  
          updateCurrentLevel();  
          return;  
        }  
        packcount = 0;  
        break;  
      }  
    case ADDWEAPON2:  
      {  
        if (Length(data)<1) return;  
        j = data[1]-32;  
        JString name;  
  
        // Try to add the new weapon  
        if (j==0) {
          if (Length(data)<2) return;  
  
          // Type 0: a default weapon, just add the name to the myweapons list  
          i = data[2]-32;  
          if (i<0 || i>=giveitemcount) return;  
          name = (*itemnames)[i];  
          if (myweapons->indexOf(name)>=0) return;  
          myweapons->Add((*itemnames)[i]);  
  
        } else {  
          if (Length(data)<4 || !Assigned(level)) return;  
  
          // Type 1: a weapon which called the 'toweapons' script command  
          // Lets search for the npc and try to add a new weapon
          npcid = ((data[2]-32) << 14) + ((data[3]-32) << 7) + (data[4]-32);  
          for (i=0; i<level->npcs->count; i++) {  
            TServerNPC* npc = (TServerNPC*)(*level->npcs)[i];  
            if (Assigned(npc) && (npc->id==npcid)) {  
              if (Length(npc->startimage)<=0) return;  
  
              name = npc->GetWeaponName();  
              if (Length(name)<=0) return;  
              // Check if we already have this weapon;  
              // if yes, then don't add a new one  
              if (myweapons->indexOf(name)>=0) return;  
              myweapons->Add(name);  
  
              // Search for weapons in the weapons list that  
              // we can replace; if there is no weapon with the  
              // name yet then add a new TServerWeapon  
              TServerWeapon* replaceweapon = NULL;  
              for (j=0; j<weapons->count; j++) {
                TServerWeapon* weapon = (TServerWeapon*)(*weapons)[j];  
                if (Assigned(weapon) && weapon->name==name && weapon->world==playerworld) {  
                  // Don't replace the existing weapon if it's newer  
                  // than the current level  
                  if (weapon->modtime>=level->modtime) return;  
                  replaceweapon = weapon;  
                  break;  
                }  
              }  
              // Update the found weapon / add a new weapon to the  
              // weapons list  
              if (!Assigned(replaceweapon)) {  
                replaceweapon = new TServerWeapon();  
                replaceweapon->name = name;
                replaceweapon->world = playerworld;  
              }  
              replaceweapon->image = npc->startimage;  
              replaceweapon->modtime = level->modtime;  
              replaceweapon->dataforplayer.clear();  
              if (Length(playerworld)>0)   
                replaceweapon->dataforplayer << JString((char)(Length(name)+32)) << name  
                  << (char)(0+32) << (char)(Length(npc->startimage)+32) << npc->startimage    
                  << (char)(1+32) << npc->GetProperty(1);   
              name = playerworld + name;  
              replaceweapon->fullstr.clear();  
              replaceweapon->fullstr << JString((char)(Length(name)+32)) << name  
                << (char)(0+32) << (char)(Length(npc->startimage)+32) << npc->startimage  
                << (char)(1+32) << npc->GetProperty(1);  
              if (weapons->indexOf(replaceweapon)<0)  
                weapons->Add(replaceweapon);
              SaveWeapons();
              ServerFrame->toerrorlog(JString("New weapon: ") << name);
              break;
            }
          }
        }
      }
    case SETRESPAWN:  
    case SETHORSELIFE:  
    case SETAPINCRTIME:  
    case SETBADDYRESP:  
      {  
        if ((Length(data)<4) || adminlevel<4 || !isrc) return;
  
        j = ((data[1]-32) << 21) + ((data[2]-32) << 14) + ((data[3]-32) << 7) + (data[4]-32);  
        switch (id) {  
//          case SETRESPAWN:    respawntime = j; break;  
          case SETHORSELIFE:  horselifetime = j; break;  
          case SETAPINCRTIME: apincrementtime = j; break;  
          case SETBADDYRESP:  {
              baddyrespawntime = j; 
              break;  
            }  
        }  
        break;  
      }  
    case DISPLAYER:
      {
        if ((Length(data)<2) || adminlevel<3 || !isrc) return;

        j = ((data[1]-32)<<7) + (data[2]-32);
        for (i = 0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (player->id==j) {
            if (adminlevel==5 || player->accountname==accountname ||
                (player->adminlevel<adminlevel && isadminin(player->playerworld))) {
              ServerFrame->SendRCLog(accountname+" disconnects "+player->accountname);
              ServerFrame->tolog(accountname+" disconnects "+player->accountname);
              player->SendData(DISMESSAGE,JString()+"The server administrator has disconnected you.");
              player->deleteme = true;
            } else
              ServerFrame->SendRCLog("Player "+player->accountname+": permission denied");
            break;
          }
        }
        break;
      }
    case UPDLEVELS:  
      { 
        if ((Length(data)<2) || adminlevel<1 || !isrc) return;  

        j = ((data[1]-32) << 7) + (data[2]-32);  
        data = Copy(data,3,Length(data)-2);  
        TJStringList* oldlist = new TJStringList();  
        oldlist->SetPlainMem();
        while (j>0) {
          if (Length(data)<1) break;  
          i = data[1]-32; 
          if (Length(data)<1+i) break;  
          oldlist->Add(Copy(data,2,i));  
          data = Copy(data,2+i,Length(data)-1-i);  
        }  
        ServerFrame->UpdateLevels(oldlist);  
        delete(oldlist);  
        break;  
      }
    case ADMINMSG:  
      {  
        if ((Length(data)<1) || (adminlevel<4) || !isrc) return;  
  
        data = JString() + "Administrator " + accountname + ":§" + data;  
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];  
          if (Assigned(player) && player!=this && isadminin(player->playerworld)) 
            player->SendData(SADMINMSG,data);  
        }  
        break;  
      }  
    case PRIVADMINMSG:  
      {  
        if ((Length(data)<3) || (adminlevel<2) || !isrc) return;  
 
        j = ((data[1]-32)<<7) + (data[2]-32);
        data = Copy(data,3,Length(data)-2);
        if (adminlevel<4) data = JString() + "Police " + accountname + ":§" + data;
        else data = JString() + "Administrator " + accountname + ":§" + data;
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (Assigned(player) && player->id==j) {
            if (isadminin(player->playerworld)) player->SendData(SADMINMSG,data);
            return;
          }
        }
        break;
      }
    case APPLYREASON:
      {
        if (Length(data)<=0 || adminlevel<3 || !isrc) return;  

        ServerFrame->tolog("Reason for the follwing account modification:\n"+  
          AddLineEnds(data));  
        break;  
      }
    case LISTSFLAGS:  
      {  
        i = ServerFrame->serverflags->count;  
        props << (char)((i >> 7)+32) << (char)((i & 0x7f)+32);  
        for (i=0; i<ServerFrame->serverflags->count; i++)  
          props << (char)(Length((*ServerFrame->serverflags)[i])+32) <<  
            (*ServerFrame->serverflags)[i];  
        SendData(SSERVERFLAGS,props);  
        break;  
      } 
    case SETSFLAGS:  
      {  
        if (Length(data)<2 || adminlevel<4 || !isrc) return;  

        j = ((data[1]-32)<<7) + (data[2]-32);  
        data = Copy(data,3,Length(data)-2);  
        TJStringList* newflags = new TJStringList();
        newflags->SetPlainMem();
        while (j>0) {
          len = data[1]-32;  
          if (len<=0) break;  
          props = Trim(Copy(data,2,len));  
          if (iscorrect(props) && Pos("server.",props)==1)  
            newflags->Add(props);  
          data = Copy(data,2+len,Length(data)-1-len);  
          j--;  
        }
  
        // Delete the flags from serverflags which are not in newflags
        for (i=0; i<ServerFrame->serverflags->count; i++) {  
          props = (*ServerFrame->serverflags)[i];
          if (newflags->indexOf(props)<0)  
            ServerFrame->SendDelServerFlag(props,this);  
        }  
        // Add the flags from newflags which are not in serverflags  
        for (i=0; i<newflags->count; i++) {  
          props = (*newflags)[i];  
          if (ServerFrame->serverflags->indexOf(props)<0)  
            ServerFrame->SendAddServerFlag(props,this);  
        }  
        delete(ServerFrame->serverflags);  
        ServerFrame->serverflags = newflags; 
        break;  
      }

    case DADDACCOUNT: {
        if (Length(data)<=0 || !isrc || adminlevel<4) return;
        JString accname = AddAccountFromRC(data,adminlevel);
        if (Length(dberror)>0)
          ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        else {
          ServerFrame->SendRCLog(accountname+" added the account "+accname);
          ServerFrame->tolog(accountname+" added the account "+accname);
        }
        break;
      }
    case DDELACCOUNT: {
        if (Length(data)<=0 || !isrc || adminlevel<4) return;
        DeleteAccount(data,adminlevel);
        if (Length(dberror)>0)
          ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        else
{
          ServerFrame->SendRCLog(accountname+" deleted the account "+data);
          ServerFrame->tolog(accountname+" deleted the account "+data);
        }
        break;
      }

    case DWANTACCLIST: {
        if (Length(data)<2 || !isrc || adminlevel<3) return;

        len = data[1]-32;
        if (len<0) return;
        props = Trim(Copy(data,2,len)); // like string
        data = Copy(data,2+len,Length(data)-1-len);
        len = data[1]-32;
        if (len<0) return;
        props2 = Trim(Copy(data,2,len)); // where string

        JString acclist = GetAccountsList(props,props2);
        if (Length(dberror)>0)
          ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        else if (Length(acclist)>60000)
          SendData(DRCLOG,"Accounts list too long!");
        else
          SendData(DACCLIST,acclist);
        break;
      }
    case DWANTPLPROPS: {
        if ((Length(data)<2) || adminlevel<3 || !isrc) return;

        j = ((data[1]-32)<<7) + (data[2]-32);
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (player->id==j) {
            if (adminlevel==5 || (isadminin(player->playerworld) &&
                ((adminlevel>=3 && player->adminlevel<adminlevel) || player->accountname==accountname))) {
              SendRCPlayerProps(player,false);
              ServerFrame->SendRCLog(accountname+" loaded attributes of player "+player->accountname);
            } else
              SendData(DRCLOG,"Player "+player->accountname+": permission denied");
            break;
          }
        }
        break;
      }
    case DWANTACCPLPROPS: {
        if (Length(data)<1 || !isrc || adminlevel<1) return;

        len = data[1]-32;
        if (len<0) return;
        props = Trim(Copy(data,2,len)); // accname
        data = Copy(data,2+len,Length(data)-1-len);
        if (Length(data)<1) return;
        len = data[1]-32;
        if (len<0) return;
        props2 = Trim(Copy(data,2,len)); // world
        if (props2==startworld) props2 = "";

        if (!isadminin(props2)) {
          if (Length(props2)<=0) props2 << startworld;
          SendData(DRCLOG,"You are not admin in "+props2+"!");
          return;
        }

        unsigned char sep = '§';
        if (Length(props2)>0 && props2[Length(props2)]!=sep)
          props2 << "§";

        bool found = false;
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (Assigned(player) && !player->isrc && player->accountname==props && player->playerworld==props2) {
            if (adminlevel==5 ||
                (adminlevel>=3 && player->adminlevel<adminlevel) || player->accountname==accountname) {
              SendRCPlayerProps(player,false);
              ServerFrame->SendRCLog(accountname+" loaded attributes of player "+player->accountname);
            } else
              SendData(DRCLOG,"Player "+props+": permission denied (adminlevel higher than allowed)");
            found = true;
            break;
          }
        }
        if (!found) {
          caccount = GetAccount(props);
          if (Assigned(caccount)) {
            if (adminlevel==5 ||
                (adminlevel>=3 && caccount->adminlevel<adminlevel) || caccount->name==accountname) {
              player = new TServerPlayer(NULL,0);
              LoadDBAccount(player,props,props2);
              if (Length(dberror)>0)
                ServerFrame->SendRCLog(accountname+" prob: "+dberror);
              else {
                player->accountname = props;
                player->playerworld = Copy(props2,1,Length(props2)-1);
                SendRCPlayerProps(player,true);
                ServerFrame->SendRCLog(accountname+" loaded player attributes of account "+player->accountname+
                  " from world "+player->playerworld);
              }
              delete(player);
            } else
              SendData(DRCLOG,"Account "+props+": permission denied (adminlevel higher than allowed)");
            delete(caccount);
          } else
            ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        }
        break;
      }
    case DSETPLPROPS: {
        if ((Length(data)<2) || adminlevel<3 || !isrc) return;

        j = ((data[1]-32)<<7) + (data[2]-32);
        data = Copy(data,3,Length(data)-2);
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (player->id==j) {
            if (adminlevel==5 || (isadminin(player->playerworld) &&
                ((adminlevel>=3 && player->adminlevel<adminlevel) || player->accountname==accountname))) {
              SetRCPlayerProps(data,player,false);
              ServerFrame->SendRCLog(accountname+" set attributes of player "+player->accountname);
            } else
              SendData(DRCLOG,"Player "+player->accountname+": permission denied");
            break;
          }
        }
        break;
      }
    case DSETACCPLPROPS: {
        if (Length(data)<1 || !isrc || adminlevel<3) return;

        len = data[1]-32;
        if (len<0) return;
        props = Trim(Copy(data,2,len)); // accname
        data = Copy(data,2+len,Length(data)-1-len);
        if (Length(data)<1) return;
        len = data[1]-32;
        if (len<0) return;
        props2 = Trim(Copy(data,2,len)); // world
        if (props2==startworld) props2 = "";
        data = Copy(data,2+len,Length(data)-1-len);

        if (!isadminin(props2)) {
          if (Length(props2)<=0) props2 << startworld;
	  props2 = Copy(props2,1,Length(props2)-1);
          SendData(DRCLOG,"You are not admin in "+props2+"!");
          return;
        }

        unsigned char sep = '§';
        if (Length(props2)>0 && props2[Length(props2)]!=sep)
          props2 << "§";

        bool found = false;
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (Assigned(player) && !player->isrc && player->accountname==props && player->playerworld==props2) {
            if (adminlevel==5 ||
                (adminlevel>=3 && player->adminlevel<adminlevel) || player->accountname==accountname) {
              SetRCPlayerProps(data,player,false);
              ServerFrame->SendRCLog(accountname+" set attributes of player "+player->accountname);
            } else
              SendData(DRCLOG,"Player "+props+": permission denied (adminlevel higher than allowed)");
            found = true;
            break;
          }
        }
        if (!found) {
          caccount = GetAccount(props);
          if (Assigned(caccount)) {
            if (adminlevel==5 ||
                (adminlevel>=3 && caccount->adminlevel<adminlevel) || caccount->name==accountname) {
              player = new TServerPlayer(NULL,0);
              LoadDBAccount(player,props,props2);
              if (Length(dberror)>0)
                ServerFrame->SendRCLog(accountname+" prob: "+dberror);
              else {
                player->accountname = props;
                player->playerworld = Copy(props2,1,Length(props2)-1);
                SetRCPlayerProps(data,player,true);
                ServerFrame->SendRCLog(accountname+" set player attributes of account "+player->accountname+
                  " from world "+player->playerworld);
              }
              delete(player);
            } else
              SendData(DRCLOG,"Account "+props+": permission denied (adminlevel higher than allowed)");
            delete(caccount);
          } else
            ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        }
        break;
      }
    case DWANTACCOUNT: {
        if (Length(data)<=0 || !isrc || adminlevel<3) return;
        caccount = GetAccount(data);
        if (Assigned(caccount)) {
          if (adminlevel==5 || adminlevel>caccount->adminlevel) {
            props << (char)(Length(caccount->name)+32) << caccount->name
              << (char)(0+32) // password (don't send it)
              << (char)(Length(caccount->email)+32) << caccount->email
              << (char)(caccount->blocked+32)
              << (char)(caccount->onlyload+32)
              << (char)(caccount->adminlevel+32)
              << (char)(Length(caccount->adminworlds)+32) << caccount->adminworlds;
            SendData(DACCOUNT,props);
            ServerFrame->SendRCLog(accountname+" loaded account "+caccount->name);
          } else
            SendData(DRCLOG,"Account "+data+": permission denied (adminlevel higher than allowed)");
          delete(caccount);
        } else
          ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        break;
      }
    case DSETACCOUNT: {
        if (Length(data)<=0 || !isrc || adminlevel<3) return;
        JString accname = SetAccount(data,adminlevel);
        if (Length(dberror)>0)
          ServerFrame->SendRCLog(accountname+" prob: "+dberror);
        else {
          ServerFrame->SendRCLog(accountname+" changed the account "+accname);
          ServerFrame->tolog(accountname+" changed the account "+accname);
        }
        break;
      }
    case DRCCHAT: {
        if (Length(data)<=0 || !isrc || adminlevel<1) return;
// ADDED: Massive new functions! oh yeah ;)
        //
        // Normal Commands:
        // /me <text>            - Do a action
        //
        if (CheckPartofString(LowerCase(data), "/me ")) {  // Command to make actions
          ServerFrame->SendRCLog("*: " + accountname + " " + Copy(data, 5, Length(data)-4));
        } else ServerFrame->SendRCLog(accountname+": "+data);
        // Components:
        //
        // help->help            - General Help (PRIV)
        // help->subdirs         - Subdirectory's Help (PRIV)
        // help->server          - Server Help (PRIV)
        // subdirs->refresh      - Refresh subdirectory's cache (ALL)
        // subdirs->count        - Number of subdirectory's in cache (PRIV)
        // subdirs->list <index> - Return subdirectory name by index (PRIV)
        // subdirs->listall      - Return all subdirectory's names in cache (PRIV)
        // server->uptime        - Report uptime (ALL)
        // server->version       - Report server version (ALL)
        // server->shutdown      - Shutdown server (ALL)
        // server->msg <msg>     - Message to server       

	//MySQL Guild Managment, Coded By Gregovich <BEGIN>
        if (CheckPartofString(LowerCase(data), "guild->del ")) {
	  data = Copy(data, 12, Length(data)-11);
	  ServerFrame->SendPrivRCLog(this, "Deleting guild:");
	  ServerFrame->SendPrivRCLog(this, "" + data);
	  DeleteGuild(data);
	} else if (CheckPartofString(LowerCase(data), "guild->memberdel ")) {
	  data = Copy(data, 18, Length(data)-17);
char * pch;
printf ("Splitting string \"%s\" in tokens:\n", data.text());
pch = strtok((char *)data.text(),",");
JString accname = pch;
pch = strtok (NULL, " ,.");
JString guildname = pch;
ServerFrame->SendPrivRCLog(this, "Removing GuildMember: " + accname + "\njFrom Guild: " + guildname + " \nj");
     DelGuildMember(accname, guildname);
	} else if (CheckPartofString(LowerCase(data), "guild->add ")) {
ServerFrame->SendPrivRCLog(this, "Creating Guild:\nj");
data = Copy(data, 12, Length(data)-11);
char * pch;
printf ("Splitting string \"%s\" in tokens:\n", data.text());
pch = strtok((char *)data.text(),",");
JString accname = pch;
pch = strtok (NULL, " ,.");
JString nickname = pch;
pch = strtok (NULL, " ,.");
JString guildname = pch;
JString rank = 5;
ServerFrame->SendPrivRCLog(this, "Master Account: " + accname + "\njMaster Nickname: " + nickname + "\n");
ServerFrame->SendPrivRCLog(this, "Guildname: " + guildname);
CreateGuild(accname, nickname, rank, guildname);
        } else if (CheckPartofString(LowerCase(data), "guild->memberadd ")) {
ServerFrame->SendPrivRCLog(this, "Adding Member to Guild:\nj");
data = Copy(data, 18, Length(data)-17);
char * pch;
printf ("Splitting string \"%s\" in tokens:\n", data.text());
pch = strtok((char *)data.text(),",");
JString accname = pch;
pch = strtok (NULL, " ,.");
JString nickname = pch;
pch = strtok (NULL, " ,.");
JString guildname = pch;
pch = strtok (NULL, " ,.");
JString rank = pch;
ServerFrame->SendPrivRCLog(this, "Account: " + accname + "\njNickname: " + nickname + "\n");
ServerFrame->SendPrivRCLog(this, "Guildname: " + guildname);
CreateGuild(accname, nickname, rank, guildname);
	} else if (LowerCase(data) == "guild->list") {
	  props = ""; // like string
          props2 = ""; // where string

          JString acclist = ListGuild(props,props2);
          if (Length(dberror)>0)
            ServerFrame->SendRCLog(accountname+" prob: "+dberror);
          else if (Length(acclist)>60000)
            SendData(DRCLOG,"Accounts list too long!");
          else
            ServerFrame->SendPrivRCLog(this, acclist);
  	} else if (CheckPartofString(LowerCase(data), "guild->members ")) {
	  data = Copy(data, 16, Length(data)-15);
          props = ""; // like string
          props2 = ""; // where string

          JString acclist = ListGuildMembers(props,props2, data);
          if (Length(dberror)>0)
            ServerFrame->SendRCLog(accountname+" prob: "+dberror);
          else if (Length(acclist)>60000)
            SendData(DRCLOG,"Accounts list too long!");
          else
            ServerFrame->SendPrivRCLog(this, acclist);
	} else if (LowerCase(data) == "help->guild") {
	  ServerFrame->SendPrivRCLog(this, "Help \"guild\" component:");
	  ServerFrame->SendPrivRCLog(this, "");
	  ServerFrame->SendPrivRCLog(this, "guild->list");
	  ServerFrame->SendPrivRCLog(this, "  Lists all the guilds that exists in the database");
          ServerFrame->SendPrivRCLog(this, "guild->add [account],[nickname],[guildname]");
          ServerFrame->SendPrivRCLog(this, "  Create a guild.");
          ServerFrame->SendPrivRCLog(this, "guild->del [guildname]");
          ServerFrame->SendPrivRCLog(this, "  Delete a guild.");
	  ServerFrame->SendPrivRCLog(this, "guild->members [guildname]");
          ServerFrame->SendPrivRCLog(this, "  List all members in a guild.");
	  ServerFrame->SendPrivRCLog(this, "guild->memberadd [account],[nickname],[guildname],[rank]");
          ServerFrame->SendPrivRCLog(this, "  Add a member to a guild. Rank is a number form 1 to 5 five is best.");
	  ServerFrame->SendPrivRCLog(this, "guild->memberdel [account],[guildname]");
          ServerFrame->SendPrivRCLog(this, "  Delete a member from a guild");
	//MySQL Guild Managment, Coded By Gregovich <END>
        } else if (LowerCase(data) == "help->help") {
          ServerFrame->SendPrivRCLog(this, "General Help:");
          ServerFrame->SendPrivRCLog(this, "Use [component]->[command] to call a component");
          ServerFrame->SendPrivRCLog(this, "Ex; help->help");
          ServerFrame->SendPrivRCLog(this, "To get information about a component, use;");
          ServerFrame->SendPrivRCLog(this, "help->[component]");
          ServerFrame->SendPrivRCLog(this, "");
          ServerFrame->SendPrivRCLog(this, "Available components;");
          ServerFrame->SendPrivRCLog(this, "  help");
          ServerFrame->SendPrivRCLog(this, "  subdirs");
          ServerFrame->SendPrivRCLog(this, "  server");
          ServerFrame->SendPrivRCLog(this, "  guild");
          ServerFrame->SendPrivRCLog(this, "End of General Help");
        } else if (LowerCase(data) == "help->subdirs") {
          ServerFrame->SendPrivRCLog(this, "Help \"subdirs\" component:");
          ServerFrame->SendPrivRCLog(this, "");
          ServerFrame->SendPrivRCLog(this, "subdirs->refresh");
          ServerFrame->SendPrivRCLog(this, "  Refresh the cache of subdirectory's at \\levels\\ directory");
          ServerFrame->SendPrivRCLog(this, "subdirs->count");
          ServerFrame->SendPrivRCLog(this, "  Return number of subdirectory's in cache");
          ServerFrame->SendPrivRCLog(this, "subdirs->list [index]");
          ServerFrame->SendPrivRCLog(this, "  Return the name of the subdirectory at index in the cache");
          ServerFrame->SendPrivRCLog(this, "subdirs->listall");
          ServerFrame->SendPrivRCLog(this, "  Return the name of all subdirectory's in cache");
          ServerFrame->SendPrivRCLog(this, "");
          ServerFrame->SendPrivRCLog(this, "End of Help at \"subdirs\" component");
        } else if (LowerCase(data) == "help->server") {
          ServerFrame->SendPrivRCLog(this, "Help \"server\" component:");
          ServerFrame->SendPrivRCLog(this, "");
          ServerFrame->SendPrivRCLog(this, "server->uptime");
          ServerFrame->SendPrivRCLog(this, "  Report to all admins about server uptime");
          ServerFrame->SendPrivRCLog(this, "server->version");
          ServerFrame->SendPrivRCLog(this, "  Report to all admins about server version");
#ifdef ALLOW_SHUTDOWN
          ServerFrame->SendPrivRCLog(this, "server->shutdown");
#else
          ServerFrame->SendPrivRCLog(this, "server->shutdown (DISABLED)");
#endif
          ServerFrame->SendPrivRCLog(this, "  Shutdown server");
          ServerFrame->SendPrivRCLog(this, "server->msg [message]");
          ServerFrame->SendPrivRCLog(this, "  Message to the server window");
          ServerFrame->SendPrivRCLog(this, "");
          ServerFrame->SendPrivRCLog(this, "End of Help \"server\" component");
        } else if (LowerCase(data) == "subdirs->refresh") {
          subdirs->Clear();
          RefindAllSubdirs(applicationdir+"levels");
          ServerFrame->SendPrivRCLog(this, "Subdirs Component - Refresh: Complete, "+inttostr(subdirs->count)+" Subdirectory(s)");
          ServerFrame->SendOthersRCLog(this, "Subdirectory's information has been refreshed");
        } else if (LowerCase(data) == "subdirs->count") {
          ServerFrame->SendPrivRCLog(this, "Subdirs Component - Count: "+inttostr(subdirs->count)+" Subdirectory(s)");
        } else if ((Length(data) >= 14) && (Copy(LowerCase(data), 0, 15) == "subdirs->list ")) {
          int index = strtoint(Copy(LowerCase(data), 15, Length(data)-14));
          if ((index >= 0) && (index < subdirs->count)) {
            ServerFrame->SendPrivRCLog(this, "Subdirs Component - List "+inttostr(index)+": "+(*subdirs)[index]);
          }
        } else if (LowerCase(data) == "subdirs->listall") {
          int icnt;
          ServerFrame->SendPrivRCLog(this, "Subdirs Component - List All:");
          for (icnt=0; icnt<subdirs->count; icnt++) {
            ServerFrame->SendPrivRCLog(this, inttostr(icnt)+": "+(*subdirs)[icnt]);
          }
        } else if (LowerCase(data) == "server->uptime") {
          upseconds = time(NULL)-starttime_global;
          ServerFrame->SendRCLog("Server uptime: " + inttostr(upseconds / 3600) + " hrs " + inttostr((upseconds / 60) % 60) + " mins");
        } else if (LowerCase(data) == "server->version") {
          ServerFrame->SendRCLog("Server version: 1.39 Plus - Version 1.1");
        } else if (LowerCase(data) == "server->version") {
	  LoadWeapons("weapons-back.txt");
          ServerFrame->SendRCLog("Weapon cache has been reloaded");
        } else if (LowerCase(data) == "server->shutdown") {
#ifdef ALLOW_SHUTDOWN
          if (adminlevel==5 || (isadminin(player->playerworld))) {
            ServerFrame->SendRCLog(accountname + " accept to shutdown server: ...");
            signalhandler(0x1337);
          } else {
            ServerFrame->SendRCLog("Error: "+accountname+" dont have admin level 5 to shutdown server");
          }
#else
          ServerFrame->SendRCLog("Error: Command disabled");
#endif
        } else if (CheckPartofString(LowerCase(data), "server->msg ")) {
          ServerFrame->SendRCLog(accountname + " to SERVER: " + Copy(data, 13, Length(data)-12));
          std::cout << "MSG from " << accountname << ": " << Copy(data, 13, Length(data)-12) << std::endl;  
        }
// It's Over!
        break;
      }
    case DGETPROFILE: {
        props = GetProfile(data);
//        if (Length(dberror)>0)
//          std::cout << "get profile error: " << dberror << std::endl;
//        else
        if (Length(props)>0) {
          for (i=0; i<ServerFrame->players->count; i++) { 
            player = (TServerPlayer*)(*ServerFrame->players)[i]; 
            if (Assigned(player) && !player->isrc && player->accountname==data) {
              j = player->onlinesecs;
              props2 = JString() << (j/3600) << " hrs " << ((j/60)%60) << " mins " << (j%60) << "secs"; 
              props << (char)(Length(props2)+32) << props2;
              props2 = JString(player->kills);
              props << (char)(Length(props2)+32) << props2;
              props2 = JString(player->deaths);
              props << (char)(Length(props2)+32) << props2;
              break; 
            }
          } 
          SendData(DPROFILE,props);
        }
        break;
      }
    case DSETPROFILE: {
        SetProfile(data,accountname,adminlevel);
        if (Length(dberror)>0)
          std::cout << "set profile error: " << dberror << std::endl;
        break;
      }
    case WARPPLAYER: {
        if ((Length(data)<2) || adminlevel<2 || !isrc) return;

        j = ((data[1]-32)<<7) + (data[2]-32);
        data = Copy(data,3,Length(data)-2);
        for (i=0; i<ServerFrame->players->count; i++) {
          player = (TServerPlayer*)(*ServerFrame->players)[i];
          if (player->id==j) {
            if (adminlevel==5 || (isadminin(player->playerworld) &&
                ((adminlevel>=2 && player->adminlevel<adminlevel) || player->accountname==accountname))) {

              player->SendData(PLAYERWARPED,data);
              allowoutjail = true;
              player->parse(JString((char)(LEVELWARP+32)) << data);
              allowoutjail = false;

              ServerFrame->SendRCLog(accountname+" warped player "+player->accountname);
            } else
              SendData(DRCLOG,"Player "+player->accountname+": permission denied");
            break;
          }
        }
        break;
      }
    default:
      {
        JString out = "Client '"+accountname+"' has sent illegal data: \n";
        out << line << "\n";
        ServerFrame->toerrorlog(out);
        SendData(DISMESSAGE,JString()+"The server got illegal data from your connection.");
        deleteme = true;
        break;
      }
  }
}

void TServerPlayer::parseNewNPC(JString data) {  
  if (!Assigned(level)) return; 
  
  if (Pos("police",level->plainfile)==1) {  
    JString out = "Client '"+accountname+"' has added an npc in "+level->plainfile;  
    ServerFrame->toerrorlog(out);  
    SendData(DISMESSAGE,JString()+"Adding npcs is not allowed in this level.");  
    deleteme = true;  
    return;  
  } 
  if (level->npcs->count>=50) {  
//    JString out = "Client '"+accountname+"' has added the 51th npc in "+level->plainfile;  
//    ServerFrame->toerrorlog(out);  
//    updateCurrentLevel();  
    return;  
  }  
  if (level->plainfile=="level13.graal") return;  
  
  if (Length(data)<1) return;  
  int len = data[1]-32;  
  JString gifname = Copy(data,2,len);  
  data = Copy(data,2+len,Length(data)-1-len);  
  if (Length(data)<1) return;  
  len = data[1]-32;  
  JString name = Copy(data,2,len);  
  data = Copy(data,2+len,Length(data)-1-len);  
  if (Length(data)<2) return;  
  double cx = ((double)(data[1]-32))/2;  
  double cy = ((double)(data[2]-32))/2;  
  
  JString action = "";  
  if (!LevelFileExists(name) && Length(playerworld)>0)  
    name = playerworld + name; 
  if (LevelFileExists(name)) {  
    TStream* stream = new TStream();  
    stream->LoadFromFile(GetLevelFile(name));  
    action = RemoveLineEnds(stream->buf);   
    delete(stream);  
  }  
   
#ifdef SERVERSIDENPCS 
  TServerNPC* npc = new TServerSideNPC(gifname,action,cx,cy);  
#else  
  TServerNPC* npc = new TServerNPC(gifname,action,cx,cy);  
#endif  
  level->npcs->Add(npc);   
  npc->level = level;   
  
  for (int i = 0; i<level->players->count; i++) {  
    TServerPlayer* player = (TServerPlayer*)(*level->players)[i];  
    player->SendData(SNPCPROPS, npc->GetCodedID() << npc->GetPropertyList(player));  
  }  
}  
  
void TServerPlayer::SendRCPlayerProps(TServerPlayer* player, bool sendaccount) {
  if (!Assigned(player)) return;

  JString props;
  props = player->GetCodedID();   
  props << (char)(Length(player->accountname)+32) << player->accountname;   
  props << (char)(Length(player->playerworld)+32) << player->playerworld;   

  JString proplist; 
  for (int i=0; i<playerpropertycount; i++) if (sendrcprops[i]) { 
    if (i==STATUS) { 
      int j = (player->status & (4+16+64)); 
      proplist << (char)(STATUS+32) << (char)(j+32); 
    } else 
      proplist << (char)(i+32) << player->GetProperty(i); 
  } 
  props << (char)(Length(proplist)+32) << proplist;   

  int c = player->actionflags->count;   
  props << (char)(((c >> 7) & 0x7f)+32) << (char)((c & 0x7f)+32);   
  for (int j=0; j<c; j++) {
    JString str = (*player->actionflags)[j];
    props << (char)(Length(str)+32) << str;   
  }
   
  c = player->openedchests->count;   
  props << (char)(((c >> 7) & 0x7f)+32) << (char)((c & 0x7f)+32);   
  for (int j=0; j<c; j++) {  
    JString str = (*player->openedchests)[j];
    props << (char)(Length(str)+32) << str;   
  }

  c = player->myweapons->count;   
  if (c>200) c = 200;   
  props << (char)(c+32);   
  for (int j=0; j<c; j++) {   
    JString str = (*player->myweapons)[j];   
    props << (char)(Length(str)+32) << str;   
  }

  if (sendaccount)
    SendData(DACCPLPROPS,props);
  else
    SendData(DPLPROPS,props);
}

void TServerPlayer::SetRCPlayerProps(JString data, TServerPlayer* player, bool setaccount) {
  if (!Assigned(player)) return;

  JString str;
  str << accountname << " changes ";
  if (!setaccount) str << "(the active) ";
  str << player->accountname;
  ServerFrame->tolog(str);

  TServerPlayer* player2 = new TServerPlayer(NULL,0);
  JString props;
  for (int i=0; i<playerpropertycount; i++)
    props << (char)(i+32) << player->GetProperty(i);
  player2->SetProperties(props);
  player2->accountname = player->accountname;
  player2->playerworld = player->playerworld;

  unsigned char sep = '§';
  if (player2->playerworld[Length(player2->playerworld)]!=sep) {
	player2->playerworld << "§";
  }

  if (Length(data)<1) { delete(player2); return; }
  int len = data[1]-32;
  player2->SetProperties(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);

  if (Length(data)<2) { delete(player2); return; }
  int j = ((data[1]-32)<<7) + (data[2]-32);
  data = Copy(data,3,Length(data)-2);
  player2->actionflags->Clear();
  while (j>0) {
    if (Length(data)<1) { delete(player2); return; }
    len = data[1]-32;
    if (len<0) { delete(player2); return; }
    player2->actionflags->Add(Copy(data,2,len));
    data = Copy(data,2+len,Length(data)-1-len);
    j--;
  }

  if (Length(data)<2) { delete(player2); return; }
  j = ((data[1]-32)<<7) + (data[2]-32);
  data = Copy(data,3,Length(data)-2);
  player2->openedchests->Clear();
  while (j>0) {
    if (Length(data)<1) { delete(player2); return; }
    len = data[1]-32;
    if (len<0) { delete(player2); return; }
    player2->openedchests->Add(Copy(data,2,len));
    data = Copy(data,2+len,Length(data)-1-len);
    j--;
  }

  if (Length(data)<1) { delete(player2); return; }
  j = data[1]-32;
  data = Copy(data,2,Length(data)-1);
  player2->myweapons->Clear();
  while (j>0) {
    if (Length(data)<1) { delete(player2); return; }
    len = data[1]-32;
    if (len<0) { delete(player2); return; }
    JString w = Trim(Copy(data,2,len));
    if (Length(w)>1 && player2->myweapons->indexOf(w)<0)
      player2->myweapons->Add(w);
    data = Copy(data,2+len,Length(data)-1-len);
    j--;
  }

  if (setaccount) {
    player2->kills = player->kills;
    player2->deaths = player->deaths;
    player2->onlinesecs = player->onlinesecs;
    SaveDBAccount(player2);
  } else
    player->ApplyAccountChange(player2,true);
  delete(player2);
}

void TServerPlayer::loginerror(const JString& str) {
  JString out = "Login error: "+str;
  ServerFrame->toerrorlog(out);
  SendData(DISMESSAGE,JString()+"The server got corrupt login data from your connection.");
  deleteme = true;
}

void TServerPlayer::SendAccount(const JString& line) {
  JString version, data, name, password, props, props2, props3;
  int len, i, j, clienttype;  
  TServerAccount* account;  
  TServerPlayer* player;  
  
  // Parse the data sent by the client (-> client type, version, account name, password)  
  initialized = true;  
  if (Length(line)<9) { loginerror(line); return; } 
  firstdata = false; 
  clienttype = line[1]-32; 
  if (clienttype!=CLIENTPLAYER && clienttype!=CLIENTRC) 
    clienttype = CLIENTPLAYER; 
  version = Copy(line,2,8); 
  ver = 0; 
#ifdef NEWWORLD 
  if (version=="G2NRC004") ver = 19;
  if (version=="G2N29070") ver = 27;
#else
  if (version=="GSERV019") ver = 19;
  else if (version=="GNW02070") ver = 27;
  else if (version=="GNW07080") ver = 28;
  else if (version=="GNW12080") ver = 29;
#endif

  if ((clienttype==CLIENTPLAYER && ver<27) || (clienttype==CLIENTRC && ver<18)) {
#ifdef NEWWORLD
    if (clienttype==CLIENTPLAYER) {
      SendData(DISMESSAGE,JString()+"You need Newworld version 1.39 to play on this server.");
    } else {
      SendData(DISMESSAGE,JString()+"You need RC 2000/8/19 to connect to this server.");
    }
#else
    if (clienttype==CLIENTPLAYER) {
      SendData(DISMESSAGE,JString()+"You need version 1.38 rev1 - 1.39 rev 1 to play on this server.");
    } else {
      SendData(DISMESSAGE,JString()+"You need RC 2000/7/7 to connect to this server.");
    }
#endif
    deleteme = true;    return;
  }

  data = Copy(line,10,Length(line)-9);  
  if (Length(data)<1) { loginerror(line); return; }  
  len = data[1]-32;  
  if (Length(data)<1+len) { loginerror(line); return; }  
  name = Copy(data,2,len);  
  data = Copy(data,2+len,Length(data)-1-len);  
  if (Length(data)<1) { loginerror(line); return; }  
  len = data[1]-32;  
  if (Length(data)<1+len) { loginerror(line); return; }  
  password = Copy(data,2,len);

  // Get the account
  account = GetAccount(name);
  if (!Assigned(account)) {
	//No user account found. Use a guest account.
	for (int i = 0; i < sizeof(guestaccounts)/sizeof(guestaccounts[0]); i++) {
		if ((guestips[i] < 200) or (guestips[i] == sock->ip)) {
			//There is a spare guest account. Give it to unknown client.
			name = guestaccounts[i];
			account = GetAccount(name);
			if (!Assigned(account)) {
				password = 1998;
			} else {
				account->password=password;
				guestips[i] = sock->ip;
				SendData(DISMESSAGE,JString()+"ID: " + sock->ip + "/" + guestips[i] + " Account: " + name);
				startmessagespecial = "You have joined the server using the '" +name+ "' guest account!";
				break;
			}
		}
		if (i == sizeof(guestaccounts)/sizeof(guestaccounts[0])-1){
			//Out of guest accounts. Kick unknown client.
			if (password == 1998) {
				SendData(DISMESSAGE,JString()+"The account name is incorrect! Want an account? Check the Discord!");
			} else {
				SendData(DISMESSAGE,JString()+"There is no account with the name " + name + "! Want an account? Check the Discord!");
			}
			deleteme = true;
			return;
		}
	}
  }
  
  if (!comparepassword(account->password,password)) {
    SendData(DISMESSAGE,JString()+"The password is wrong!");
    delete(account);
    deleteme = true;
    return;
  }
  if (account->blocked) {
    SendData(DISMESSAGE,JString()+"The account has been disabled by an admin.");
    delete(account);
    deleteme = true;
    return;
  }

  // Send the account data to the player
  switch (clienttype) {
    case CLIENTPLAYER:
      {
        // check for idle players using this account & disconnect them
        bool playeronaccount = false;
        if (!account->onlyload) {
          player = ServerFrame->GetPlayerForAccount(account->name);
          if (Assigned(player)) {
            if (player->noactioncounter>30) {
              player->SaveMyAccount();
              player->accountname = "";
              player->SendData(DISMESSAGE,JString()+"Another client logged in using your account.");
              player->deleteme = true;
            } else
              playeronaccount = true;
          }
        }

        if (playeronaccount) {
          SendData(DISMESSAGE,JString()+"The account is already in use.");
          deleteme = true;
        } else {

          // copy the account props to this player entity
          accountname = account->name;
          adminlevel = account->adminlevel;

          if (account->onlyload) accountname = "#" + accountname;
          adminworlds = account->adminworlds;
          LoadDBAccount(this,account->name,"");

          if (maxpower<3) maxpower = 3;
          if (power<=0) power = 3;
          status |= 1; // pause
          changecolorscounter = 0;

          // send the player attributes to the player
          props = "";
          for (i = 0; i<playerpropertycount; i++)
            if (sendinit[i] && !(account->onlyload && ((i==HEADGIF) || (i==PLAYERCOLORS))))
              props << (char)(i+32) << GetProperty(i);
          SendData(SPLAYERPROPS,props);

          // send the script flags and npc weapons to the player
          for (i = 0; i<actionflags->count; i++)
            SendData(SSETFLAG,(*actionflags)[i]);
          for (i = 0; i<ServerFrame->serverflags->count; i++)
            SendData(SSETFLAG,(*ServerFrame->serverflags)[i]);
          for (i = 0; i<myweapons->count; i++)
            weaponstosend->Add((*myweapons)[i]);
          SendOutgoing();

          // send informations about this player to all other players
          // and send informations about the other players to this player
          props = GetCodedID();
          for (i = 0; i<playerpropertycount; i++) if (sendothersinit[i])
            props << (char)(i+32) << GetProperty(i);
          props2 = GetCodedID();
          props2 << (char)(Length(accountname)+32) << accountname;
          props2 << (char)(CURLEVEL+32) << GetProperty(CURLEVEL);
          props2 << (char)(NICKNAME+32) << GetProperty(NICKNAME);
          props2 << (char)(HEADGIF+32) << GetProperty(HEADGIF);
          for (i = 0; i<ServerFrame->players->count; i++) {
            player = (TServerPlayer*)(*ServerFrame->players)[i];
            if (Assigned(player) && player!=this && player->loggedin()) {
              props3 = player->GetCodedID();
              for (j = 0; j<playerpropertycount; j++) if (sendothersinit[j])
                props3 << (char)(j+32) << player->GetProperty(j);
              SendData(OTHERPLPROPS,props3);

              if (player->isrc)
                player->SendData(SADDPLAYER,props2);
              else {
                player->SendData(OTHERPLPROPS,props);
              }
            }
          }
          SendOutgoing();
		  //If a special message is set give it to the player.
		  player->SendData(SADMINMSG,startmessagespecial);
		  startmessagespecial = "";
          // enter the saved level
          if (!LevelFileExists(levelname)) levelname = startlevel;
          props = GetProperty(PLAYERX) + GetProperty(PLAYERY) + levelname;
          SendData(PLAYERWARPED,props);
          enterLevel(levelname,x,y,JString());
#ifdef NEWWORLD
          i = ServerFrame->nwtime;
          props = JString() + (char)(((i >> 21) & 0x7f)+32) + (char)(((i >> 14) & 0x7f)+32) +
            (char)(((i >> 7) & 0x7f)+32) + (char)((i & 0x7f)+32);
          SendData(NEWWORLDTIME,props);
#endif
          SendOutgoing();

          // send the start message to the player
          if (Length(ServerFrame->startmes)>0) SendData(STARTMESSAGE,ServerFrame->startmes);

        }
        delete(account);
        break;
      }
    case CLIENTRC:
      {
        if (account->adminlevel<1) {
          SendData(DISMESSAGE,JString()+"You don't have GP / admin rights.");
          deleteme = true;
        } /*else if (!IsIPInList(sock,account->name,ServerFrame->getDir()+"data/ipgps.txt")) {
          SendData(DISMESSAGE,JString()+"Your IP is not listed in the GP ip list.");
          deleteme = true;
        } */else {
          isrc = true;
          accountname = account->name;
          adminlevel = account->adminlevel;
          ServerFrame->tolog(accountname+" is logged in from "+sock->GetIP());
          adminworlds = account->adminworlds;

          // Initialize attributes (for the client player lists)
          nickname = accountname + " - away";
          plainnick = nickname;
          plainguild = "";
          headgif = "head25.gif";
          levelname = "";

          // disconnect all remotecontrols that have the
          // same IP like this new client
          for (i=0; i<ServerFrame->players->count; i++) {
            player = (TServerPlayer*)(*ServerFrame->players)[i];
            if (Assigned(player) && player->isrc && player!=this && player->sock->ip==sock->ip) {
              player->SendData(DISMESSAGE,JString()+"Another RC logged in from the same IP.");
              player->deleteme = true;
            }
          }

          // send all player attributes to the client
          props = GetCodedID();
          for (i=0; i<playerpropertycount; i++) if (sendothersinit[i])
            props << (char)(i+32) << GetProperty(i);
          props2 = GetCodedID();
          props2 << (char)(Length(accountname)+32) << accountname;
          props2 << (char)(CURLEVEL+32) << GetProperty(CURLEVEL);
          props2 << (char)(NICKNAME+32) << GetProperty(NICKNAME);
          props2 << (char)(HEADGIF+32) << GetProperty(HEADGIF);
          for (i = 0; i<ServerFrame->players->count; i++) {
            player = (TServerPlayer*)(*ServerFrame->players)[i];
            if (Assigned(player) && player!=this && player->loggedin()) {
              props3 = player->GetCodedID();
              props3 << (char)(Length(player->accountname)+32) << player->accountname;
              props3 << (char)(CURLEVEL+32) << player->GetProperty(CURLEVEL);
              props3 << (char)(NICKNAME+32) << player->GetProperty(NICKNAME);
              props3 << (char)(HEADGIF+32) << player->GetProperty(HEADGIF);
              SendData(SADDPLAYER,props3);

              if (player->isrc) {
                SendData(DRCLOG,"New RC: "+player->accountname);
                player->SendData(SADDPLAYER,props2);
                player->SendData(DRCLOG,"New RC: "+accountname);
              } else {
                player->SendData(OTHERPLPROPS,props);
              }
            }
          }
        }
        delete(account);
        break;
      }
  }
}

void TServerPlayer::SaveMyAccount() {
  if (!isrc && Pos('#',accountname)!=1) SaveDBAccount(this);
}  
  
void TServerPlayer::gotoNewWorld(const JString& newworld, const JString& filename,  
    double newx, double newy) {  
  weaponstosend->Clear();  
  // The new level is from another player world, so  
  // let's save the account and load the account for  
  // that player world 
  if (Assigned(carrynpc)) {  
    // If we carry an npc, then delete it in the old world  
    SendData(SDELNPC,carrynpc->GetCodedID());  
    ServerFrame->DeleteNPC(carrynpc);  
    carrynpc = NULL;  
    spritenum = spritenum%4;  
#ifdef SERVERSIDENPCS
    cursprite = 0; 
#endif  
    SendData(SPLAYERPROPS,JString((char)(PLAYERSPRITE+32)) << GetProperty(PLAYERSPRITE) <<  
      (char)(CARRYNPC+32) << GetProperty(CARRYNPC));  
  }  
  // Save the account from the old world  
  SaveMyAccount();  
  // Load the account from the new world  
  // We don't want to change filename/x/y/headgif/colors,  
  // that's why we must first get a correct property list.  
  // We do that by creating a temporary TServerPlayer object,  
  // set the properties, copy the mentioned properties  
  // that should stay the same, then create a new property  
  // list by calling taccount->Update()  
  playerworld = newworld;  
  TServerPlayer* player = new TServerPlayer(NULL,0);   

  LoadDBAccount(player,accountname,playerworld);

  player->levelname = filename;
  player->x = newx;
  player->y = newy;
  player->headgif = headgif;
#ifdef NEWWORLD 
  for (int i=0; i<8; i++) player->colors[i] = colors[i]; 
#else 
  for (int i=0; i<5; i++) player->colors[i] = colors[i]; 
#endif

  ApplyAccountChange(player,false);
  delete(player);
}  
  
void TServerPlayer::sendPosToWorld() {  
  JString props;  
  props << GetCodedID() << (char)(PLAYERCOLORS+32) << GetProperty(PLAYERCOLORS)   
    << (char)(CURLEVEL+32) << GetProperty(CURLEVEL)  
    << (char)(PLAYERX+32) << GetProperty(PLAYERX)   
    << (char)(PLAYERY+32) << GetProperty(PLAYERY);  
  for (int i=0; i<ServerFrame->players->count; i++) {  
    TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
    if (Assigned(player) && player!=this && player->loggedin()) {
      if (!player->isblocked()) player->SendData(OTHERPLPROPS,props);
      if (!isblocked() && !player->isrc) {  
        JString props2;  
        props2 << player->GetCodedID()   
          << (char)(PLAYERCOLORS+32) << player->GetProperty(PLAYERCOLORS)   
          << (char)(CURLEVEL+32) << player->GetProperty(CURLEVEL)  
          << (char)(PLAYERX+32) << player->GetProperty(PLAYERX)   
          << (char)(PLAYERY+32) << player->GetProperty(PLAYERY);
        SendData(OTHERPLPROPS,props2);  
      }  
    }  
  }
} 
  
void TServerPlayer::enterLevel(JString filename, double newx, double newy,  
    JString modtime) {

  // Load the level
  filename = LowerCaseFilename(filename);
  level = ServerFrame->LoadLevel(filename);

  if (Assigned(level)) {
    // Check if the new level is from another player world
    JString newworld;
    int i = LastDelimiter(JString()+"§",filename);
    if (i>1) newworld = Copy(filename,1,i);
    bool switchedplayerworld = false;
    if (newworld!=playerworld) {
      gotoNewWorld(newworld,filename,newx,newy);
      switchedplayerworld = true;
    }

    levelname = filename;
    x = newx;
    y = newy;
    if (level->GetLeavingTimeEntry(this)>=0) {
      // The player already knows this level;
      // only send the board modifications made since last visit
      TJStringList* modlist = level->GetModifyList(this);
      JString str;
      for (i=0; i<modlist->count; i++) str << (*modlist)[i];
      delete(modlist);
      SendData(LEVELBOARD,str);
    } else {
      // Send new level when it has been modified since last download
      if (modtime!=level->GetModTime()) {
        JString codedboard;
#ifdef SunOS
        short *buf2 = new short[64*64];
        for (int k=0; k<64*64; k++) {
          short val = level->board[k];
          buf2[k] = (val >> 8) + (val << 8);
        }
        codedboard.addbuffer((char*)buf2,64*64*2);
        delete(buf2);
#else
        codedboard.addbuffer((char*)level->board,64*64*2);
#endif

        int len = 2+codedboard.length();
        SendData(100,JString((char)(((len >> 14) & 0x7F)+32)) << (char)(((len >> 7) & 0x7F)+32) <<
          (char)((len & 0x7F)+32));
        SendData(101,codedboard);

        // Send level name, links, signs, modification time
        if (firstlevel) SendData(LEVELNAME,filename);
        firstlevel = false;
        if (Assigned(level->links)) {
          for (i = 0; i<level->links->count; i++) {
            TServerLevelLink* link = (TServerLevelLink*)(*level->links)[i];
            SendData(LEVELLINK,link->GetJStringRepresentation());
          }
        } else if (Assigned(level->linkstrings)) {
          for (i = 0; i<level->linkstrings->count; i++)
            SendData(LEVELLINK,(*level->linkstrings)[i]);
        }
        for (i = 0; i<level->signs->count; i++)
          SendData(LEVELSIGN,(*level->signs)[i]);
        SendData(LEVELMODTIME,level->GetModTime());
      } else
        SendData(LEVELBOARD,JString());
      // Send board modifications
      TJStringList* modlist = level->GetModifyList(this);
      for (i=0; i<modlist->count; i++)
        SendData(SBOARDMODIFY,(*modlist)[i]);
      delete(modlist);

      // Send chests
      for (i = 0; i<level->chests->count; i++) {
        TServerChest* chest = (TServerChest*)(*level->chests)[i];
        bool found = false;
        for (int j=0; j<openedchests->count; j++) {
          JString str = (*openedchests)[j];
          if (Length(str)<3) continue;
          int ix = str[1]-32;
          int iy = str[2]-32;
          JString chestfile = Copy(str,3,Length(str)-2);
          if ((ix==chest->x) && (iy==chest->y) && (chestfile==filename)) {
            found = true;
            break;
          }
        }
        if (found) SendData(LEVELCHEST,chest->GetOpened()); else SendData(LEVELCHEST,chest->GetClosed());
      }
    }
    // Send horses
    for (i = 0; i<level->horses->count; i++) {
      TServerHorse* horse = (TServerHorse*)(*level->horses)[i];
      if (!isblocked()) SendData(SADDHORSE,horse->GetJStringRepresentation());
    }
    // Send baddy attributes
    for (i=0; i<level->baddies->count; i++) {
      TServerBaddy* baddy = (TServerBaddy*)(*level->baddies)[i];
      JString proplist = baddy->GetPropertyList(this);
      if (Length(proplist)>0) SendData(SBADDYPROPS,JString((char)(baddy->id+32)) << proplist);
    }
    // Send npc attributes
    for (i = 0; i<level->npcs->count; i++) {
      TServerNPC* npc = (TServerNPC*)(*level->npcs)[i];
      JString proplist = npc->GetPropertyList(this);
      if (Length(proplist)>0) SendData(SNPCPROPS,npc->GetCodedID() << proplist);
#ifdef SERVERSIDENPCS
      ((TServerSideNPC*)npc)->doNPCAction("playerenters",this);
#endif
    }
    // Update carrynpc attribute modification time so
    // that the players in this new level get all changed attributes
    if (Assigned(carrynpc)) for (i=0; i<npcpropertycount; i++)
      if (carrynpc->modifytime[i]>0) carrynpc->modifytime[i] = curtime;
    // Send player attributes
    for (i = 0; i<level->players->count; i++) {
      TServerPlayer* player = (TServerPlayer*)(*level->players)[i];
      if (Assigned(carrynpc))
        player->SendData(SNPCPROPS,carrynpc->GetCodedID() << carrynpc->GetPropertyList(player));
      JString proplist = player->GetCodedID() << (char)(50+32) << (char)(1+32) << player->GetPropertyList(this);
      SendData(OTHERPLPROPS,proplist);
      proplist = GetCodedID() << (char)(50+32) << (char)(1+32) << GetPropertyList(player);
      player->SendData(OTHERPLPROPS,proplist);
    }

    if (Assigned(carrynpc)) {
      level->npcs->Add(carrynpc);
      carrynpc->level = level;
    }
    level->players->Add(this);
    level->RemoveFromList(this); 
    if (level->players->count==1) SendData(ISLEADER,JString());

    // Send position for other peoples map
    if (switchedplayerworld)
      sendPosToWorld();
    else {
      JString proplist = GetCodedID() << (char)(CURLEVEL+32) << GetProperty(CURLEVEL) <<
        (char)(PLAYERX+32) << GetProperty(PLAYERX) << (char)(PLAYERY+32) << GetProperty(PLAYERY);
      for (i=0; i<ServerFrame->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
        if (Assigned(player) && player->loggedin() && !player->isblocked() &&
            (player->isrc || (level->players->indexOf(player)<0 && player->playerworld==playerworld)))
          player->SendData(OTHERPLPROPS,proplist);
      }
    }
  }
}

void TServerPlayer::leaveLevel() {

  if (Assigned(level)) {
    // Set someone else as 'leader' if we are the player #0
    if ((level->players->count>=2) && ((*level->players)[0]==this))
      ((TServerPlayer*)(*level->players)[1])->SendData(ISLEADER,JString());
    level->players->Remove(this);
    level->AddToList(this);
    // Inform other players about the npc moving out of the level

    if (Assigned(carrynpc)) {
      level->npcs->Remove(carrynpc);
      for (int i=0; i<ServerFrame->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
        if (Assigned(player) && player->loggedin() && !player->isrc && player!=this &&
            level->GetLeavingTimeEntry(player)>=0)
          player->SendData(SDELNPC,carrynpc->GetCodedID());
      }
      carrynpc->level = NULL;
      carrynpc->moved = true;
      carrynpc->orgindex = -1;
    }
#ifdef SERVERSIDENPCS
    for (int i=0; i<level->npcs->count; i++) {
      TServerSideNPC* npc = (TServerSideNPC*)(*level->npcs)[i];
      npc->doNPCAction("playerleaves",this);
    }
#endif
    // Inform other players about us leaving the level
    for (int i=0; i<level->players->count; i++) {
      TServerPlayer* player = (TServerPlayer*)(*level->players)[i];
      player->SendData(OTHERPLPROPS,GetCodedID() << (char)(50+32) << (char)(0+32));
      if (Assigned(carrynpc))
        player->SendData(SDELNPC,carrynpc->GetCodedID());
      SendData(OTHERPLPROPS,player->GetCodedID() << (char)(50+32) << (char)(0+32));
    }
  }
  level = NULL;
//  levelname = "";
}

JString TServerPlayer::GetProperty(int index) {  
  int sp;  
  JString str, str2;  
  
  switch (index) {  
    case NICKNAME:    return JString((char)(Length(nickname)+32)) << nickname;  
    case MAXPOWER:    return JString((char)(maxpower+32));  
    case CURPOWER:    return JString((char)((power*2)+32));  
    case RUPEESCOUNT: return JString((char)(((rubins >> 14) & 0x7F)+32)) <<  
      (char)(((rubins >> 7) & 0x7F)+32) << (char)((rubins & 0x7F)+32);  
    case ARROWSCOUNT: return JString((char)(darts+32));  
    case BOMBSCOUNT:  return JString((char)(bombscount+32));  
    case GLOVEPOWER:  return JString((char)(glovepower+32));  
    case BOMBPOWER:   return JString((char)(bombpower+32));
#ifdef NEWWORLD
    case PLAYERANI:   return JString((char)(Length(ani)+32)) << ani;
#else
    case SWORDPOWER: {
        sp = swordpower+30;
        if (sp<10) sp = 10;
        if (sp>50) sp = 50;
        if (sp==30) sp = 0;
	str = JString((char)(sp+32));
        str2 = RemoveGifExtension(swordgif);
        if (sp>=10) str << (char)(Length(str2)+32) << str2;
        return str;
      }
#endif
    case SHIELDPOWER: {  
        sp = shieldpower + 10;  
        if (sp<10) sp = 10;  
        if (sp>13) sp = 13;  
        if (sp==10) sp = 0;  
	str = JString((char)(sp+32));  
        str2 = RemoveGifExtension(shieldgif);  
        if (sp>=10) str << (char)(Length(str2)+32) << str2;  
        return str;  
      }  
    case SHOOTGIF: return shootgif;  
    case HEADGIF: {  
        if (Length(headgif)>0) {  
          sp = getGifNumber(headgif,JString("head"));  
          if ((sp>=0) && (sp<100))  
            return JString((char)(sp+32));  
          else {  
            str = RemoveGifExtension(headgif);  
            if (Length(str)>100) str = Copy(str,1,100);  
            return JString((char)(100+Length(str)+32)) << str;  
          }  
        } else  
          return JString((char)(0+32));  
      }  
    case CURCHAT:      return JString((char)(Length(curchat)+32)) << curchat;  
    case PLAYERCOLORS: {  
#ifdef NEWWORLD  
         return JString((char)(colors[0]+32)) << (char)(colors[1]+32)  
           << (char)(colors[2]+32) <<  (char)(colors[3]+32) << (char)(colors[4]+32)  
           << (char)(colors[5]+32) <<  (char)(colors[6]+32) << (char)(colors[7]+32);  
#else  
         return JString((char)(colors[0]+32)) << (char)(colors[1]+32)  
           << (char)(colors[2]+32) << (char)(colors[3]+32) << (char)(colors[4]+32); 
#endif 
      } 
    case PLAYERID: return GetCodedID(); 
    case PLAYERX:      return JString((char)((x*2)+32));  
    case PLAYERY:      return JString((char)((y*2)+32));  
    case PLAYERSPRITE: return JString((char)(spritenum+32));  
    case STATUS:       return JString((char)(status+32));  
    case CARRYSPRITE:  return JString((char)(carrysprite+32));  
    case CURLEVEL: {  
        /*if (Assigned(level)) str = level->plainfile;  
        else*/ str = levelname;  
        return JString((char)(Length(str)+32)) << str;  
      }  
    case HORSEGIF: {  
        str = RemoveGifExtension(horsegif);  
        return JString((char)(Length(str)+32)) << str;  
      }  
    case HORSEBUSHES: return JString((char)(horsebushes+32));  
    case EFFECTCOLORS: return JString((char)(0+32));  
    case CARRYNPC: {  
        if (Assigned(carrynpc))  
          return carrynpc->GetProperty(17);  
        else  
          return JString((char)(32)) << (char)(32) << (char)(32);
      }  
    case APCOUNTER: {  
        sp = apcounter+1;  
        return JString((char)(((sp >> 7) & 0x7F)+32)) << (char)((sp & 0x7F)+32);  
      }  
    case MAGICPOINTS: return JString((char)(magicpoints+32));  
    case KILLSCOUNT:  return JString((char)(((kills >> 14) & 0x7F)+32)) <<  
      (char)(((kills >> 7) & 0x7F)+32) << (char)((kills & 0x7F)+32);  
    case DEATHSCOUNT: return JString((char)(((deaths >> 14) & 0x7F)+32)) <<  
      (char)(((deaths >> 7) & 0x7F)+32) << (char)((deaths & 0x7F)+32);  
    case ONLINESECS:  return JString((char)((onlinesecs >> 14)+32)) <<  
      (char)(((onlinesecs >> 7) & 0x7F)+32) << (char)((onlinesecs & 0x7F)+32);  
    case LASTIP: {  
        unsigned int ip = 0;  
        if (Assigned(sock)) ip = sock->ip;  
        else ip = lastip;  
        return JString((char)(((ip >> 28) & 0x7F)+32)) << (char)(((ip >> 21) & 0x7F)+32) <<  
          (char)(((ip >> 14) & 0x7F)+32) << (char)(((ip >> 7) & 0x7F)+32) <<
          (char)((ip & 0x7F)+32);  
      }  
    case UDPPORT:      return JString((char)(((udpport >> 14) & 0x7F)+32)) <<
      (char)(((udpport >> 7) & 0x7F)+32) << (char)((udpport & 0x7F)+32);  
    case PALIGNMENT:   return JString((char)(alignment+32));  
    case PADDITFLAGS:  return JString((char)(additionalflags+32));  
    case PACCOUNTNAME: return JString((char)(Length(accountname)+32)) << accountname;  
  }  
  return JString();  
}  

bool isimage(JString filename) {
  if (Length(filename)<=0) return false;

  // Check for plain file
  int i = LastDelimiter("/\\",filename);
  if (i>0) return false;

  // Check for the right extension
  i = LastDelimiter('.',filename);
  if (i==0) return false;
  JString ext = Copy(filename,i+1,Length(filename)-i);
  if (ext!="gif" && ext!="png" && ext!="mng") return false;

  return true;
}

void TServerPlayer::SetShield(JString newshield) {
  newshield = LowerCaseFilename(newshield);
  if (isimage(newshield) && FileExists(curdir+"shields"+fileseparator+newshield)) {
    shieldgif = newshield;
    JString props;
    props << (char)(SHIELDPOWER+32) << GetProperty(SHIELDPOWER);
    SendData(SPLAYERPROPS,props);
    SetProperties(props);
  }
}

//#ifndef NEWWORLD
void TServerPlayer::SetSword(JString newsword) {
  newsword = LowerCaseFilename(newsword);
  if (newsword=="policesword.gif" && adminlevel<2) return;
  int newsp = swordpower;
  if (newsword=="sword1.gif") newsp = 1;
  if (newsword=="sword2.gif") newsp = 2;
  if (newsword=="sword3.gif") newsp = 3;
  if (newsword=="sword4.gif") newsp = 4;
  // Make sure that the / is appended
  // Matriark TerVel
  if (curdir[Length(curdir)]!='/') curdir << "/";
  if ((newsp==swordpower || (swordpower==0 && newsp==1)) &&
      isimage(newsword) && FileExists(curdir+"swords"+fileseparator+newsword)) {
    swordgif = newsword;
    JString props;
    props << (char)(SWORDPOWER+32) << GetProperty(SWORDPOWER);
    SendData(SPLAYERPROPS,props);
    SetProperties(props);
  }
}
//#endif

void TServerPlayer::SendToWorld(int attribute) {
  if (loggedin()) {
    JString props = GetCodedID() << (char)(attribute+32) << GetProperty(attribute);
    for (int i=0; i<ServerFrame->players->count; i++) {
      TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
      if (Assigned(player) && player!=this && player->loggedin() && !player->isrc &&
          !player->isblocked() && player->playerworld==playerworld)
        player->SendData(OTHERPLPROPS,props);
    }
  }
}

void TServerPlayer::SetProperties(JString str) {
  int i, j, index, datalen, len, sp;
  TServerPlayer* player;
  TServerNPC* npc;
  JString toothers,toudpless,toadmins,props,guild;
#ifdef SERVERSIDENPCS
  bool moved = false;
#endif

  toothers << GetCodedID();  
  while (Length(str)>=2) {  
  
    index = str[1]-32;
    datalen = 1;  
    bool dosend = false;  
    if (index>=0 && index<=PACCOUNTNAME)  
      dosend = sendtoothers[index];  
  
    switch (index) {  
      case NICKNAME: {  
          len = str[2]-32;
          if (len<0) len = 0;
          props = Copy(str,3,len);
          datalen = 1+len;

          if (Length(props)>200) props = Copy(props,1,200);  
          nickname = Trim(props);
          guild = "";
          i = Pos('(',nickname);  
          if (i>0) {  
            guild = Trim(Copy(nickname,i+1,Length(nickname)-i));  
            nickname = Trim(Copy(nickname,1,i-1));  
            i = Pos(')',guild);  
            if (i>0) guild = Trim(Copy(guild,1,i-1)); else guild = "";  
          }
          if (nickname==accountname)  
            nickname = "*" + nickname;  
          else while (Pos('*',nickname)==1)  
            nickname = Copy(nickname,2,Length(nickname)-1);  
          if (Length(nickname)<=0) nickname = "unknown";  
          plainnick = nickname;  
  
          if (VerifyGuildName(guild,nickname,accountname)) {  
            plainguild = guild;
            nickname << " (" << guild << ")";  
          } else {  
            plainguild = "";  
          }  
          if (isrc && adminworlds=="all" && nickname==JString('*')+accountname) {  
            nickname << " (RC)";  
            plainguild = "RC";  
          }  
  
          if (loggedin()) {
            if (nickname!=props)
              SendData(SPLAYERPROPS,JString((char)(NICKNAME+32)) << GetProperty(NICKNAME));
            props = GetCodedID() << (char)(NICKNAME+32) << GetProperty(NICKNAME);
            for (i = 0; i<ServerFrame->players->count; i++) {
              player = (TServerPlayer*)(*ServerFrame->players)[i];
              if (Assigned(player) && player!=this && player->loggedin())
                player->SendData(OTHERPLPROPS,props);
            }
          }
          break;  
        }  
      case MAXPOWER: {  
          maxpower = str[2]-32;  

          struct CfgStrings CfgSettings[] =
          {
             { "maxpower", NULL },
             { NULL, NULL }
          };

          CfgRead("data/settings.txt", CfgSettings);
    
          int maxlimit = 20;
          
          for (int i = 0; i < 5; i++) {
            if (CfgSettings[i].name == "maxpower")
              maxlimit = atoi(CfgSettings[i].data);
          }

          if (maxpower>maxlimit) {
            maxpower = maxlimit;  
            if (loggedin()) {  
              props = JString((char)(MAXPOWER+32))+GetProperty(MAXPOWER);  
              SendData(SPLAYERPROPS,props);  
              SetProperties(props);  
              dosend = false;  
            }
          }  
          break;  
        }  
      case CURPOWER: {  
          if (((status & 32)==0) || (power<=0))  
            power = ((double)(str[2]-32))/2;
          if (power>maxpower) {
            power = maxpower;  
            if (loggedin())  
              SendData(SPLAYERPROPS,JString()+(char)(CURPOWER+32)+GetProperty(CURPOWER));  
          }
          break;
        }  
      case RUPEESCOUNT: {
          if (Length(str)>=4) {  
            rubins = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
            if (rubins>9999999) {  
              rubins = 9999999;  
              if (loggedin())  
                SendData(SPLAYERPROPS,JString()+(char)(RUPEESCOUNT+32)+GetProperty(RUPEESCOUNT));
            }
          }
          datalen = 3;
          break;
        }
      case ARROWSCOUNT: {
          darts = str[2]-32;
          if (darts>99) {
            darts = 99;
            if (loggedin())
              SendData(SPLAYERPROPS,JString()+(char)(ARROWSCOUNT+32)+GetProperty(ARROWSCOUNT));
          }
          break;
        }
      case BOMBSCOUNT: {
          bombscount = str[2]-32;
          if (bombscount>99) {
            bombscount = 99;
            if (loggedin())  
              SendData(SPLAYERPROPS,JString()+(char)(BOMBSCOUNT+32)+GetProperty(BOMBSCOUNT));  
          }
          break;
        }
      case GLOVEPOWER: glovepower = str[2]-32; break;
      case BOMBPOWER: {
          sp = str[2]-32;
          if (sp>=0 && sp<=3) bombpower = sp;
          break;
        }
#ifdef NEWWORLD
      case PLAYERANI: {
          len = str[2]-32;
          if (len<0) len = 0;
          ani = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
#else
      case SWORDPOWER: {
          sp = str[2]-32;
          props = "";
          if (sp>=10) {
            sp = sp-30;
            len = str[3]-32;
            if (len<0) len = 0;
            if (len>0) {
              props = Copy(str,4,len);
              if (len>0 && props==RemoveExtension(props))
                props << ".gif";
            }
            datalen = 2+len;
          } else {
            switch (sp) {
              case 0: props = ""; break;
              case 1: props = "sword1.gif"; break;
              case 2: props = "sword2.gif"; break;
              case 3: props = "sword3.gif"; break;
              case 4: props = "sword4.gif"; break;
            }
          }
          if ((sp<=3 || (Length(playerworld)>0 && Pos("classic",playerworld)!=1)) &&
              (props!="policesword.gif" || adminlevel>=2) &&
              FileExists(curdir+"swords"+fileseparator+props)) {
            swordpower = sp;
            swordgif = props;
          } else {
            dosend = false;
            if (loggedin())
              SendData(SPLAYERPROPS,JString()+(char)(SWORDPOWER+32)+GetProperty(SWORDPOWER));
          }
          break;
        }
#endif
      case SHIELDPOWER: {
          sp = str[2]-32;
          props = "";
          if (sp>=10) {
            sp = sp-10;
            len = str[3]-32;
            if (len<0) len = 0;
            if (len>0) {
              props = Copy(str,4,len);
              if (len>0 && props==RemoveExtension(props))
                props << ".gif";
            }
            datalen = 2+len;
          } else {
            switch (sp) {
              case 0: props = ""; break;
              case 1: props = "shield1.gif"; break;
              case 2: props = "shield2.gif"; break;
              case 3: props = "shield3.gif"; break;
            }
          }
          if (sp<=3 && (props!="policeshield.gif" || adminlevel>=2) &&
              FileExists(curdir+"shields"+fileseparator+props)) {
            shieldpower = sp;
            shieldgif = props;
          } else {
            dosend = false;
            if (loggedin())
              SendData(SPLAYERPROPS,JString()+(char)(SHIELDPOWER+32)+GetProperty(SHIELDPOWER));
          }
          break;
        }
      case SHOOTGIF: {
          sp = str[2]-32;
          if (sp<10) shootgif = JString((char)str[2]);
          else {
            sp = sp-10;  
            shootgif = Copy(str,2,1+sp);  
            datalen = 1+sp;  
          }  
          break;  
        }
      case HEADGIF: {  
          len = str[2]-32;
          if (len<0)
            headgif = "head0.gif";
          else if (len<100)
            headgif = JString("head") << len << ".gif";
          else {
            len = len-100;
            headgif = LowerCase(Copy(str,3,len));
            if (len>0 && headgif==RemoveExtension(headgif))
              headgif << ".gif";
            datalen = 1+len;
          }
          if (!isimage(headgif) || !FileExists(curdir+"heads"+fileseparator+headgif))
            headgif = "head0.gif";
          if (loggedin()) {
            props = GetCodedID() << (char)(HEADGIF+32) << GetProperty(HEADGIF);
            for (i = 0; i<ServerFrame->players->count; i++) {
              player = (TServerPlayer*)(*ServerFrame->players)[i];
              if (player!=this && player->loggedin()) player->SendData(OTHERPLPROPS,props);
            }
          }
          break;  
        }  
      case CURCHAT: {  
          len = str[2]-32;  
          if (len<0) len = 0;  
          curchat = Copy(str,3,len);  
          if (Length(curchat)>220) curchat = Copy(curchat,1,220);  
          datalen = 1+len;  

          if (loggedin()) parseChat();  
  
          noactioncounter = 0;  
          break;  
        }  
      case PLAYERCOLORS: {
#ifdef NEWWORLD
          datalen = 8;
          if (Length(str)>=9 && changecolorscounter<=0) { 
            for (i = 0; i<8; i++) { 
#else
          datalen = 5;
          if (Length(str)>=6 && changecolorscounter<=0) {
            for (i = 0; i<5; i++) {
#endif  
              int col = str[2+i]-32;
              if (col<0) col = 0;  
              if (col>19) col = 19;  
              colors[i] = col;  
            }  
            changecolorscounter = changecolorstimeout;
            SendToWorld(PLAYERCOLORS);
            dosend = false;
          }
          break;  
        }  
      case PLAYERID: {
          datalen = 2;
          break;
        }
      case PLAYERX: {
          if (power>0) x = ((double)(str[2]-32))/2;
          status &= (-1-1); // unpause
          nomovementcounter = 0;
          noactioncounter = 0;
#ifdef SERVERSIDENPCS
          moved = true;
#endif
          break;
        }
      case PLAYERY: {
          if (power>0) y = ((double)(str[2]-32))/2;
          status &= (-1-1); // unpause
          nomovementcounter = 0;
          noactioncounter = 0;
#ifdef SERVERSIDENPCS
          moved = true;
#endif
          break;
        }  
      case PLAYERSPRITE: {  
          if (power>0) spritenum = str[2]-32;  
          if (spritenum<0 || spritenum>=132) spritenum = 2;  
#ifdef SERVERSIDENPCS  
          cursprite = spritenum/4;  
          dir = spritenum%4;  
#endif  
          noactioncounter = 0;  
          break;  
        }  
      case STATUS: {  
          i = status;  
          status = str[2]-32;  
          if ((i & 8)==0 && (status & 8)>0) deaths++;  
          if ((status & 8)>0 && Assigned(level) && loggedin()) {  
            if ((level->players->count>=2) && ((*level->players)[0]==this)) {  
              level->players->Remove(this);  
              level->players->Add(this);
              ((TServerPlayer*)(*level->players)[0])->SendData(ISLEADER,JString());  
            }  
          }  
          break;  
        }  
      case CARRYSPRITE: carrysprite = str[2]-32; break;  
      case CURLEVEL:  
        {  
          len = str[2]-32;  
          if (len<0) len = 0;  
          levelname = Copy(str,3,len);  
          datalen = 1+len;  
          break;
        }  
      case HORSEGIF: {  
          len = str[2]-32;  
          if (len<0) len = 0;  
          if (len>0) {  
            horsegif = Copy(str,3,len);  
            if (len>0 && horsegif==RemoveExtension(horsegif))  
              horsegif << ".gif";  
          } else  
            horsegif = "";  
          datalen = 1+len;  
          break;  
        }  
      case HORSEBUSHES: horsebushes = str[2]-32; break;
      case EFFECTCOLORS: {  
          if ((str[2]-32) > 0) datalen = 1+4;  
          break;  
        }
      case CARRYNPC: {  
          if (Length(str)>=4) {  
            i = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
            if (loggedin() && !(Assigned(carrynpc) && (carrynpc->id==i))) {  
              if (Assigned(carrynpc) && (carrynpc->id != i)) {  
                carrynpc = NULL;  
              }  
              if (i>0) {  
                npc = NULL;  
                for (j = 0; j<level->npcs->count; j++) {  
                  npc = (TServerNPC*)(*level->npcs)[j];  
                  if (npc->id==i) break; else npc = NULL;  
                }  
                if (Assigned(npc)) for (j = 0; j<level->players->count; j++) {
                  player = (TServerPlayer*)(*level->players)[j];  
                  if ((player != this) && (player->carrynpc==npc)) {  
                    npc = NULL;  
                    break;  
                  }  
                }  
                if (Assigned(npc)) {  
//                  npc->visflags = npc->visflags || 1;  
                  carrynpc = npc;
                } else {  
                  spritenum = spritenum % 4;  
#ifdef SERVERSIDENPCS 
                  cursprite = 0; 
#endif 
                  SendData(SPLAYERPROPS,JString((char)(PLAYERSPRITE+32)) << GetProperty(PLAYERSPRITE) <<  
                    (char)(CARRYNPC+32) << GetProperty(CARRYNPC));  
                }
              }  
            }  
          }  
          datalen = 3;  
          break;  
        }  
      case APCOUNTER: {  
          if (Length(str)>=3) {  
            apcounter = ((str[2]-32) << 7) + (str[3]-32) -1;  
            if (apcounter<0 || apcounter>apincrementtime)  
              apcounter = apincrementtime;  
          }  
          datalen = 2;  
          break;  
        }
      case MAGICPOINTS: {  
          magicpoints = str[2]-32;  
          if (magicpoints>100) {  
            magicpoints = 100;
            if (loggedin())  
              SendData(SPLAYERPROPS,JString((char)(MAGICPOINTS+32)) << GetProperty(MAGICPOINTS));  
          }  
          break;  
        }  
      case KILLSCOUNT: {  
          if (Length(str)>=4)  
            kills = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
          datalen = 3;  
          break;  
        }  
      case DEATHSCOUNT: {
          if (Length(str)>=4)  
            deaths = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
          datalen = 3;  
          break;  
        }  
      case ONLINESECS: {  
          if (Length(str)>=4) {  
            onlinesecs = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
            if (onlinesecs>220*128*128 || onlinesecs<0) onlinesecs = 0;  
          }  
          datalen = 3;  
          break;  
        }  
      case LASTIP: {  
          if (Length(str)>=6) {
            lastip = ((str[2]-32) << 28) + ((str[3]-32) << 21) + ((str[4]-32) << 14) +
              ((str[5]-32) << 7) + (str[6]-32);  
          }  
          datalen = 5;  
          break;  
        }  
      case UDPPORT: {  
          if (Length(str)>=4) {  
            // Disable UDP because it's too buggy!
            //udpport = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);  
            if (loggedin()) {  
              props = GetCodedID() << (char)(UDPPORT+32) << GetProperty(UDPPORT);  
              for (i = 0; i<ServerFrame->players->count; i++) {  
                player = (TServerPlayer*)(*ServerFrame->players)[i];  
                if (player!=this && !player->isrc && player->loggedin())  
                  player->SendData(OTHERPLPROPS,props);  
              }
            }  
          }  
          datalen = 3;  
          break;  
        }  
      case PALIGNMENT: {  
          alignment = str[2]-32;  
          if (alignment>100) alignment = 100;  
          if (alignment<0) alignment = 0;  
          break;  
        }
      case PADDITFLAGS: additionalflags = str[2]-32; break;  
      case PACCOUNTNAME: {  
          // cannot be set with SetProperties()  
          len = str[2]-32;  
          if (len<0) len = 0;  
          datalen = 1+len;
          break;  
        }  
  
      default: {  
          if (id>0) {   
            JString out = "Client "+accountname+" has sent wrong player attributes ("+index+")";  
            ServerFrame->toerrorlog(out);  
            SendData(DISMESSAGE,JString()+"The server got illegal player attributes data from your connection.");  
            deleteme = true;  
          }  
          return;  
        }  
    }  
    if (dosend) toothers << Copy(str,1,1+datalen);
#ifdef NEWWORLD
    if (index==PLAYERANI || index==CURCHAT || (index>=PLAYERX && index<=CARRYSPRITE))
#else
    if (index==CURCHAT || (index>=PLAYERX && index<=CARRYSPRITE))
#endif
      toudpless << Copy(str,1,1+datalen);
    if (sendtoadmins[index]) toadmins << Copy(str,1,1+datalen);
//    str = Copy(str,2+datalen,Length(str)-1-datalen);
    str.setTempStart(2+datalen);
  }
  if (Assigned(level) && loggedin()) {
    toudpless = toothers + toudpless;
    if (Length(toudpless)>2)
        for (i = 0; i<level->players->count; i++) {
      player = (TServerPlayer*)(*level->players)[i];
      if (Assigned(player) && player!=this)
        if (udpport<=0 || player->udpport<=0)
          player->SendData(OTHERPLPROPS,toudpless);
        else if (Length(toothers)>2)
          player->SendData(OTHERPLPROPS,toothers);
    }
  }
  if (Length(toadmins)>0 && loggedin()) {
    toadmins = GetCodedID() << toadmins;
    for (i = 0; i<ServerFrame->players->count; i++) {
      player = (TServerPlayer*)(*ServerFrame->players)[i];
      if (Assigned(player) && player->isrc && player!=this && player->loggedin())
        player->SendData(SCHPLPROPS,toadmins);
    }
  }
#ifdef SERVERSIDENPCS
  if (loggedin() && moved && Assigned(level))
    level->TestPlayerTouchsNPC(this);
#endif
}

void TServerPlayer::updateCurrentLevel() {
  if (!Assigned(level)) return;

  TJStringList* list = new TJStringList();
  list->SetPlainMem();
  list->Add(level->plainfile);
  ServerFrame->UpdateLevels(list);
  delete(list);
}
extern int bank(const JString& accname,const char* transt,int value);
void TServerPlayer::parseChat() {  
  if (curchat=="unstuck me" || curchat=="unstick me") {  
    curchat = "";  
    // move him to the starting level  
    if (nomovementcounter>=30) {
      nomovementcounter = 0;
      ToStartLevel(accountname);
    } else {
      curchat = JString("Don't move for 30 seconds before doing 'unstick me'!");
      SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
    }
  } else if (Pos("update level",curchat)==1) {  
    JString toupdate = Trim(Copy(curchat,13,Length(curchat)-12));  
    if (Length(toupdate)<=0 && Assigned(level))  
      toupdate = level->plainfile;  
    curchat = "";  
    if (adminlevel>=1 && isadminin(playerworld) && Length(toupdate)>0) {  
      // reload the level in the server's cache  
      TJStringList* list = new TJStringList();
      list->SetPlainMem();
      list->Add(toupdate);
      ServerFrame->UpdateLevels(list);  
      delete(list);  
    }
  } else if (curchat=="showkills") {
    // send him his current kills count as his own chat text
    curchat = JString("kills: ") << kills;
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  }
if (strstr(level->plainfile.text(), "bank")) {
 if (curchat=="bank balance") {
    // send him his bank balance as the current chat
    curchat = JString("Your bank balance is: ") << bank(accountname,"balance",0);
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } 
 else if (strstr(curchat.text(),"bank deposit")) {
    JString bvalue = Trim(Copy(curchat,13,Length(curchat)-12));
    if (rubins>=strtoint(bvalue)) {
      if (bank(accountname,"deposit",strtoint(bvalue))) {
	 curchat = JString("Your new balance is: ") << bank(accountname,"balance",0); 
	 rubins -= strtoint(bvalue);
	}
      else curchat = JString("Transactioin failed to complete.");
    }
    SendData(SPLAYERPROPS,JString()+(char)(RUPEESCOUNT+32)+GetProperty(RUPEESCOUNT));
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } 
else if (strstr(curchat.text(),"bank withdraw")) {
    JString bvalue = Trim(Copy(curchat,14,Length(curchat)-13));
    if (bank(accountname,"withdraw",strtoint(bvalue))) { 
	curchat = JString("Your new balance is: ") << bank(accountname,"balance",0);
	rubins += strtoint(bvalue);
    }
    else curchat = JString("Transactioin failed to complete.");
    SendData(SPLAYERPROPS,JString()+(char)(RUPEESCOUNT+32)+GetProperty(RUPEESCOUNT));
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } 
}
else if (curchat=="showdeaths") {
    // send him his current deaths count as his own chat text
    curchat = JString("deaths: ") << deaths;
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } else if (curchat=="showonlinetime") {
    // send him his current online time as his own chat text
    curchat = "";
    int sp = onlinesecs;
    curchat << (sp%60) << "s";
    sp = sp / 60;
    if (sp>0) curchat = JString(sp%60) << "m " << curchat;
    sp = sp / 60;
    if (sp>0) curchat = JString(sp) << "h " << curchat;
    curchat = JString("onlinetime: ") << curchat;
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } else if (curchat=="showadmins") {
    curchat = "admins: ";
    int j = 0;
    for (int i=0; i<ServerFrame->players->count; i++) {
      TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
      if (Assigned(player) && player->isrc) {
        if (j>0) curchat << ", ";
        curchat << player->accountname;
        j++;
      }
    }
    if (j<=0) curchat << "(no one)";
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } else if (Pos("showguild",curchat)==1) {
    JString otherguild = Trim(Copy(curchat,10,Length(curchat)-9));
    if (Length(otherguild)<=0) otherguild = plainguild;
    if (Length(otherguild)>0) {
      curchat = "";
      for (int i=0; i<ServerFrame->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
        if (player->plainguild==otherguild) {
          if (Length(curchat)>0) curchat << ", ";
          curchat << player->plainnick;
        }
      }
      if (Length(curchat)<=0) curchat << "(no one)";
      curchat = JString("members of '") << otherguild << "': " << curchat;
    } else
      curchat = "(you are not in a guild)";
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } else if (Pos("toguild:",curchat)==1) {
    JString message = Trim(Copy(curchat,9,Length(curchat)-8));
    int count = 0;
    if (Length(plainguild)>0 && Length(message)>0) {
      JString props;
      props << GetCodedID() << "\"Guild message:\",\"" << message << "\"";
      for (int i=0; i<ServerFrame->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
        if (Assigned(player) && player!=this && player->loggedin() && !player->isblocked() &&
            player->plainguild==plainguild) {
          player->SendData(SPRIVMESSAGE,props);
          count++;
        }
      }
    }
    if (count==1)
      curchat = JString("(") << count << " guild member got your message)";
    else
      curchat = JString("(") << count << " guild members got your message)";
    SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));
  } else if (Pos("sethead",curchat)==1) {
    JString newhead = LowerCaseFilename(Trim(Copy(curchat,8,Length(curchat)-7)));
    if (Length(newhead)>100) newhead = Copy(newhead,1,100);

    if (isimage(newhead) && FileExists(curdir+"heads"+fileseparator+newhead)) {
      headgif = newhead;
      JString props;
      props << (char)(HEADGIF+32) << GetProperty(HEADGIF);
      SendData(SPLAYERPROPS,props);
      SetProperties(props);
    }
  } /*else if (Pos("setbody",curchat)==1) {
    JString newhead = LowerCaseFilename(Trim(Copy(curchat,8,Length(curchat)-7)));
    if (Length(newhead)>100) newhead = Copy(newhead,1,100);

    if (isimage(newhead) && FileExists(curdir+"bodies"+fileseparator+newhead)) {
      headgif = newhead;
      JString props;
      props << (char)(BODYIMG+32) << GetProperty(BODYIMG);
      SendData(SPLAYERPROPS,props);
      SetProperties(props);
    }
  } */else if (Pos("setshield",curchat)==1) {
    SetShield(Trim(Copy(curchat,10,Length(curchat)-9)));
  } else if (Pos("setsword",curchat)==1) {
    SetSword(Trim(Copy(curchat,9,Length(curchat)-8)));
  } else if (Pos("setnick",curchat)==1) {
    if (changenickcounter<=0) {
      changenickcounter = changenicktimeout;
      JString props = Trim(Copy(curchat,8,Length(curchat)-7));
      if (Length(props)>200) props = Copy(props,1,200);
      props = JString()+(char)(NICKNAME+32)+(char)(Length(props)+32)+props;
      SendData(SPLAYERPROPS,props);
      SetProperties(props);
    } else {
      curchat = "Wait 10 seconds before changing your nick again!";
      SendData(SPLAYERPROPS,JString((char)(CURCHAT+32)) << GetProperty(CURCHAT));   
    }  
  } else if (Pos("warpto",curchat)==1 && adminlevel>=1 && isadminin(playerworld)) {  
    JString props = Trim(Copy(curchat,7,Length(curchat)-6));  
    int i = Pos(' ',props);  
    if (i>0) {  
      double tox = strtofloat(Copy(props,1,i-1));  
      props = Trim(Copy(props,i+1,Length(props)-i));  
      i = Pos(' ',props);  
      if (i==0) i = Length(props)+1;  
      double toy = strtofloat(Copy(props,1,i-1));  
      props = Trim(Copy(props,i+1,Length(props)-i));  
      x = tox;  
      y = toy;  
      if (Length(props)>0) {
        JString newworld;   
        i = LastDelimiter(JString()+"§",props);   
        if (i>1) newworld = Copy(props,1,i);   
        if (isadminin(newworld)) {
          props = GetProperty(PLAYERX) << GetProperty(PLAYERY) << props;  
          SendData(PLAYERWARPED,props);  
          parse(JString((char)(LEVELWARP+32)) + props);  
        }
      } else {
        props = JString((char)(PLAYERX+32)) << GetProperty(PLAYERX) <<
          (char)(PLAYERY+32) << GetProperty(PLAYERY);
        SendData(SPLAYERPROPS,props);
        SetProperties(props);
      }
    }
  } else if (Pos("setskin",curchat)==1) {
    int newcolor = colors[0];
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "white") newcolor = 0;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "yellow") newcolor = 1;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "orange") newcolor = 2;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "pink") newcolor = 3;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "red") newcolor = 4;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkred") newcolor = 5;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgreen") newcolor = 6;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "green") newcolor = 7;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkgreen") newcolor = 8;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightblue") newcolor = 9;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "blue") newcolor = 10;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkblue") newcolor = 11;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "brown") newcolor = 12;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "cynober") newcolor = 13;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "purple") newcolor = 14;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkpurple") newcolor = 15;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgray") newcolor = 16;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "gray") newcolor = 17;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "black") newcolor = 18;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "transparent") newcolor = 19;
    colors[0] = newcolor;
    JString props = JString((char)(PLAYERCOLORS+32)) << GetProperty(PLAYERCOLORS);
    SendData(SPLAYERPROPS,props);
    SendData(OTHERPLPROPS, props);
    SetProperties(props);
  } else if (Pos("setcoat",curchat)==1) {
    int newcolor = colors[1];
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "white") newcolor = 0;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "yellow") newcolor = 1;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "orange") newcolor = 2;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "pink") newcolor = 3;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "red") newcolor = 4;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkred") newcolor = 5;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgreen") newcolor = 6;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "green") newcolor = 7;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkgreen") newcolor = 8;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightblue") newcolor = 9;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "blue") newcolor = 10;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkblue") newcolor = 11;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "brown") newcolor = 12;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "cynober") newcolor = 13;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "purple") newcolor = 14;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkpurple") newcolor = 15;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgray") newcolor = 16;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "gray") newcolor = 17;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "black") newcolor = 18;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "transparent") newcolor = 19;
    colors[1] = newcolor;
    JString props = JString((char)(PLAYERCOLORS+32)) << GetProperty(PLAYERCOLORS);
    SendData(SPLAYERPROPS,props);
    SendData(OTHERPLPROPS, props);
    SetProperties(props);
  } else if (Pos("setsleeve",curchat)==1) {
    int newcolor = colors[2];
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "white") newcolor = 0;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "yellow") newcolor = 1;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "orange") newcolor = 2;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "pink") newcolor = 3;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "red") newcolor = 4;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "darkred") newcolor = 5;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "lightgreen") newcolor = 6;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "green") newcolor = 7;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "darkgreen") newcolor = 8;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "lightblue") newcolor = 9;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "blue") newcolor = 10;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "darkblue") newcolor = 11;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "brown") newcolor = 12;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "cynober") newcolor = 13;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "purple") newcolor = 14;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "darkpurple") newcolor = 15;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "lightgray") newcolor = 16;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "gray") newcolor = 17;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "black") newcolor = 18;
    if (LowerCase(Trim(Copy(curchat,11,Length(curchat)-10))) == "transparent") newcolor = 19;
    colors[2] = newcolor;
    JString props = JString((char)(PLAYERCOLORS+32)) << GetProperty(PLAYERCOLORS);
    SendData(SPLAYERPROPS,props);
    SendData(OTHERPLPROPS, props);
    SetProperties(props);
  } else if (Pos("setshoe",curchat)==1) {
    int newcolor = colors[3];
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "white") newcolor = 0;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "yellow") newcolor = 1;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "orange") newcolor = 2;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "pink") newcolor = 3;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "red") newcolor = 4;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkred") newcolor = 5;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgreen") newcolor = 6;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "green") newcolor = 7;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkgreen") newcolor = 8;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightblue") newcolor = 9;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "blue") newcolor = 10;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkblue") newcolor = 11;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "brown") newcolor = 12;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "cynober") newcolor = 13;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "purple") newcolor = 14;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkpurple") newcolor = 15;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgray") newcolor = 16;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "gray") newcolor = 17;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "black") newcolor = 18;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "transparent") newcolor = 19;
    colors[3] = newcolor;
    JString props = JString((char)(PLAYERCOLORS+32)) << GetProperty(PLAYERCOLORS);
    SendData(SPLAYERPROPS,props);
    SendData(OTHERPLPROPS, props);
    SetProperties(props);
  } else if (Pos("setbelt",curchat)==1) {
    int newcolor = colors[4];
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "white") newcolor = 0;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "yellow") newcolor = 1;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "orange") newcolor = 2;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "pink") newcolor = 3;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "red") newcolor = 4;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkred") newcolor = 5;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgreen") newcolor = 6;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "green") newcolor = 7;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkgreen") newcolor = 8;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightblue") newcolor = 9;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "blue") newcolor = 10;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkblue") newcolor = 11;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "brown") newcolor = 12;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "cynober") newcolor = 13;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "purple") newcolor = 14;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "darkpurple") newcolor = 15;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "lightgray") newcolor = 16;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "gray") newcolor = 17;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "black") newcolor = 18;
    if (LowerCase(Trim(Copy(curchat,9,Length(curchat)-8))) == "transparent") newcolor = 19;
    colors[4] = newcolor;
    JString props = JString((char)(PLAYERCOLORS+32)) << GetProperty(PLAYERCOLORS);
    SendData(SPLAYERPROPS,props);
    SendData(OTHERPLPROPS, props);
    SetProperties(props);
  }
}


JString TServerPlayer::GetPropertyList(TServerPlayer* player) {
  int i;
  JString str;

  str = "";
  for (i = 0; i<playerpropertycount; i++) {
    if (sendtoothers[i] ||
      ((udpport<=0 || (Assigned(player) && player->udpport<=0)) &&
#ifdef NEWWORLD
        (i==PLAYERANI || i==CURCHAT || (i>=PLAYERX && i<=CARRYSPRITE))))
#else
        (i==CURCHAT || (i>=PLAYERX && i<=CARRYSPRITE))))
#endif
      str << (char)(i+32) << GetProperty(i);
  }
  return str;
}

JString TServerPlayer::GetCodedID() {
  return JString((char)((id >> 7)+32)) << char((id & 0x7f)+32);
}

void TServerPlayer::ApplyAccountChange(TServerPlayer* player, bool dowarp) {
  int i,j;
  JString props,props2;
  if (!Assigned(player)) return;

  props = "";
  for (i = 0; i<playerpropertycount; i++)
    if (propsapply[i] && (GetProperty(i) != player->GetProperty(i)))
      props << (char)(i+32) << player->GetProperty(i);
  props2 = JString((char)(PLAYERX+32)) << player->GetProperty(PLAYERX) <<
    (char)(PLAYERY+32) << player->GetProperty(PLAYERY);
  if (GetProperty(CURLEVEL)==player->GetProperty(CURLEVEL)) props << props2;
  SetProperties(props);
  SendData(SPLAYERPROPS,props);

  for (i = 0; i<actionflags->count; i++) {
    props = (*actionflags)[i];
    j = player->actionflags->indexOf(props);
    if (j<0) SendData(SUNSETFLAG,props);
  }
  for (i = 0; i<player->actionflags->count; i++) {
    props = (*player->actionflags)[i];
    j = actionflags->indexOf(props);
    if (j<0) SendData(SSETFLAG,props);
  }
  actionflags->Assign(player->actionflags);

#ifndef NEWWORLD
  props = "bomb";
  if (player->myweapons->indexOf(props)<0)
    player->myweapons->Add(props);
  props = "bow";
  if (player->myweapons->indexOf(props)<0)
    player->myweapons->Add(props);
#endif
  for (i = 0; i<myweapons->count; i++) {
    props = (*myweapons)[i];
    j = player->myweapons->indexOf(props);
    if (j<0) SendData(SDELNPCWEAPON,props);
  }
  for (i = 0; i<player->myweapons->count; i++) {
    props = (*player->myweapons)[i];
    j = myweapons->indexOf(props);
    if (j<0) weaponstosend->Add(props);
  }
  myweapons->Assign(player->myweapons);

  openedchests->Assign(player->openedchests);

  if (dowarp && GetProperty(CURLEVEL)!=player->GetProperty(CURLEVEL)) {
    props = player->GetProperty(PLAYERX) << player->GetProperty(PLAYERY) << player->levelname;
    SendData(PLAYERWARPED,props);
    allowoutjail = true;
    parse(JString((char)(LEVELWARP+32)) << props);
    allowoutjail = false;
  }
}
 
//--- TServerFrame ---  
  
#ifdef NEWWORLD  
int TServerFrame::CalcTime() {  
  return (time(NULL) - 11078*24*60*60)*2/15;  
}  
#endif  
  
void TServerFrame::Timer1Timer() {  
  int i,j,k,l;  
  TServerLevel* level;  
  TServerLevelModify* modify;  
  TServerPlayer* player;  
  TServerHorse* horse;  
  TServerExtra* extra;  
  TServerBaddy* baddy;  
  TServerNPC* npc;  
  JString data, props;  
  TStream* stream;  
  
#ifdef NEWWORLD  
  // Send a timer signal to newworld clients every game minute.
  i = CalcTime();  
  if (nwtime!=i) {
    nwtime = i;
    props = JString((char)(((i >> 21) & 0x7f)+32)) << (char)(((i >> 14) & 0x7f)+32) <<
      (char)(((i >> 7) & 0x7f)+32) << (char)((i & 0x7f)+32);
    for (i=0; i<players->count; i++) {
      player = (TServerPlayer*)(*players)[i];
      if (Assigned(player) && player->loggedin())
        player->SendData(NEWWORLDTIME,props);
    }
#ifdef SERVERSIDENPCS
    i = nwtime;
    nwmin = i%60;
    i = i/60;
    nwhour = i%24;
    i = i/24;
    nwday = i%28;
    i = i/28;
    nwmonth = i%10;
    i = (i/10)+1000;
    nwyear = i;
#endif // SERVERSIDENPCS
  }
#endif // NEWWORLD

  // Animate objects in the levels
  i = 0;
  while (i<levels->count) {
    level = (TServerLevel*)(*levels)[i];
    level->DoRespawnCounter();
    level->savecounter--;
    if (level->savecounter<=0) {
      level->savecounter = saveleveldelay;
      level->SaveTemp();
    }
    // Increment the emptytime counter and the save&delete the level if it
    // exceeds the respawntime+60 seconds
    if (level->players->count==0) {  
#ifndef SERVERSIDENPCS  
      level->emptytime++;  
#endif  
      if (level->emptytime>=respawntime+60) {  
        level->SaveTemp();  
        level->orgnpcs->Clear();  
        if (level->playerleaved->count>0) {  
          TDestroyedLevel* dlevel = new TDestroyedLevel(level);
          destroyedlevels->Add(dlevel);
        }  
        delete(level);  
        levels->Delete(i);  
      } else i++;  
    } else {  
      level->emptytime = 0;  
      i++;  
    }  
  }  
  
  // Increment all player's counter for alignment, no movement, account saving  
  for (i=0; i<players->count; i++) {  
    player = (TServerPlayer*)(*players)[i];  
    if (Assigned(player)) {  
      if (player->compout.length()>=128*1024) player->outbufferfullcounter++;  
      else player->outbufferfullcounter = 0;  
      if (!player->isrc) {  
        player->onlinesecs++;  
        if (player->apcounter>=0 && player->loggedin() && (player->status & 1)==0) { // check for not paused  
          player->apcounter--;  
          if (player->apcounter<0) {  
            player->apcounter = apincrementtime;  
            if (player->alignment<90) {  
              player->alignment++;  
              data = JString((char)(PALIGNMENT+32)) << player->GetProperty(PALIGNMENT);  
              player->SendData(SPLAYERPROPS,data);  
              player->SetProperties(data);  
            }  
          }  
        }  
        player->savewait--;  
        if (player->savewait<=0) {  
          player->SaveMyAccount();  
          player->savewait = 30;  
        }  
        player->nomovementcounter++;  
        player->noactioncounter++;  
        player->nodatacounter++;  
        if (player->changenickcounter>0) player->changenickcounter--;  
        if (player->changecolorscounter>0) player->changecolorscounter--;  
        if (player->noactioncounter>maxnoaction) {  
          toerrorlog(JString()+"Client "+player->accountname+" didn't move for >20 mins.");  
          player->SendData(DISMESSAGE,JString()+"You have been disconnected because you didn't move.");  
          player->deleteme = true;  
/*        } else if (player->nodatacounter>maxnodata) {  
          toerrorlog(JString()+"Client "+player->accountname+" has a too high lag.");  
          player->SendData(DISMESSAGE,JString()+"Communication timeout (lag >"+maxnodata+" secs).");  
          player->deleteme = true;*/  
        }  
      }
    }  
  }  
  
  // Set/unset Matia day flag  

  savepropscounter = savepropscounter-1;
  // Save accounts all 300 seconds
  if (savepropscounter<=0) {
    savepropscounter = savepropsdelay;
#ifdef NEWWORLD
    UpdateNewLevels();
#endif
 //   SaveHTMLStats(true);
unsigned int upseconds = time(NULL)-starttime;
UpdateStats((JString()+players->count),(JString()+GetAccountCount()),(JString()+(upseconds/3600)+" hrs "+((upseconds/60)%60)+" mins"),/*(JString()+maxplayersonserver)*/"75");
    serverflags->SaveToFile(getDir()+"data/serverflags.txt");  
  }  
}  
// oh noesss more crappy coding replaced w/ leet wholesome mysql goodness!! yeahhhh!!
// actually we're gonna toss this up to where SaveHTMLStats is called to avoid the
// extra functions overhead as well as comment UDP so ppl can't use it ;)
/*void TServerFrame::SaveHTMLStats(bool serverup) {  
  unsigned int upseconds = time(NULL)-starttime;
UpdateStats((JString()+players->count),(JString()+GetAccountCount()),(JString()+(upseconds/3600)+" hrs "+((upseconds/60)%60)+" mins"),(JString()+maxplayersonserver));
}

void TServerFrame::doUDPResponse() {
  if (responsesock && responsesock->sock) {
    JString str = responsesock->Read();
    if (str.length()>0) {  
      int bytessent = -2;  
      if (str=="online") bytessent = responsesock->Send("yes");  
      else if (str=="players") bytessent = responsesock->Send(JString()+players->count);  
      else {  
        bool found = false;  
        for (int i=0; i<players->count; i++) {  
          TServerPlayer* player = (TServerPlayer*)(*players)[i];  
          if (Assigned(player) && str==player->accountname) {  
            found = true;  
            break;  
          }  
        }  
        responsesock->Send(found? "yes":"no");  
      }  
    }  
  }  
}  
*/
void signalhandler(int sig)
{
  // A program error or program quit command occured,
  // log it in the error log file and delete the gserver
  // object (-> saving the accounts)
  JString out="";
  switch (sig) {
    case SIGQUIT: out << "Program quit by the system."; break;
    case SIGINT: out << "Program canceled by user."; break;
    case SIGTERM: out << "Program killed by user."; break;
    case SIGSEGV: out << "Program error."; break;
    case 0x1337: out << "Admin closed server."; break;
  }
  TServerFrame *oldserver = ServerFrame;
  ServerFrame = NULL;
  if (oldserver) {
    oldserver->toerrorlog(out);
    delete(oldserver);
  }
  CloseDatabase();
  exit(0);
}

char *my_getcwd()
{
  char *ptr           = NULL;
  int   dir_name_size = 128;

  while( ptr==NULL || (getcwd(ptr, dir_name_size)==NULL && errno==ERANGE))
  {
    free(ptr);
    dir_name_size *= 2;
    ptr = (char*)malloc(dir_name_size);
    if (ptr==NULL) exit(0);
  }

  if (ptr==NULL) exit(0);

  return ptr;
}

//extern const char *databasetype;
  
int main(int argc, char *argv[])
{
  int i;  
  
  // Installing the signal handler  
  signal(SIGHUP, SIG_IGN);  
  signal(SIGQUIT, signalhandler);  
  signal(SIGINT, signalhandler);  
  signal(SIGTERM, signalhandler);  
  signal(SIGSEGV, signalhandler);  
  signal(SIGPIPE, SIG_IGN);  

  // Setting environment attributes and initializing global variables  
  //struct lconv * pconv = localeconv();  
  //pconv->decimal_point = ".";  
  applicationdir = ExtractFilepath(JString()+my_getcwd()+fileseparator);
  
  std::cout << std::endl << "Zed Server v1.39.22" << std::endl;
  //std::cout << "Sorta made by: Mafukie, Mister Bubbles, & Hell Raven" << std::endl;
  //std::cout << "Modified by: JustBurn, Gregovich and Lucifer" << std::endl;
  //std::cout << "Database access type: " << databasetype << std::endl << std::endl;
  
  std::cout << "applicationdir: " << applicationdir.text() << std::endl;  

  int RetDatabase = InitDatabase();
  if (RetDatabase < 1) {
    std::cout << "Database failed to open..." << std::endl;  
	exit(-1);
  }
  if (RetDatabase == 1) std::cout << "Database ready..." << std::endl;  
  if (RetDatabase == 2) {
    std::cout << "Database created... user: admin / pass: admin" << std::endl;
    std::cout << "WARNING: Change the password in RC now!" << std::endl;
  }

  subdirs->Clear();
  RefindAllSubdirs(applicationdir+"levels");

  itemnames->SetPlainMem();
  itemnames->SetCommaText(JString()+"greenrupee,bluerupee,redrupee,bombs,darts,heart,glove1,"+  
    "bow,bomb,shield,sword,fullheart,superbomb,battleaxe,goldensword,mirrorshield,"+  
    "glove2,lizardshield,lizardsword,goldrupee,fireball,fireblast,nukeshot,joltbomb,"+  
    "spinattack");  
  baddygifs->SetPlainMem();
  baddygifs->SetCommaText(JString()+"baddygray.gif,baddyblue.gif,baddyred.gif,baddyblue.gif,"+  
    "baddygray.gif,baddyhare.gif,baddyoctopus.gif,baddygold.gif,baddylizardon.gif,"+  
    "baddydragon.gif");  
  baddynameslower->SetPlainMem();
  baddynameslower->SetCommaText(JString()+"graysoldier,bluesoldier,"+  
    "redsoldier,shootingsoldier,swampsoldier,frog,octopus"+  
    "goldenwarrior,lizardon,dragon");  
  memset(npcidused,0,npcidbufsize);  
  memset(playeridused,0,playeridbufsize);  
  
   
  // Starting the gserver object
  JString serverport;
  if (argc>1) serverport << argv[1];
  else serverport << "14900";
  ServerFrame = new TServerFrame();
  ServerFrame->startServer(serverport);
  if (ServerFrame->sock == 0) return 0;  
  
 
  // Endless loop: accept new socket connections, read from/send to sockets  
  // and call Timer1Timer every second  
  unsigned int lasttime;
#ifdef SERVERSIDENPCS  
  struct timeval lastloopend;  
  gettimeofday(&lastloopend,NULL);  
#endif  
  while (true) {
//    ServerFrame->doUDPResponse();  
    // Accept new socket connections  
    while (ServerFrame->sock) {  
      TSocket* newsock = ServerFrame->sock->Accept();  
      if (!Assigned(newsock)) break;  
      ServerFrame->CreatePlayer(newsock);  
// Load the startmessage upon a new connection
      ServerFrame->LoadStartMessage();
      if (!Assigned(newsock)) {
         #ifdef DEBUG
	  printf("Error at line gserver.cpp:4078, newsock not Assigned()\n");
	 #endif
	 break;  
      } 
    }

  
#ifdef SERVERSIDENPCS  
    // If data is available, read it  
    i = 0;  
    while (i<ServerFrame->players->count) {  
      int oldcount = ServerFrame->players->count;  
      TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
      if (Assigned(player) && !player->deleteme)  
        player->ClientReceive();
      if (player->deleteme) {  
        player->SendOutgoing();  
        delete(player);  
      }  
      if (oldcount==ServerFrame->players->count) i++;  
    }  
    // Send data in the outgoing buffer to the players  
    for (i=0; i<ServerFrame->players->count; i++) {  
      TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
      if (Assigned(player)) {
        player->SendWeaponsAndImages();  
        player->SendOutgoing();  
      }  
    }  
    // Animate the server-side npcs  
    for (i=0; i<ServerFrame->levels->count; i++) {  
      TServerLevel* level = (TServerLevel*)(*ServerFrame->levels)[i];  
      if (Assigned(level)) level->runScripts();  
    }  
#else  
    int usecs = 0;  
    struct timeval tm;  
    gettimeofday(&tm,NULL);  
    if (tm.tv_sec==lasttime) usecs = 1000000 - tm.tv_usec;  
    tm.tv_sec = 0;  
    tm.tv_usec = usecs;
    if (ServerFrame->players->count>0) {  
      // Check on which players's sockets data is available  
      fd_set set;  
      FD_ZERO(&set);  
      int highestsock;  
      for (i=0; i<ServerFrame->players->count; i++) {  
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
        if (Assigned(player)) {  
          FD_SET(player->sock->sock,&set);  
          if (player->sock->sock>highestsock) highestsock = player->sock->sock;  
        }  
      }  
      select(highestsock+1,&set,NULL,NULL,&tm);  
  
      // If data is available, read it
      i = 0;  
      while (i<ServerFrame->players->count) {  
        int oldcount = ServerFrame->players->count;  
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
        if (Assigned(player) && FD_ISSET(player->sock->sock,&set) && !player->deleteme)  
          player->ClientReceive();  
        if (player->deleteme) {  
          player->SendOutgoing();  
          delete(player);  
        }  
        if (oldcount==ServerFrame->players->count) i++;
      }  
      // Send data in the outgoing buffer to the players  
      for (i=0; i<ServerFrame->players->count; i++) {  
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];  
        if (Assigned(player)) {  
          player->SendWeaponsAndImages();  
          player->SendOutgoing();  
        }  
      }  
    } else {  
#ifdef SunOS  
      select(0, NULL, NULL, NULL, &tm);  
#else  
      usleep(usecs);  
#endif  
    }  
#endif  
  
    curtime = time(NULL);
    if (curtime!=lasttime) {
      lasttime = curtime;
      // Once every second:
      // Call Timer1Timer
      ServerFrame->Timer1Timer();  
    }  
#ifdef SERVERSIDENPCS  
    struct timeval curenttime, timediff;  
    gettimeofday(&curenttime,NULL);
    timediff.tv_sec = (curenttime.tv_sec-lastloopend.tv_sec);
    timediff.tv_usec = (curenttime.tv_usec-lastloopend.tv_usec);  
    if (timediff.tv_sec==0 && timediff.tv_usec<100000) {
      timediff.tv_usec = 100000-timediff.tv_usec;
#ifdef SunOS

      select(0, NULL, NULL, NULL, &timediff);
#else
      usleep(timediff.tv_usec);
#endif  
    }  
    gettimeofday(&lastloopend,NULL);  
#endif  
  }  
  delete(ServerFrame);  
  return 0;  
}  
