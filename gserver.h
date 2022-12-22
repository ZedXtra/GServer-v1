#ifndef GSERVER_H 
#define GSERVER_H 
 
// Some class instance counters for debugging 
extern int cplayer; 
extern int cplayermodify; 
extern int caccounts; 
 
#include "levels.h" 
#include "convertedclasses.h" 
#include "otherstuff.h" 
#include "pascalstrings.h" 
 
#define playerpropertycount 35
#ifdef NEWWORLD
  #define savepropsdelay 30
#else
  #define savepropsdelay 300
#endif
#define giveitemcount 25  
  
// Directions  
enum {  
  UP,          // 0  
  LEFT,        // 1  
  DOWN,        // 2  
  RIGHT        // 3  
};  
  
// Client package types 
enum {  
  LEVELWARP,    //  0 
  BOARDMODIFY,  //  1 
  PLAYERPROPS,  //  2 
  NPCPROPS,     //  3  
  ADDBOMB,      //  4  
  DELBOMB,      //  5  
  TOALLMSG,     //  6  
  ADDHORSE,     //  7
  DELHORSE,     //  8  
  ADDARROW,     //  9  
  FIRESPY,      // 10  
  CARRYTHROWN,  // 11  
  ADDEXTRA,     // 12  
  DELEXTRA,     // 13  
  CLAIMPKER,    // 14  
  BADDYPROPS,   // 15  
  HURTBADDY,    // 16  
  ADDBADDY,     // 17  
  SETFLAG,      // 18  
  UNSETFLAG,    // 19  
  OPENCHEST,    // 20  
  ADDNPC,       // 21  
  DELNPC,       // 22  
  WANTGIF,      // 23  
  NPCWEAPONIMG, // 24  
  EMPTY25,      // 25 
  HURTPLAYER,   // 26 
  EXPLOSION,    // 27 
  PRIVMESSAGE,  // 28 
  DELNPCWEAPON, // 29 
  MODLEVELWARP, // 30 
  PACKCOUNT,    // 31 
  TAKEEXTRA,    // 32 
  ADDWEAPON2,   // 33 
  EMPTY34,      // 34 
  EMPTY35,      // 35 
  EMPTY36,      // 36 
  EMPTY37,      // 37 
  EMPTY38,      // 38 
  EMPTY39,      // 39
  EMPTY40,      // 40 
  EMPTY41,      // 41 
  EMPTY42,      // 42  
  EMPTY43,      // 43  
  EMPTY44,      // 44  
  EMPTY45,      // 45  
  EMPTY46,      // 46  
  EMPTY47,      // 47  
  EMPTY48,      // 48  
  EMPTY49,      // 49  
  EMPTY50,      // 50  
  WANTACCPROPS, // 51  
  SETACCPROPS,  // 52  
  ADDACCOUNT,   // 53  
  DELACCOUNT,   // 54
  SETRESPAWN,   // 56
  SETHORSELIFE, // 57
  SETAPINCRTIME,// 58
  SETBADDYRESP, // 59
  WANTPLPROPS,  // 60
  SETPLPROPS,   // 61
  DISPLAYER,    // 62
  UPDLEVELS,    // 63
  ADMINMSG,     // 64
  PRIVADMINMSG, // 65
  LISTRCS,      // 66
  DISCONNECTRC, // 67
  APPLYREASON,  // 68
  LISTSFLAGS,   // 69
  SETSFLAGS,    // 70

  DADDACCOUNT,    // 71
  DDELACCOUNT,    // 72
  DWANTACCLIST,   // 74
  DWANTPLPROPS,   // 75
  DWANTACCPLPROPS,// 76
  DSETPLPROPS,    // 75
  DSETACCPLPROPS, // 76
  DWANTACCOUNT,   // 77
  DSETACCOUNT,    // 78
  DRCCHAT,        // 79
  DGETPROFILE,    // 80
  DSETPROFILE,    // 81
  WARPPLAYER,     // 82
};

// Server package types
enum {
  LEVELBOARD,   //  0
  LEVELLINK,    //  1
  SBADDYPROPS,  //  2
  SNPCPROPS,    //  3
  LEVELCHEST,   //  4
  LEVELSIGN,    //  5
  LEVELNAME,    //  6
  SBOARDMODIFY, //  7
  OTHERPLPROPS, //  8
  SPLAYERPROPS, //  9
  ISLEADER,     // 10  
  SADDBOMB,     // 11   
  SDELBOMB,     // 12  
  STOALLMSG,    // 13 
  PLAYERWARPED, // 14  
  LEVELFAILED,  // 15  
  DISMESSAGE,   // 16  
  SADDHORSE,    // 17 
  SDELHORSE,    // 18  
  SADDARROW,    // 19  
  SFIRESPY,     // 20  
  SCARRYTHROWN, // 21  
  SADDEXTRA,    // 22
  SDELEXTRA,    // 23  
  NPCMOVED,     // 24 
  UNLIMITEDSIG, // 25  
  NPCACTION,    // 26  
  SHURTBADDY,   // 27  
  SSETFLAG,     // 28  
  SDELNPC,      // 29  
  GIFFAILED,    // 30  
  SUNSETFLAG,   // 31  
  SNPCWEAPONIMG,// 32  
  SADDNPCWEAPON,// 33  
  SDELNPCWEAPON,// 34  
  SADMINMSG,    // 35  
  SEXPLOSION,   // 36  
  SPRIVMESSAGE, // 37  
  PUSHAWAY,     // 38
  LEVELMODTIME, // 39
  SPLAYERHURT,  // 40
  STARTMESSAGE, // 41
  NEWWORLDTIME, // 42 
  SADDDEFWEAPON,// 43
  SEMPTY44,     // 44
  SEMPTY45,     // 45
  SEMPTY46,     // 46
  SEMPTY47,     // 47
  SEMPTY48,     // 48
  SEMPTY49,     // 49
  SADDACCOUNT,  // 50
  SACCSTATUS,   // 51
  SACCNAME,     // 52
  SDELACCOUNT,  // 53
  SACCPROPS,    // 54
  SADDPLAYER,   // 55
  SDELPLAYER,   // 56
  SACCPLPROPS,  // 57
  SCHPLACCOUNT, // 58
  SCHPLPROPS,   // 59
  SEMPTY60,     // 60
  SSERVERFLAGS, // 61
  SEMPTY62,     // 62
  SEMPTY63,     // 63
  SEMPTY64,     // 64
  SEMPTY65,     // 65
  SEMPTY66,     // 66
  SEMPTY67,     // 67
  SEMPTY68,     // 68
  SEMPTY69,     // 69
  DACCLIST,     // 70
  DPLPROPS,     // 71
  DACCPLPROPS,  // 72
  DACCOUNT,     // 73
  DRCLOG,       // 74
  DPROFILE,     // 75
};
 
// Client types 
enum { 
  CLIENTPLAYER, // 0 
  CLIENTRC      // 1 
}; 
 
// Player properties types 
enum { 
  NICKNAME,     //  0 
  MAXPOWER,     //  1 
  CURPOWER,     //  2 
  RUPEESCOUNT,  //  3 
  ARROWSCOUNT,  //  4 
  BOMBSCOUNT,   //  5 
  GLOVEPOWER,   //  6 
  BOMBPOWER,    //  7
#ifdef NEWWORLD
  PLAYERANI,    //  8
#else
  SWORDPOWER,   //  8
#endif
  SHIELDPOWER,  //  9
  SHOOTGIF,     // 10 
  HEADGIF,      // 11 
  CURCHAT,      // 12 
  PLAYERCOLORS, // 13 
  PLAYERID,     // 14 
  PLAYERX,      // 15 
  PLAYERY,      // 16 
  PLAYERSPRITE, // 17 
  STATUS,       // 18 
  CARRYSPRITE,  // 19 
  CURLEVEL,     // 20 
  HORSEGIF,     // 21 
  HORSEBUSHES,  // 22 
  EFFECTCOLORS, // 23 
  CARRYNPC,     // 24 
  APCOUNTER,    // 25 
  MAGICPOINTS,  // 26 
  KILLSCOUNT,   // 27 
  DEATHSCOUNT,  // 28 
  ONLINESECS,   // 29 
  LASTIP,       // 30 
  UDPPORT,      // 31 
  PALIGNMENT,   // 32
  PADDITFLAGS,  // 33
  PACCOUNTNAME  // 34
};


class TList;
class TServerAccount;
class TTempAccount;
class TJStringList;
class TSocket;
class TServerFrame;
class TServerPlayer;
class TServerLevel;
class TServerNPC;

extern TJStringList* itemnames;
extern int respawntime;
extern JString startlevel;
extern double startx;
extern double starty;
extern const char fileseparator;
extern unsigned int curtime;

#ifdef LIMITED
extern int maxplayer;
#endif

extern TServerFrame* ServerFrame;  
extern TList* weapons; // of TServerWeapon


class TServerFrame {
 public:
  TSocket *sock;
  TSocket* responsesock;
  TList* levels; // of TServerLevel
  TList* players; // of TServerPlayer  
  TJStringList* serverflags;  
  int savepropscounter; 
  unsigned int starttime; 
  JString startmes; 
#ifdef NEWWORLD 
  int nwtime; 
#endif 
 
  void startServer(const JString& port); 
  ~TServerFrame(); 
  void LoadStartMessage(); 
  void tolog(const JString& str); 
  void toerrorlog(JString str); 
  void Timer1Timer(); 
  void SaveHTMLStats(bool serverup); 
  void doUDPResponse(); 
  JString getDir(); 
  void DeleteNPCID(int id); 
  int GetFreeNPCID(); 
  void DeletePlayerID(int id); 
  int GetFreePlayerID(); 
#ifdef NEWWORLD 
  int CalcTime(); 
#endif 
 
  TServerLevel* LoadLevel(JString filename); 
  void CreatePlayer(TSocket* clientsock); 
  void DeletePlayer(TServerPlayer* player); 
  void RemoveDestroyedLevel(JString filename); 
  void UpdateLevel(TServerLevel* level);
  void UpdateLevels(TJStringList* files);
#ifdef NEWWORLD
  void UpdateNewLevels();
#endif
  void DeleteNPC(TServerNPC* npc);
  void SendAddServerFlag(const JString& flag, TServerPlayer* from); 
  void SendDelServerFlag(const JString& flag, TServerPlayer* from); 
  TServerPlayer* GetPlayerForAccount(const JString& accname);
  void SendRCLog(const JString& logtext);
  void SendPrivRCLog(TServerPlayer *player, const JString& logtext);
  void SendOthersRCLog(TServerPlayer *playerme, const JString& logtext);
}; 
 
class TServerPlayer { 
 private: 
  void parseBoardModify(JString data); 
  void parseNewNPC(JString data); 
  void sendPosToWorld(); 
  void SendToWorld(int attribute);
#ifndef NEWWORLD
  void SetSword(JString newsword);
#endif
  void SetShield(JString newshield);
  void SendRCPlayerProps(TServerPlayer* player, bool sendaccount);
  void SetRCPlayerProps(JString data, TServerPlayer* player, bool setaccount);
  void parseChat(); 
  void updateCurrentLevel();  
  void loginerror(const JString& str); 
 
 public:  
  bool deleteme; 
  bool initialized; 
  bool isrc; 
 
  TSocket* sock; 
  int ver; 
  int adminlevel; 
  JString adminworlds;
  JString buffer; 
  JString rowout;  
  JString compout;  
  bool firstlevel;  
  bool firstdata;  
  JString accountname; 
  int savewait;  
  int nomovementcounter; 
  int noactioncounter; 
  int nodatacounter; 
  int changenickcounter; 
  int changecolorscounter; 
  int outbufferfullcounter; 
  TServerLevel* level;  
  JString playerworld;  
  JString plainnick; 
  JString plainguild; 
  int packcount; 
  TJStringList* weaponstosend; 
  TJStringList* imagestosend;

  JString nickname;  
  int maxpower;  
  double power;  
  int rubins;  
  int darts;  
  int bombscount;  
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
  JString shootgif;  
  JString headgif;  
  JString curchat; 
#ifdef NEWWORLD 
  int colors[8];  
#else 
  int colors[5]; 
#endif 
  int id; 
  double x; 
  double y; 
  int spritenum; 
#ifdef SERVERSIDENPCS 
  int cursprite; 
  int dir; 
#endif 
  int status; 
  int carrysprite; 
  JString levelname; // only for the account system
  JString horsegif;
  int horsebushes;
  TServerNPC* carrynpc;
  int apcounter;
  int magicpoints;
  int kills;
  int deaths;
  int onlinesecs;
  unsigned int lastip;
  int udpport;
  int alignment;
  int additionalflags;

  TJStringList* actionflags;
  TJStringList* openedchests;
  TJStringList* myweapons;

  TServerPlayer(TSocket* clientsock, int id);
  ~TServerPlayer();
  bool loggedin();
  bool isblocked();
  void ClientReceive();
  void AddRowToComp();
  void SendData(int id, const JString& str);
  void SendOutgoing();
  void SendWeaponsAndImages();
  void parse(const JString& line);
  void SendAccount(const JString& line);
  void SaveMyAccount();
  void gotoNewWorld(const JString& newworld, const JString& filename,
    double newx, double newy);
  void enterLevel(JString filename, double newx, double newy,
    JString modtime);
  void leaveLevel();
  bool isadminin(JString world);
  void ApplyAccountChange(TServerPlayer* player2, bool dowarp);

  JString GetProperty(int index);
  void SetProperties(JString str);
  JString GetPropertyList(TServerPlayer* player);
  JString GetCodedID();
};

 
JString GetLevelFile(JString filename); 
bool LevelFileExists(const JString& filename);  
unsigned int GetUTCFileModTime(JString filename); 
JString AddFileSeparators(JString str); 
JString RemoveExtension(JString gifname); 
 
JString RemoveGifExtension(JString gifname); 
int getGifNumber(JString gifname, const JString& leading); 
 
  
#endif // GSERVER_H 

