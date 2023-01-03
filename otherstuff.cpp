#include "includes.h"
#include "gserver.h"
#include "otherstuff.h"

using namespace std;

// ADDED: Sub-directory support (DONT REMOVE!)
#include "subdir.cpp"

// --- TServerAccount

TServerAccount::TServerAccount() {
  caccounts++;
  blocked = false;
  onlyload = false;
  adminlevel = 0;
}

void ImportWeapons(const char* loadfile) {
	//Works only once her server restart.
  unsigned int curtime = time(NULL);
  TJStringList* list = new TJStringList();
  list->LoadFromFile(ServerFrame->getDir() << "config/" << loadfile);
  
cout << loadfile << endl;
  int k;
  int z;
  for (int i=0; i<list->count; i++) {
    JString str,name,world,image;
    int j,proptype,len;
    str << (*list)[i];

    j = 1;
    if (Length(str)<j) continue;
    len = str[j]-32;
    if (len<=0 || Length(str)<=1+len) continue;
    name = Trim(Copy(str,j+1,len));
    if (!iscorrect(name) || Pos('}',name)>0) continue;
    j = j+1+len;
    if (Length(str)<j+1) continue;
    proptype = str[j]-32;
    if (proptype!=NPCGIF) continue;
    len = str[j+1]-32;
    if (len<=0 || len>100) continue;
    image = Copy(str,j+2,len);

    k = LastDelimiter("§",name);
//cout << "[Debug: k]" << k << endl;
    if (k>1) {
      world = Copy(name,1,k);
//cout << "[Debug: World]" << world << endl;
      name = Copy(name,k+1,Length(name)-k);
//cout << "[Debug: Name]" << name << endl;
    }

    TServerWeapon* weapon;
    bool found = false;
    for (z=0; z<weapons->count; z++) {
      weapon = (TServerWeapon*)(*weapons)[z];
      if (name==weapon->name && world==weapon->world) {
        found = true;
        break;
      }
    }
    if (found) continue;

cout << "[Debug: continue]" << endl;
    weapon = new TServerWeapon();
    weapon->name = name;
    weapon->image = image;
    weapon->world = world;
    weapon->modtime = curtime;
    weapon->fullstr = str;
    //cout << "[Debug: weapon]" << "Weapon added:" << weapon->name << endl;
    //cout << "[Debug: weapon]" << weapon->image << endl;
    //cout << "[Debug: weapon]" << weapon->world << endl;
    //cout << "[Debug: weapon]" << weapon->fullstr << endl;
    //cout << "[Debug: weapon]" << weapon->dataforplayer << endl;
    if (Length(world)>0) {
      weapon->dataforplayer = JString((char)(Length(name)+32)) << name
        << Copy(str,j,Length(str)-j+1);
	} 
	//Write weapon to its own text file.
	TJStringList* writelist = new TJStringList();
	writelist->AddLine(str);
	writelist->SaveToFile(ServerFrame->getDir() << "config/weapons/" << name << ".txt");
	delete(writelist);
	CreateNewDBWeapon(name, image, world, curtime); //Add weapon to database!
		
    weapons->Add(weapon);
  }
  delete(list);
  
  
  
}




void LoadWeapons() {
	//Load weapons from the database.
	for (int i=0; i<CountDBTable("weapons"); i++) {
		std::cout << i << std::endl;
		LoadDBWeapons(i);
	}
}

// Save the weapons list to weapons.txt (just take the fullstr
// attribute of the weapons)
void SaveWeapons() {
  TJStringList* list = new TJStringList();
  for (int i=0; i<weapons->count; i++) {
    TServerWeapon* weapon = (TServerWeapon*)(*weapons)[i];
    if (Assigned(weapon)) list->Add(weapon->fullstr);
  }
  list->SaveToFile(ServerFrame->getDir() << "config/weapons.txt");
  delete(list);
}


bool LevelIsJail(const JString& str) {
  if (str=="police2.graal" || str=="police4.graal" ||
    str=="police5.graal") return true;
  return false;
}

void ToWaypointLevel(const JString& accname, JString waypoint_level, double waypoint_x, double waypoint_y) {
	if (Length(accname)<=0) return;

	TServerPlayer* player2 = ServerFrame->GetPlayerForAccount(accname);
	if (Assigned(player2) && !player2->isrc && Length(player2->playerworld)>0 && !LevelIsJail(LowerCaseFilename(player2->levelname))) 
	player2->gotoNewWorld("",startlevel,startx,starty);

	TServerPlayer* player = new TServerPlayer(NULL,0);
	LoadDBAccount(player,accname,"");
	if (!LevelIsJail(LowerCaseFilename(player->levelname))) {
		player->levelname = waypoint_level; 
		player->x = waypoint_x; 
		player->y = waypoint_y; 
		player->status = (player->status | 16); 
		if (Assigned(player2)) {
			player2->ApplyAccountChange(player,true);
		} else if (Pos('#',accname)!=1) {
			SaveDBAccount(player);
		}
	}
	delete(player);
} 

void ToStartLevel(const JString& accname) {
	if (Length(accname)<=0) return;

	TServerPlayer* player2 = ServerFrame->GetPlayerForAccount(accname);
	if (Assigned(player2) && !player2->isrc && Length(player2->playerworld)>0 && !LevelIsJail(LowerCaseFilename(player2->levelname))) 
	player2->gotoNewWorld("",startlevel,startx,starty);

	TServerPlayer* player = new TServerPlayer(NULL,0);
	LoadDBAccount(player,accname,"");
	if (!LevelIsJail(LowerCaseFilename(player->levelname))) {
		player->levelname = startlevel; 
		player->x = startx; 
		player->y = starty; 
		player->status = (player->status | 16); 
		if (Assigned(player2)) {
			player2->ApplyAccountChange(player,true);
		} else if (Pos('#',accname)!=1) {
			SaveDBAccount(player);
		}
	}
	delete(player);
} 
// Modified to use Mysql ftw!
extern bool IsInGuild(const JString& accname,const JString& name,const JString& guild);
bool VerifyGuildName(JString guild, const JString& nick, const JString& accountname) {

  if (Length(guild)<1) return false;
  while (true) {
    int i = Pos(' ',guild);
    if (i<=0) break; 
    guild = Copy(guild,1,i-1) << "_" << Copy(guild,i+1,Length(guild)-i); 
  } 
  if (IsInGuild(accountname,nick,guild)) return true;
  else return false; 
} 

bool IsIPInList(TSocket* sock, const JString& accname, const JString& listfile) {
  if (!Assigned(sock)) return false;

  TJStringList* list = new TJStringList();
  list->SetPlainMem();
  list->LoadFromFile(listfile);
  for (int j=0; j<list->count; j++) {
    JString str = (*list)[j];
    if (Length(accname)>0) {
      if (Pos(accname+":",str)!=1) continue;
      str = Copy(str,Length(accname)+2,Length(str)-Length(accname)-1);
    }

    int i = Pos('.',str);
    if (i<2) continue;
    JString part1 = Copy(str,1,i-1);
    str = Copy(str,i+1,Length(str)-i);
    i = Pos('.',str);
    if (i<2) continue;
    JString part2 = Copy(str,1,i-1);
    str = Copy(str,i+1,Length(str)-i);
    i = Pos('.',str);
    if (i<2) continue;
    JString part3 = Copy(str,1,i-1);
    JString part4 = Copy(str,i+1,Length(str)-i);

    if ((part1=="*" || strtoint(part1)==( sock->ip        & 0xFF)) &&
        (part2=="*" || strtoint(part2)==((sock->ip >>  8) & 0xFF)) &&
        (part3=="*" || strtoint(part3)==((sock->ip >> 16) & 0xFF)) &&
        (part4=="*" || strtoint(part4)==((sock->ip >> 24) & 0xFF))) {
      delete(list);
      return true;
    }
  }
  delete(list);
  return false;
}

JString CompressBuf(const JString& arr) { 
  JString str;

  cout << "[OUT]: " << arr << endl;

  if (Length(arr)<=0) return str;

  unsigned long clen = 200000;
  int err = compress((unsigned char*)filesbuf,&clen,(unsigned 
char*)arr.text(),arr.length());
  if (err!=Z_OK) return str;

  if (clen>0 && clen<=200000) str.addbuffer(filesbuf,clen);
  return str;
}

JString DecompressBuf(const JString& arr) {
  JString str;

  if (Length(arr)<=0) return str;

  unsigned long clen = 200000;
  int err = uncompress((unsigned char*)filesbuf,&clen,(unsigned 
char*)arr.text(),arr.length());

  if (err!=Z_OK) {
    ServerFrame->toerrorlog(JString("Decompress error: ") << err);
    return str;
  }

  if (clen>0 && clen<=200000) str.addbuffer(filesbuf,clen);

  cout << "[IN]: " << str << endl;

  return str;
}



