#include <unistd.h>
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <time.h>
#include <signal.h> 
#include <stdio.h> 
#include <iostream> 
#include <string.h> 
#include <fstream.h> 
#include <math.h> 
#include <dirent.h> 
#include <sys/stat.h> 
#include <errno.h>
#include <sys/param.h>
#include <locale.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <ctype.h> 

#include "npcs.h"

bool lastcommandwasif = false;
bool endparsing = false;
TServerPlayer* actionplayer = NULL;
JString curaction;

int nwsec = 0;
int nwmin = 0;
int nwhour = 14;
int nwday = 0;
int nwmonth = 0;
int nwyear = 1000;

bool useblockflag = true;
double doubleprecision = 0.0001;


//--- TServerSideNPC ---

TServerSideNPC::TServerSideNPC(const JString& filename, const JString& action, double x, double y) :
    TServerNPC(filename,action,x,y) {
  commandlist = NULL;
  actionvars = new TList();
  dontblock = false;
  lastifwastrue = false;
  created = false;
  canwarp = false;

  actionflags = new TJStringList();
  actionplayers = new TList();
  doNPCAction("created",NULL);
}

TServerSideNPC::~TServerSideNPC() {
  if (Assigned(commandlist)) {
    for (int i=0; i<commandlist->count; i++) delete((TNPCCommand*)(*commandlist)[i]);
    delete(commandlist);
    commandlist = NULL;
  }
  if (Assigned(actionvars)) {
    for (int i=0; i<actionvars->count; i++) delete((TNPCVariable*)(*actionvars)[i]);
    delete(actionvars);
    actionvars = NULL;
  }
  if (Assigned(actionflags)) { delete(actionflags); actionflags = NULL; }
  if (Assigned(actionplayers)) { delete(actionplayers); actionplayers = NULL; }
}

//--- functions needed for scripting ---

double abs(double val) {
  if (val<0) return -val; else return val;
}

double min(double a, double b) {
  if (a<b) return a; else return b;
}

int TruncX(double val) { 
  val += doubleprecision; 
  int res = (int)val; 
  if (val<0 && val!=res) 
    res--; 
  return res;
} 

JString floattostrX(double val) {
  if (abs(val)<doubleprecision)
    return "0";
  else
    return JString()<<val;
}

// Searchs for a character 'test', beginning at position 'start'
int indexOf(int start, char test, const JString& str) {
  int len = Length(str);
  if (start>len) return 0;
  for (int i=start; i<=len; i++) if (test==str[i]) return i;
  return 0;
}

bool startsWith(const JString& test, const JString& str) {
  int len = Length(test);
  if (Length(str)<len) return false;
  for (int i=1; i<=len; i++) if (test[i]!=str[i]) return false;
  return true;
}


// Searchs for the end of [] or () brackets
int findBracketsEnd(const JString& str) {
  char startb, endb;
  int i,j,bcount,curpos;
  if (Length(str)<1) return 0;

  startb = (char)str[1];
  endb = '}';
  if (startb=='(') endb = ')';
  if (startb=='[') endb = ']';

  curpos = 1;
  bcount = 1;
  while (bcount>0) {
    i = indexOf(curpos+1,startb,str);
    j = indexOf(curpos+1,endb,str);
    if (i>0 && (j==0 || i<j)) {
      bcount++;
      curpos = i;
    } else if (j>0) {
      bcount--;
      curpos = j;
    } else
      break;
  }
  if (bcount==0) return curpos;
  return 0;
}

// Searchs for the string 'code' in 'str', stuff inside
// of brackets like [] or () will not be considered 
int findCode(const JString& code, JString str) {
  int i,j,k,curpos;

  curpos = 1;
  while (true) {
    i = Pos(code,str);
    j = Pos('(',str);
    k = Pos('[',str);
    if (k>0 && (k<j || j==0)) j = k;
    if (j==0 || i<=j) {
      if (i>0) return curpos+i-1;
      return 0;
    }

    curpos += j-1;
    str = Copy(str,j,Length(str)-j+1);
    k = findBracketsEnd(str);
    if (k>0) {
      str = Copy(str,k+1,Length(str)-k);
      curpos += k;
    } else
      return 0;
  }
}

int findLastCode(const JString& code, JString str) {
  int i,j,k,curpos,res;

  res = 0;
  curpos = 1;
  i = Pos(code,str);
  j = Pos('(',str);
  k = Pos('[',str);
  if (k>0 && (k<j || j==0)) j = k;
  while (true) {
    if (i==0) return res;
    if (i<j || j==0) {
      res = curpos+i-1;
      str = Copy(str,i+Length(code),Length(str)-Length(code)+1);
      curpos += i+Length(code)-1;
      if (j>0) j = j-i-Length(code)+1;
      i = Pos(code,str);
    } else {
      curpos = curpos+j-1;
      str = Copy(str,j,Length(str)-j+1);
      k = findBracketsEnd(str);
      if (k>0) {
        str = Copy(str,k+1,Length(str)-k);
        curpos += k;
        i = Pos(code,str);
        j = Pos('(',str);
        k = Pos('[',str);
        if (k>0 && (k<j || j==0)) j = k;
      } else
        return res;
    }
  }
}

// Removes '/* */' , '//' -comments and '§' (lineends);
// add a ';' to all '}'
JString removeComments(JString str) {
  int i,j;
  JString str2;

  while (true) {
    i = Pos("/*",str);
    if (i==0) break;
    j = Pos("*/",Copy(str,i,Length(str)-i+1));
    if (j==0) j = i; else j = i-1+j;
    str = Copy(str,1,i-1)+Copy(str,j+2,Length(str)-j-1);
  }
  while (true) {
    i = Pos("//",str);
    if (i==0) break;
    j = Pos("§",Copy(str,i,Length(str)-i+1));
    if (j==0) j = Length(str)+1; else j += i-1;
    str = Copy(str,1,i-1)+Copy(str,j,Length(str)-j+1);
  }
  while (true) {
    i = Pos("§",str);
    if (i==0) break;
    str = Copy(str,1,i-1)+Copy(str,i+1,Length(str)-i);
  }
  str2 = str;
  str.clear();
  while (true) {
    i = Pos('}',str2);
    if (i==0) {
      str << str2;
      break;
    }
    str << Copy(str2,1,i) << ';';
    str2 = Copy(str2,i+1,Length(str2)-i);
  }
  return str;
}

// Expect a string like 'S(S)' as input format and
// returns a list with all S
TJStringList* GetStrings(JString str, JString format) {
  int i,curpos;
  TJStringList* list;
  bool addnext;

  if (Length(format)<=0) return NULL;
  str = Trim(str);

  list = new TJStringList();
  addnext = false;
  curpos = 1;
  while (Length(format)>0) {
    // Format==S means we must add a string to the
    // string position list next time we find a bracket
    // character; so lets remember that by setting 'addnext'
    if (format[1]=='S') addnext = true;
    else {
      // Search for a bracket like [ ] ( )
      i = findCode((char)format[1],str);
      if (i==0) {
        // We didn't found the bracket characters, so the
        // string is wrong formatted and we can't return a
        // correct list
        delete(list);
        return NULL;
      }
      if (addnext) {
        // Add the string before the bracket character to
        // the string list
        list->Add(Copy(str,1,i-1));
        addnext = false;
      } else if (Length(Trim(Copy(str,1,i-1)))>0) {
        // We didn't expect to find something between two bracket characters,
        // so return without a result
        delete(list);
        return NULL;
      }
      str = Copy(str,i+1,Length(str)-i);
      curpos++;
    }
    format = Copy(format,2,Length(format)-1);
  }
  // Add the last part of the string to the
  // string list
  if (addnext) list->Add(str);
  return list;
}

// Find brackets and return a list of the parts of 'str'
// like 'player[i].x' will be converted to ('player,'[','i',']','.x')
TJStringList* GetStringsAndFormat(JString str) {
  int i,j,k,curpos,curpos2;
  JString str2;
  TJStringList* list;
  str = Trim(str);

  list = new TJStringList();
  i = findCode(' ',str);
  if (i>0) return list;
  while (Length(str)>0) {
    i = Pos('(',str);
    j = Pos('[',str);
    if (i==0 || (j>0 && j<i)) i = j;
    // Found a bracket, so add the string before the brackets,
    // the left bracket, the string inside the brackets and
    // the right bracket to the list
    if (i>0) {
      if (i>1) list->Add(Copy(str,1,i-1));
      j = findBracketsEnd(Copy(str,i,Length(str)-i+1));
      if (j==0) return list;
      list->Add((char)str[i]);
      str2 = Copy(str,i+1,j-2);
      // Also search for commas in the inside string
      while (true) {
        k = findCode(',',str2);
        if (k==0) k = Length(str2)+1;
        list->Add(Copy(str2,1,k-1));
        if (k>Length(str2)) break;
        list->Add(',');
        str2 = Copy(str2,k+1,Length(str2)-k);
      }
      list->Add((char)str[i+j-1]);

      str = Copy(str,i+j,Length(str)-i-j+1);
    } else {
      // No brackets found, so add the rest
      list->Add(str);
      str = "";
    }
  }
  return list;
}


//--- TNPCVariable ---

TNPCVariable::TNPCVariable(JString name) {
  this->name = name;
  isarray = false;
  value = 0;
  arrsize = 0;
  val = NULL;
}

TNPCVariable::~TNPCVariable() {
  if (val) {
    free(val);
    val = NULL;
  }
  isarray = false;
  arrsize = 0;
}

void TNPCVariable::SetArraySize(int size) {
  if (size>=0 && size<=10000 && size!=arrsize) {
    if (size==0) {
      if (val) {
        free(val);
        val = NULL;
      }
      isarray = false;
    } else {
      val = (double*)realloc(val,size*sizeof(double));
      if (size>arrsize)
        memset((double*)(val+arrsize),0,(size-arrsize)*sizeof(double));
      isarray = true;
    }
    arrsize = size;
  }
}


//--- TNPCOperation ---

TNPCOperation::TNPCOperation(int id, double value) {
  this->id = id;
  this->value = value;
  op1 = NULL;
  op2 = NULL;
  arrlist = NULL;
  pvar = NULL;
}

TNPCOperation::~TNPCOperation() {
  if (Assigned(op1)) { delete(op1); op1 = NULL; }
  if (Assigned(op2)) { delete(op2); op2 = NULL; }
  if (Assigned(arrlist)) {
    for (int i=0; i<arrlist->count; i++)
      delete((TNPCOperation*)(*arrlist)[i]);
    delete(arrlist);
    arrlist = NULL;
  }
}


//--- TNPCBoolOperation ---

TNPCBoolOperation::TNPCBoolOperation() {
  bop1 = NULL;
  bop2 = NULL;
  op1 = NULL;
  op2 = NULL;
}

TNPCBoolOperation::~TNPCBoolOperation() {
  if (Assigned(op1)) { delete(op1); op1 = NULL; }
  if (Assigned(op2)) { delete(op2); op2 = NULL; }
  if (Assigned(bop1)) { delete(bop1); bop1 = NULL; }
  if (Assigned(bop2)) { delete(bop2); bop2 = NULL; }
}


//--- TNPCCommand ---

TNPCCommand::TNPCCommand() {
  op1 = NULL;
  op2 = NULL;
  op3 = NULL;
  op4 = NULL;
  op5 = NULL;
  op6 = NULL;
  bop = NULL;
}

TNPCCommand::~TNPCCommand() {
  if (Assigned(op1)) { delete(op1); op1 = NULL; }
  if (Assigned(op2)) { delete(op2); op2 = NULL; }
  if (Assigned(op3)) { delete(op3); op3 = NULL; }
  if (Assigned(op4)) { delete(op4); op4 = NULL; }
  if (Assigned(op5)) { delete(op5); op5 = NULL; }
  if (Assigned(op6)) { delete(op6); op6 = NULL; }
  if (Assigned(bop) && id!=203) { delete(bop); bop = NULL; }
}


//--- Scripting consts ---

char* colornames[] = {
  "white","yellow","orange","pink","red","darkred","lightgreen",
  "green","darkgreen","lightblue","blue","darkblue","brown",
  "cynober","purple","darkpurple","lightgray","gray","black",
  "transparent"
};
int colornamescount = 20;

int getColorIndex(JString str) {
  str = Trim(str);
  for (int i=0; i<colornamescount; i++)
    if (str==colornames[i]) return i;
  return -1;
}

#define ARGCOMMAND   0
#define IFCOMMAND    1
#define ELSECOMMAND  2
#define FORCOMMAND   3
#define WHILECOMMAND 4

typedef struct comdescription {
  char* name;
  int type;
  char* args;
} comdescription;

comdescription commands[] = {
#ifdef NEWWORLD
  {"say",ARGCOMMAND,"S"},
#else
  {"say",ARGCOMMAND,"R"},
#endif
  {"lay ",ARGCOMMAND,"I"},
  {"take ",ARGCOMMAND,"I"},
  {"hurt ",ARGCOMMAND,"R"},
  {"set ",ARGCOMMAND,"M"},
  {"if",IFCOMMAND,""},
  {"play ",ARGCOMMAND,"F"},
  {"unset ",ARGCOMMAND,"M"},
  {"setgif ",ARGCOMMAND,"F"},
  {"removecompus",ARGCOMMAND,""},
  {"setlevel ",ARGCOMMAND,"M"},
  {"setskincolor",ARGCOMMAND,"C"},
  {"setcoatcolor",ARGCOMMAND,"C"},
  {"setsleevecolor",ARGCOMMAND,"C"},
  {"setshoecolor",ARGCOMMAND,"C"},
  {"setbeltcolor",ARGCOMMAND,"C"},
  {"setplayerdir",ARGCOMMAND,"D"},
  {"putcomp",ARGCOMMAND,"BRR"},
  {"takeplayercarry",ARGCOMMAND,""},
  {"dontblocklocal",ARGCOMMAND,""},
  {"drawoverplayer",ARGCOMMAND,""},
  {"enableweapons",ARGCOMMAND,""},
  {"disableweapons",ARGCOMMAND,""},
  {"shootarrow",ARGCOMMAND,"D"},
  {"shootfireball",ARGCOMMAND,"D"},
  {"shootfireblast",ARGCOMMAND,"D"},
  {"shootnuke",ARGCOMMAND,"D"},
  {"putbomb",ARGCOMMAND,"RRR"},
  {"putexplosion",ARGCOMMAND,"RRR"},
  {"putleaps",ARGCOMMAND,"RRR"},
  {"shootball",ARGCOMMAND,""},
  {"timershow",ARGCOMMAND,""},
  {"toinventory",ARGCOMMAND,"S"},
  {"followplayer",ARGCOMMAND,""},
  {"else",ELSECOMMAND,""},
  {"openurl",ARGCOMMAND,"M"},
  {"seturllevel",ARGCOMMAND,"M"},
  {"stopmidi",ARGCOMMAND,""},
  {"blockagainlocal",ARGCOMMAND,""},
  {"drawunderplayer",ARGCOMMAND,""},
  {"puthorse",ARGCOMMAND,"MRR"},
  {"canbecarried",ARGCOMMAND,""},
  {"cannotbecarried",ARGCOMMAND,""},
  {"noplayerkilling",ARGCOMMAND,""},
  {"canbepushed",ARGCOMMAND,""},
  {"cannotbepushed",ARGCOMMAND,""},
  {"canbepulled",ARGCOMMAND,""},
  {"cannotbepulled",ARGCOMMAND,""},
  {"message",ARGCOMMAND,"S"},
  {"setsword",ARGCOMMAND,"MR"},
  {"setshield",ARGCOMMAND,"MR"},
  {"putnewcomp",ARGCOMMAND,"BRRMR"},
  {"putnpc",ARGCOMMAND,"FFRR"},
  {"destroy",ARGCOMMAND,""},
  {"freezeplayer",ARGCOMMAND,"R"},
  {"sethead",ARGCOMMAND,"F"},
  {"toweapons",ARGCOMMAND,"M"},
  {"showimg",ARGCOMMAND,"RFRR"},
  {"hideimg",ARGCOMMAND,"R"},
  {"hidesword",ARGCOMMAND,"R"},
  {"for",FORCOMMAND,""},
  {"hitcompu",ARGCOMMAND,"RRRR"},
  {"hitplayer",ARGCOMMAND,"RRRR"},
  {"setbow",ARGCOMMAND,"S"},
  {"updateboard",ARGCOMMAND,"RRRR"},
  {"putobject",ARGCOMMAND,"ORR"},
  {"setarray",ARGCOMMAND,"RR"},
  {"sleep",ARGCOMMAND,"R"},
  {"while",WHILECOMMAND,""},
  {"hideplayer",ARGCOMMAND,"R"},
  {"timereverywhere",ARGCOMMAND,""},
  {"setminimap",ARGCOMMAND,"FFRR"},
  {"setmap",ARGCOMMAND,"FFRR"},
  {"showcharacter",ARGCOMMAND,""},
  {"setplayerprop",ARGCOMMAND,"MM"},
  {"setcharprop",ARGCOMMAND,"MM"},
  {"showfile",ARGCOMMAND,"F"},
  {"setstring",ARGCOMMAND,"MM"},
  {"setbackpal",ARGCOMMAND,"F"},
  {"setletters",ARGCOMMAND,"F"},
  {"setgifpart",ARGCOMMAND,"FRRRR"},
  {"setfocus",ARGCOMMAND,"RR"},
  {"resetfocus",ARGCOMMAND,""},
  {"setbody",ARGCOMMAND,"F"},
  {"carryobject",ARGCOMMAND,"L"},
  {"throwcarry",ARGCOMMAND,""},
  {"showlocal",ARGCOMMAND,""},
  {"hidelocal",ARGCOMMAND,""},
  {"dontblock",ARGCOMMAND,""},
  {"blockagain",ARGCOMMAND,""},
  {"setbacktile",ARGCOMMAND,"R"},
  {"takeplayerhorse",ARGCOMMAND,""},
  {"removebomb",ARGCOMMAND,"R"},
  {"removearrow",ARGCOMMAND,"R"},
  {"removeitem",ARGCOMMAND,"R"},
  {"removeexplo",ARGCOMMAND,"R"},
  {"removehorse",ARGCOMMAND,"R"},
  {"explodebomb",ARGCOMMAND,"R"},
  {"reflectarrow",ARGCOMMAND,"R"},
  {"lay2",ARGCOMMAND,"IRR"},
  {"take2",ARGCOMMAND,"R"},
  {"takehorse",ARGCOMMAND,"R"},
  {"playlooped",ARGCOMMAND,"F"},
  {"stopsound",ARGCOMMAND,"F"},
  {"changeimgpart",ARGCOMMAND,"RRRRR"},
  {"setlevel2",ARGCOMMAND,"MRR"},
  {"hitnpc",ARGCOMMAND,"RRRR"},
  {"canwarp",ARGCOMMAND,""},
  {"setani",ARGCOMMAND,"SS"},
  {"setcharani",ARGCOMMAND,"SS"},
  {"show",ARGCOMMAND,""},
  {"hide",ARGCOMMAND,""},
};
int commandscount = 112;

char* dirnames[] = {"up","left","down","right"};

char* carrynames[] = {"bush","sign","vase","stone","blackstone",
  "bomb","hotbomb","superbomb","joltbomb","hotjoltbomb","none"};
int carrynamescount = 11;

char opcodes[] = {'*','/','%','^'};

#ifdef NEWWORLD
char* timevals[] = {"nwday","nwweek","nwyear","nwweekday","nwmonth",
  "nwtime","nwhour","nwmin"};
int timevalscount = 8;
#endif

char* countvars[] = {"compuscount","playerscount","bombscount",
  "arrowscount","itemscount","exploscount","horsescount","npcscount"};
int countvarscount = 8;

char* objcompus[] = {"compus", "x","y","type","dir","headdir","power","mode",
             "","","","","","","","","","","",""};
char* objbombs[] = {"bombs",  "x","y","power","time",
             "","","","","","","","","","","","","","",""};
char* objarrows[] = {"arrows", "x","y","dir","dx","dy","type","from",
             "","","","","","","","","","","",""};
char* objitems[] = {"items",  "x","y","type","time",
             "","","","","","","","","","","","","","",""};
char* objexplos[] = {"explos", "x","y","power","time","dir",
             "","","","","","","","","","","","","",""};
char* objplayers[] = {"players","x","y","rupees","darts","bombs","hearts","fullhearts",
             "dir","glovepower","bombpower","shootpower","swordpower",
             "shieldpower","headset","sprite","id","saysnumber","mp",
             "ap"};
char* objhorses[] = {"horses","x","y","dir","bushes","bombs","bombpower","type",
             "","","","","","","","","","","",""};
char* objnpcs[] = {"npcs","x","y","width","height","rupees","darts","bombs","hearts","timeout",
             "glovepower","swordpower","shieldpower",
             "hurtdx","hurtdy","id","save[","dir","sprite","ap"};

char** objectvars[] = {
  objcompus,
  objbombs,
  objarrows,
  objitems,
  objexplos,
  objplayers,
  objhorses,
  objnpcs,
};
int objectvarscount = 8;
int objectvarc[] = {7,4,7,4,5,19,7,19};
int objectvarid[] = {600,640,660,680,700,800,720,740};
#define OVCOMPUS  0
#define OVBOMBS   1
#define OVARROWS  2
#define OVITEMS   3
#define OVEXPLOS  4
#define OVPLAYERS 5
#define OVHORSES  6
#define OVNPCS    7

char* mathfuncs[] = {"sin","cos","arctan","int","abs","arraylen"};
int mathfuncscount = 6;

char* param2funcs[] = {"random","testcompu","testplayer","testbomb",
  "testitem","testexplo","testhorse","testnpc"};
int param2funcscount = 8;
int param2funcids[] = {400,420,421,422,423,424,425,426};

char* cmpstrings[] = {"==","!=","<=","=<",">=","=>",
  "<>","<",">","="};
int cmpstringscount = 10;
char* saystrings[] = {"playersays","playersays2","strequals",
  "strcontains"};
int saystringscount = 4;
char* xyteststrings[] = {"onwall","onwater"};
int xyteststringscount = 2;
int xytestids[] = {400,406};

char* assignstrings[] = {"+=","-=","*=","++","--",":=","="};
int assignstringscount = 7;

int spritenumarr[] = {0, 4,5,4,3,2,1,2,3, 6,7,8,9,10,
  11,12,13,11,12, 14,15,16,17, 23, 18, 19,20,19,18,21,22,21,18, 24, 25,26,27,
  28, 29, 32};
int spritenumarrcount = 40;



//--- Script parsing ---

void TServerSideNPC::parseString(JString str) {
  int i,j,k;

  lastcommandwasif = false;
  // Search for ';' and convert the string before it to
  // a script command
  while (!endparsing) {
    i = findCode(';',str);
    if (i==0) {
      parseCommand(str);
      break;
    }
    // find the end of commands like 'if'
    // which can contain a { } block
    j = findCode('{',str);
    if (j>0 && j<i) {
      k = findBracketsEnd(Copy(str,j,Length(str)-j+1));
      if (k>0)
        i = j+k-1+findCode(';',Copy(str,j+k,Length(str)-j-k+1));
    }
    parseCommand(Copy(str,1,i-1));
    str = Copy(str,i+1,Length(str)-i);
  }
}

// Parses a single command
void TServerSideNPC::parseCommand(JString str) {
  int i,j,command,argstart,opnum,argnum;
  JString arg,args,curarg;
  bool added,formaterror;
  TNPCCommand *com,*com2;
  TNPCOperation* val;
  TJStringList* list;
//  TEditorObject* obj;

  str = Trim(str);
  if (Length(str)==0) return;
  if (str[1]=='{' && str[Length(str)]=='}') {
    parseString(Copy(str,2,Length(str)-2));
    return;
  }
  if (str[1]=='{') return;

  command = -1;
  for (i=0; i<commandscount; i++)
    if (startsWith(commands[i].name,str)) { command = i; break; }

  if (command>=0) {
    j = strlen(commands[command].name);
    arg = Trim(Copy(str,j+1,Length(str)-j));

    added = false;
    com = new TNPCCommand();
    com->id = command;

    switch (commands[command].type) {
      case ARGCOMMAND: {
          args = commands[command].args;
          if (Length(args)==0) {
            // Expect no arguments, just add the command to the list
            if (Length(arg)<=0) {
              commandlist->Add(com);
              added = true;
            }
          } else {
            opnum = 1;
            argnum = 1;
            while (Length(args)>0) {
              // Get the next paramter in arg and store it in curarg
              formaterror = false;
              i = findCode(',',arg);
              if (Length(args)>1) {
                if (i==0) formaterror = true;
                else {
                  curarg = Trim(Copy(arg,1,i-1));
                  arg = Trim(Copy(arg,i+1,Length(arg)-i));
                }
              } else {
                if (i>0 && args[1]!='S') formaterror = true;
                else curarg = arg;
              }
              if (formaterror) {
                if (Assigned(com)) delete(com);
                return;
              }
              switch (args[1]) {
                case 'R': {
                    val = parseValue(curarg);
                    if (opnum==1) com->op1 = val;
                    else if (opnum==2) com->op2 = val;
                    else if (opnum==3) com->op3 = val;
                    else if (opnum==4) com->op4 = val;
                    else if (opnum==5) com->op5 = val;
                    else com->op6 = val;
                    opnum++;
                    break;
                  }
                case 'S':
                case 'M': {
                    if (argnum==1) com->arg1 = curarg;  else com->arg2 = curarg;
                    argnum++;
                    break;
                  }
                case 'F': {
                    if (Pos('#',curarg)>0 || LevelFileExists(curarg)) {
                    if (argnum==1) com->arg1 = curarg;  else com->arg2 = curarg;
                      argnum++;
                    }
                    break;
                  }
                case 'I': {
                    com->val1 = itemnames->indexOf(curarg);
                    break;
                  }
                case 'C': {
                    com->val1 = getColorIndex(curarg);
                    break;
                  }
                case 'D': {
                    j = -1;
                    for (i=0; i<4; i++) if (curarg==dirnames[i]) { j = i;  break; }
                    if (j>=0) val = new TNPCOperation(10,j);
                    else val = parseValue(curarg);

                    if (opnum==1) com->op1 = val;
                    else if (opnum==2) com->op2 = val;
                    else if (opnum==3) com->op3 = val;
                    else if (opnum==4) com->op4 = val;
                    else if (opnum==5) com->op5 = val;
                    else com->op6 = val;
                    opnum++;
                    break;
                  }
                case 'B': {
                    com->val1 = baddynameslower->indexOf(curarg);
                    break;
                  }
                case 'O': {
                    // search for curarg in the editors list of
                    // predefined objects that you can put onto the level
                    // the list doesn't exist yet, so just assign -1
                    com->val1 = -1;
                    break;
                  }
                case 'L': {
                    j = -1;
                    for (i=0; i<carrynamescount; i++) if (curarg==carrynames[i]) { j = i; break; }
                    com->val1 = j;
                    break;
                  }
              }
              args = Copy(args,2,Length(args)-1);
            }
            commandlist->Add(com);
            added = true;
          }
          break;
        }
      case IFCOMMAND: {
          list = GetStrings(arg,"(S)S");
          if (Assigned(list)) {
            com->bop = parseBoolValue((*list)[0]);
            if (Assigned(com->bop)) {
              commandlist->Add(com);
              added = true;

              i = commandlist->count;
              parseString((*list)[1]+";");
              com->jumppos = commandlist->count;

              if (!(commandlist->count<=i+1)) {
                com2 = new TNPCCommand();
                com2->id = 203;
                com2->bop = com->bop;
                commandlist->Add(com2);
              }
            }
            delete(list);
          }
          break;
        }
      case ELSECOMMAND: {
          if (lastcommandwasif && Length(arg)>0) {
            commandlist->Add(com);
            added = true;

            i = commandlist->count;
            parseString(arg+";");

            if (commandlist->count>=i+1) {
              com2 = (TNPCCommand*)(*commandlist)[i];

              if (com2->id==5 && com2->jumppos>=commandlist->count-1) {
                com->id = 204;
                com->bop = com2->bop;
                commandlist->Delete(i);
                com2->bop = NULL;
                delete(com2);
                for (j=i; j<commandlist->count; j++) {
                  com2 = (TNPCCommand*)(*commandlist)[j];
                  com2->jumppos--;
                }
              } else if (!(commandlist->count<=i+1)) {
                com2 = new TNPCCommand();
                com2->id = 201;
                commandlist->Add(com2);
              }
            }
            com->jumppos = commandlist->count;
          }
          break;
        }
      case FORCOMMAND: {
          list = GetStrings(arg,"(S;S;S)S");
          if (Assigned(list)) {
            com->bop = parseBoolValue((*list)[1]);
            if (Assigned(com->bop)) {
              parseString((*list)[0]+";");
              com->op1 = new TNPCOperation(10,0);
              i = commandlist->count;
              commandlist->Add(com);
              added = true;

              parseString((*list)[3]+";");
              parseString((*list)[2]+";");

              com2 = new TNPCCommand();
              com2->id = 202;
              com2->jumppos = i;
              commandlist->Add(com2);

              com->jumppos = commandlist->count;
            }
            delete(list);
          }
          break;
        }
      case WHILECOMMAND: {
          list = GetStrings(arg,"(S)S");
          if (Assigned(list)) {
            com->bop = parseBoolValue((*list)[0]);
            if (Assigned(com->bop)) {
              com->op1 = new TNPCOperation(10,0);
              i = commandlist->count;
              commandlist->Add(com);
              added = true;

              parseString((*list)[1]+";");

              com2 = new TNPCCommand();
              com2->id = 202;
              com2->jumppos = i;
              commandlist->Add(com2);

              com->jumppos = commandlist->count;
            }
            delete(list);
          }
          break;
        }
    }
    if (Assigned(com) && !added) delete(com);
    if (commands[command].type!=ELSECOMMAND)
      lastcommandwasif = (commands[command].type==IFCOMMAND);
  } else {
    lastcommandwasif = false;
    parseAssignment(str);
  }
}

void TServerSideNPC::parseAssignment(JString str) {
  int i,j,varlen;
  JString str1,str2;
  TNPCCommand* com;
  str = Trim(str);
  
  for (i=0; i<assignstringscount; i++) {
    j = Pos(assignstrings[i],str);
    if (j>0) {
      varlen = strlen(cmpstrings[i]);
      str1 = Copy(str,1,j-1);
      str2 = Copy(str,j+varlen,Length(str)+1-(j+varlen));

      com = new TNPCCommand();
      com->id = 200;
      com->op1 = parseValue(str1);
      switch (i) {
        case 0: { str2 = str1+"+("+str2+")"; break; }
        case 1: { str2 = str1+"-("+str2+")"; break; }
        case 2: { str2 = str1+"*("+str2+")"; break; }
        case 3: { str2 = str1+"+1"; break; }
        case 4: { str2 = str1+"-1"; break; }
//        case 5: case 6: dont change str2
      }
      com->op2 = parseValue(str2);
      if (Assigned(com->op1) && Assigned(com->op2))
        commandlist->Add(com);
      else
        delete(com);
      return;
    }
  }
}

void TServerSideNPC::optimizeOperation(TNPCOperation* op) {
  if (Assigned(op) && op->id<10 &&
      Assigned(op->op1) && op->op1->id==10 &&
      Assigned(op->op2) && op->op2->id==10) {
    op->value = calcOperation(op);
    op->id = 10;
    delete(op->op1); op->op1 = NULL;
    delete(op->op2); op->op2 = NULL;
  }
}

bool isnumber(const JString& str) {
  for (int i=1; i<Length(str); i++) {
    char c = (char)str[i];
    if ((c<'0' || c>'9') && c!='.' && c!='-')
      return false;
  }
  return true;
}

TNPCOperation* TServerSideNPC::parseValue(JString str) {
  TNPCOperation *op,*op2;
  int i,j,k;
  JString str1,str2,str3,str4,str5,str6,str7,str8,teststr;
  TNPCVariable* pvar;
  bool onlysign;
  TJStringList* list;
  str = Trim(str);

  op = new TNPCOperation(10,0);
  if (Length(str)<=0) return op;
  if (str[Length(str)]==';') str = Copy(str,1,Length(str)-1);
  if (str[1]=='{' && str[Length(str)]=='}') {
    str = Copy(str,2,Length(str)-2);
    op->id = 22;
    op->arrlist = new TList();
    while (true) {
      i = findCode(',',str);
      if (i==0) i = Length(str)+1;
      op2 = parseValue(Copy(str,1,i-1));
      if (Assigned(op2))
        op->arrlist->Add(op2);
      if (i>Length(str))
        break;
      str = Copy(str,i+1,Length(str)-i);
    }
    return op;
  }
  while (str[1]=='(' && str[Length(str)]==')' && findBracketsEnd(str)==Length(str))
    str = Trim(Copy(str,2,Length(str)-2));

  i = findLastCode('+',str);
  j = findLastCode('-',str);
  if (i>0 || j>0) {
    if (i>j) op->id = 0; else { op->id = 1; i = j; }
    str1 = Copy(str,1,i-1);

    onlysign = false;
    teststr = Trim(str1);
    if (Length(teststr)>0)
        for (k=0; k<4; k++) if (teststr[Length(teststr)]==opcodes[k]) {
      onlysign = true;
      break;
    }
    if (!onlysign) {
      str2 = Copy(str,i+1,Length(str)-i);
      op->op1 = parseValue(str1);
      op->op2 = parseValue(str2);
      if (Assigned(op->op1) && Assigned(op->op2)) {
        optimizeOperation(op);
        return op;
      } else {
        op->id = 10;
        if (Assigned(op->op1)) { delete(op->op1); op->op1 = NULL; }
        if (Assigned(op->op2)) { delete(op->op2); op->op2 = NULL; }
        return op;
      }
    }
  }
  for (j=0; j<4; j++) {
    i = findCode(opcodes[j],str);
    if (i>0) {
      str1 = Copy(str,1,i-1);
      str2 = Copy(str,i+1,Length(str)-i);
      if (Length(str1)>0 && Length(str2)>0) {
        op->op1 = parseValue(str1);
        op->op2 = parseValue(str2);
        if (Assigned(op->op1) && Assigned(op->op2)) {
          op->id = 2+j;
          optimizeOperation(op);
          return op;
        } else {
          if (Assigned(op->op1)) { delete(op->op1); op->op1 = NULL; }
          if (Assigned(op->op2)) { delete(op->op2); op->op2 = NULL; }
          return op;
        }
      }
    }
  }

  list = GetStringsAndFormat(str);
  if (list->count>=1) str1 = (*list)[0];
  if (list->count>=2) str2 = (*list)[1];
  if (list->count>=3) str3 = (*list)[2];
  if (list->count>=4) str4 = (*list)[3];
  if (list->count>=5) str5 = (*list)[4];
  if (list->count>=6) str6 = (*list)[5];
  if (list->count>=7) str7 = (*list)[6];
  if (list->count>=8) str8 = (*list)[7];
  int listc = list->count;
  delete(list);
  if (listc==0) return op;

  switch (listc) {
    case 1: { // time vars, count vars, current player attributes, current npc attributes
#ifdef NEWWORLD
        for (i=0; i<timevalscount; i++) if (str1==timevals[i]) {
          op->id = 102+i;
          return op;
        }
#endif
        for (i=0; i<countvarscount; i++) if (str1==countvars[i]) {
          op->id = 200+i;
          return op;
        }
        for (i=1; i<=objectvarc[OVPLAYERS]; i++) if (str1==JString("player")+objplayers[i]) {
          op->op1 = new TNPCOperation(10,0);
          op->id = objectvarid[OVPLAYERS]+i-1;
          return op;
        }
        for (i=1; i<=objectvarc[OVNPCS]; i++) if (str1==objnpcs[i]) {
          op->op1 = new TNPCOperation(10,-1);
          op->id = objectvarid[OVNPCS]+i-1;
          return op;
        }
        break;
      }
    case 4: { // board[a], attributename[a], normal arrays, strtofloat(a), math functions like sin(a)
        if  (str2=='[' && str4==']') {
          if (str1=="board") {
            op->op1 = parseValue(str3);
            if (Assigned(op->op1)) op->id = 100;
            return op;
          }
          for (i=1; i<=objectvarc[OVNPCS]; i++) if (str1+"["==objnpcs[i]) {
            op->op2 = parseValue(str3);
            if (Assigned(op->op2)) {
              op->op1 = new TNPCOperation(10,-1);
              op->id = objectvarid[OVNPCS]+i-1;
            }
            return op;
          }
          op->op1 = parseValue(str3);
          str = str1;
          if (!Assigned(op->op1)) return op;

        } else if (str2=='(' && str4==')') {
          if (str1=="strtofloat") {
            op->id = 101;
            op->strvalue = Trim(str3);
            return op;
          }
          for (i=0; i<mathfuncscount; i++) if (str1==mathfuncs[i]) {
            op->op1 = parseValue(str3);
            if (Assigned(op->op1)) op->id = 401+i;
            return op;
          }
        }
        break;
      }
    case 5: { // objectname[a].attributename vars
        if (str2=='[' && str4==']' && Pos('.',str5)==1)
            for (j=0; j<objectvarscount; j++) if (str1==objectvars[j][0]) {
          for (i=1; i<=objectvarc[j]; i++) if (str5==JString('.')+objectvars[j][i]) {
            op->op1 = parseValue(str3);
            if (Assigned(op->op1)) {
              op->id = objectvarid[j]+i-1;
              break;
            }
          }
          return op;
        }
        break;
      }
    case 6: { // random(a,b), testplayer(x,y) etc.
        if (str2=='(' && str4==',' && str6==')')
            for (i=0; i<param2funcscount; i++) if (str1==param2funcs[i]) {
          op->op1 = parseValue(str3);
          op->op2 = parseValue(str5);
          if (Assigned(op->op1) && Assigned(op->op2)) op->id = param2funcids[i];
          else {
            if (Assigned(op->op1)) { delete(op->op1); op->op1 = NULL; }
            if (Assigned(op->op2)) { delete(op->op2); op->op2 = NULL; }
          }
          return op;
        }
      }
    case 8: { // objectname[a].attributename[b] vars
        if (str2=='[' && str4==']' && Pos('.',str5)==1 && str6=='[' && str8==']')
            for (j=0; j<objectvarscount; j++) if (str1==objectvars[j][0]) {
          for (i=1; i<=objectvarc[j]; i++) if (str5+'['==JString('.')+objectvars[j][i]) {
            op->op1 = parseValue(str3);
            op->op2 = parseValue(str7);
            if (Assigned(op->op1) && Assigned(op->op2)) op->id = objectvarid[j]+i-1;
            else {
              if (Assigned(op->op1)) { delete(op->op1); op->op1 = NULL; }
              if (Assigned(op->op2)) { delete(op->op2); op->op2 = NULL; }
            }
          }
          return op;
        }
        break;
      }
  }

  if (startsWith("this.",str)) {
    str = Copy(str,6,Length(str)-5);
    if (Length(str)>0) {
      op->id = 21;
      for (i=0; i<actionvars->count; i++) {
        pvar = (TNPCVariable*)(*actionvars)[i];
        if (Assigned(pvar) && pvar->name==str) {
          op->pvar = pvar;
          return op;
        }
      }
      pvar = new TNPCVariable(str);
      actionvars->Add(pvar);
      op->pvar = pvar;
    }
    return op;
  }

  if (isnumber(str)) {
    op->value = strtofloat(str);
    return op;
  }

  if (Assigned(level)) {
    op->id = 20;
    for (i=0; i<level->actionvars->count; i++) {
      pvar = (TNPCVariable*)(*level->actionvars)[i];
      if (pvar->name==str) {
        op->pvar = pvar;
        return op;
      }
    }
    pvar = new TNPCVariable(str);
    level->actionvars->Add(pvar);
    op->pvar = pvar;
  }
  return op;
}

TNPCBoolOperation* TServerSideNPC::parseBoolValue(JString str) {
  int i,j,varlen;
  JString arg,str1,str2,str3,str4,str5,str6;
  TNPCBoolOperation *bop,*bop2;
  TJStringList* list;
  str = Trim(str);

  if (Length(str)==0) return NULL;
  bop = new TNPCBoolOperation();

  while (str[1]=='(' && str[Length(str)]==')' && findBracketsEnd(str)==Length(str))
    str = Copy(str,2,Length(str)-2);

  if (str=="true") {
    bop->id = 10;
    bop->value = true;
    return bop;
  }
  if (str=="false") {
    bop->id = 10;
    bop->value = false;
    return bop;
  }

  i = findCode("||",str);
  if (i>0) {
    bop->id = 1;
    str1 = Copy(str,1,i-1);
    str2 = Copy(str,i+2,Length(str)-i-1);
    bop->bop1 = parseBoolValue(str1);
    bop->bop2 = parseBoolValue(str2);
    if (Assigned(bop->bop1) && Assigned(bop->bop2)) {
      if ((bop->bop1->id==10 && bop->bop1->value==true) ||
          (bop->bop2->id==10 && bop->bop2->value==true) ||
          (bop->bop1->id==10 && bop->bop2->id==10)) {
        bop->id = 10;
        bop->value = (bop->bop1->value || bop->bop2->value);
        delete(bop->bop1); bop->bop1 = NULL;
        delete(bop->bop2); bop->bop2 = NULL;
      }
      return bop;
    } else if (Assigned(bop->bop1)) {
      bop2 = bop->bop1;
      bop->bop1 = NULL;
      delete(bop);
      return bop2;
    } else if (Assigned(bop->bop2)) {
      bop2 = bop->bop2;
      bop->bop2 = NULL;
      delete(bop);
      return bop2;
    } else {
      bop->id = 10;
      bop->value = false;
      return bop;
    }
  }
  i = findCode("&&",str);
  if (i>0) {
    bop->id = 2;
    str1 = Copy(str,1,i-1);
    str2 = Copy(str,i+2,Length(str)-i-1);
    bop->bop1 = parseBoolValue(str1);
    bop->bop2 = parseBoolValue(str2);
    if (!Assigned(bop->bop1) || !Assigned(bop->bop2)) {
      bop->id = 10;
      bop->value = false;
      if (Assigned(bop->bop1)) { delete(bop->bop1); bop->bop1 = NULL; }
      if (Assigned(bop->bop2)) { delete(bop->bop2); bop->bop1 = NULL; }
    } else {
      if ((bop->bop1->id==10 && bop->bop1->value==false) ||
          (bop->bop2->id==10 && bop->bop2->value==false) ||
          (bop->bop1->id==10 && bop->bop2->id==10)) {
        bop->id = 10;
        bop->value = (bop->bop1->value && bop->bop2->value);
        delete(bop->bop1); bop->bop1 = NULL;
        delete(bop->bop2); bop->bop2 = NULL;
      }
    }
    return bop;
  }
  i = findCode(" in ",str);
  if (i>0) {
    str1 = Copy(str,1,i-1);
    str2 = Copy(str,i+4,Length(str)-i-3);
    bop->op1 = parseValue(str1);
    bop->op2 = parseValue(str2);
    if (Assigned(bop->op1) && Assigned(bop->op2)) bop->id = 3;
    else {
      if (Assigned(bop->bop1)) { delete(bop->bop1); bop->bop1 = NULL; }
      if (Assigned(bop->bop2)) { delete(bop->bop2); bop->bop1 = NULL; }
    }
    return bop;
  }

  if (startsWith('!',str)) {
    bop->id = 0;
    bop->bop1 = parseBoolValue(Copy(str,2,Length(str)-1));
    if (!Assigned(bop->bop1)) {
      bop->id = 10;
      bop->value = false;
    } else {
      if (bop->bop1->id==10) {
        bop->id = 10;
        bop->value = !bop->bop1->value;
        delete(bop->bop1); bop->bop1 = NULL;
      }
    }
    return bop;
  }

  if (startsWith("playeringuild",str)) {
    bop->id = 407;
    bop->flagstring = Trim(Copy(str,14,Length(str)-13));
    return bop;
  }

  for (i=0; i<cmpstringscount; i++) {
    j = findCode(cmpstrings[i],str);
    if (j>0) {
      varlen = strlen(cmpstrings[i]);
      str1 = Copy(str,1,j-1);
      str2 = Copy(str,j+varlen,Length(str)+1-(j+varlen));
      bop->op1 = parseValue(str1);
      bop->op2 = parseValue(str2);
      if (Assigned(bop->op1) && Assigned(bop->op2)) {
        bop->id = 200+i;
        if (bop->op1->id==10 && bop->op2->id==10) {
          bop->value = calcBoolOperation(bop);
          bop->id = 10;
          delete(bop->op1); bop->op1 = NULL;
          delete(bop->op2); bop->op2 = NULL;
        }
      } else {
        if (Assigned(bop->bop1)) { delete(bop->bop1); bop->bop1 = NULL; }
        if (Assigned(bop->bop2)) { delete(bop->bop2); bop->bop1 = NULL; }
      }
      return bop;
    }
  }

  list = GetStringsAndFormat(str);
  if (list->count>=1) str1 = (*list)[0];
  if (list->count>=2) str2 = (*list)[1];
  if (list->count>=3) str3 = (*list)[2];
  if (list->count>=4) str4 = (*list)[3];
  if (list->count>=5) str5 = (*list)[4];
  if (list->count>=6) str6 = (*list)[5];
  int listc = list->count;
  delete(list);
  if (listc==0) return bop; 

  if (listc==4 && str1=="hasweapon" && str2=='(' && str4==')') {
    bop->id = 405;
    bop->flagstring = Trim(str3);
    return bop;
  }
  if (listc==6 && str2=='(' && str4==',' && str6==')')
      for (j=0; j<xyteststringscount; j++) if (str1==xyteststrings[j]) {
    bop->op1 = parseValue(str3);
    bop->op2 = parseValue(str5);
    if (Assigned(bop->op1) && Assigned(bop->op2)) bop->id = xytestids[j];
    else {
      if (Assigned(bop->bop1)) { delete(bop->bop1); bop->bop1 = NULL; }
      if (Assigned(bop->bop2)) { delete(bop->bop2); bop->bop1 = NULL; }
    }
    return bop;
  }

  for (j=0; j<saystringscount; j++) if (str1==saystrings[j]) {
    if (j==2 || j==3) {
      if (listc==6 && str2=='(' && str4==',' && str6==')') {
        bop->id = 401+j;
        bop->flagstring = str3;
        bop->flagstring2 = str5;
      }
    } else {
      if ((listc==4 && str2=='(' && str4==')') ||
          (listc==6 && str2=='(' && str4==',' && str6==')')) {
        if (listc==6) {
          bop->op1 = parseValue(str3);
          bop->flagstring = LowerCase(Trim(str5));
        } else {
          bop->op1 = new TNPCOperation(10,0);
          bop->flagstring = LowerCase(Trim(str3));
        }
        if (Assigned(bop->op1)) bop->id = 401+j;
      }
    }
    return bop;
  }

  bop->id = 20;
  bop->flagstring = str;
  return bop;
}

void TServerSideNPC::calcAssignment(TNPCCommand* com) {
  int i,j,opid;
  double value;
  int ivalue;
  TNPCOperation *op,*op2;
  TServerBaddy* compu;
//  TAdventureBomb* bomb;
//  TAdventureFlying* arrow;
  TServerExtra* item;
//  TAdventureExplosion* explo;
  TServerHorse* horse;
  TServerNPC* npc;
  TServerPlayer* player;

  if (!Assigned(com->op1) || !Assigned(com->op2)) return;
  op = com->op1;
  op2 = com->op2;
  value = calcOperation(op2);
  ivalue = TruncX(value);
  opid = op->id;

  compu = NULL;
//  bomb = NULL;
//  arrow = NULL;
  item = NULL;
//  explo = NULL;
  horse = NULL;
  npc = NULL;
  player = NULL;
  opid = op->id;
  if (opid>=600 && opid<640) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->baddies->count)
      compu = (TServerBaddy*)(*level->baddies)[i];
    else
      return;
  }
  if (opid>=640 && opid<660) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && (i<level->bombs->count)
//      bomb = (*level->bombs)[i];
//    else
      return;
  }
  if (opid>=660 && opid<680) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && i<level->flyingobjs->count)
//      arrow = (*level->flyingobjs)[i];
//    else
      return;
  }
  if (opid>=680 && opid<700) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->extras->count)
      item = (TServerExtra*)(*level->extras)[i];
    else
      return;
  }
  if (opid>=700 && opid<720) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && i<level->explos->count)
//      explo = (*level->explos)[i];
//    else
      return;
  }
  if (opid>=720 && opid<740) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->horses->count)
      horse = (TServerHorse*)(*level->horses)[i];
    else
      return;
  }
  if (opid>=740 && opid<800) {
    npc = GetNPC(TruncX(calcOperation(op->op1)));
    if (!Assigned(npc)) return;
  }
  if (opid>=800 && opid<1000) {
    player = GetPlayer(TruncX(calcOperation(op->op1)));
    if (!Assigned(player)) return;
  }

  switch (opid) {
    case 20:
    case 21: {
        if (Assigned(op->pvar)) {
          if (Assigned(op2) && Assigned(op2->arrlist)) {
            op->pvar->SetArraySize(op2->arrlist->count);
            for (i=0; i<op2->arrlist->count; i++)
              op->pvar->val[i] = calcOperation((TNPCOperation*)(*op2->arrlist)[i]);
          } else if (op->pvar->isarray && Assigned(op->op2)) {
            i = TruncX(calcOperation(op->op2));
            if (i>=0 && i<op->pvar->arrsize)
              op->pvar->val[i] = value;
          } else
            op->pvar->value = value;
        }
        break;
      }
    case 100: {
        if (Assigned(level) && Assigned(op2)) {
          i = TruncX(calcOperation(op2));
          if (i>=0 && i<64*64)
            level->board[i] = (int)value;
        }
        break;
      }

    case 600: { if (value>=0 && value<64) compu->x = value; break; }
    case 601: { if (value>=0 && value<64) compu->y = value; break; }
    case 602: { if (ivalue>=0 && ivalue<10) compu->comptype = ivalue; break; }
    case 603: { if (ivalue>=0 && ivalue<4) compu->dirs = (compu->dirs & 0x3) + (ivalue<<2); break; }
    case 604: { if (ivalue>=0 && ivalue<4) compu->dirs = (compu->dirs & 0xc) + ivalue; break; }
    case 605: { if (ivalue>=1) compu->power = ivalue; break; }
    case 606: { if (ivalue>=0 && value<10) compu->mode = ivalue; break; }

    case 740: {
        double oldval = npc->x;
        if (value>=-2 && value<64) npc->x = value;
        if (Assigned(npc->character)) npc->character->x = npc->x;
        if (oldval!=npc->x) curmodified[NPCX] = true;
        break;
      }
    case 741: {
        double oldval = npc->y;
        if (value>=-2 && value<64) npc->y = value;
        if (Assigned(npc->character)) npc->character->y = npc->y;
        if (oldval!=npc->y) curmodified[NPCY] = true;
        break;
      }
//    case 742: npc->pixelwidth
//    case 743: npc->pixelheight
    case 744: {
        npc->rupees = ivalue;
        if (npc->rupees<0) npc->rupees = 0;
        break;
      }
    case 745: {
        npc->darts = ivalue;
        if (npc->darts<0) npc->darts = 0;
        break;
      }
    case 746: {
        npc->bombs = ivalue;
        if (npc->bombs<0) npc->bombs = 0;
        break;
      }
    case 747: {
       double oldval = npc->hearts;
#ifdef NEWWORLD
        value = TruncX(value*50)/50;
#else
        value = TruncX(value*2)/2;
#endif
        if (value<0) value = 0;
        if (value!=npc->hearts) {
          npc->hearts = TruncX(value);
          if (Assigned(npc->character))
            character->power = value;
        }
        if (oldval!=npc->hearts) curmodified[NPCPOWER] = true;
        break;
      }
    case 748: {
        if (value<0) value = 0;
        npc->timeout = TruncX(value*10);
        npc->timeoutline = -1;
        break;
      }
    case 749: {
        npc->glovepower = ivalue;
        if (npc->glovepower<0) npc->glovepower = 0;
        break;
      }
#ifndef NEWWORLD
    case 750: {
        int oldval = swordpower;
        npc->swordpower = ivalue;
        if (npc->swordpower<0) npc->swordpower = 0;
        if (oldval!=npc->swordpower) curmodified[NSWORDGIF] = true;
        break;
      }
#endif
    case 751: {
        int oldval = shieldpower; 
        npc->shieldpower = ivalue; 
        if (npc->shieldpower<0) npc->shieldpower = 0; 
        if (oldval!=npc->shieldpower) curmodified[NSHIELDGIF] = true; 
      }
    case 752: {
        npc->hurtdx = value;
        if (npc->hurtdx<-1) npc->hurtdx = -1;
        if (npc->hurtdx>1) npc->hurtdx = 1;
        break;
      }
    case 753: {
        npc->hurtdy = value;
        if (npc->hurtdy<-1) npc->hurtdy = -1;
        if (npc->hurtdy>1) npc->hurtdy = 1;
        break;
      }
//    case 754: npc->id;
    case 755: {
        if (Assigned(op->op2)) {
          i = (int)calcOperation(op->op2);
          if (i>=0 && i<=9)
            npc->save[i] = ivalue;
        }
        break;
      }
    case 756: {
        int oldval = npc->dir;
        npc->dir = ivalue-TruncX((double)ivalue/4)*4;
        npc->spritenum = npc->cursprite*4 + npc->dir;
        if (Assigned(npc->character)) {
          npc->character->dir = npc->dir;
          npc->character->cursprite = npc->cursprite;
          if (npc->cursprite==40)
            npc->character->status |= 8;
          else if (npc->cursprite>=0 && npc->cursprite<spritenumarrcount) {
            npc->character->spritenum = spritenumarr[npc->cursprite]*4+npc->dir;
            npc->character->status &= -1-8;
          }
        }
        if (oldval!=npc->dir) curmodified[NPCSPRITE] = true;
        break;
      }
    case 757: {
        int oldval = npc->cursprite;
        if (ivalue>=0 && ivalue<=40) {
          npc->cursprite = ivalue;
          npc->spritenum = npc->cursprite*4 + npc->dir;
          if (Assigned(npc->character)) {
            npc->character->cursprite = npc->cursprite;
            if (npc->cursprite==40)
              npc->character->status |= 8;
            else if (npc->cursprite>=0 && npc->cursprite<spritenumarrcount) {
              npc->character->spritenum = spritenumarr[npc->cursprite]*4+npc->dir;
              npc->character->status &= -1-8;
            }
          }
        }
        if (oldval!=npc->cursprite) curmodified[NPCSPRITE] = true;
        break;
      }
    case 758: {
        int oldval = npc->alignment;
        if (ivalue>=0 && ivalue<=100)
          npc->alignment = ivalue;
        if (oldval!=npc->alignment) curmodified[NALIGNMENT] = true;
        break;
      }

    case 800: { if (value>=0 && value<64) player->x = value; break; }
    case 801: { if (value>=0 && value<64) player->y = value; break; }
    case 802: { if (ivalue>=0 && ivalue<=9999999) player->rubins = ivalue; break; }
    case 803: { if (ivalue>=0 && ivalue<=99) player->darts = ivalue; break; }
    case 804: { if (ivalue>=0 && ivalue<=99) player->bombscount = ivalue; break; }
    case 805: {
#ifdef NEWWORLD
        value = (TruncX(value*50))/50;
#else
        value = (TruncX(value*2))/2;
#endif
        if (value<0) value = 0;
        if (value!=player->power)
          player->power = value;
        break;
      }
    case 806: {
        if (ivalue>=0 && ivalue<=20) player->maxpower = ivalue;
        break;
      }
    case 807: {
        player->dir = ivalue%4;
        player->spritenum = (player->spritenum & 0xfc) + player->dir;
        break;
      }
    case 808: { if (ivalue>=0) player->glovepower = ivalue; break; }
    case 809: { if (ivalue>=0 && ivalue<=3) player->bombpower = ivalue; break; }
//    case 810: player->shootpower;
#ifndef NEWWORLD
    case 811: { if (ivalue>=0 && ivalue<=4) player->swordpower = ivalue; break; }
#endif
    case 812: { if (ivalue>=0 && ivalue<=4) player->shieldpower = ivalue; break; }
    case 813: {
        if (ivalue>=0) {
          JString newhead = JString("head")+ivalue+".gif";
          if (FileExists(ServerFrame->getDir()+"heads"+fileseparator+newhead))
            player->headgif = newhead;
        }
        break;
      }
    case 814: {
        if (ivalue>=0 && ivalue<40) {
          player->cursprite = ivalue;
          if (player->cursprite==40)
            player->status |= 8;
          else if (player->cursprite>=0 && player->cursprite<spritenumarrcount) {
            player->spritenum = spritenumarr[player->cursprite]*4+player->dir;
            player->status &= -1-8;
          }
        }
        break;
      }
//    case 815: player->id;
//    case 816: GetSaysNumber(player->curchat);
    case 817: { if (ivalue>=0 && ivalue<=100) player->magicpoints = ivalue; break; }
    case 818: { if (ivalue>=0 && ivalue<=100) player->alignment = ivalue; break; }
  }
}

double TServerSideNPC::calcOperation(TNPCOperation* op) {
  int i,j,opid;
  double val1,val2,cx,cy;
  TServerBaddy* compu;
//  TAdventureBomb* bomb;
//  TAdventureFlying* arrow;
  TServerExtra* item;
//  TAdventureExplosion* explo;
  TServerHorse* horse;
  TServerNPC* npc;
  TServerPlayer* player;

  if (!Assigned(op)) return 0;

  compu = NULL;
//  bomb = NULL;
//  arrow = NULL;
  item = NULL;
//  explo = NULL;
  horse = NULL;
  npc = NULL;
  player = NULL;
  opid = op->id;
  if (opid>=600 && opid<640) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->baddies->count)
      compu = (TServerBaddy*)(*level->baddies)[i];
    else
      return 0;
  }
  if (opid>=640 && opid<660) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && (i<level->bombs->count)
//      bomb = (*level->bombs)[i];
//    else
      return 0;
  }
  if (opid>=660 && opid<680) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && i<level->flyingobjs->count)
//      arrow = (*level->flyingobjs)[i];
//    else
      return 0;
  }
  if (opid>=680 && opid<700) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->extras->count)
      item = (TServerExtra*)(*level->extras)[i];
    else
      return 0;
  }
  if (opid>=700 && opid<720) {
//    i = TruncX(calcOperation(op->op1));
//    if (i>=0 && Assigned(level) && i<level->explos->count)
//      explo = (*level->explos)[i];
//    else
      return 0;
  }
  if (opid>=720 && opid<740) {
    i = TruncX(calcOperation(op->op1));
    if (i>=0 && Assigned(level) && i<level->horses->count)
      horse = (TServerHorse*)(*level->horses)[i];
    else
      return 0;
  }
  if (opid>=740 && opid<800) {
    npc = GetNPC(TruncX(calcOperation(op->op1)));
    if (!Assigned(npc)) return 0;
  }
  if (opid>=800 && opid<1000) {
    player = GetPlayer(TruncX(calcOperation(op->op1)));
    if (!Assigned(player)) return 0;
  }

  switch (opid) {
    case 0: return calcOperation(op->op1)+calcOperation(op->op2);
    case 1: return calcOperation(op->op1)-calcOperation(op->op2);
    case 2: return calcOperation(op->op1)*calcOperation(op->op2);
    case 3: {
        val1 = calcOperation(op->op1);
        val2 = calcOperation(op->op2);
        if (val2!=0) return val1/val2;
        break;
      }
    case 4: {
        val1 = calcOperation(op->op1);
        val2 = calcOperation(op->op2);
        if (val2!=0) return val1 - TruncX(val1/val2)*val2;
        break;
      }
    case 5: {
        val1 = calcOperation(op->op1);
        val2 = calcOperation(op->op2);
        if (val1>=0 || (val2>0 && val2==(int)val2)) 
          return pow(val1,val2);
        break;
      }
    case 10: return op->value;
    case 20:
    case 21: {
        if (Assigned(op->pvar)) {
          if (op->pvar->isarray && Assigned(op->op1)) {
            i = TruncX(calcOperation(op->op1));
            if (i>=0 && i<op->pvar->arrsize)
              return op->pvar->val[i];
          } else
            return op->pvar->value;
        }
        break;
      }
    case 100: {
        if (Assigned(level)) {
          i = TruncX(calcOperation(op->op1));
          if (i>=0 && i<64*64)
            return level->board[i];
        }
        break;
      }
    case 101: return strtofloat(GetRealMessage(op->strvalue,false));
#ifdef NEWWORLD
    case 102: return nwday+1;
    case 103: return nwmonth*4+TruncX(nwday/7)+1;
    case 104: return nwyear;
    case 105: return (nwday&7)+1;
    case 106: return nwmonth+1;
    case 107: return nwhour*60+nwmin;
    case 108: return nwhour;
    case 109: return nwmin;
#endif

    case 200: {
        if (Assigned(level))
          return level->baddies->count;
        break;
      }
    case 201: return GetPlayerCount();
    case 202: {
//        if (Assigned(level))
//          return level->bombs->count;
        break;
      }
    case 203: {
//        if (Assigned(level))
//          return level->flyingobjs->count;
        break;
      }
    case 204: {
        if (Assigned(level))
          return level->extras->count;
        break;
      }
    case 205: {
//        if (Assigned(level))
//          return level->explos->count;
        break;
      }
    case 206: {
        if (Assigned(level))
          return level->horses->count;
        break;
      }
    case 207: {
        if (Assigned(level))
          return level->npcs->count;
        break;
      }

    case 400: {
        val1 = calcOperation(op->op1);
        val2 = calcOperation(op->op2);
        if (val1==val2) return val1;
        return (abs(val2-val1)*rand()/(RAND_MAX+1.0))+min(val1,val2);
      }
    case 401: return sin(calcOperation(op->op1));
    case 402: return cos(calcOperation(op->op1));
    case 403: return atan(calcOperation(op->op1));
    case 404: return TruncX(calcOperation(op->op1));
    case 405: return abs(calcOperation(op->op1));
    case 406: {
        if (Assigned(op->op1->pvar) && op->op1->pvar->isarray) return op->op1->pvar->arrsize;
        break;
      }

/*    case 420: // testcompu
      begin
        return -1;
        if (Assigned(actionplayer->level))
        begin
          val1 = calcOperation(op->op1);
          val2 = calcOperation(op->op2);

          for i = 0 to actionplayer->level->baddies->count-1 do
          begin
            compu = actionplayer->level->baddies[i];
            if ((compu->actionmode=HURTED) or (compu->actionmode=BUMPED) or (compu->actionmode=DIE)
              or ((compu->actionmode=SWAMPSHOT) and (compu->seecount>0))
              or ((compu->actionmode=SWAMPSHOT) and ((compu->anicount<22) or (compu->anicount>36)))
              or ((compu->actionmode=HAREJUMP) and ((compu->anicount<17) or (compu->anicount>37))))
                continue;
            cx = compu->x+1;
            cy = compu->y+1;
            if ((compu->actionmode=HAREJUMP) and (compu->anicount>=17) and (compu->anicount<=37))
              cy = cy-jumphare[10-Abs(compu->anicount-27)];
            if ((Abs(val1-cx)<=1) and (Abs(val2-cy)<=1))
            begin
              return i;
              break;
            }
          }
        }
      }
    case 421: // testplayer
      begin
        return -2;
        if (Assigned(actionplayer->level))
        begin
          val1 = calcOperation(op->op1);
          val2 = calcOperation(op->op2);
          if (actionplayer->isOnPlayer(val1,val2))
            return 0;
          if ((Result<=-2) and not Assigned(client))
          begin
            i = actionplayer->isOnOtherPlayer(val1,val2);
            if (i>=0)
              return GetPlayerIndex(Adventure->Player[i]);
          }
          if ((Result<=-2) and Assigned(client))
          begin
            i = actionplayer->isOnOtherClient(val1,val2);
            if (i>=0)
              return GetPlayerIndex(client->others[i]);
          }
          if (Result<=-2)
          begin
            i = actionplayer->level->isOnNPC(val1,val2,false);
            if (i>=0)
            begin
              npc = actionplayer->level->npcs[i];
              if (Assigned(npc) and Assigned(npc->character))
                return GetPlayerIndex(npc->character);
            }
          }
        }
      }
    case 422:
    case 423:
    case 424:
    case 425:
    case 426: // testbomb, testitem, testexplo, testhorse, testnpc
      begin
        return -1;
        if (Assigned(actionplayer->level))
        begin
          val1 = calcOperation(op->op1);
          val2 = calcOperation(op->op2);
          case opid of
            422: return actionplayer->level->isOnBomb(val1,val2);
            423: return actionplayer->level->isOnExtra(val1,val2);
            424: return actionplayer->level->isOnExplosion(val1,val2);
            425: return actionplayer->level->isOnHorse(val1,val2);
            426: return actionplayer->level->isOnNPC(val1,val2,false);
          }
        }
      }*/

    case 600: return compu->x;
    case 601: return compu->y;
    case 602: return compu->comptype;
    case 603: return (compu->dirs >> 2);
    case 604: return (compu->dirs & 0x3);
    case 605: return compu->power;
    case 606: return compu->mode;

/*    case 640: return bomb->x;
    case 641: return bomb->y;
    case 642: return bomb->power;
    case 643: return bomb->anicount/20;

    case 660: return arrow->x;
    case 661: return arrow->y;
    case 662:
      begin
        if (Abs(arrow->dx)<Abs(arrow->dy))
          if (arrow->dy<0) return UP else return DOWN
        else
          if (arrow->dx<0) return LEFT else return RIGHT;
      }
    case 663: return arrow->dx;
    case 664: return arrow->dy;
    case 665: return arrow->shottype; // -1 -> ball, 0->->3 -> arrow
    case 666: return arrow->objtype; // from (0 -> baddy, 1 -> player)*/

    case 680: return item->x;
    case 681: return item->y;
    case 682: return item->itemindex;
    case 683: return ((double)item->counter)/20; // time

/*    case 700: return explo->x;
    case 701: return explo->y;
    case 702: return explo->bombpower;
    case 703: return explo->anicount/20;
    case 704: return explodir[explo->exptype];*/

    case 720: return horse->x;
    case 721: return horse->y;
    case 722: return horse->dir;
//    case 723: return horse->bushes;
//    case 724: return horse->bombs;
//    case 725: return horse->bombpower;
//    case 726: if (horse->boat) return 1 else return 0; // type

    case 740: return npc->x;
    case 741: return npc->y;
    case 742: return (npc->pixelwidth/bw);
    case 743: return (npc->pixelheight/bw);
    case 744: return npc->rupees;
    case 745: return npc->darts;
    case 746: return npc->bombs;
    case 747: return npc->hearts;
    case 748: return npc->timeout/10;
    case 749: return npc->glovepower;
#ifndef NEWWORLD
    case 750: return npc->swordpower;
#endif
//    case 751: return npc->shieldpower;
    case 752: return npc->hurtdx;
    case 753: return npc->hurtdy;
    case 754: return npc->id;
    case 755: {
        if (Assigned(op->op2)) {
          i = TruncX(calcOperation(op->op2));
          if (i>=0 && i<=9)
            return npc->save[i];
        }
        break;
      }
    case 756: return npc->dir;
    case 757: return npc->cursprite;
    case 758: return npc->alignment;

    case 800: return player->x;
    case 801: return player->y;
    case 802: return player->rubins;
    case 803: return player->darts;
    case 804: return player->bombscount;
    case 805: return player->power;
    case 806: return player->maxpower;
    case 807: return player->dir;
    case 808: return player->glovepower;
    case 809: return player->bombpower;
//    case 810: player->shootpower;
#ifndef NEWWORLD
    case 811: return player->swordpower;
#endif
    case 812: return player->shieldpower;
    case 813: return getGifNumber(player->headgif,"head");
    case 814: return player->cursprite;
    case 815: return player->id;
//    case 816: return GetSaysNumber(player->curchat);
    case 817: return player->magicpoints;
    case 818: return player->alignment;
  }
  return 0;
}

bool TServerSideNPC::compareOperations(TNPCOperation* op1, TNPCOperation* op2) {
  int i,len,len2;
  bool ispvar1,ispvar2;
  double val1,val2;

  if (!Assigned(op1) || !Assigned(op2)) return false;

  ispvar1 = (Assigned(op1->pvar) && op1->pvar->isarray);
  ispvar2 = (Assigned(op2->pvar) && op2->pvar->isarray);
  if ((Assigned(op1->arrlist) || ispvar1) && (Assigned(op2->arrlist) || ispvar2)) {
    if (ispvar1) len = op1->pvar->arrsize; else len = op1->arrlist->count;
    if (ispvar2) len2 = op2->pvar->arrsize; else len2 = op2->arrlist->count;
    if (len!=len2) return false;
    else for (i=0; i<len; i++) {
      if (ispvar1) val1 = op1->pvar->val[i]; else val1 = calcOperation((TNPCOperation*)(*op1->arrlist)[i]);
      if (ispvar2) val2 = op2->pvar->val[i]; else val2 = calcOperation((TNPCOperation*)(*op2->arrlist)[i]);
      if (abs(val1-val2)>=doubleprecision) return false;
    }
    return true;
  } else
    return (abs(calcOperation(op1)-calcOperation(op2))<doubleprecision);
}

bool TServerSideNPC::calcBoolOperation(TNPCBoolOperation* bop) {
  int i,j;
  double testx,testy,val,val2;
  bool isblocking,res;
  JString str,str2;
  TServerPlayer* player;
  JString border = ".,;:-_><|!\"§$%&/\()=?`´{[]}+*~#''^ ";

  if (!Assigned(bop)) return false;

  switch (bop->id) {
    case 0: return !calcBoolOperation(bop->bop1);
    case 1: return (calcBoolOperation(bop->bop1) || calcBoolOperation(bop->bop2));
    case 2: return (calcBoolOperation(bop->bop1) && calcBoolOperation(bop->bop2));
    case 3: { // in
        if (Assigned(bop->op1) && Assigned(bop->op2)) {
          val = calcOperation(bop->op1);
          if (Assigned(bop->op2->arrlist)) {
            for (i=0; i<bop->op2->arrlist->count; i++) {
              val2 = calcOperation((TNPCOperation*)(*bop->op2->arrlist)[i]);
              if (abs(val-val2)<doubleprecision) return true;
            }
          } else if (Assigned(bop->op2->pvar) && bop->op2->pvar->isarray) {
            for (i=0; i<bop->op2->pvar->arrsize; i++)
              if (abs(val-bop->op2->pvar->val[i])<doubleprecision) return true;
          }
        }
        break;
      }
    case 10: return bop->value;
    case 20: return isflagset(bop->flagstring);
    case 200:
    case 209: return compareOperations(bop->op1,bop->op2);
    case 201:
    case 206: return !compareOperations(bop->op1,bop->op2);
    case 202:
    case 203: return ((calcOperation(bop->op1)-calcOperation(bop->op2))<doubleprecision); // <=
    case 204:
    case 205: return ((calcOperation(bop->op1)-calcOperation(bop->op2))>-doubleprecision); // >=
    case 207: return ((calcOperation(bop->op1)-calcOperation(bop->op2))<-doubleprecision); // <
    case 208: return ((calcOperation(bop->op1)-calcOperation(bop->op2))>doubleprecision); // >

    case 400: { // onwall
        if (!Assigned(level)) return true;
        testx = calcOperation(bop->op1);
        testy = calcOperation(bop->op2);
        res = false;
        if (testx<0 || testx>=64 || testy<0 || testy>=64) return true;
        else {
          isblocking = dontblock;
          dontblock = true;
          res = level->isOnWall(testx,testy);
          dontblock = isblocking;
        }
        if (!res && Assigned(character))
          res = level->isOnPlayer(testx,testy);
        return res;
      }
    case 401:
    case 402: { // playersays/playersays2
        // not supported anymore
      }
    case 403: { // strequals
        str = LowerCase(Trim(GetRealMessage(bop->flagstring,false)));
        str2 = LowerCase(Trim(GetRealMessage(bop->flagstring2,false)));
        return (str==str2);
      }
    case 404: { // strcontains
        str = LowerCase(Trim(GetRealMessage(bop->flagstring,false)));
        str2 = LowerCase(Trim(GetRealMessage(bop->flagstring2,false)));
        j = Length(str2);
        if (j>0) while (Length(str)>0) {
          i = Pos(str2,str);
          if (i==0) break;
          if ((i==1 || Pos(str[i-1],border)>0) &&
              (i+j>Length(str) || Pos(str[i+j],border)>0))
            return true;
          str = Trim(Copy(str,i+j,Length(str)-i-j+1));
        }
        return false;
      }
    case 405: { // hasweapon
        if (Assigned(actionplayer)) {
          str = LowerCase(bop->flagstring);
          for (i=0; i<actionplayer->myweapons->count; i++)
            if (LowerCase((*actionplayer->myweapons)[i])==str) return true;
        }
        return false;
      }
    case 406: { // onwater
        if (!Assigned(level)) return false;
        testx = calcOperation(bop->op1);
        testy = calcOperation(bop->op2);
        if (testx<0 || testx>=64 || testy<0 || testy>=64) return false;
        return level->isOnWater(testx,testy);
      }
    case 407: { // playeringuild (flagstring)
        return (Assigned(actionplayer) && actionplayer->plainguild==bop->flagstring);
      }
  }
  return false;
}

JString TServerSideNPC::GetRealMessage(JString msg, bool withlineends) {
  int i,j,codelen,len,value;
  char testchar;
  JString str,codestr;
  TServerPlayer* player;

  while (true) {
    i = Pos('#',msg);
    if (i==0 || i>=Length(msg)) break;
    str = "";
    testchar = (char)msg[i+1];
    len = 2;
    switch (testchar) {
      case '#': { str = '#'; break; }
      case 'v': {
          if (i<=Length(msg)-3 && msg[i+2]=='(') {
            j = findBracketsEnd(Copy(msg,i+2,Length(msg)-i-1));
            if (j>0) {
              len = 2+j;
              codestr = GetRealMessage(Copy(msg,i+3,j-2),false);
              str = floattostrX(calcValue(codestr));
            }
          }
          break;
        }
      case 's': {
          if (Assigned(actionplayer) && i<=Length(msg)-3 && msg[i+2]=='(') {
            j = findBracketsEnd(Copy(msg,i+2,Length(msg)-i-1));
            if (j>0) {
              len = 2+j;
              codestr = GetRealMessage(Trim(Copy(msg,i+3,j-2)),false);
              str = actionplayer->actionflags->getValue(codestr);
            }
          }
          break;
        }
      case 'n':
      case 'g':
      case 'c':
      case 'a':
      case '1':
      case '2':
      case '3':
      case '5':
      case '6':
      case '7':
      case '8':
      case 'C': {
          codelen = 1;
          if (testchar=='C') codelen = 2;
          len = 1+codelen;
          player = NULL;
          if (i<=Length(msg)-2-codelen && msg[i+1+codelen]=='(') {
            j = findBracketsEnd(Copy(msg,i+1+codelen,Length(msg)-i-codelen));
            if (j>0) {
              len = 1+codelen+j;
              value = TruncX(calcValue(Copy(msg,i+1+codelen+1,j-2)));
              player = GetPlayer(value);
            }
          } else
            player = actionplayer;

          if (Assigned(player)) switch(testchar) {
            case 'n': { str = player->nickname; break; }
            case 'g': { str = player->plainguild; break; }
            case 'c': { str = player->curchat; break; }
            case 'a': { str = player->accountname; break; }
//#ifndef NEWWORLD
            case '1': { str = player->swordgif; break; }
//#endif
            case '2': { str = player->shieldgif; break; }
            case '3': { str = player->headgif; break; }
            case '5': { str = player->horsegif; break; }
            case '6': {
                if (Assigned(player->carrynpc))
                  str = LowerCaseFilename(player->carrynpc->filename);
                break;
              }
            case '7': { str = player->shootgif; break; }
            case '8': { str = "body.png"; break; }
            case 'C':
#ifdef NEWWORLD
              if (i<=Length(msg)-2 && msg[i+2]>='0' && msg[i+2]<='7')
#else
              if (i<=Length(msg)-2 && msg[i+2]>='0' && msg[i+2]<='5')
#endif
                str = colornames[player->colors[msg[i+2]-'0']];
          }
          break;
        }
      case 'L': { if (Assigned(level)) str = level->plainfile; break; }
      case 'f': { str = LowerCaseFilename(filename); break; }
      case 'w': {
//          if actionplayer.selectedweapon>=0 then
//            str := TAdventureWeapon(actionplayer.weaponnpcs[actionplayer.selectedweapon]).name;
          break;
        }
#ifdef NEWWORLD
      case 'S': {
//          if actionplayer.selectedsword>=0 then
//            str := TAdventureWeapon(actionplayer.weaponnpcs[actionplayer.selectedsword]).name;
          break;
        }
#endif
      case 'b': { if (withlineends) str = (char)10; break; }
      default: { len = 1; break; }
    }
    msg = Copy(msg,1,i-1) + str + Copy(msg,i+len,Length(msg)+1-i-len);
  }
  return msg;
}

TServerNPC* TServerSideNPC::GetNPC(int index) {
  int j,k;
  TServerNPC* npc;

  if (index==-1) return this;
  if (Assigned(level)) {
    j = 0;
    if (level->npcs->indexOf(this)>=0) {
      if (j==index) return this;
      else j++;
    }

    for (k=0; k<level->npcs->count; k++) {
      npc = (TServerNPC*)(*level->npcs)[k];
      if (Assigned(npc) && npc!=this) {
        if (j==index) return npc;
        else j++;
      }
    }
  }
  return NULL;
}

TServerPlayer* TServerSideNPC::GetPlayer(int index) {
  int j,k;
  TServerNPC* npc;
  TServerPlayer* player;

  if (index==-1) return character;
  if (Assigned(level)) {
    j = 0;
    if (Assigned(actionplayer) && level->players->indexOf(actionplayer)>=0) {
      if (j==index) return actionplayer;
      else j++;
    }

    for (k=0; k<level->players->count; k++) {
      player = (TServerPlayer*)(*level->players)[k];
      if (Assigned(player) && player!=actionplayer) {
        if (j==index) return player;
        else j++;
      }
    }
    for (k=0; k<level->npcs->count; k++) {
      npc = (TServerNPC*)(*level->npcs)[k];
      if (Assigned(npc) && npc!=this && Assigned(npc->character)) {
        if (j==index) return npc->character;
        else j++;
      }
    }
  }
  return NULL;
}

int TServerSideNPC::GetPlayerCount() {
  if (!Assigned(level)) return 0;
  int count = level->players->count;
  for (int i=0; i<level->npcs->count; i++) {
    TServerNPC* npc = (TServerNPC*)(*level->npcs)[i];
    if (Assigned(npc) && npc!=this && Assigned(npc->character))
      count++;
  }
  return count;
}

double TServerSideNPC::calcValue(JString str) {
  double res = 0;
  TNPCOperation* op = parseValue(str);
  if (Assigned(op)) {
    res = calcOperation(op);
    delete(op);
  }
  return res;
}

void TServerSideNPC::parseList(int startindex, int endindex) {
  if (!Assigned(commandlist)) return;
  if (startindex<0) startindex = 0;
  if (endindex>commandlist->count) endindex = commandlist->count;

  int c = startindex; // commandlist index
  while (c<endindex) {
    TNPCCommand* com = (TNPCCommand*)(*commandlist)[c];
    c++;
    if (!Assigned(com)) continue;
    int command = com->id;
    JString arg = com->arg1;

    switch (command) {
      case 4: { // set
          if (Assigned(actionplayer) && actionplayer->actionflags->indexOf(arg)<0) {
            actionplayer->actionflags->Add(arg);
            actionplayer->SendData(SSETFLAG,arg);
          }
          break;
        }
      case 5: { // if
          com->bop->value = calcBoolOperation(com->bop);
          lastifwastrue = com->bop->value;
          if (!lastifwastrue) c = com->jumppos;
          break;
        }
      case 7: { // unset
          if (Assigned(actionplayer)) {
            int i = actionplayer->actionflags->indexOf(arg);
            if (i>=0) {
              actionplayer->actionflags->Delete(i);
              actionplayer->SendData(SUNSETFLAG,arg);
            }
          }
          break;
        }
      case 8:
      case 80: { // setgif, setgifpart
          JString oldval1 = filename;
          JString oldval2 = gifpart;
          int oldval3 = visflags;

          JString msg = GetRealMessage(Trim(arg),false);
          if (LevelFileExists(msg)) {
            filename = msg;
            visflags |= 1;
            int partx = 0;
            int party = 0;
            int partw = 0;
            int parth = 0;
            if (command==80) {
              partx = TruncX(calcOperation(com->op1));
              party = TruncX(calcOperation(com->op2));
              partw = TruncX(calcOperation(com->op3));
              parth = TruncX(calcOperation(com->op4));
              if (partx<0) partx = 0;
              if (partx>16000) partx = 16000;
              if (party<0) party = 0;
              if (party>16000) party = 16000;
              if (partw<0) partw = 0;
              if (partw>220) partw = 220;
              if (parth<0) parth = 0;
              if (parth>220) parth = 220;
            }
            gifpart = JString() <<
              (char)(((partx >> 7) & 0x7f)+32) << (char)((partx & 0x7f)+32) <<
              (char)(((party >> 7) & 0x7f)+32) << (char)((party & 0x7f)+32) <<
              (char)(partw+32) << (char)(parth+32);
            if (Assigned(character)) { delete(character); character = NULL; }

          }

          if (oldval1!=filename) curmodified[NPCGIF] = true;
          if (oldval2!=gifpart) curmodified[NPCGIFPART] = true;
          if (oldval3!=visflags) curmodified[VISFLAGS] = true;
          break;
        }
      case 11:
      case 12:
      case 13:
      case 14:
      case 15: { // setxxxcolor
          if (Assigned(actionplayer))
            actionplayer->colors[command-11] = TruncX(com->val1);
          break;
        }
      case 34: { if (lastifwastrue) c = com->jumppos; break; } // else
      case 48: { // message
          JString oldval = chatmsg;
          JString msg = GetRealMessage(Trim(arg),false);
          chatmsg = msg;
          if (Assigned(character))
            character->curchat = msg;
          if (oldval!=chatmsg) curmodified[NPCMESSAGE] = true;
          break;
        }
      case 56: { // toweapons
          if (!Assigned(actionplayer) || !Assigned(level) || Length(filename)<=0) return;

          JString wname = Trim(arg);
          if (Length(wname)<=0) return;

          // Check if we already have this weapon;
          // if yes, then don't add a new one
          if (actionplayer->myweapons->indexOf(wname)>=0) return;
          actionplayer->myweapons->Add(wname);
          actionplayer->weaponstosend->Add(wname);

          // Search for weapons in the weapons list that
          // we can replace; if there is no weapon with the
          // name yet then add a new TServerWeapon
          TServerWeapon* replaceweapon = NULL;
          bool doreplace = true;
          for (int j=0; j<weapons->count; j++) {
            TServerWeapon* weapon = (TServerWeapon*)(*weapons)[j];
            if (Assigned(weapon) && weapon->name==wname && weapon->world==actionplayer->playerworld) {
              // Don't replace the existing weapon if it's newer
              // than the current level
              if (weapon->modtime>=level->modtime) doreplace = false;
              replaceweapon = weapon;
              break;
            }
          }
          if (doreplace)  {
            // Update the found weapon / add a new weapon to the
            // weapons list
            if (!Assigned(replaceweapon)) {
              replaceweapon = new TServerWeapon();
              replaceweapon->name = wname;
              replaceweapon->world = actionplayer->playerworld;
            }
            replaceweapon->image = filename;
            replaceweapon->modtime = level->modtime;
            replaceweapon->dataforplayer.clear();
            int actionlen = Length(action);
            if (Length(actionplayer->playerworld)>0)
              replaceweapon->dataforplayer << JString((char)(Length(wname)+32)) << wname
                << (char)(0+32) << (char)(Length(filename)+32) << filename
                << (char)(1+32) << JString((char)(((actionlen >> 7) & 0x7F)+32))
                << (char)((actionlen & 0x7F)+32) << action;
            wname = actionplayer->playerworld + wname;
            replaceweapon->fullstr.clear();
            replaceweapon->fullstr << JString((char)(Length(wname)+32)) << wname
              << (char)(0+32) << (char)(Length(filename)+32) << filename
              << (char)(1+32) << JString((char)(((actionlen >> 7) & 0x7F)+32))
              << (char)((actionlen & 0x7F)+32) << action;
            if (weapons->indexOf(replaceweapon)<0)
              weapons->Add(replaceweapon);
            SaveWeapons();
          }
          break;
        }
      case 60:
      case 68: { // for/while
          com->bop->value = calcBoolOperation(com->bop);
          com->op1->value++;
          if (!com->bop->value || com->op1->value>10000)
            c = com->jumppos;
          break;
        }
      case 66: { // setarray
          if (Assigned(com->op1) && (com->op1->id==20 || com->op1->id==21) &&
               Assigned(com->op1->pvar))
            com->op1->pvar->SetArraySize(TruncX(calcOperation(com->op2)));
          break;
        }
      case 67: { // sleep
          double value = calcOperation(com->op1);
          if (value<0)  value = 0;
          timeout = TruncX(value*10);
          if (timeout>0) {
            timeoutline = c;
            endparsing = true;
          }
          break;
        }
      case 73: { // showcharacter
          if (!Assigned(character)) {
            filename = "#c#";
            character = new TServerPlayer(NULL,0);
            character->nickname = nickname;
            character->x = x;
            character->y = y;
            curmodified[NPCGIF] = true;
          }
          break;
        }
      case 75: { // setcharprop
          if (Length(arg)>=2 && arg[1]=='#' && Assigned(character)) {
            JString msg = GetRealMessage(Trim(com->arg2),false);
            switch ((char)arg[2]) {
              case 'n': {
                  JString oldval = nickname;
                  nickname = msg;
                  character->nickname = msg;
                  if (oldval!=nickname) curmodified[NPCNICKNAME] = true;
                  break;
                }
              case 'c': {
                  JString oldval = chatmsg;
                  chatmsg = msg;
                  character->curchat = msg;
                  if (oldval!=chatmsg) curmodified[NPCMESSAGE] = true;
                  break;
                }
#ifndef NEWWORLD
              case '1': {
                  JString oldval = swordgif;
                  if (LevelFileExists(msg)) {
                    swordgif = msg;
                    character->swordgif = msg;
                  }
                  if (oldval!=swordgif) curmodified[NSWORDGIF] = true;
                  break;
                }
#endif
              case '2': {
                  JString oldval = shieldgif;
                  if (LevelFileExists(msg)) {
                    shieldgif = msg;
                    character->shieldgif = msg;
                  }
                  if (oldval!=shieldgif) curmodified[NSHIELDGIF] = true;
                  break;
                }
              case '3': {
                  JString oldval = headgif;
                  if (LevelFileExists(msg)) {
                    headgif = msg;
                    character->headgif = msg;
                  }
                  if (oldval!=headgif) curmodified[NPCHEADGIF] = true;
                  break;
                }
              case '5': {
                  JString oldval = horsegif;
                  if (LevelFileExists(msg)) {
                    horsegif = msg;
                    character->horsegif = msg;
                  }
                  if (oldval!=horsegif) curmodified[NPCHORSEGIF] = true;
                  break;
                }
              case '7': {
                  JString oldval = bowgif;
                  if (LevelFileExists(msg)) {
                    bowgif = msg;
                    character->shootgif = msg;
                  }
                  if (oldval!=bowgif) curmodified[NPCBOWGIF] = true;
                  break;
                }
              case '8': {
#ifdef NEWWORLD
//                  character.bodygif.loadImage(msg);
#endif
                }
              case 'C': {
#ifdef NEWWORLD
                  if (Length(arg)>=3 && arg[3]>='0' && arg[3]<='7') {
#else
                  if (Length(arg)>=3 && arg[3]>='0' && arg[3]<='5') {
#endif
                    int j = getColorIndex(msg);
                    if (j>=0) {
                      int k = arg[3]-(int)'0';
                      int oldval = colors[k];
                      colors[k] = j;
                      character->colors[k] = j;
                      if (oldval!=colors[k]) curmodified[NPCCOLORS] = true;
                    }
                  }
                  break;
                }
            }
          }
          break;
        }
      case 77: { // setstring
          if (Assigned(actionplayer)) {
            JString msg = GetRealMessage(Trim(arg),false);
            JString msg2 = GetRealMessage(Trim(com->arg2),false);
            JString msg3 = msg + "=";
            for (int i=0; i<actionplayer->actionflags->count; i++) {
              JString str = (*actionplayer->actionflags)[i];
              if (startsWith(msg3,str)) {
                JString str2 = Copy(str,Length(msg3)+1,Length(str)-Length(msg3));
                if (msg2!=str2) {
                  actionplayer->SendData(SUNSETFLAG,str);
                  actionplayer->actionflags->Delete(i);
                  if (Length(msg2)>0) {
                    msg3 << msg2;
                    actionplayer->SendData(SSETFLAG,msg3);
                    actionplayer->actionflags->Add(msg3);
                  }
                }
                break;
              }
            }
          }
          break;
        }
      case 107: { canwarp = true; break; }
      case 108: { // setani
          #ifdef NEWWORLD
          if (Assigned(actionplayer)) {
            JString newani = Trim(arg);
            JString params = Trim(com->arg2);
            if (Length(params)>0) newani << params; 
            actionplayer->ani = newani;
          }
          break;
	  #endif
        }
      case 109: { // setcharani
          #ifdef NEWWORLD
	  JString oldani = ani; 

          JString newani = Trim(arg);
          JString params = Trim(com->arg2);
          if (Length(params)>0) newani << params; 
          ani = newani;

          if (oldani!=ani)
            curmodified[NPCANI] = true;
          break;
          #endif
        }

      case 200: { calcAssignment(com); break; } // =
      case 201: { break; } // noop
      case 202: { c = com->jumppos; break; } // jump
      case 203: { lastifwastrue = com->bop->value; break; } // set lastifwastrue
      case 204: { // else if
          if (lastifwastrue) c = com->jumppos;
          else {
            com->bop->value = calcBoolOperation(com->bop);
            lastifwastrue = com->bop->value;
            if (!lastifwastrue) c = com->jumppos;
          }
          break;
        }
    }
    if (endparsing) return;
  }
}

double linktestd[] = {1.5,1, 0.5,2, 1.5,3, 2.5,2};

void TServerSideNPC::testForLinks() {
  if (!Assigned(level) || !Assigned(level->links)) return;
  double testx = x+linktestd[(dir << 1)];
  double testy = y+linktestd[(dir << 1)+1];

  JString newfilename;
  double newx = x;
  double newy = y;

  for (int i=0; i<level->links->count; i++) {
    TServerLevelLink* link = (TServerLevelLink*)(*level->links)[i];
/*    if (link.x=0) and (link.w=1) and (link.filename=sidelevelnames[0,1]) then
    begin
      if (testx<=0) and (dir=LEFT) then
      begin
        newfilename := sidelevelnames[0,1];
        newx := x+64;
        goto dolinklevel;
      end;
      continue;
    end;
    if (link.x=63) and (link.w=1) and (link.filename=sidelevelnames[2,1]) then
    begin
      if (testx>=64) and (dir=RIGHT) then
      begin
        newfilename := sidelevelnames[2,1];
        newx := x-64;
        goto dolinklevel;
      end;
      continue;
    end;
    if (link.y=0)  and (link.h=1) and (link.filename=sidelevelnames[1,0]) then
    begin
      if (testy<=0) and (dir=UP) then
      begin
        newfilename := sidelevelnames[1,0];
        newy := y+64;
        goto dolinklevel;
      end;
      continue;
    end;
    if (link.y=63) and (link.h=1) and (link.filename=sidelevelnames[1,2]) then
    begin
      if (testy>=64) and (dir=DOWN) then
      begin
        newfilename := sidelevelnames[1,2];
        newy := y-64;
        goto dolinklevel;
      end;
      continue;
    end;*/

    if (testy<=1 && dir!=UP) continue;
    if (testx<=1 && dir!=LEFT) continue;
    if (testy>=63 && dir!=DOWN) continue;
    if (testx>=63 && dir!=RIGHT) continue;

    if (testx>=link->x && testx<=link->x+link->w && testy>=link->y &&
        testy<=link->y+link->h) {
      if (link->newx!="playerx") newx = strtofloat(link->newx);
      if (link->newy!="playery") newy = strtofloat(link->newy);
      if (dir==DOWN && newy==0) newy = -1;
      newfilename = link->filename;
      goto dolinklevel;
    }
  }
  return;

dolinklevel:
  if (Length(newfilename)>0) {
    TServerLevel* newlevel = ServerFrame->LoadLevel(newfilename);
    if (Assigned(newlevel)) {
      // Go out of the current level
      level->npcs->Remove(this);
      for (int i=0; i<ServerFrame->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*ServerFrame->players)[i];
        if (Assigned(player) && player->loggedin() && !player->isrc &&
            level->GetLeavingTimeEntry(player)>=0)
          player->SendData(NPCMOVED,GetCodedID());
      }
      for (int i=0; i<level->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*level->players)[i];
        if (Assigned(player)) player->SendData(NPCMOVED,GetCodedID());
      }

      level = newlevel;
      x = newx;
      curmodified[NPCX] = true;
      y = newy;
      curmodified[NPCY] = true;
      moved = true;
      orgindex = -1;

      for (int i=0; i<npcpropertycount; i++)
        if (modifytime[i]>0) modifytime[i] = curtime;

      // Go into the new level
      // Send changed attributes to the players of the new level
      for (int i=0; i<level->players->count; i++) {
        TServerPlayer* player = (TServerPlayer*)(*level->players)[i];
        player->SendData(SNPCPROPS,GetCodedID() << GetPropertyList(player));
      }
      level->npcs->Add(this);
    }
  }
}

void TServerSideNPC::doNPCAction(JString flagstr, TServerPlayer* player) {
  actionflags->Add(flagstr);
  actionplayers->Add(player);
}

void TServerSideNPC::removeActions(TServerPlayer* player) {
  for (int i=actionplayers->count-1; i>=0; i--)
      if ((*actionplayers)[i]==player) {
    actionplayers->Delete(i);
    actionflags->Delete(i);
  }
}

void TServerSideNPC::runScript() {
  endparsing = false;
  if (timeout>0) {
    timeout--;
    if (timeout<=0) doNPCAction("timeout",NULL);
  }

  for (int i=0; i<actionflags->count; i++) {
    curaction = (*actionflags)[i];
    actionplayer = (TServerPlayer*)(*actionplayers)[i];

    if (!created) created = true;
    if (!Assigned(commandlist)) {
      commandlist = new TList();
      parseString(removeComments(action));
    }
    for (int j=0; j<commandlist->count; j++) {
      TNPCCommand* com = (TNPCCommand*)(*commandlist)[i];
      if (Assigned(com) && Assigned(com->op1) && (com->id==60 || com->id==68))
        com->op1->value = 0;
    }
    if (curaction=="timeout" && timeoutline>=0) {
      int line = timeoutline;
      timeoutline = -1;
      parseList(line,commandlist->count);
    } else
      parseList(0,commandlist->count);

/*    if (deleteme) return;
    if Length(comlinklevel)>0) return;
    if ((x<>oldx) or (y<>oldy)) then for i := 0 to 3 do
    begin
      other := Adventure.Player[i];
      if Assigned(other) and (actionplayer.level=other.level) then
      begin
        j := other.level.isOnNPC(other.x+1.5,other.y+2,false);
        if (j>=0) and (other.level.npcs[j]=Self) then
          other.invokeNPCActions('playertouchsme',j);
      end;
    end;*/
  }
  actionflags->Clear();
  actionplayers->Clear();

  if (canwarp) testForLinks();
  JString props;
  for (int j=0; j<npcpropertycount; j++) if (curmodified[j]) {
    curmodified[j] = false;
    modifytime[j] = curtime;
    props << (char)(j+32) << GetProperty(j);
  }
/*  if (Length(props)>0) {
    cout << "props: " << props << endl;
    cout << "filename:" << npc->filename << endl;
  }*/
  if (Length(props)>0 && Assigned(level)) {
    props = GetCodedID() + props;
    for (int j=0; j<level->players->count; j++) {
      TServerPlayer* player = (TServerPlayer*)(*level->players)[j];
      if (Assigned(player)) player->SendData(SNPCPROPS,props);
    }
  }

/*  for (int i=0; i<actionvars->count; i++) {
    TNPCVariable* var = (TNPCVariable*)(*actionvars)[i];
    cout << "var this." << var->name << ": " << var->value << endl;
    if (var->isarray) {
      cout << "{";
      for (int j=0; j<var->arrsize; j++) cout << var->val[j] << ",";
      cout << "}" << endl;
    }
  }
  if (Assigned(level)) for (int i=0; i<level->actionvars->count; i++) {
    TNPCVariable* var = (TNPCVariable*)(*level->actionvars)[i];
    cout << "var " << var->name << ": " << var->value << endl;
    if (var->isarray) {
      cout << "{";
      for (int j=0; j<var->arrsize; j++) cout << var->val[j] << ",";
      cout << "}" << endl;
    }
  }*/
}

bool TServerSideNPC::isflagset(const JString& flag) {
  if (flag==curaction) return true;
  if (flag=="visible") return (visflags & 1);
  if (flag=="carriesnpc") return (Assigned(actionplayer) && Assigned(actionplayer->carrynpc));

  if (Assigned(actionplayer))
    return (actionplayer->actionflags->indexOf(flag)>=0);
  return false;
}

void testfunc() {
  JString str = JString()+
    "// v1.3 NPC§"+
    "if (created) {§"+
    "  // Initialize the attributes§"+
    "  showcharacter;§"+
    "  setcharprop #3,head0.gif;§"+
    "  setcharprop #C0,orange;§"+
    "  setcharprop #C1,black;§"+
    "  setcharprop #C2,blue;§"+
    "  setcharprop #C3,red;§"+
    "  setcharprop #C4,red;§"+
    "  setcharprop #2,shield1.gif;§"+
    "  shieldpower = 1;§"+
    "  dir = 3;§"+
    "}§"+
    "if (playerenters || wasthrown) {§"+
    "  // Initialize the this. variables§"+
    "  dirgo = {0,-1,-1,0,0,1,1,0};§"+
    "  timeout = 0.05;§"+
    "  this.x1 = 12;§"+
    "  this.x2 = 22;§"+
    "  this.speed = 0.3;§"+
    "  sprite = 0;§"+
    "}§"+
    "§"+
    "// Walking stuff§"+
    "if (timeout) {§"+
    "  sprite = (sprite%8)+1;§"+
    "  newx = x + dirgo[dir*2] * this.speed;§"+
    "  newy = y + dirgo[dir*2+1] * this.speed;§"+
    "§"+
    "  if (dir!=1 && dir!=3) dir = 3;§"+
    "  else {§"+
    "    x = newx;§"+
    "    y = newy;§"+
    "    if ((dir==1 && x<=this.x1) || (dir==3 && x>=this.x2))§"+
    "      dir = (dir+2)%4;§"+
    "  }§"+
    "  timeout = 0.05;§"+
    "}§";
  cout << str << endl;
  cout << "---" << endl;
  cout << removeComments(str) << endl;
}



