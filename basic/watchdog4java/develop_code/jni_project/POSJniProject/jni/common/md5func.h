#ifndef MD5_FUNC_H
#define MD5_FUNC_H

/*
 * Calculate the md5 value of the file 
 *
 * @param: file_path  the path of special file which you want to make md5 value
 * @param: md5str     the md5 value will be store in there 
 *                    which length must be greater than 33 bytes
 */
void md5_file(const char *file_path, char *md5str);

#endif

