#ifndef OTHERSTUFF_H 
#define OTHERSTUFF_H 
 
#include "gserver.h" 
#include "convertedclasses.h" 
#include "pascalstrings.h" 
 
class TServerPlayer; 
class TList; 
class TJStringList; 
 
extern JString dberror;
extern JString startworld;

class TServerAccount {
 public:
  JString name;
  JString password;
  JString email;
  bool blocked;
  bool onlyload;
  int adminlevel;
  JString adminworlds;

  TServerAccount(); 
  ~TServerAccount() { caccounts--; }; 
};

class TServerWeapon {
 public:
  JString name;
  JString image;
  JString world;
  unsigned int modtime;
  JString fullstr;
  JString dataforplayer;
};

void CreateNewDBAccount(JString name, JString password, int id);
void LoadDBAccount(TServerPlayer* player, const JString& name, JString world);
void SaveDBAccount(TServerPlayer* player);
TServerAccount* GetAccount(const JString& name);
JString SetAccount(JString data, int rcadminlevel); // returns the changed account name
int GetAccountCount();
JString AddAccountFromRC(JString data, int rcadminlevel); // returns the added account name
JString GetAccountsList(const JString& likestr, const JString& wherestr);
void DeleteAccount(const JString& accname, int adminlevel);
void DeleteGuild(const JString& guildname);
void DelGuildMember(const JString& accname, const JString& guildname);
JString ListGuild(const JString& likestr, const JString& wherestr);
JString ListGuildMembers(const JString& likestr, const JString& wherestr, const JString& guildname);
void CreateGuild(const JString& accname, const JString& player, const JString& rank,const JString& guildname);
void LoadWeapons(const char* loadfile);
void SaveWeapons();
void ToStartLevel(const JString& accname);
void ReadWarpAccounts();
//void ConvertAccounts();
JString GetProfile(const JString& accname);
void SetProfile(JString data, const JString& setteraccname, int setteradminlevel);
bool VerifyGuildName(JString guild, const JString& nick, const JString& accountname);
//bool InSameGuild(const JString& nick1, const JString& nick2); 
bool LevelIsJail(const JString& str); 
bool IsIPInList(TSocket* sock, const JString& accname, const JString& listfile);
JString CompressBuf(const JString& arr);
JString DecompressBuf(const JString& arr); 

// ADDED STUFF!

// Init and Close database
int InitDatabase();
void CloseDatabase();

// New added functions
bool CheckPartofString(JString str, JString str2);
void RefindAllSubdirs(JString path);
 
#endif // OTHERSTUFF_H

