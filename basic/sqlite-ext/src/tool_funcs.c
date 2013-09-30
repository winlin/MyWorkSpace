#include "tool_funcs.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

char *strip_string_space(char *tar_str)
{
    // strip left
    while (isspace(*tar_str))
        ++tar_str;
    
    // strip right
    char *str_end = strchr(tar_str, '\0');
    while (--str_end > tar_str && isspace(*str_end))
        *str_end = '\0';
    
    return tar_str;
}

char *strip_string_lspace(char *tar_str)
{
    // strip left
    while (isspace(*tar_str)) 
        ++tar_str;
    
    return (char *)tar_str;
}

char *strip_string_rspace(char *tar_str)
{
    // strip right
    char *str_end = strchr(tar_str, '\0');
    while (--str_end > tar_str && isspace(*str_end)) 
        *str_end = '\0';
    
    return tar_str;
}

char *strip_sql_rspace(char *tar_str)
{
    while (1) {
        char *strip_rspace = strip_string_rspace(tar_str);
        char *last_semi = strrchr(strip_rspace, ';');
        if (last_semi) {
            memset(last_semi+1, 0, strlen(last_semi)-1);
            char *tmp = last_semi;
            char comment_flag = 0;
            while (--tmp > strip_rspace && *tmp != '\n') {
                if (*tmp == '-' &&
                    *(tmp-1) == '-') {
                    comment_flag = 1;
                    memset(tmp, 0, strlen(tmp));
                    break;
                }
            }
            if (comment_flag == 0) {
                strip_string_rspace(tar_str);
                return tar_str;
            }
        } else {
            memset(tar_str, 0, strlen(tar_str));
            return tar_str;
        }
    }
}















