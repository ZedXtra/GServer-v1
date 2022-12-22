#ifndef CFG
#define CFG
struct CfgStrings
{
    char * name ;
    char * data ;
};

int CfgRead( char * Filename, struct CfgStrings * CfgInfo );
#endif
