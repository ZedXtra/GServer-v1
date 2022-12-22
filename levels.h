#ifndef LEVELS_H 
#define LEVELS_H 
 
// Some class instance counters for debugging 
extern int clevel; 
extern int clevelmodify; 
extern int cboardmodify; 
extern int clevellink; 
extern int cchest; 
extern int chorse; 
extern int cextra; 
extern int cnpc; 
extern int cnpcmodify; 
extern int cbaddy; 
extern int cbaddymodify; 
extern int cdlevel; 

#include "convertedclasses.h"
#include "otherstuff.h"
#include "pascalstrings.h"
 
#define npcpropertycount 35 
#define baddypropertycount 11 
#define saveleveldelay 900 
 
// NPC properties types 
enum { 
  NPCGIF,       //  0 
  ACTIONSCRIPT, //  1 
  NPCX,         //  2 
  NPCY,         //  3 
  NPCPOWER,     //  4 
  NPCRUPEES,    //  5 
  NPCARROWS,    //  6 
  NPCBOMBS,     //  7 
  NGLOVEPOWER,  //  8 
  NBOMBPOWER,   //  9 
#ifdef NEWWORLD
  NPCANI,       // 10
#else
  NSWORDGIF,    // 10
#endif
  NSHIELDGIF,   // 11
  NPCBOWGIF,    // 12 
  VISFLAGS,     // 13  
  BLOCKFLAGS,   // 14  
  NPCMESSAGE,   // 15  
  NPCHURTDXDY,  // 16  
  NPCID,        // 17  
  NPCSPRITE,    // 18  
  NPCCOLORS,    // 19
  NPCNICKNAME,  // 20  
  NPCHORSEGIF,  // 21 
  NPCHEADGIF,   // 22  
  NPCSAVE0,     // 23  
  NPCSAVE1,     // 24  
  NPCSAVE2,     // 25  
  NPCSAVE3,     // 26  
  NPCSAVE4,     // 27 
  NPCSAVE5,     // 28  
  NPCSAVE6,     // 29  
  NPCSAVE7,     // 30  
  NPCSAVE8,     // 31  
  NPCSAVE9,     // 32  
  NALIGNMENT,   // 33  
  NPCGIFPART    // 34  
};  
  
// Baddy properties types  
enum {  
  BADDYID,      //  0  
  BADDYX,       //  1  
  BADDYY,       //  2  
  BADDYTYPE,    //  3 
  BADDYGIF,     //  4  
  BADDYMODE,    //  5  
  BADDYANISTEP, //  6  
  BADDYDIR,     //  7  
  BADDYVERSE1,  //  8  
  BADDYVERSE2,  //  9  
  BADDYVERSE3   // 10  
};  
  
// Baddy modes 
enum {  
  WALK,        // 0  
  LOOK,        // 1  
  HUNT,        // 2  
  HURTED,      // 3  
  BUMPED,      // 4  
  DIE,         // 5  
  SWAMPSHOT,   // 6  
  HAREJUMP,    // 7  
  OCTOSHOT,    // 8  
  DEAD         // 9  
};  
 
class TList;  
class TJStringList; 
class TStream; 
class TServerPlayer; 
class TServerLevel; 
class TServerLevelModify; 
class TServerBaddy;
class TServerNPC; 
 
extern int horselifetime; 
extern int baddyrespawntime; 
extern int baddytypes; 
extern int baddypower[]; 
extern TJStringList* baddygifs; 
extern TJStringList* baddynameslower; 
extern int baddystartmode[]; 
extern bool baddypropsreinit[]; 
extern TList* destroyedlevels; // of TDestroyedLevel 
extern int bw; 
 
class TServerLevel { 
 public: 
  JString filename; 
  JString plainfile; 
  unsigned int modtime;  
  short board[64*64];  
  
  TList* boardmodifies; // of TBoardModify
  TList* playerleaved; // of int (player id + leaving time)
  
  TList* links; // of TServerLevelLink  
  TJStringList* linkstrings; 
  TList *npcs, *orgnpcs; // of TServerNPC  
  TJStringList* signs;  
  TList* players; // TServerPlayer  
  TList* horses; // of TServerHorse  
  TList* baddies; // of TServerBaddy  
  TList* chests; // of TServerChest 
  TList* extras; // of TServerExtra  
  int emptytime; 
  int savecounter; 
 
#ifdef SERVERSIDENPCS 
  TList* actionvars; 
#endif  
  
  TServerLevel(const JString& filename);  
  ~TServerLevel();  
  void Clear();  
  
  int GetLeavingTimeEntry(TServerPlayer* player);
  unsigned int GetLeavingTime(TServerPlayer* player);
  void AddToList(TServerPlayer* player);
  void RemoveFromList(TServerPlayer* player);  
  TJStringList* GetModifyList(TServerPlayer* player);
  void AddModify(TServerPlayer* fromplayer, JString data);
  void DoRespawnCounter();
  
  void Load(); 
  void LoadNW(const JString& filename); 
  void LoadTemp(); 
  void SaveTemp(); 
  JString GetModTime();
  JString getFullLink(const JString& linkdest); 
  void ReadLinks(TStream* data); 
  void ReadBaddies(TStream* data); 
  void ReadNPCs(TStream* data); 
  void ReadChests(TStream* data); 
  void ReadSigns(TStream* data); 
  void putHorse(double x, double y, JString gifname, int dir); 
  void removeHorse(double x, double y); 
  void removeExtra(double x, double y); 
  bool takeExtra(double x, double y, int itemindex, TServerPlayer* player); 
  TServerBaddy* GetNewBaddy(); 
 
#ifdef NEWWORLD 
  bool isWall(int num); 
  bool isWater(int num);
#endif
 
#ifdef SERVERSIDENPCS 
  bool isOnWall(double x, double y); 
  bool isOnWater(double x, double y); 
  bool isOnPlayer(double x, double y); 
  TServerNPC* isOnNPC(double x, double y);
  void runScripts();
  void TestPlayerTouchsNPC(TServerPlayer* player);
#endif
}; 
 
class TBoardModify {
 public:
  JString modify;
  JString previous;
  bool isrespawn;
  bool dorespawn;
  unsigned int modifytime;
  int counter;
 
  TBoardModify() { cboardmodify++; };
  ~TBoardModify() { cboardmodify--; }; 
}; 
 
class TDestroyedLevel { 
 public: 
  JString plainfile; 
  TList* boardmodifies; // of TBoardModify
  TList* playerleaved; // of int (player id + leaving time)

  TDestroyedLevel(TServerLevel* level);
  ~TDestroyedLevel();
  void RemoveFromList(TServerPlayer* player);
};

class TServerLevelLink {
 public:
  JString filename;
  double x,y,w,h;
  JString newx,newy;
 
  TServerLevelLink() { clevellink++; }; 
  ~TServerLevelLink() { clevellink--; }; 
 
  void Assign(TJStringList* list); 
  void Assign(const JString& str); 
  JString GetJStringRepresentation(); 
}; 
 
class TServerNPC { 
 public: 
  TServerLevel* level; 
  int orgindex; 
  bool moved; 
  unsigned int modifytime[npcpropertycount];
  bool canshowcharacter; 
  bool cansetgif; 
  JString toweaponsstr; 
  JString startimage; 
 
  JString filename; 
  JString action; 
  double x; 
  double y; 
  int hearts;  
  int rupees;  
  int darts;  
  int bombs;  
  int glovepower;  
  int bombpower;
#ifdef NEWWORLD
  JString ani;
#else
  int swordpower;
  JString swordgif;
#endif
  int shieldpower; 
  JString shieldgif;  
  JString bowgif;  
  int visflags;  
  int blockflags;  
  JString chatmsg;  
  double hurtdx;  
  double hurtdy;  
  int id;  
  int spritenum;  
#ifdef NEWWORLD 
  int colors[8];  
#else 
  int colors[5]; 
#endif 
  JString nickname;  
  JString horsegif;  
  JString headgif;  
  int save[10];  
  int alignment;  
  JString gifpart; 
 
#ifdef SERVERSIDENPCS 
  bool dontblock; 
  int timeout; 
  int pixelwidth; 
  int pixelheight; 
  int cursprite; 
  int dir; 
  TServerPlayer* character; 
  int timeoutline; 
  bool curmodified[npcpropertycount]; 
#endif 
 
  
  TServerNPC(const JString& filename, const JString& action, double x, double y);  
  ~TServerNPC();  
  
  JString GetProperty(int index);  
  void SetProperties(JString str, bool cansetaction);  
  JString GetPropertyList(TServerPlayer* player);  
  JString GetCodedID();
  JString GetWeaponName();
};


class TServerBaddy {
 public:
  int id;
  bool deleteme;
  double x;
  double y;
  int comptype;
  int power;
  JString filename;
  int mode;
  int anicount;
  int dirs;
  JString v1;
  JString v2;
  JString v3;

  TServerLevel* level;
  double orgx, orgy;
  bool respawn;
  int respawncounter;
  unsigned int modifytime[baddypropertycount];

  TServerBaddy();
  ~TServerBaddy();
  JString GetProperty(int index);
  void SetProperties(JString str);
  JString GetPropertyList(TServerPlayer* player);
};

class TServerHorse {
 public:
  double x,y;
  JString filename;
  int dir;
  int counter;

  TServerHorse() { chorse++; }; 
  ~TServerHorse() { chorse--; }; 
  JString GetJStringRepresentation(); 
}; 
 
class TServerChest { 
 public: 
  int x,y; 
  int itemindex; 
  int signindex; 
 
  TServerChest() { cchest++; }; 
  ~TServerChest() { cchest--; }; 
  void Assign(TJStringList* list); 
  JString GetClosed(); 
  JString GetOpened(); 
}; 
 
class TServerExtra { 
 public: 
  double x,y; 
  int itemindex; 
  int counter; 
 
  TServerExtra() { cextra++; }; 
  ~TServerExtra() { cextra--; }; 
  JString GetStringRepresentation(); 
  JString GetPosString(); 
}; 
 
#endif // LEVELS_H
