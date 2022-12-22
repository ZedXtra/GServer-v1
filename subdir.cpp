//
// Sub-directory's support :D
//

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef SUB_C
#define SUB_C
extern TJStringList* subdirs;
extern const char fileseparator;

void RefindAllSubdirs(JString path)
{
  DIR *dir;
  JString search;
  struct stat statx;
  struct dirent *ent;

  if ((dir = opendir(path.text())) == NULL) return;
  while ((ent = readdir(dir)) != NULL) {
    search = path + fileseparator + ent->d_name;
    if (ent->d_name[0] != '.') {
      stat(search.text(), &statx);
      if (statx.st_mode & S_IFDIR) {
        subdirs->Add(search);
        RefindAllSubdirs(search);
      }
    }
  }
  closedir(dir);
}

bool CheckPartofString(JString str, JString str2)
{
  int i;
  if (Length(str) < Length(str2)) return false;
  for (i=1; i<=Length(str2); i++) {
    if (str2[i] != str[i]) return false;
  }
  return true;
}
#endif
