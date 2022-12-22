#ifndef NPCS_H 
#define NPCS_H 
 
#include "levels.h" 
#include "pascalstrings.h" 
 
class TNPCOperation; 
class TNPCBoolOperation; 
class TNPCCommand; 
 
extern int nwsec; 
extern int nwmin; 
extern int nwhour; 
extern int nwday; 
extern int nwmonth; 
extern int nwyear;
extern bool useblockflag; 
 
JString removeComments(JString str); 
void testfunc(); 
 
class TServerSideNPC: public TServerNPC { 
 public: 
  TList* commandlist; // of TNPCCommand 
  TList* actionvars; // of TNPCVariable 
  bool lastifwastrue; 
  bool created;
  bool canwarp;
 
  TJStringList* actionflags; 
  TList* actionplayers; // of TServerPlayer 
 
  TServerSideNPC(const JString& filename, const JString& action, double x, double y); 
  ~TServerSideNPC(); 
  void parseString(JString str); 
  void parseCommand(JString str); 
  void parseAssignment(JString str); 
  TNPCOperation* parseValue(JString str); 
  TNPCBoolOperation* parseBoolValue(JString str); 
  void calcAssignment(TNPCCommand* com); 
  double calcOperation(TNPCOperation* op); 
  bool calcBoolOperation(TNPCBoolOperation* bop); 
  bool isflagset(const JString& flag);
  void parseList(int startindex, int endindex);
  void runScript();
  void doNPCAction(JString flagstr, TServerPlayer* player);
  void removeActions(TServerPlayer* player); 
 
  void optimizeOperation(TNPCOperation* op); 
  bool compareOperations(TNPCOperation* op1, TNPCOperation* op2); 
  JString GetRealMessage(JString msg, bool withlineends); 
  TServerNPC* GetNPC(int index); 
  TServerPlayer* GetPlayer(int index); 
  int GetPlayerCount(); 
  double calcValue(JString str); 
  void testForLinks();
  
};
 
class TNPCVariable { 
 public: 
  JString name; 
 
  bool isarray; 
  double value; 
 
  int arrsize; 
  double* val; 
 
  TNPCVariable(JString name); 
  ~TNPCVariable(); 
  void SetArraySize(int size); 
}; 
 
class TNPCOperation { 
 public: 
  int id; 
 
  double value; 
  JString strvalue; 
  TNPCOperation* op1; 
  TNPCOperation* op2; 
  TNPCVariable* pvar; 
  TList* arrlist; // of TNPCVariable 
 
  TNPCOperation(int id, double value); 
  ~TNPCOperation(); 
}; 
 
class TNPCBoolOperation { 
 public: 
  int id; 
 
  JString flagstring; 
  JString flagstring2; 
  bool value; 
  TNPCBoolOperation* bop1; 
  TNPCBoolOperation* bop2; 
  TNPCOperation* op1; 
  TNPCOperation* op2; 
 
  TNPCBoolOperation(); 
  ~TNPCBoolOperation(); 
}; 
 
class TNPCCommand { 
 public: 
  int id; 
 
  JString arg1; 
  JString arg2; 
  int jumppos; 
  TNPCOperation* op1; 
  TNPCOperation* op2; 
  TNPCOperation* op3; 
  TNPCOperation* op4; 
  TNPCOperation* op5; 
  TNPCOperation* op6; 
  TNPCBoolOperation* bop; 
  double val1; 
 
  TNPCCommand(); 
  ~TNPCCommand(); 
}; 
 
 
#endif // NPCS_H
