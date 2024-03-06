#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//logs an error message and a corresponding error number if one is given. 
void logError(char const* errMsg, int errorNumber)
{
    char errNumStr[256] = {0};

    if(errorNumber != 0)
    {
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
            errorNumber, 0, errNumStr, sizeof(errNumStr), NULL); 
    }
    
    time_t t;
    time(&t);
    char timeBuff[64] = {0};
    ctime_s(timeBuff, sizeof(timeBuff), &t);

    FILE* fileStream;
    errno_t err = fopen_s(&fileStream, "errorLog.txt", "a");
    if(err)
    {
        fprintf(stderr, "error trying to open errorLog.txt\nerror message that would have been logged: ");
        if(errNumStr) fprintf(stderr, "%s (%s) at %s", errMsg, errNumStr, timeBuff);
        else fprintf(stderr, "%s at ", timeBuff);
    }
    else
    {
        if(errorNumber != 0)
        {
            fprintf(fileStream, "\n\n%s errnum=%d %s at %s\n\n", 
                errMsg, errorNumber, errNumStr, timeBuff);
        }
        else 
        {
            fprintf(fileStream, "\n\n%s errnum=%d at %s\n\n", 
                errMsg, errorNumber, timeBuff);
        }

        fclose(fileStream);
    }  
}