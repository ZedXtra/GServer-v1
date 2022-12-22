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

#include "levels.h"

TList* destroyedlevels = new TList();

int horselifetime = 10;
int baddyrespawntime = 120;
int bw = 16;

int clevel = 0;
int clevelmodify = 0;
int cboardmodify = 0;
int clevellink = 0;
int cchest = 0;
int chorse = 0;
int cextra = 0;
int cnpc = 0;
int cnpcmodify = 0;
int cbaddy = 0;
int cbaddymodify = 0;
int cdlevel = 0;

int baddypower[] = {2,3,4,3,2, 1,1,6,12,8};
TJStringList* baddygifs = new TJStringList();
TJStringList* baddynameslower = new TJStringList();
int baddystartmode[] = {WALK,WALK,WALK,WALK,SWAMPSHOT,HAREJUMP,WALK,WALK,WALK,WALK};
int baddytypes = 10;
bool baddypropsreinit[] = {false,true ,true ,true ,true ,true ,true ,true ,false,false,false};

TJStringList* getStringTokens(JString str) { 
  TJStringList* res = new TJStringList(); 
  res->SetPlainMem();
  str = Trim(str); 
  while (Length(str)>0) { 
    int i = Pos(' ',str); 
    if (i==0) i = Length(str)+1; 
    res->Add(Copy(str,1,i-1)); 
    str = Trim(Copy(str,i+1,Length(str)-i)); 
  } 
  return res; 
} 


//--- TServerLevel ---

TServerLevel::TServerLevel(const JString& filename) {
  clevel++;
  this->filename = filename;
  plainfile = LowerCaseFilename(filename);
#ifdef SERVERSIDENPCS 
  links = new TList();
  linkstrings = NULL;
#else
  links = NULL;
  linkstrings = new TJStringList();
  linkstrings->SetPlainMem();
#endif
  npcs = new TList();
  orgnpcs = new TList();
  signs = new TJStringList();
  signs->SetPlainMem();
  players = new TList();
  boardmodifies = new TList();
  playerleaved = new TList();
  horses = new TList();
  baddies = new TList();
  chests = new TList();
  extras = new TList();
#ifdef SERVERSIDENPCS
  actionvars = new TList();
#endif

  emptytime = 0;
  savecounter = saveleveldelay;
  Load();
}

void TServerLevel::Clear() {
  while (orgnpcs->count>0) ServerFrame->DeleteNPC((TServerNPC*)(*orgnpcs)[0]);
  orgnpcs->Clear();
  if (Assigned(links)) {
    for (int i=0; i<links->count; i++) delete((TServerLevelLink*)(*links)[i]);
    links->Clear();
  }
  if (Assigned(linkstrings))
    linkstrings->Clear();
  while (npcs->count>0) ServerFrame->DeleteNPC((TServerNPC*)(*npcs)[0]);
  npcs->Clear();
  signs->Clear();
  for (int i=0; i<boardmodifies->count; i++) delete((TBoardModify*)(*boardmodifies)[i]);
  boardmodifies->Clear();
  playerleaved->Clear();
  for (int i=0; i<horses->count; i++) delete((TServerHorse*)(*horses)[i]);
  horses->Clear();
  for (int i=0; i<baddies->count; i++) delete((TServerBaddy*)(*baddies)[i]);
  baddies->Clear();
  for (int i=0; i<chests->count; i++) delete((TServerChest*)(*chests)[i]);
  chests->Clear();
  for (int i=0; i<extras->count; i++) delete((TServerExtra*)(*extras)[i]);
  extras->Clear();
#ifdef SERVERSIDENPCS
  for (int i=0; i<actionvars->count; i++) delete((TNPCVariable*)(*actionvars)[i]);
  actionvars->Clear();
#endif
}

TServerLevel::~TServerLevel() {
  clevel--;
  Clear();
  if (Assigned(links)) delete(links);
  if (Assigned(linkstrings)) delete(linkstrings);
  delete(npcs);
  delete(orgnpcs);
  delete(signs);
  delete(players);
  delete(boardmodifies);
  delete(playerleaved);
  delete(horses);
  delete(baddies);
  delete(chests);
  delete(extras);
#ifdef SERVERSIDENPCS
  delete(actionvars);
#endif
}

JString TServerLevel::GetModTime() {
  return JString() +
    (char)(((modtime >> 28) & 0x7F)+32) +
    (char)(((modtime >> 21) & 0x7F)+32) +
    (char)(((modtime >> 14) & 0x7F)+32) +
    (char)(((modtime >>  7) & 0x7F)+32) +
    (char)(( modtime        & 0x7F)+32);
}

void TServerLevel::Load() {
  filename = GetLevelFile(plainfile);
  if (Length(filename)>0) {
    modtime = GetUTCFileModTime(filename);

#ifdef NEWWORLD
    if (Copy(filename,Length(filename)-2,3)==".nw") {
      LoadNW(filename);
      LoadTemp();
      return;
    }
#endif
    TStream* data = new TStream();
    data->LoadFromFile(filename);
    data->Position = 0;
    char id[8];
    data->Read(id,8);
    JString idstr;
    for (int i=0; i<8; i++) idstr << id[i];

    int boardindex = 0;
    int bitsread = 0;
    int codebits = 12;
    if ((idstr=="GR-V1.02") || (idstr=="GR-V1.03"))
      codebits = 13;
    int buffer = 0;
    int count = 1;
    int firstcode = -1;
    bool twicerepeat = false;
    unsigned char databyte;

    while (boardindex<64*64 && data->Position<data->Size) {
      while (bitsread<codebits) {
        data->Read(&databyte,1);
        buffer += (databyte << bitsread);
        bitsread += 8;
      }

      int code = buffer & ((1 << codebits) - 1);
      buffer >>= codebits;
      bitsread -= codebits;

      if ((code & (1 << (codebits-1))) > 0) {
        if ((code & 0x100) > 0) twicerepeat = true;
        count = code & 0xFF;
      } else if (count==1) {
        board[boardindex] = (short)code;
        boardindex++;
      } else {
        if (twicerepeat) {
          if (firstcode < 0) firstcode = code;
          else {
            for (int i = 1; i<=count && boardindex<64*64-1; i++) {
              board[boardindex] = (short)firstcode;
              board[boardindex+1] = (short)code;
              boardindex += 2;
            }
            firstcode = -1;
            twicerepeat = false;
            count = 1;
          }
        } else {
          for (int i = 1; i<=count && boardindex<64*64; i++) {
            board[boardindex] = (short)code;
            boardindex++;
          }
          count = 1;
        }
      }
    }

    ReadLinks(data);
    ReadBaddies(data);
    bool b0 = (idstr=="GR-V1.00");
    bool b1 = (idstr=="GR-V1.01");
    bool b2 = (idstr=="GR-V1.02");
    bool b3 = (idstr=="GR-V1.03");
    if (b0 || b1 || b2 || b3) ReadNPCs(data);
    if (b1 || b2 || b3) ReadChests(data);
    ReadSigns(data);

    delete(data);
    LoadTemp();
  }
}

void TServerLevel::LoadTemp() {
  // Load file with the saved npc attributes
  JString npcsname = ServerFrame->getDir() + "levelnpcs/" + plainfile + ".save";
  if (FileExists(npcsname)) {
    unsigned int tempmodtime = GetUTCFileModTime(npcsname);
    if (tempmodtime>modtime)
      LoadNW(npcsname);
  }
  for (int i=0; i<destroyedlevels->count; i++) {
    TDestroyedLevel* dlevel = (TDestroyedLevel*)(*destroyedlevels)[i];
    if (Assigned(dlevel) && dlevel->plainfile==plainfile) {
      if (Assigned(dlevel->boardmodifies) && Assigned(dlevel->playerleaved)) {
        delete(boardmodifies); // still empty so we don't need to delete the entries
        delete(playerleaved);
        boardmodifies = dlevel->boardmodifies;
        playerleaved = dlevel->playerleaved;
        dlevel->boardmodifies = NULL;
        dlevel->playerleaved = NULL;
      }
      destroyedlevels->Delete(i);
      delete(dlevel);
      break;
    }
  }
}

JString TServerLevel::getFullLink(const JString& linkdest) {
  // When we are in a playerworld, then test if the
  // linked level exists in the playerworld
  int i = Pos("§",linkdest);
  if (i==0) {
    int j = LastDelimiter("§",plainfile);
    if (j>1) {
      JString str;
      str << Copy(plainfile,1,j) << linkdest;
      if (LevelFileExists(str))
        return str;
    }
  }
  return linkdest;
}

void TServerLevel::ReadLinks(TStream* data) {
  if (!Assigned(data)) return;
  while (true) {
    JString str = data->readLine();
    if ((Length(str)<1) || (str=="#")) return;
    TServerLevelLink* link = new TServerLevelLink();
    link->Assign(str);
    link->filename = getFullLink(link->filename);
    // Check if the link destination exists, otherwise
    // delete and forget it
    if (Length(link->filename)>0 && LevelFileExists(link->filename)) {
#ifdef SERVERSIDENPCS
      links->Add(link);
#else
      linkstrings->Add(link->GetJStringRepresentation());
      delete(link);
#endif
    } else 
      delete(link);
  }
}

void TServerLevel::ReadBaddies(TStream* data) {
  while (true)  {
    JString str;
    while (true) {
      unsigned char databyte;
      int bytesread = data->Read(&databyte,1);
      if ((bytesread<1) || ((databyte==10) && (Length(str)>=3)))
        break;
      str << (char)(databyte);
    }
    if ((Length(str)<3) || (((char)str[1])<0)) return;

    JString str1;
    JString str2;
    JString str3;
    if (Length(str)>3) {
      JString s = Copy(str,4,Length(str)-3);
      int i = Pos(fileseparator,s);
      if (i>0) {
        str1 = Copy(s,1,i-1);
        s = Copy(s,i+1,Length(s)-i);
        i = Pos(fileseparator,s);
        if (i>0) {
          str2 = Copy(s,1,i-1);
          str3 = Copy(s,i+1,Length(s)-i);
        }
      }
    }
    TServerBaddy* baddy = GetNewBaddy();
    if (Assigned(baddy)) {
      baddy->v1 = str1;
      baddy->v2 = str2;
      baddy->v3 = str3;
      baddy->x = str[1];
      if (baddy->x<0) baddy->x = 0;
      if (baddy->x>62) baddy->x = 62;
      baddy->y = str[2];
      if (baddy->y<0) baddy->y = 0;
      if (baddy->y>62) baddy->y = 62;
      baddy->orgx = baddy->x;
      baddy->orgy = baddy->y;
      baddy->comptype = str[3];
      if (baddy->comptype<0) baddy->comptype = 0;
      if (baddy->comptype>=baddytypes) baddy->comptype = baddytypes-1;
      baddy->mode = baddystartmode[baddy->comptype];
      baddy->power = baddypower[baddy->comptype];
      baddy->filename = (*baddygifs)[baddy->comptype];
      baddy->respawn = true;
    }
  }
}

void TServerLevel::ReadNPCs(TStream* data) {
  if (!Assigned(data)) return;
  while (true) {
    JString str = data->readLine();
    if ((Length(str)<1) || (str=="#")) return;
    if (npcs->count>=50) continue; 

    JString str2( Copy(str,3,Length(str)-2) );
    int i = Pos('#',str2);
#ifdef SERVERSIDENPCS
    TServerNPC* npc = new TServerSideNPC(Copy(str2,1,i-1),
#else
    TServerNPC* npc = new TServerNPC(Copy(str2,1,i-1),
#endif
      Copy(str2,i+1,Length(str2)-i), str[1]-32, str[2]-32);
    npcs->Add(npc);
    orgnpcs->Add(npc);
    npc->orgindex = npcs->count-1;
    npc->level = this;
  }
}

void TServerLevel::ReadChests(TStream* data) {
  if (!Assigned(data)) return;
  while (true) {
    JString str = data->readLine();
    if ((Length(str)<1) || (str=="#")) return;

    TServerChest* chest = new TServerChest();
    chest->x = str[1]-32;
    if (chest->x<0) chest->x = 0;
    if (chest->x>62) chest->x = 62;
    chest->y = str[2]-32;
    if (chest->y<0) chest->y = 0;
    if (chest->y>62) chest->y = 62;
    chest->itemindex = str[3]-32;
    chest->signindex = str[4]-32;
    chests->Add(chest);
  }
}

void TServerLevel::ReadSigns(TStream* data) {
  if (!Assigned(data)) return;
  while (true) {
    JString str = data->readLine();
    if (Length(str)<1) return;
    signs->Add(str);
  }
}

JString base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
JString pcode = JString()+"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"+
  "0123456789!?-.,#>()#####\"####':/~&### <####;\n";
JString ccode = "ABXYudlrhxyz#4.";
int ctablen[] = {1,1,1,1,1,1,1,1,2,1,1,1,1,2,1};
int ctabindex[] = {0,1,2,3,4,5,6,7,8,10,11,12,13,14,16};
int ctab[] = {91,92,93,94,77,78,79,80,74,75,71,72,73,86,87,88,67};

JString getSignCodes(const JString& str) {
  JString codetext;
  int j = 1;
  while (j<=Length(str)) {
    char c = (char)str[j];
    if (c=='#') {
      j = j+1;
      if (j<=Length(str)) {
        c = str[j];
        int index = Pos(c,ccode);
        if (index>0)
          for (int k=0; k<ctablen[index-1]; k++)
            codetext << (char)(ctab[ctabindex[index-1]+k]+32);
      }
    } else {
      int index = Pos(c,pcode);
      if (index>0)
        codetext << (char)(index-1+32);
    }
    j = j+1;
  }
  return codetext;
}

void TServerLevel::LoadNW(const JString& filename) {
  TJStringList* list = new TJStringList();
  list->LoadFromFile(filename);
  JString filesig = (*list)[0];
  if (list->count>=1 && ((*list)[0]=="GLEVNW01" || (*list)[0]=="GSERVL01"))
      for (int i=1; i<list->count; i++) {
    TJStringList* arglist = getStringTokens((*list)[i]);
    if (arglist->count>0) {
      JString arg = (*arglist)[0];
      arglist->Delete(0);
      if (arg=="BOARD") {
        if (arglist->count>=5) {
          int x = strtoint((*arglist)[0]);
          int y = strtoint((*arglist)[1]);
          int w = strtoint((*arglist)[2]);

          JString str = (*arglist)[4];
          if (x>=0 && x<64 && y>=0 && y<64 && w>0 && x+w<=64 && Length(str)>=w*2)
              for (int j=0; j<w; j++) {
            int k = Pos((char)str[1+j*2],base64)-1;
            if (k<0) break;
            int num = k << 6;
            k = Pos((char)str[2+j*2],base64)-1;
            if (k<0) break;
            num = num + k;
            board[x+j+y*64] = num;
          }
        }
      } else if (arg=="LINK") {
        TServerLevelLink* link = new TServerLevelLink();
        link->Assign(arglist);
        link->filename = getFullLink(link->filename);
        // Check if the link destination exists, otherwise
        // delete and forget it
        if (Length(link->filename)>0 && LevelFileExists(link->filename)) {
#ifdef SERVERSIDENPCS
          links->Add(link); 
#else
          linkstrings->Add(link->GetJStringRepresentation());
          delete(link);
#endif
        } else 
          delete(link); 
      } else if (arg=="CHEST") {
        TServerChest* chest = new TServerChest();
        chest->Assign(arglist);
        chests->Add(chest);
      } else if (arg=="SIGN") {
        int x = 0;
        int y = 0;
        if (arglist->count>=2) {
          x = strtoint((*arglist)[0]);
          if (x<0) x = 0;
          if (x>62) x = 62;
          y = strtoint((*arglist)[1]);
          if (y<0) y = 0;
          if (y>62) y = 62;
        }
        JString signstr = JString() + (char)(x+32) + (char)(y+32);
        for (i++; i<list->count && (*list)[i]!="SIGNEND"; i++)
          signstr << getSignCodes((*list)[i]+"\n");
        signs->Add(signstr);
      } else if (arg=="NPC") {
        double x = 0;
        double y = 0;
        JString npcimg;
        JString npcaction;
        if (arglist->count>=3) {
          if ((*arglist)[0]!="-") npcimg = (*arglist)[0];
          x = strtofloat((*arglist)[1]);
          y = strtofloat((*arglist)[2]);
        }
        for (i++; i<list->count && (*list)[i]!="NPCEND"; i++)
          npcaction << (*list)[i] << "§";
#ifdef SERVERSIDENPCS
        TServerNPC* npc = new TServerSideNPC(npcimg,npcaction,x,y);
#else
        TServerNPC* npc = new TServerNPC(npcimg,npcaction,x,y);
#endif
        npcs->Add(npc);
        orgnpcs->Add(npc);
        npc->orgindex = npcs->count-1;
        npc->level = this;
      } else if (arg=="BADDY") {
        int x = 0;
        int y = 0;
        int comptype = 0;
        JString v1,v2,v3;
        if (arglist->count>=3) {
          x = strtoint((*arglist)[0]);
          if (x<0) x = 0;
          if (x>62) x = 62;
          y = strtoint((*arglist)[1]);
          if (y<0) y = 0;
          if (y>62) y = 62;
          JString str = (*arglist)[2];
          comptype = strtoint(str);
          int j = baddynameslower->indexOf(str);
          if (j>=0) comptype = j;
          if (comptype<0) comptype = 0;
          if (comptype>=baddytypes) comptype = baddytypes-1;
        }
        int j = i;
        for (i++; i<list->count && (*list)[i]!="BADDYEND"; i++) {
          switch (i-j) {
            case 1: { v1 = (*list)[i]; break; }
            case 2: { v2 = (*list)[i]; break; }
            case 3: { v3 = (*list)[i]; break; }
          }
        }
        TServerBaddy* baddy = GetNewBaddy();
        if (Assigned(baddy)) {
          baddy->v1 = v1;
          baddy->v2 = v2;
          baddy->v3 = v3;
          baddy->x = x;
          baddy->y = y;
          baddy->orgx = baddy->x;
          baddy->orgy = baddy->y;
          baddy->comptype = comptype;
          baddy->mode = baddystartmode[baddy->comptype];
          baddy->power = baddypower[baddy->comptype];
          baddy->filename = (*baddygifs)[baddy->comptype];
          baddy->respawn = true;
        }
      } else if (arg=="REPLACENPCS") { // see SaveTemp()
        TList* oldnpcs = new TList();
        for (int j=0; j<npcs->count; j++) oldnpcs->Add((*npcs)[j]);
        npcs->Clear();

        for (i++; i<list->count && (*list)[i]!="REPLACENPCSEND"; i++) {
          JString props = (*list)[i];
          int j = Pos(' ',props);
          if (j==0) j = Length(props)+1;
          int orgindex = strtoint(Copy(props,1,j-1));
          props = Trim(Copy(props,j+1,Length(props)-j));
          if (Length(props)>0 && props[1]=='\"' && props[Length(props)]=='\"')
            props = Copy(props,2,Length(props)-2);

          TServerNPC* npc = NULL;
          if (orgindex>=0 && orgindex<oldnpcs->count) {
            npc = (TServerNPC*)(*oldnpcs)[orgindex];
          } else {
            orgindex = -1;
#ifdef SERVERSIDENPCS
            npc = new TServerSideNPC("", "", 0, 0);
#else
            npc = new TServerNPC("", "", 0, 0);
#endif
            npc->orgindex = -1;
            npc->level = this;
          }
          if (Assigned(npc) && npcs->indexOf(npc)<0) {
            npcs->Add(npc);
            npc->SetProperties(props,(orgindex<0));
          }
        }
        for (int j=0; j<oldnpcs->count; j++) {
          TServerNPC* npc = (TServerNPC*)(*oldnpcs)[j];
          if (npcs->indexOf(npc)<0) {
            orgnpcs->Remove(npc);
            delete(npc);
          }
        }
        delete(oldnpcs);
      }
    }
    delete(arglist);
  }
  delete(list);
}

void TServerLevel::SaveTemp() {
  TJStringList* list = new TJStringList();
  list->Add("GSERVL01");
  list->Add("REPLACENPCS");
  for (int i=0; i<npcs->count; i++) {
    TServerNPC* npc = (TServerNPC*)(*npcs)[i];
    if (Assigned(npc)) {
      JString props;
      for (int j=0; j<npcpropertycount; j++) if (j!=NPCID && (j!=ACTIONSCRIPT || npc->orgindex<0))
        props << (char)(j+32) << npc->GetProperty(j);
      if (Pos('\n',props)==0)
        list->Add(JString()+npc->orgindex+" \""+props+"\"");
      else
        list->Add(JString()+npc->orgindex);
    }
  }
  list->Add("REPLACENPCSEND");
  list->SaveToFile(ServerFrame->getDir() + "levelnpcs/" + plainfile + ".save");
  delete(list);
}

int TServerLevel::GetLeavingTimeEntry(TServerPlayer* player) {
  if (!Assigned(player)) return -1;

  for (int i=0; i<playerleaved->count-1; i+=2) {
    int playerid = (int)(*playerleaved)[i];
    if (playerid==player->id)
      return i;
  }
  return -1;
}

unsigned int TServerLevel::GetLeavingTime(TServerPlayer* player) {
  int i = GetLeavingTimeEntry(player);
  if (i>=0)
    return (unsigned int)(*playerleaved)[i+1];
  return 0;
}

void TServerLevel::AddToList(TServerPlayer* player) {
  if (!Assigned(player)) return;

  // Remember the leaving time for the player
  // Check if there is already an entry (this
  // shouldn't happen but we should check for it anyway)
  int i = GetLeavingTimeEntry(player);
  if (i>=0) {
    playerleaved->buf[i+1] = (void*)curtime;
    return;
  }
  // Add the player id and leaving time to the list
  playerleaved->Add((void*)player->id);
  playerleaved->Add((void*)curtime);
}

void TServerLevel::RemoveFromList(TServerPlayer* player) {
  if (!Assigned(player)) return;

  int i = GetLeavingTimeEntry(player);
  if (i>=0) {
    playerleaved->Delete(i); // delete player id + leave time
    playerleaved->Delete(i);
  }
  // Delete all respawn stuff if there is no player anymore
  // who might come back to this place
  if (playerleaved->count<=0) for (i=boardmodifies->count-1; i>=0; i--) {
    TBoardModify* bmodify = (TBoardModify*)(*boardmodifies)[i];
    if (bmodify->isrespawn) {
      boardmodifies->Delete(i);
      delete(bmodify);
    }
  }
}

TJStringList* TServerLevel::GetModifyList(TServerPlayer* player) {
  TJStringList* modlist = new TJStringList();
  if (!Assigned(player)) return modlist;

  unsigned int leavingtime = GetLeavingTime(player);
  for (int i=0; i<boardmodifies->count; i++) {
    TBoardModify* bmodify = (TBoardModify*)(*boardmodifies)[i];
    if (bmodify->modifytime>=leavingtime && !(leavingtime==0 && bmodify->isrespawn))
      modlist->Add(bmodify->modify);
  }
  return modlist;
}

short replfields[] = {
  0x1ff,0x1ff,0x1ff,0x1ff, // grass
  0x3ff,0x3ff,0x3ff,0x3ff, // grass
  0x7ff,0x7ff,0x7ff,0x7ff, // grass
  0x2ac,0x2ad,0x2bc,0x2bd, // vase
      2,    3, 0x12, 0x13, // bush
  0x200,0x201,0x210,0x211, // sign
   0x22, 0x23, 0x32, 0x33, // stone
  0x3de,0x3df,0x3ee,0x3ef, // black stone
  0x1a4,0x1a5,0x1b4,0x1b5, // swamp
  0x14a,0x14b,0x15a,0x15b, // stake 1 (for hammer)
  0x674,0x675,0x684,0x685  // stake 2
  };
short goodrepl[] = {
  0x72a,0x72b,0x73a,0x73b, // hole
  0x72a,0x72b,0x73a,0x73b, // hole
  0x72a,0x72b,0x73a,0x73b, // hole 
  0x6ea,0x6eb,0x6fa,0x6fb, // vase field  
  0x2a5,0x2a6,0x2b5,0x2b6, // cut bush  
  0x70a,0x70b,0x71a,0x71b, // sign hole  
  0x72a,0x72b,0x73a,0x73b, // hole  
  0x72a,0x72b,0x73a,0x73b, // hole
  0x2a7,0x2a8,0x2b7,0x2b8, // cut swamp  
  0x70a,0x70b,0x71a,0x71b, // sign hole  
  0x70a,0x70b,0x71a,0x71b  // sign hole 
  };

void TServerLevel::AddModify(TServerPlayer* fromplayer, JString data) {
  if (Length(data)<4) return;
  int i,j;
  JString posdata = Copy(data,1,4);
  JString previous = posdata;
  int x = data[1]-32;
  int y = data[2]-32;
  int w = data[3]-32;
  int h = data[4]-32;
  // Test for illegal data
  if (x<0 || x>63 || w<1 || x+w>64 ||
      y<0 || y>63 || h<1 || y+h>64 || Length(data)<4+w*h*2) return;

  // No board modifes <> (2,2) allowed at this time
  if (w!=2 || h!=2) return;

  for (j=0; j<h; j++) for (i=0; i<w; i++) {
    int field = board[x+i+(y+j)*64];
    previous << (char)(((field >> 7) & 0x7F)+32) << (char)((field & 0x7F)+32);

    // Test for wrong hacker data
    int newfield = ((data[5+(i+j*w)*2]-32) << 7) + (data[6+(i+j*w)*2]-32);  
    bool found = false;  
    for (int k=0; k<11*4; k++) if (field==replfields[k] && newfield==goodrepl[k]) {  
      found = true;
      break;
    }
    if (!found) return;
  }

  // Send it to all playes in the current level
  for (i=0; i<players->count; i++) {
    TServerPlayer* player = (TServerPlayer*)(*players)[i];
    if (Assigned(player) && player!=fromplayer && !player->isblocked())
      player->SendData(SBOARDMODIFY,data);
  }

  // Delete previous modifications to the same spot
  TBoardModify* bmodify = NULL;
  for (i=0; i<boardmodifies->count; i++) {
    TBoardModify* bmodify2 = (TBoardModify*)(*boardmodifies)[i];
    if (Assigned(bmodify2) && Copy(bmodify2->modify,1,4)==posdata) {
      bmodify = bmodify2;
      break;
    }
  }
  // Add this modify to the boardmodifies list
  if (!Assigned(bmodify) && boardmodifies->count<100)
    bmodify = new TBoardModify();
  if (Assigned(bmodify)) {
    bmodify->modify = data;
    bmodify->previous = previous;
    bmodify->isrespawn = false;
    bmodify->dorespawn = ((w==2) && (h==2));
    bmodify->modifytime = curtime;
    bmodify->counter = (int)((((double)respawntime)*3/4) +
      (((double)respawntime)/2)*((double)(rand()%100))/100);
    if (boardmodifies->indexOf(bmodify)<0)
      boardmodifies->Add(bmodify);
  }
}

void TServerLevel::DoRespawnCounter() {
  // Test all respawn objects if their respawn counter is less or equal 0
  for (int i=boardmodifies->count-1; i>=0; i--) {
    TBoardModify* bmodify = (TBoardModify*)(*boardmodifies)[i];
    if (bmodify->dorespawn) {
      bmodify->counter--;
      if (bmodify->counter<=0) {
        // Send the previous board fields to all players in the currentlevel
        for (int k=0; k<players->count; k++) {
          TServerPlayer* player = (TServerPlayer*)(*players)[k];
          if (Assigned(player) && !player->isblocked())
            player->SendData(SBOARDMODIFY,bmodify->previous);
        }
        if (playerleaved->count>0) {
          bmodify->isrespawn = true;
          bmodify->dorespawn = false;
          bmodify->modify = bmodify->previous;
          bmodify->previous = "";
          bmodify->modifytime = curtime;
        } else {
          boardmodifies->Delete(i); 
          delete(bmodify);
        }
      }
    }
  }
  // Delete horses after 10 seconds
  for (int i=horses->count-1; i>=0; i--) {
    TServerHorse* horse = (TServerHorse*)(*horses)[i];
    horse->counter--;
    if (horse->counter<=0) {
      JString data = JString((char)((horse->x*2)+32)) << (char)((horse->y*2)+32);
      for (int k=0; k<players->count; k++) {
        TServerPlayer* player = (TServerPlayer*)(*players)[k];
        if (Assigned(player) && !player->isblocked())
          player->SendData(SDELHORSE,data);
      }
      horses->Delete(i);
      delete(horse);
    }
  }
  // Delete extras (rupees) after 20 seconds
  for (int i=extras->count-1; i>=0; i--) {
    TServerExtra* extra = (TServerExtra*)(*extras)[i];
    extra->counter--;
    if (extra->counter<=0) {
      JString data = extra->GetPosString();
      for (int k=0; k<players->count; k++) {
        TServerPlayer* player = (TServerPlayer*)(*players)[k];
        if (Assigned(player) && !player->isblocked())
          player->SendData(SDELEXTRA,data);
      }
      extras->Delete(i);
      delete(extra);
    }
  }
  // Respawn baddies
  for (int i=0; i<baddies->count; i++) {
    TServerBaddy* baddy = (TServerBaddy*)(*baddies)[i];
    if (baddy->mode==DEAD && baddy->respawn) {
      baddy->respawncounter--;
      if (baddy->respawncounter<0) {
        baddy->mode = baddystartmode[baddy->comptype];
        baddy->power = baddypower[baddy->comptype];
        baddy->anicount = 0;
        baddy->x = baddy->orgx;
        baddy->y = baddy->orgy;
        baddy->dirs = (2 << 2) + 2;
        JString data;
        for (int k=0; k<baddypropertycount; k++) if (baddypropsreinit[k])
          data << (char)(k+32) << baddy->GetProperty(k);
        baddy->SetProperties(data);
        for (int k=0; k<players->count; k++) {
          TServerPlayer* player = (TServerPlayer*)(*players)[k];
          data = JString((char)(baddy->id+32)) << baddy->GetPropertyList(player);
          player->SendData(SBADDYPROPS,data);
        }
      }
    }
  }
}


void TServerLevel::putHorse(double x, double y, JString gifname, int dir) {
  TServerHorse* horse;

  removeHorse(x,y);
  if (horses->count>=50) return;
  horse = new TServerHorse();
  horse->x = ((int)(2*x))/2;
  if (horse->x<0) horse->x = 0;
  if (horse->x>62) horse->x = 62;
  horse->y = ((int)(2*y))/2;
  if (horse->y<0) horse->y = 0;
  if (horse->y>62) horse->y = 62;
  horse->filename = gifname;
  horse->dir = dir;
  horse->counter = horselifetime;
  horses->Add(horse);
}

void TServerLevel::removeHorse(double x, double y) {
  int i;
  TServerHorse* horse;

  x = ((int)(2*x))/2;
  y = ((int)(2*y))/2;
  i = 0;
  while (i<horses->count) {
    horse = (TServerHorse*)(*horses)[i];
    if ((horse->x==x) && (horse->y==y)) {
      horses->Delete(i);
      delete(horse);
    } else i = i+1;
  }
}

TServerBaddy* TServerLevel::GetNewBaddy(){
  int i,id;
  TServerBaddy* baddy;

  id = 1;
  i = 0;
  while (i<baddies->count) {
    baddy = (TServerBaddy*)(*baddies)[i];
    if ((id==baddy->id) && (baddy->mode==DEAD) && !baddy->respawn)
      return baddy;
    else if (id<baddy->id)
      break;
    i = i+1;
    id = id+1;
  }
  if (id>50) return NULL;
  baddy = new TServerBaddy();
  baddy->id = id;
  baddy->x = 30.5;
  baddy->y = 30;
  baddy->comptype = 0;
  baddy->power = baddypower[baddy->comptype];
  baddy->filename = (*baddygifs)[baddy->comptype];
  baddy->mode = WALK;
  baddy->anicount = 0;
  baddy->dirs = (2 << 2)+2;
  baddy->v1 = "";
  baddy->v2 = "";
  baddy->v3 = "";
  baddy->respawn = false;
  baddy->level = this;
  baddies->Add(baddy);
  return baddy;
}

void TServerLevel::removeExtra(double x, double y) {
  x = ((int)(2*x))/2;
  y = ((int)(2*y))/2;
  int i = 0;
  while (i<extras->count) {
    TServerExtra* extra = (TServerExtra*)(*extras)[i];
    if ((extra->x==x) && (extra->y==y)) {
      extras->Delete(i);
      delete(extra);
    } else
      i++;
  }
}

bool TServerLevel::takeExtra(double x, double y, int itemindex, TServerPlayer* player) {
  if (!Assigned(player)) return false;

  for (int i=0; i<extras->count; i++) {
    TServerExtra* extra = (TServerExtra*)(*extras)[i];
    if ((extra->x==x) && (extra->y==y) && (extra->itemindex==itemindex)) {
      int sendprop = -1;
      switch (itemindex) {
        case 0: { player->rubins++; sendprop = 3; break; }
        case 1: { player->rubins+=5; sendprop = 3; break; }
        case 2: { player->rubins+=30; sendprop = 3; break; }
        case 3: {
            if (player->bombscount>=99) return false;
            player->bombscount+=5;
            sendprop = 5;
            break;
          }
        case 4: {
            if (player->darts>=99) return false;
            player->darts+=5;
            sendprop = 4;
            break;
          }
        case 5: {
            if (player->power>=player->maxpower) return false;
            player->power++;
            sendprop = 2;
            break;
          }
        case 19: { player->rubins+=100; sendprop = 3; break; }
        case 24: {
            if (player->status & 64) return false;
            player->status |= 64;
            sendprop = 18;
            break;
          }
      }
      if (player->rubins>9999999) player->rubins = 9999999;
      if (player->darts>99) player->darts = 99;
      if (player->bombscount>99) player->bombscount = 99;
      if (player->power>player->maxpower) player->power = player->maxpower;
      if (sendprop>=0) {
        JString str;
        str << (char)(sendprop+32) << player->GetProperty(sendprop);
        player->SendData(SPLAYERPROPS,str);
        player->SetProperties(str);
      }
      extras->Delete(i);
      delete(extra);
      return true;
    }
  }
  return false;
}

#ifdef NEWWORLD
bool TServerLevel::isWall(int num) {
  int tilex,tiley;

  tilex = num%16;
  tiley = num/16;
  return (num==2*16) || // black tile
    ((tilex>=2 && tiley>=26 && tiley<28) || // lift objects
     (tiley>=30 && tiley<48) || // chest, movestone, jumpstone, throwthrough
     (tiley>=84 && tiley<96) || // lower 12 lines of foreground
     (tiley>=116 && tiley<128) || // lower 12 lines of foreground
     (tiley>=128 && ((tiley/16) & 1))); // lower half of normal tiles
}

bool TServerLevel::isWater(int num) {
  int tilex,tiley;

  tilex = num%16;
  tiley = num/16;
  return (num>=4*16) && (num<18*16);
}

#endif


#ifdef SERVERSIDENPCS

bool TServerLevel::isOnWall(double x, double y) {
  if (x<0 || x>=64 || y<0 || y>=64) return true;
#ifdef NEWWORLD
  return isWall(board[(int)x+((int)y)*64]);
#else
  return false;
#endif
}

bool TServerLevel::isOnWater(double x, double y) {
  if (x<0 || x>=64 || y<0 || y>=64) return false;
#ifdef NEWWORLD
  return isWater(board[(int)x+((int)y)*64]);
#else
  return false;
#endif
}

bool TServerLevel::isOnPlayer(double x, double y) {
  return false;
}

TServerNPC* TServerLevel::isOnNPC(double x, double y) {
#ifdef NEWWORLD
  for (int i=0; i<npcs->count; i++) {
    TServerSideNPC* npc = (TServerSideNPC*)(*npcs)[i];
    if (!useblockflag || !npc->dontblock) {
      if (Assigned(npc->character)) {
        if (x>=npc->x+0.5 && x<=npc->x+2.5 && y>=npc->y+1 && y<=npc->y+3)
          return npc;
      } else if (Length(npc->filename)>0) {
        double ow = 2; // npc.pixelwidth/bw;
        double oh = 2; // npc.pixelheight/bw;
        if (x>=npc->x && x<npc->x+ow && y>=npc->y && y<npc->y+oh)
          return npc;
      }
    }
  }
#endif
  return NULL;
}

void TServerLevel::runScripts() {
  for (int i=npcs->count-1; i>=0; i--) {
    TServerSideNPC* npc = (TServerSideNPC*)(*npcs)[i];
    npc->runScript();
  }
}

double touchtestd[] = {1.5,0.5, 0,2, 1.5,3.5, 3,2};

void TServerLevel::TestPlayerTouchsNPC(TServerPlayer* player) {
  if (!Assigned(player)) return;
  TServerSideNPC* npc = (TServerSideNPC*)isOnNPC(player->x+touchtestd[player->dir*2],
    player->y+touchtestd[player->dir*2+1]);

  if (Assigned(npc))
    npc->doNPCAction("playertouchsme",player);
}


#endif


//--- TServerLevelLink ---

void TServerLevelLink::Assign(TJStringList* list) {
  if (!Assigned(list) || list->count<5) return;

  filename = LowerCaseFilename((*list)[0]);
  x = strtofloat((*list)[1]);
  y = strtofloat((*list)[2]);
  w = strtofloat((*list)[3]);
  h = strtofloat((*list)[4]);

  if (list->count>=6) {
    newx = (*list)[5];
    if (newx=="-1") newx = "playerx";
  } else
    newx = "playerx";
    
  if (list->count>=7) {
    newy = (*list)[6];
    if (newy=="-1") newy = "playery";
  } else
    newy = "playery";
}

void TServerLevelLink::Assign(const JString& str) {
  TJStringList* list = getStringTokens(str);
  Assign(list);
  delete(list);
}

JString TServerLevelLink::GetJStringRepresentation() {
  int index;
  JString res;

  res = JString()+" "+floattostr(x)+" "+floattostr(y)+" "+floattostr(w)+" "+floattostr(h)+" ";
  while (true) {
    index = Pos(' ',newx);
    if (index==0) break;
    newx = Copy(newx,1,index-1) + Copy(newx,index+1,Length(newx)-index);
  }
  res << newx << " ";
  while (true) {
    index = Pos(' ',newy);
    if (index==0) break;
    newy = Copy(newy,1,index-1) + Copy(newy,index+1,Length(newy)-index);
  }
  res << newy;

  while (true) {
    index = Pos(',',res);
    if (index==0) break;
    res[index] = '.';
  }
  return filename + res;
}

//--- TDestroyedLevel ---

TDestroyedLevel::TDestroyedLevel(TServerLevel* level) {
  cdlevel++;
  plainfile = level->plainfile;
  playerleaved = new TList();
  playerleaved->Assign(level->playerleaved);
  boardmodifies = level->boardmodifies;
  level->boardmodifies = new TList();
}

TDestroyedLevel::~TDestroyedLevel() {
  cdlevel--;
  if (Assigned(boardmodifies)) {
    for (int i=0; i<boardmodifies->count; i++) delete((TBoardModify*)(*boardmodifies)[i]);
    delete(boardmodifies);
    boardmodifies = NULL;
  }
  if (Assigned(playerleaved)) {
    delete(playerleaved);
    playerleaved = NULL;
  }
}

void TDestroyedLevel::RemoveFromList(TServerPlayer* player) {
  if (!Assigned(player) || !Assigned(playerleaved)) return;

  for (int i=0; i<playerleaved->count-1; i+=2) {
    int playerid = (int)(*playerleaved)[i];
    if (playerid==player->id) {
      playerleaved->Delete(i); // delete player id + leave time
      playerleaved->Delete(i);
      return;
    }
  }
}


//--- TServerNPC ---

TServerNPC::TServerNPC(const JString& filename, const JString& action, double x, double y) {
  int i;
  cnpc++;
  level = NULL;

  this->filename = filename;
  this->action = action;
  startimage = filename;
  canshowcharacter = (Pos("showcharacter",action)>0);
  cansetgif = (Pos("setgif",action)>0);
  this->x = x;
  this->y = y;
  hearts = 0;
  rupees = 0;
  darts = 0;
  bombs = 0;
  glovepower = 0;
  bombpower = 0;
#ifdef NEWWORLD
  ani = "idle";
#else
  swordpower = 0;
//  swordgif = "";
#endif
  shieldpower = 0;
//  shieldgif = "";
  bowgif = JString((char)(0+32));
  visflags = 1;
  blockflags = 0;
//  chatmsg = "";
  hurtdx = 0;
  hurtdy = 0;
  id = ServerFrame->GetFreeNPCID();
  spritenum = 0;
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
//  nickname = "";
//  horsegif = "";
//  headgif = "";
  for (i=0; i<10; i++)
    save[i] = 0;
  alignment = 50;
  gifpart = JString() + (char)(0+32) + (char)(0+32) + (char)(0+32) + (char)(0+32) +
    (char)(0+32) + (char)(0+32);

  for (i = 0; i<npcpropertycount; i++) modifytime[i] = 0;
  modifytime[NPCGIF] = curtime;
  modifytime[ACTIONSCRIPT] = curtime;
  modifytime[NPCX] = curtime;
  modifytime[NPCY] = curtime;
  modifytime[VISFLAGS] = curtime;

  orgindex = -1;
  moved = false;

#ifdef SERVERSIDENPCS
  dontblock = false;
  timeout = 0;
  pixelwidth = 32;
  pixelheight = 32;
  cursprite = 0;
  dir = 2;
  character = NULL;
  timeoutline = -1;
  for (i=0; i<npcpropertycount; i++) curmodified[i] = false;
#endif
}

TServerNPC::~TServerNPC() {
  cnpc--;

#ifdef SERVERSIDENPCS
  if (Assigned(character)) { delete(character); character = NULL; }
#endif
  ServerFrame->DeleteNPCID(id);
}

JString TServerNPC::GetProperty(int index) {
  JString str;
  int len;

  switch (index) {
    case NPCGIF: {
        str = RemoveGifExtension(filename);
        return JString((char)(Length(str)+32)) << str;
      }
    case ACTIONSCRIPT: {
#ifdef SERVERSIDENPCS
        len = 0;
        return JString((char)(((len >> 7) & 0x7F)+32)) << (char)((len & 0x7F)+32);
#else
        len = Length(action);
        return JString((char)(((len >> 7) & 0x7F)+32)) << (char)((len & 0x7F)+32) << action;
#endif
      }
    case NPCX:        return JString((char)((x*2)+32));
    case NPCY:        return JString((char)((y*2)+32));
    case NPCPOWER:    return JString((char)(hearts+32));
    case NPCRUPEES:   return JString((char)(((rupees >> 14) & 0x7F)+32)) <<
      (char)(((rupees >> 7) & 0x7F)+32) << (char)((rupees & 0x7F)+32);
    case NPCARROWS:   return JString((char)(darts+32));
    case NPCBOMBS:    return JString((char)(bombs+32));
    case NGLOVEPOWER: return JString((char)(glovepower+32));
    case NBOMBPOWER:  return JString((char)(bombpower+32));
#ifdef NEWWORLD
    case NPCANI:      return JString((char)(Length(ani)+32)) << ani;
#else
    case NSWORDGIF: {
        int sp = swordpower+30;
        if (sp<10) sp = 10;
        if (sp>50) sp = 50;
        if (sp==30) sp = 0;
        if (swordgif=="sword1.gif") sp = 1;
        if (swordgif=="sword2.gif") sp = 2;
        if (swordgif=="sword3.gif") sp = 3;
        if (swordgif=="sword4.gif") sp = 4;
        str = JString((char)(sp+32));
        JString str2 = RemoveGifExtension(swordgif);
        if (sp>=10) str << (char)(Length(str2)+32) << str2;
        return str;
      }
#endif
    case NSHIELDGIF: {
        int sp = shieldpower + 10;
        if (sp<10) sp = 10;
        if (sp>13) sp = 13;
        if (sp==10) sp = 0;
        if (shieldgif=="shield1.gif") sp = 1;
        if (shieldgif=="shield2.gif") sp = 2;
        if (shieldgif=="shield3.gif") sp = 3;
        str = JString((char)(sp+32));
        JString str2 = RemoveGifExtension(shieldgif);
        if (sp>=10) str << (char)(Length(str2)+32) << str2;
        return str;
      }
    case NPCBOWGIF:   return bowgif;
    case VISFLAGS:    return JString((char)(visflags+32));
    case BLOCKFLAGS:  return JString((char)(blockflags+32));
    case NPCMESSAGE: {
        if (Length(chatmsg)>200) chatmsg = Copy(chatmsg,1,200);
        return JString((char)(Length(chatmsg)+32)) << chatmsg;
      }
    case NPCHURTDXDY: return JString((char)(hurtdx*32+64)) << (char)(hurtdy*32+64);
    case NPCID:       return GetCodedID();
#ifdef NEWWORLD
    case NPCSPRITE:   return JString((char)((spritenum%4)+32)); // only send the direction
#else
    case NPCSPRITE:   return JString((char)(spritenum+32)); 
#endif
    case NPCCOLORS: {
#ifdef NEWWORLD
        return JString((char)(colors[0]+32)) << (char)(colors[1]+32) 
          << (char)(colors[2]+32) << (char)(colors[3]+32) << (char)(colors[4]+32)
          << (char)(colors[5]+32) << (char)(colors[6]+32) << (char)(colors[7]+32);
#else
        return JString((char)(colors[0]+32)) << (char)(colors[1]+32) <<
          (char)(colors[2]+32) << (char)(colors[3]+32) << (char)(colors[4]+32);
#endif        
      }
    case NPCNICKNAME: return JString((char)(Length(nickname)+32)) << nickname;
    case NPCHORSEGIF: {
        str = RemoveGifExtension(horsegif);
        return JString((char)(Length(str)+32)) << str;
      }
    case NPCHEADGIF: {
        if (Length(headgif)>0) {
          int sp = getGifNumber(headgif,JString() + "head");
          if ((sp>=0) && (sp<100))
            return JString((char)(sp+32));
          else {
            str = RemoveGifExtension(headgif);
            if (Length(str)>100) str = Copy(str,1,100);
            return JString((char)(100+Length(str)+32)) << str;
          }
        }
        else
          return JString((char)(0+32));
      }
    case NPCSAVE0:
    case NPCSAVE1:
    case NPCSAVE2:
    case NPCSAVE3:
    case NPCSAVE4:
    case NPCSAVE5:
    case NPCSAVE6:
    case NPCSAVE7:
    case NPCSAVE8:
    case NPCSAVE9: {
        int sp = save[index-NPCSAVE0];
        if (sp<0) sp = 0;
        if (sp>220) sp = 220;
        return JString((char)(sp+32));
      }
    case NALIGNMENT: return JString((char)(alignment+32));
    case NPCGIFPART: return gifpart;
  }
  return JString();
}

void TServerNPC::SetProperties(JString str, bool cansetaction) {
  int i, index, datalen, len;
  JString liststr, oldprop;

  while (Length(str)>=2) {
    index = str[1]-32;
    oldprop = GetProperty(index);
    datalen = 1;
    switch (index) {
      case NPCGIF: {
          len = str[2]-32;
          if (len<0) len = 0;
          datalen = 1+len;

          JString filestr = Trim(Copy(str,3,len));
          if (filestr=="#c#") {
            if (canshowcharacter || cansetaction) filename = filestr;
          } else if (cansetgif || cansetaction) {
            filename = filestr;
            if (Length(filename)>0 && filename==RemoveExtension(filename))
              filename << ".gif";
            if (Length(filename)>0)
              startimage = filename; 
          }
          break;
        }
      case ACTIONSCRIPT: {
          if (Length(str)>=3) {
            len = ((str[2]-32) << 7) + (str[3]-32);
            if (len<0) len = 0;
            datalen = 2+len;

            if (cansetaction) {
              action = Copy(str,4,len);
              canshowcharacter = (Pos("showcharacter",action)>0);
              cansetgif = (Pos("setgif",action)>0);
            }
          }
          break;
        }
      case NPCX: {
          x = ((double)(str[2]-32))/2;
          if (x<0) x = 0;
          if (x>63) x = 63;
          break;
        }
      case NPCY: {
          y = ((double)(str[2]-32))/2;
          if (y<0) y = 0;
          if (y>63) y = 63;
          break;
        }
      case NPCPOWER: hearts = str[2]-32; break;
      case NPCRUPEES: {
          if (Length(str)>=4)
            rupees = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);
          datalen = 3;
          break;
        }
      case NPCARROWS:   darts = str[2]-32; break;
      case NPCBOMBS:    bombs = str[2]-32; break;
      case NGLOVEPOWER: glovepower = str[2]-32; break;
      case NBOMBPOWER:  bombpower = str[2]-32; break;
#ifdef NEWWORLD
      case NPCANI: {
          len = str[2]-32;
          if (len<0) len = 0;
          ani = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
#else
      case NSWORDGIF: {
          int sp = str[2]-32;
          swordgif = "";
          if (sp>=10) {
            sp = sp-30;
            len = str[3]-32;
            if (len<0) len = 0;
            if (len>0) {
              swordgif = Copy(str,4,len);
              if (len>0 && swordgif==RemoveExtension(swordgif))
                swordgif << ".gif";
            }
            datalen = 2+len;
          } else {
            switch (sp) {
              case 0: swordgif = ""; break;
              case 1: swordgif = "sword1.gif"; break;
              case 2: swordgif = "sword2.gif"; break;
              case 3: swordgif = "sword3.gif"; break;
              case 4: swordgif = "sword4.gif"; break;
            }
          }
          swordpower = sp;
          break;
        }
#endif
      case NSHIELDGIF: {
          int sp = str[2]-32;
          shieldgif = "";
          if (sp>=10) {
            sp = sp-10;
            len = str[3]-32;
            if (len<0) len = 0;
            if (len>0) {
              shieldgif = Copy(str,4,len);
              if (len>0 && shieldgif==RemoveExtension(shieldgif))
                shieldgif << ".gif";
            }
            datalen = 2+len;
          } else {
            switch (sp) {
              case 0: shieldgif = ""; break;
              case 1: shieldgif = "shield1.gif"; break;
              case 2: shieldgif = "shield2.gif"; break;
              case 3: shieldgif = "shield3.gif"; break;
            }
          }
          if (sp>3) sp = 3;
          shieldpower = sp;
          break;
        }
      case NPCBOWGIF: {
          int sp = str[2]-32;
          bowgif = JString() + (char)str[2];
          if (sp>=10) {
            sp = sp-10;
            bowgif = Copy(str,2,1+sp);
            datalen = 1+sp;
          }
          break;
        }
      case VISFLAGS: visflags = str[2]-32; break;
      case BLOCKFLAGS: blockflags = str[2]-32; break;
      case NPCMESSAGE: {
          len = str[2]-32;
          if (len<0) len = 0;
          chatmsg = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
      case NPCHURTDXDY: {
          if (Length(str)>=3) {
            hurtdx = ((double)(str[2]-64))/32;
            hurtdy = ((double)(str[3]-64))/32;
          }
          datalen = 2;
          break;
        }
      case NPCID: {
//            id = ((str[2]-32) << 14) + ((str[3]-32) << 7) + (str[4]-32);
          datalen = 3;
          break;
        }
      case NPCSPRITE: {
          spritenum = str[2]-32; break;
          if (spritenum<0 || spritenum>=132) spritenum = 0;
        }
      case NPCCOLORS: {
#ifdef NEWWORLD
          if (Length(str)>=9) {
            for (i = 0; i<8; i++) 
              colors[i] = str[2+i]-32; 
          }
          datalen = 8; 
#else
          if (Length(str)>=6) {
            for (i = 0; i<5; i++)
              colors[i] = str[2+i]-32;
          }
          datalen = 5;
#endif
          break;
        }
      case NPCNICKNAME: {
          len = str[2]-32;
          if (len<0) len = 0;
          nickname = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
      case NPCHORSEGIF: {
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
      case NPCHEADGIF: {
          len = str[2]-32;
          if (len<0)
            headgif = "head0.gif";
          else if (len<100)
            headgif = JString() + "head" + len + ".gif";
          else
          {
            len = len-100;
            headgif = Copy(str,3,len);
            if (len>0 && headgif==RemoveExtension(headgif))
              headgif << ".gif";
            datalen = 1+len;
          }
          break;
        }
      case NPCSAVE0:
      case NPCSAVE1:
      case NPCSAVE2:
      case NPCSAVE3:
      case NPCSAVE4:
      case NPCSAVE5:
      case NPCSAVE6:
      case NPCSAVE7:
      case NPCSAVE8:
      case NPCSAVE9: {
          save[index-NPCSAVE0] = str[2]-32;
          break;
        }
      case NALIGNMENT: {
          alignment = str[2]-32;
          if (alignment>100) alignment = 100;
          if (alignment<0) alignment = 0;
          break;
        }
      case NPCGIFPART: {
          datalen = 6;
          JString str2 = Copy(str,2,6);
          if (Length(str2)==6) gifpart = str2;
          break;
        }

      default: return;
    }
    if (oldprop!=GetProperty(index))
      modifytime[index] = curtime;
//    str = Copy(str,2+datalen,Length(str)-1-datalen);
    str.setTempStart(2+datalen);
  }
}

JString TServerNPC::GetPropertyList(TServerPlayer* player) {
  JString str;
  if (!Assigned(player) || !Assigned(level)) return str;

  unsigned int leavingtime = level->GetLeavingTime(player);
  for (int i=0; i<npcpropertycount; i++) {
    if (modifytime[i]>0 && modifytime[i]>=leavingtime)
      str << (char)(i+32) << GetProperty(i);
  }
  return str;
}

JString TServerNPC::GetCodedID() {
  return JString() + (char)(((id >> 14) & 0x7F)+32) + (char)(((id >> 7) & 0x7F)+32) + (char)((id & 0x7F)+32);
}

JString TServerNPC::GetWeaponName() {
  if (toweaponsstr=="-") return JString();
  if (Length(toweaponsstr)>0) return toweaponsstr;
  int i = Pos("toweapons",action);
  if (i==0) {
    toweaponsstr = "-";
    return JString();
  }
  JString str = TrimLeft(Copy(action,i+9,Length(action)-9));
  i = Pos(';',str);
  int j = Pos('}',str);
  if (j>0 && j<i) i = j;
  if (i==0) {
    toweaponsstr = "-";
    return JString();
  }
  toweaponsstr = Trim(Copy(str,1,i-1));
  return toweaponsstr;
}


//--- TServerBaddy ---

TServerBaddy::TServerBaddy() {
  cbaddy++;
  deleteme = false;
  for (int i=0; i<baddypropertycount; i++)
    modifytime[i] = curtime;
}

TServerBaddy::~TServerBaddy() {
  cbaddy--;
}

JString TServerBaddy::GetProperty(int index) {
  JString gifname;

  switch (index) {
    case BADDYID:   return JString() + (char)(id+32);
    case BADDYX:    return JString() + (char)((x*2)+32);
    case BADDYY:    return JString() + (char)((y*2)+32);
    case BADDYTYPE: return JString() + (char)(comptype+32);
    case BADDYGIF:
      {
        gifname = "";
        if (filename != (*baddygifs)[comptype])
          gifname = (*baddygifs)[comptype];
        return JString() + (char)(power+32) + (char)(Length(gifname)+32) + gifname;
      }
    case BADDYMODE:    return JString() + (char)(mode+32);
    case BADDYANISTEP: return JString() + (char)(anicount+32);
    case BADDYDIR:     return JString() + (char)(dirs+32);
    case BADDYVERSE1:  return JString() + (char)(Length(v1)+32) + v1;
    case BADDYVERSE2:  return JString() + (char)(Length(v2)+32) + v2;
    case BADDYVERSE3:  return JString() + (char)(Length(v3)+32) + v3;
  }
  return JString();
}

void TServerBaddy::SetProperties(JString str) {
  int i, index, datalen, len;

  while (Length(str)>1) {
    index = str[1]-32;
    datalen = 1;
    switch (index) {
      case BADDYID: id = str[2]-32; break;
      case BADDYX:
        {
          x = ((double)(str[2]-32))/2;
          if (x<0) x = 0;
          if (x>63) x = 63;
          break;
        }
      case BADDYY:
        {
          y = ((double)(str[2]-32))/2;
          if (y<0) y = 0;
          if (y>63) y = 63;
          break;
        }
      case BADDYTYPE:
        {
          comptype = str[2]-32;
          if (comptype<0) comptype = 0;
          if (comptype>=baddytypes) comptype = baddytypes-1;
          break;
        }
      case BADDYGIF:
        {
          if (Length(str)>=3) {
            power = str[2]-32;
            len = str[3]-32;
            if (len<0) len = 0;
            datalen = 2+len;
            if (len>0)
              filename = Copy(str,4,len);
            else
              filename = (*baddygifs)[comptype];
          } 
          break;
        }
      case BADDYMODE:
        {
          mode = str[2]-32;
          if (mode==DEAD && respawn)
            respawncounter = baddyrespawntime;
          break;
        }
      case BADDYANISTEP: anicount = str[2]-32; break;
      case BADDYDIR: dirs = str[2]-32; break;
      case BADDYVERSE1:
        {
          len = str[2]-32;
          if (len<0) len = 0;
          v1 = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
      case BADDYVERSE2:
        {
          len = str[2]-32;
          if (len<0) len = 0;
          v2 = Copy(str,3,len);
          datalen = 1+len;
          break;
        }
      case BADDYVERSE3:
        {
          len = str[2]-32;
          if (len<0) len = 0;
          v3 = Copy(str,3,len);
          datalen = 1+len;
          break;
        }

      default: return;
    }
    modifytime[index] = curtime;
//    str = Copy(str,2+datalen,Length(str)-1-datalen);
    str.setTempStart(2+datalen);
  }
}

JString TServerBaddy::GetPropertyList(TServerPlayer* player) {
  JString str;
  if (!Assigned(player) || !Assigned(level)) return str;

  unsigned int leavingtime = level->GetLeavingTime(player);
  if (leavingtime==0 && mode==DEAD) return str;

  for (int i=0; i<baddypropertycount; i++) {
    if (modifytime[i]>0 && modifytime[i]>=leavingtime)
      str << (char)(i+32) << GetProperty(i);
  }
  return str;
}


//--- TServerChest ---

void TServerChest::Assign(TJStringList* list) {
  if (!Assigned(list) || list->count<4) return;

  x = strtoint((*list)[0]);
  if (x<0) x = 0;
  if (x>62) x = 62;
  y = strtoint((*list)[1]);
  if (y<0) y = 0;
  if (y>62) y = 62;

  JString str = (*list)[2];
  itemindex = strtoint(str);
  int i = itemnames->indexOf(str);
  if (i>=0) itemindex = i;

  signindex = strtoint((*list)[3]);
}

JString TServerChest::GetClosed() {
  return JString() + (char)(0+32) + (char)(x+32) + (char)(y+32) + (char)(itemindex+32)+
    (char)(signindex+32);
}

JString TServerChest::GetOpened() {
  return JString() + (char)(1+32) + (char)(x+32) + (char)(y+32);
}

//--- TServerHorse ---

JString TServerHorse::GetJStringRepresentation() {
  filename = RemoveGifExtension(filename);
  return JString() + (char)(x*2+32) + (char)(y*2+32) + (char)(dir+32) + filename;
}

//--- TServerExtra ---

JString TServerExtra::GetStringRepresentation() {
  return JString((char)((int)(x*2)+32)) + (char)((int)(y*2)+32) +
    (char)(itemindex+32);
}

JString TServerExtra::GetPosString() {
  return JString((char)((int)(x*2)+32)) + (char)((int)(y*2)+32);
}

