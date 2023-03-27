#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

//logs an error message and a corresponding errno if one is given. 
void logError(char const* errMsg, int errorNumber)
{
    char const* errStr = (errorNumber != 0) ? strerror(errorNumber) : NULL;
    
    FILE* fp = fopen("errorLog.txt", "w");  
    if(!fp) 
    {
        fprintf(stderr, "error trying to open errorLog.txt\nerror message that would have been logged: ");
        time_t time;
        fprintf(stderr, "%s (%s) at %s", errMsg, errStr, ctime(&time));
    }
    
    time_t time;
    fprintf(fp, "%s (%s) at %s", errMsg, errStr, ctime(&time));

    fclose(fp);
}