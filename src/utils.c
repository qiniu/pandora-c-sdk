#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

char *pandora_strdup(const char *src)
{
    char *dest;

    if (src == NULL)
        return NULL;

    dest = malloc(sizeof (char) * (strlen(src) + 1));
    if (dest == NULL)
        return NULL;

    strcpy(dest, src);
    return dest;
}

char *current_gmt()
{
    time_t rawtime;
    time (&rawtime);
    struct tm *ptm = gmtime (&rawtime);
    char *buf = malloc(30);
    strftime(buf, 30, "%a, %d %b %Y %H:%M:%S GMT", ptm);
    return buf;
}


