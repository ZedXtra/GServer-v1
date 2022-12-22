#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cfg.h"

#define LINE_LEN_MAX    128                  /* actual max line length  */
#define BUFFERSIZE      LINE_LEN_MAX +2      /* ... including \n and \0 */

enum RetVal
{
    NO_PROBLEMS,
    ERR_FOPEN,
    ERR_MEM,
};

int CfgRead( char * Filename, struct CfgStrings * CfgInfo )
{
    char Buffer[ BUFFERSIZE ];
    char * WorkPtr ;
    char * CfgName;
    char * CfgData ;
    struct CfgStrings * Cfg ;
    FILE * CfgFile ;

    CfgFile = fopen( Filename, "r" );
    if( CfgFile == NULL)
    {
    	printf("CfgRead(): Unable to open %s for reading!\n",Filename);
        return ERR_FOPEN ;
    }

    while( fgets( Buffer, BUFFERSIZE, CfgFile ) != NULL)
    {
        /* clip off optional comment tail indicated by a semi-colon
        */
        if((WorkPtr = strchr( Buffer, ';' )) != NULL) *WorkPtr = '\0';
        else WorkPtr = Buffer + strlen( Buffer );

        /* clip off trailing and leading white space
        */
        WorkPtr--;
        while( isspace( *WorkPtr ) && WorkPtr >= Buffer ) *WorkPtr-- = '\0';
        WorkPtr = Buffer;
        while( isspace( *WorkPtr ))  WorkPtr++;
        if( 0 == strlen( WorkPtr ))  continue;

        CfgName = strtok( WorkPtr, " =" );
        if( NULL != CfgName )
        {
            /* Condition the name (lower case required),
               and strip leading white and a 'late' = from data part.
            */
            for(int i=0; i<strlen(CfgName); ++i)
               CfgName[i]=tolower(CfgName[i]);

            CfgData = strtok( NULL, "" );
            while( isspace( *CfgData ))
                CfgData++;
            if( '=' == *CfgData )
                CfgData++;
            while( isspace( *CfgData ))
                CfgData++;

            /* look for matching 'name'
            */
            Cfg = CfgInfo ;
            while( NULL != Cfg->name && 0 != strcmp( Cfg->name, CfgName ))
                Cfg++;

            /* duplicate the data if the name is found.
            */
            if( NULL != Cfg->name )
            {
                Cfg->data = strdup( CfgData ); /* strdup is not ANSI    */
                                           /* memory leaks if Cfg->data */
                                           /* is malloc'ed already      */
                if( NULL == Cfg->data )
                {
                    fclose( CfgFile );
                    return ERR_MEM;
                }
            }   /* undetected error on failure should not be a problem  */
                /* as configuration reading should be done early.       */
        }       /* but test and handle it anyway ...                    */
    }
    fclose( CfgFile );
    return NO_PROBLEMS ;
}
