#include "includes.h"
#include "gserver.h"
#include "cfg.h"
#include "passwords.h"

JString dberror;
bool sqlconnected = false;
bool sqlok = false;
JString startworld = "classic";

const char *databasetype = "MySQL";


// MySQL handle
MYSQL* mysql = NULL;

int InitDatabase()
{
  // MySQL always ready to kick arses ;)
  // but should create database when they dont
  // exist... well... TODO!
  return 1;
}

void CloseDatabase()
{
  // Hmm! MySQL should close!

  if (sqlconnected) {
    // Disconnect MySQL
  }
  if (mysql != NULL) {
    // Destroy MySQL handle
  }
}

void connecttodb() {
  if (!mysql && !(mysql = mysql_init(mysql))) return;
  if (!sqlconnected) {
  	
    // Reads MySQL Settings from a file,
    // Coded by Mister Bubbles.
    
    struct CfgStrings CfgMySQL[] =
    {
       { "server", NULL },
       { "username", NULL },
       { "password", NULL },
       { "database", NULL },
       { "socket", NULL },
       { NULL, NULL }
    };

    char* mys_server = "localhost";
    char* mys_username = "gserver";
    char* mys_password = "testserver";
    char* mys_database = "gserver";
    char* mys_socket = "/tmp/mysql.sock";  	
  
    CfgRead("data/mysql.txt", CfgMySQL);
    
    for (int i = 0; i < 5; i++) {
      if (CfgMySQL[i].name == "server")
        mys_server = CfgMySQL[i].data;
      if (CfgMySQL[i].name == "username")
        mys_username = CfgMySQL[i].data;
      if (CfgMySQL[i].name == "password")
        mys_password = CfgMySQL[i].data;
      if (CfgMySQL[i].name == "database")
        mys_database = CfgMySQL[i].data;
      if (CfgMySQL[i].name == "socket")
        mys_socket = CfgMySQL[i].data;
    }

    if (!mysql_real_connect(mysql,mys_server,mys_username,mys_password,mys_database,0,mys_socket,0)) {
      sqlok = false;
      std::cout << "Can't connect to MySQL." << std::endl;
      std::cout << mysql_error(mysql) << std::endl;
      dberror = JString("Can't connect to the db: ") << mysql_error(mysql);
      return;
    }
    sqlconnected = true;
    sqlok = true;
  } else {
    sqlok = !mysql_ping(mysql);
    if (!sqlok) dberror = JString("Can't connect to the db: ") << mysql_error(mysql);
  }    
}

void CreateNewDBAccount(JString name, JString password, int id) {
	dberror = "";
	JString query2 = "INSERT INTO accounts (accname, encrpass, email) VALUES ('"+name+"','"+password+"','"+id+"')";
	if (mysql_query(mysql,query2.text())) {
		dberror = JString("Loading failed: couldn't create a db entry: ") << mysql_error(mysql) ;
		std::cout << std::endl << dberror << std::endl;
	}
}

void LoadDBAccount(TServerPlayer* player, const JString& name, JString world) {
  //This function deals with loading the world data of the connect account.
  //World data isn't being created if none exists. There seems to be code to do so, but it isn't working.
  dberror = "";
  if (!Assigned(player) || Length(name)<=0) {
    dberror = "Loading account failed: empty name or player object";
    return; 
  } 
  if (Length(world)<=0) world << startworld;
  unsigned char sep = '§';   
  if (Length(world)>0 && world[Length(world)]==sep)   
    world = Copy(world,1,Length(world)-1);   

  connecttodb();
  if (!sqlok) return;

  MYSQL_RES* res;
  // Error Prevention
  // - Matriark TerVel
  if (Length(world)<1 && Length(startworld)>0) world << startworld;
  if (Length(world)<1) world = JString()+"classic";
  //
  JString query = "SELECT * FROM "+world+" WHERE accname='"+escaped39(name)+"'";
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Loading account failed: ") << mysql_error(mysql);
    return;
  }

  MYSQL_ROW row = mysql_fetch_row(res);
  if (!row) {
    mysql_free_result(res);
    // Create an entry if not exists
    JString query2 = "INSERT INTO "+world+" (accname) VALUES ('"+escaped39(name)+"')";
    if (mysql_query(mysql,query2.text())) {
      dberror = JString("Loading failed: couldn't create a db entry: ") << mysql_error(mysql) ;
      return;
    }
    // Try again to get the row
    if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
      dberror = JString("Loading account failed: ") << mysql_error(mysql);
      return;
    }
    row = mysql_fetch_row(res);
  }

  if (row) {
    player->nickname = row[1];
    player->x = strtofloat(row[2]);
    player->y = strtofloat(row[3]);
    player->levelname = row[4];
    player->maxpower = atoi(row[5])/2;
    player->power = atoi(row[6])/2;
    player->rubins = atoi(row[7]);
    player->darts = atoi(row[8]);
    player->bombscount = atoi(row[9]);
    player->glovepower = atoi(row[10]);
#ifndef NEWWORLD
    player->swordpower = atoi(row[11]);
#endif
    player->shieldpower = atoi(row[12]);
    player->headgif = row[13];
    //player->bodyimg = row[14];
//#ifndef NEWWORLD
    player->swordgif = row[15];
//#endif
    player->shieldgif = row[16];
    JString colorstr = row[17];
#ifdef NEWWORLD
    if (Length(colorstr)>=8)
      for (int i=0; i<8; i++) player->colors[i] = colorstr[i+1]-'a';
#else
    if (Length(colorstr)>=5)
      for (int i=0; i<5; i++) player->colors[i] = colorstr[i+1]-'a';
#endif
    player->spritenum = atoi(row[18]);
    player->status = atoi(row[19]);
    player->horsegif = row[20];
    player->horsebushes = atoi(row[21]);
    player->magicpoints = atoi(row[22]);
    player->kills = atoi(row[23]);
    player->deaths = atoi(row[24]);
    player->onlinesecs = atol(row[25]);
    player->lastip = atol(row[26]);
    player->alignment = atoi(row[27]);
    player->myweapons->SetCommaText(row[28]);
    player->openedchests->SetCommaText(row[29]);
    player->actionflags->SetCommaText(row[30]);
    player->apcounter = atoi(row[31]);
  } else
    dberror = JString("Loading account failed: no database entry found");
    mysql_free_result(res);
}

void SaveDBAccount(TServerPlayer* player) {
  dberror = "";
  if (!Assigned(player) || Length(player->accountname)<=0 ||
      Pos('#',player->accountname)==1) {
    dberror = JString("Saving account failed: empty player object or no legal accountname");
    return;
  }
  JString name = player->accountname;
  JString world = player->playerworld;
  if (Length(world)<=0) world << startworld;
  unsigned char sep = '§';
  if (Length(world)>0 && world[Length(world)]==sep)
    world = Copy(world,1,Length(world)-1);

  connecttodb();
  if (!sqlok) return;
  // Error Prevention
  // - Matriark TerVel
  if (Length(world)<1 && Length(startworld)>0) world << startworld;
  if (Length(world)<1) world = JString()+"classic";
  //
  JString query = "UPDATE "+world+" SET ";
//  query << " nickname='" << escaped39(player->nickname) << "', ";
  query << " nickname='" << escaped39(player->nickname) << "', ";
  query << " x=" << player->x << ", ";
  query << " y=" << player->y << ", ";
  query << " level='" << escaped39(player->levelname) << "', ";
  query << " maxhp=" << (int)(player->maxpower*2) << ", ";
  query << " hp=" << (int)(player->power*2) << ", ";
  query << " rupees=" << player->rubins << ", ";
  query << " arrows=" << player->darts << ", ";
  query << " bombs=" << player->bombscount << ", ";
  query << " glovepower=" << player->glovepower << ", ";
#ifndef NEWWORLD
  query << " swordpower=" << player->swordpower << ", ";
#endif
  query << " shieldpower=" << player->shieldpower << ", ";
  query << " headimg='" << escaped39(player->headgif) << "', ";
  //query << " bodyimg='" << escaped39(player->bodyimg) << "', ";
#ifndef NEWWORLD
  query << " swordimg='" << escaped39(player->swordgif) << "', ";
#endif
  query << " shieldimg='" << escaped39(player->shieldgif) << "', ";
  JString colorstr;
#ifdef NEWWORLD
  for (int i=0; i<8; i++) colorstr << (char)(player->colors[i]+'a');
#else
  for (int i=0; i<5; i++) colorstr << (char)(player->colors[i]+'a');
#endif
  query << " colors='" << escaped39(colorstr) << "', ";
  query << " sprite=" << player->spritenum << ", ";
  query << " status=" << player->status << ", ";
  query << " horseimg='" << escaped39(player->horsegif) << "', ";
  query << " horsebushes=" << player->horsebushes << ", ";
  query << " magic=" << player->magicpoints << ", ";
  query << " kills=" << player->kills << ", ";
  query << " deaths=" << player->deaths << ", ";
  query << " onlinetime=" << player->onlinesecs << ", ";
  unsigned int lastip = player->lastip;
  if (Assigned(player->sock)) lastip = player->sock->ip;
  query << " lastip=" << lastip << ", ";
  query << " alignment=" << player->alignment << ", ";
  query << " weapons='" << player->myweapons->GetCommaText() << "', ";
  query << " chests='" << player->openedchests->GetCommaText() << "', ";
  query << " flags='" << player->actionflags->GetCommaText() << "', ";
  query << " apcounter=" << player->apcounter << " ";

  query << " WHERE accname='"+escaped39(name)+"'";
  if (mysql_query(mysql,query.text())) {
    dberror = JString("Saving account failed: ") << mysql_error(mysql);
    std::cout << "set prob with: " << query << std::endl;
  }
}

TServerAccount* GetAccount(const JString& name) {
  dberror = "";
  if (Length(name)<=0) {
    dberror = JString("Couldn't get account: empty account name");
    return NULL;
  }

  connecttodb();
  if (!sqlok) return NULL;

  MYSQL_RES* res;
#ifdef NEWWORLD
  JString query = "SELECT * FROM accountsnw WHERE accname='"+escaped39(name)+"'";
#else
  JString query = "SELECT * FROM accounts WHERE accname='"+escaped39(name)+"'";
#endif
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get account: ") << mysql_error(mysql);
    return NULL;
  }

  TServerAccount* account = NULL;
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row) {
    account = new TServerAccount();
    account->name = row[0];
    account->password = row[1];
    account->email = row[2];
    account->blocked = (atoi(row[3])>0);
    account->onlyload = (atoi(row[4])>0);
    account->adminlevel = atoi(row[5]);
    account->adminworlds = row[6];
//    account->lastused = row[7];
  } else
    dberror = JString("Couldn't get account: no database entry found");
  mysql_free_result(res);
  return account;
}

// New Function To Use Mysql instead of .txt file for guilds
bool IsInGuild(const JString& accname,const JString& name,const JString& guild) {
  if (Length(name)<=0) {
    return false;
  }

  connecttodb();
  if (!sqlok) return false;

  MYSQL_RES* res;
  JString query = "SELECT * FROM guilds WHERE guildname='"+escaped39(guild)+"' AND accname='"+escaped39(accname)+"'";
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) { 
  return false;
  }
  else {
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (LowerCase(row[1])!=LowerCase(name)) { return false; }
		else { mysql_free_result(res); return true; }
	}
return false;
}
}

JString SetAccount(JString data, int rcadminlevel) {
  int i,len,adminlevel;
  JString accname,pass,email,adminworlds;
  bool blocked,onlyload;
  dberror = "Couldn't set account: wrong data";

  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  accname = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);
  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  pass = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);
  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  email = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);

  if (Length(data)<4) return JString();
  blocked = (data[1]-32)>0;
  onlyload = (data[2]-32)>0;
  adminlevel = data[3]-32;
  if (rcadminlevel<5 && adminlevel>=rcadminlevel)
    adminlevel = rcadminlevel-1;
  data = Copy(data,4,Length(data)-3);

  len = data[1]-32;
  if (len<0) return JString();
  adminworlds = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);

  dberror = "";
  connecttodb();
  if (!sqlok) return JString();

  if (rcadminlevel==5 || rcadminlevel>adminlevel) {
#ifdef NEWWORLD
    JString query = "UPDATE accountsnw SET";
#else
    JString query = "UPDATE accounts SET";
#endif
    query << " email='" << escaped39(email) << "'";
    if (Length(pass)>0)
      query << ", encrpass='" << escaped39(pass) << "'";
    if (blocked) i = 1; else i = 0;
    query << ", blocked=" << i;
    if (onlyload) i = 1; else i = 0;
    query << ", onlyload=" << i;
    query << ", adminlevel=" << (int)adminlevel;
    query << ", adminworlds='" << escaped39(adminworlds) << "'";
    query << " WHERE accname='" << escaped39(accname) << "'";
    if (rcadminlevel<5)
      query << " AND adminlevel<" << rcadminlevel;

    if (mysql_query(mysql,query.text()))
      dberror = JString("Couldn't set account: ") << mysql_error(mysql);
    else
      return accname;
  } else
    dberror = JString("Couldn't set account: permission denied (adminlevel higher than allowed)");
  return JString();
}

void AddAccount(const JString& accname, const JString& encrpass, const JString& email,
    bool blocked, bool onlyload, int adminlevel, const JString& adminworlds) {
  dberror = "";
  connecttodb();
  if (!sqlok) return;

  int i,j;
  if (blocked) i = 1; else i = 0;
  if (onlyload) j = 1; else j = 0;

#ifdef NEWWORLD
  JString query = "INSERT INTO accountsnw (accname,encrpass,email,blocked,onlyload,adminlevel,adminworlds) VALUES";
#else
  JString query = "INSERT INTO accounts (accname,encrpass,email,blocked,onlyload,adminlevel,adminworlds) VALUES";
#endif
  query << " ('" << escaped39(accname) << "','" << escaped39(encrpass) << "','" << escaped39(email) << "'," 
    << i << "," << j << "," << adminlevel << ",'" << escaped39(adminworlds) << "')";
  if (mysql_query(mysql,query.text()))
    dberror = JString("Couldn't add account: ") << mysql_error(mysql);
}

int GetAccountCount() {
  dberror = "";
  connecttodb();
  if (!sqlok) return 0;

  MYSQL_RES* res;
#ifdef NEWWORLD
  JString query = "SELECT count(*) FROM accountsnw";
#else
  JString query = "SELECT count(*) FROM accounts";
#endif
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get account count: ") << mysql_error(mysql);
    return 0;
  }

  int count = 0;
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row)
    count = atoi(row[0]);
  mysql_free_result(res);
  return count;
}

JString AddAccountFromRC(JString data, int rcadminlevel) {
  int i,j,len,adminlevel;
  JString accname,pass,email,adminworlds;
  bool blocked,onlyload;
  dberror = JString("Couldn't add account: wrong data");

  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  accname = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);
  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  pass = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);
  if (Length(data)<1) return JString();
  len = data[1]-32;
  if (len<0) return JString();
  email = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);

  if (Length(data)<4) return JString();
  blocked = (data[1]-32)>0;
  onlyload = (data[2]-32)>0;
  adminlevel = data[3]-32;
  if ((rcadminlevel<5) && (adminlevel>=rcadminlevel))
    adminlevel = rcadminlevel-1;
  data = Copy(data,4,Length(data)-3);

  len = data[1]-32;
  if (len<0) return JString();
  adminworlds = Trim(Copy(data,2,len));
  data = Copy(data,2+len,Length(data)-1-len);

  dberror = "";
  connecttodb();
  if (!sqlok) return JString();

  if ((rcadminlevel==5) || (rcadminlevel>adminlevel)) {
    AddAccount(accname,encryptpassword(pass),email,blocked,onlyload,adminlevel,adminworlds);
    if (Length(dberror)<=0) return accname;
  } else
    dberror = JString("Couldn't add account: permission denied (adminlevel higher than allowed)");
  return JString();
}

JString GetAccountsList(const JString& likestr, const JString& wherestr) {
  dberror = "";
  connecttodb();
  if (!sqlok) return JString();

  if (Pos("encrpass",wherestr)>0) {
    dberror = JString("Couldn't get accounts list: encrpass not allowed as search parameter");
    return JString();
  }

  MYSQL_RES* res;
#ifdef NEWWORLD
  JString query = "SELECT accname FROM accountsnw";
#else
  JString query = "SELECT accname FROM accounts";
#endif
  if (Length(likestr)>0 || Length(wherestr)>0)
    query << " WHERE";
  if (Length(likestr)>0)
    query << " accname like '" << escaped39(likestr) << "'";
  if (Length(wherestr)>0) {
    if (Length(likestr)>0) query << " and";
    query << " " << wherestr;
  }

  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get accounts list: ") << mysql_error(mysql);
    return JString();
  }

  JString str,str2;
  MYSQL_ROW row;
  while ((row=mysql_fetch_row(res))) {
    str2 = row[0];
    str << (char)(Length(str2)+32) << str2;
  }
  mysql_free_result(res);
  return str;
}

TJStringList* GetWorlds() {
  TJStringList* list = new TJStringList();
  list->SetPlainMem();
  dberror = "";
  connecttodb();
  if (!sqlok) return list;

  MYSQL_RES* res;
#ifdef NEWWORLD
  JString query = "SELECT worldname FROM worldsnw";
#else
  JString query = "SELECT worldname FROM worlds";
#endif
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get worlds list: ") << mysql_error(mysql);
    return list;
  }

  MYSQL_ROW row;
  while ((row=mysql_fetch_row(res)))
    list->Add(row[0]);
  mysql_free_result(res);
  return list;
}

int GetAdminLevel(const JString& accname) {
  dberror = "";
  connecttodb();
  if (!sqlok) return 0;
  
  MYSQL_RES* res;
#ifdef NEWWORLD
  JString query = "SELECT adminlevel FROM accountsnw WHERE accname='"+escaped39(accname)+"'";
#else
  JString query = "SELECT adminlevel FROM accounts WHERE accname='"+escaped39(accname)+"'";
#endif
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get admin level: ") << mysql_error(mysql);
    return 0;
  }
  int level = 0;
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row) level = atoi(row[0]);
  mysql_free_result(res);
  return level;
}

int bank(const JString& accname,const char* transt,int value) {
int bank = 0;
  connecttodb();
  if (!sqlok) return 0;
  MYSQL_RES* res;
  JString query = "SELECT bank FROM classic WHERE accname='"+escaped39(accname)+"'";
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get bank balance: ") << mysql_error(mysql);
    return 0;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row) bank = atoi(row[0]);
  mysql_free_result(res);
//std::cout << bank << std::endl;
if (strcmp(transt,"balance")==0) {
// pray for uber cash!
  return bank;
}
else if (strcmp(transt,"deposit")==0) {
if (value) {
  bank += value;
  MYSQL_RES* res;
  //std::cout << bank << std::endl;
  JString query = "UPDATE classic set bank='"+escaped39(bank)+"' WHERE accname='"+escaped39(accname)+"'";
  if (mysql_query(mysql,query.text())) {
    dberror = JString("Couldn't update the bank balance: ") << mysql_error(mysql);
    return 0;
  }
}
if (bank) return bank;
else return 0;
}
else if (strcmp(transt,"withdraw")==0) {
// fucking sluts want their money bank
if ((int)value<=(int)bank) {
std::cout << value << std::endl;
std::cout << bank << std::endl;
  bank -= value;
std::cout << bank << std::endl;
  MYSQL_RES* res;
  JString query = "UPDATE classic set bank='"+escaped39(bank)+"' WHERE accname='"+escaped39(accname)+"'";
  if (mysql_query(mysql,query.text())) {
    dberror = JString("Couldn't update the bank balance: ") << mysql_error(mysql);
    return 0;
  }
}
if (bank) return bank;
else return 0;
}
return 0;
}

void DeleteAccount(const JString& accname, int adminlevel) {
  dberror = "";
  connecttodb();
  if (!sqlok) return;

  if (adminlevel<5) {
    int level = GetAdminLevel(accname);
    if (level>=adminlevel) {
      dberror = JString("Couldn't delete account: permission denied (adminlevel higher than allowed)");
      return;
    }
  }
#ifdef NEWWORLD
  JString query = "DELETE FROM accountsnw WHERE accname='"+escaped39(accname)+"'";
#else
  JString query = "DELETE FROM accounts WHERE accname='"+escaped39(accname)+"'";
#endif
  if (mysql_query(mysql,query.text())) {
    dberror = JString("Couldn't delete account: ") << mysql_error(mysql);
    return;
  }
  TJStringList* list = GetWorlds();
  for (int i=0; i<list->count; i++) {
    JString worldname = (*list)[i];
    query = "DELETE FROM "+worldname+" WHERE accname='"+escaped39(accname)+"'";
    mysql_query(mysql,query.text());
  }
  delete(list);
}
void CreateGuild(const JString& accname, const JString& player, const JString& rank,const JString& guildname) {
  dberror = "";
  connecttodb();
  if (!sqlok) return;

  JString query = "INSERT INTO guilds (accname,player,rank,guildname) VALUES";
  query << " ('" << escaped39(accname) << "', '" << escaped39(player) << "', " << escaped39(rank) << ", '" << escaped39(guildname) << "')";
  if (mysql_query(mysql,query.text()))
    dberror = JString("Couldn't add account: ") << mysql_error(mysql);
}

void DeleteGuild(const JString& guildname) {
  dberror = "";
  connecttodb();
  if (!sqlok) return;

  JString queryn = "DELETE FROM guilds WHERE guildname='"+escaped39(guildname)+"'";
  if (mysql_query(mysql,queryn.text())) {
    dberror = JString("Couldn't delete guild: ") << mysql_error(mysql) << JString(" Guildname: ") << escaped39(guildname);
    return;
  }
}
void DelGuildMember(const JString& accname, const JString& guildname) {
  dberror = "";
  connecttodb();
  if (!sqlok) return;

  JString queryn = "DELETE FROM guilds WHERE guildname='"+escaped39(guildname)+"' and accname='" + accname + "'";
  if (mysql_query(mysql,queryn.text())) {
    dberror = JString("Couldn't delete guild: ") << mysql_error(mysql) << JString(" Guildname: ") << escaped39(guildname);
    return;
  }
}

JString ListGuild(const JString& likestr, const JString& wherestr) {
  dberror = "";
  connecttodb();
  if (!sqlok) return JString();

  if (Pos("encrpass",wherestr)>0) {
    dberror = JString("Couldn't get accounts list: encrpass not allowed as search parameter");
    return JString();
  }

  MYSQL_RES* res;
  JString query = "SELECT guildname FROM guilds WHERE rank=5 ORDER BY guildname ASC";
  if (Length(likestr)>0 || Length(wherestr)>0)
    query << " WHERE";
  if (Length(likestr)>0)
    query << " accname like '" << escaped39(likestr) << "'";
  if (Length(wherestr)>0) {
    if (Length(likestr)>0) query << " and";
    query << " " << wherestr;
  }

  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get accounts list: ") << mysql_error(mysql);
    return JString();
  }

  JString str,str2;
  MYSQL_ROW row;
  while ((row=mysql_fetch_row(res))) {
    str2 = row[0];
    str << "\nj" << str2;
  }
  mysql_free_result(res);
  str2 = "Guilds:";
  str2 << str;
  return str2;
}
JString ListGuildMembers(const JString& likestr, const JString& wherestr, const JString& guildname) {
  dberror = "";
  connecttodb();
  if (!sqlok) return JString();

  if (Pos("encrpass",wherestr)>0) {
    dberror = JString("Couldn't get accounts list: encrpass not allowed as search parameter");
    return JString();
  }

  MYSQL_RES* res;
  JString query = "SELECT * FROM guilds WHERE guildname='" + guildname + "' ORDER BY rank DESC";
  if (Length(likestr)>0 || Length(wherestr)>0)
    query << " WHERE";
  if (Length(likestr)>0)
    query << " accname like '" << escaped39(likestr) << "'";
  if (Length(wherestr)>0) {
    if (Length(likestr)>0) query << " and";
    query << " " << wherestr;
  }

  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get accounts list: ") << mysql_error(mysql);
    return JString();
  }

  JString str,str2;
  MYSQL_ROW row;
  while ((row=mysql_fetch_row(res))) {
    str2 = row[0];
    str << "\nj" << str2;
  }
  mysql_free_result(res);
  str2 = "Guild Members of " + guildname + ":";
  str2 << str;
  return str2;
}


JString GetProfile(const JString& accname) {
  JString emptyres = JString((char)(Length(accname)+32)) << accname << "         ";
  dberror = "";
  if (Length(accname)<=0) {
    dberror = JString("Couldn't get profile: empty account name");
    return emptyres;
  }

  connecttodb();
  if (!sqlok) return emptyres;

  MYSQL_RES* res;
  JString query = "SELECT * FROM profiles WHERE accname='"+escaped39(accname)+"'";
  if(mysql_query(mysql,query.text()) || !(res=mysql_store_result(mysql))) {
    dberror = JString("Couldn't get profile: ") << mysql_error(mysql);
    return emptyres;
  }

  JString str;
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row) {
    for (int i=0; i<10; i++) {
      JString str2 = row[i];
      str << (char)(Length(str2)+32) << str2;
    }
  } else {
    str = emptyres;
  }
  mysql_free_result(res);
  return str;
}

void SetProfile(JString data, const JString& setteraccname, int setteradminlevel) {
  JString accname;
  dberror = "Couldn't set profile: wrong data";

  TJStringList* list = new TJStringList();
  list->SetPlainMem();
  for (int i=0; i<10; i++) {
    if (Length(data)<1) { delete(list); return; }
    int len = data[1]-32; 
    if (len<0) { delete(list); return; } 
    list->Add(Trim(Copy(data,2,len)));
    data = Copy(data,2+len,Length(data)-1-len); 
  }
  if (list->count<10) { delete(list); return; } 
  accname = (*list)[0];
  if (setteraccname!=accname && setteradminlevel<3) { 
    dberror = JString("Couldn't set profile: permission denied");
    delete(list); 
    return; 
  }

  dberror = "";
  connecttodb();
  if (!sqlok) { delete(list); return; }

  JString query = "INSERT INTO profiles (accname) VALUES('"+escaped39(accname)+"')";
  mysql_query(mysql,query.text());

  query = "UPDATE profiles SET";
  query << " realname='" << escaped39((*list)[1]) << "',";
  query << " age=" << atoi((*list)[2].text()) << ",";
  query << " sex='" << escaped39((*list)[3]) << "',";
  query << " country='" << escaped39((*list)[4]) << "',";
  query << " icq='" << escaped39((*list)[5]) << "',";
  query << " email='" << escaped39((*list)[6]) << "',";
  query << " webpage='" << escaped39((*list)[7]) << "',";
  query << " favhangout='" << escaped39((*list)[8]) << "',";
  query << " favquote='" << escaped39((*list)[9]) << "'";
  query << " WHERE accname='" << escaped39(accname) << "'";

  if (mysql_query(mysql,query.text()))
    dberror = JString("Couldn't set profile: ") << mysql_error(mysql);
  delete(list);
}

void UpdateStats(JString ouser,JString tuser,JString online,JString muser) {
  dberror = "";
  connecttodb();
  if (!sqlok) { return; }

  JString query = "UPDATE stats SET";
  query << " ouser='" << escaped39(ouser) << "',";
  query << " tuser=" << escaped39(tuser) << ",";
  query << " online='" << escaped39(online) << "',";
  query << " muser='" << escaped39(muser) << "';";
  if (mysql_query(mysql,query.text()))
    dberror = JString("Couldn't update statistics: ") << mysql_error(mysql);
}
