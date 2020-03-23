#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

#include <string>

#define LOG_TO_STDOUT   0
#define LOG_TO_FILE     1
//#define LOG_TARGET      LOG_TO_FILE
#define LOG_TARGET      LOG_TO_STDOUT

#if LOG_TARGET  //log output to file

#else
#define dbgDebug(format, ...) printf("DEBUG-[%s, %d] : " format, __FUNCTION__, __LINE__, ##__VA_ARGS/*__)
#define dbgWarn(format, ...) printf("WARN-[%s, %d] : " format, __FUNCTION__, __LINE__, ##__VA_ARGS__*/)
#define dbgDebug(format, ...)
#define dbgWarn(format, ...)
#define dbgInfo(format, ...) printf("INFO-[%s, %d] : " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define dbgError(format, ...) printf("ERROR-[%s, %d] : " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#define MY_AUTHOR_INFO "->->->->Author:EricWu; Email:wujl_0351@163.com; Bug or Feature to this Email is OK. I will modify or add when I donot busy."

#define BSFP_HEAD_LENGTH                28
#define BSFP_MARK                       (0xbf9d1fdb)
#define SUNDAY_SEARCH_TABLE_LEN         256
#define MAX_FILE_PATH                   (256)

typedef unsigned char fsUchar;
typedef unsigned short int fsUint16;
typedef unsigned int fsUint32;
typedef unsigned long long int fsUint64;

typedef char fsChar;
typedef short int fsInt16;
typedef int fsInt32;
typedef long long int fsInt64;

typedef size_t fsSizeT;
#if defined(Q_OS_WIN64)
typedef unsigned long fsSsizeT;
#else
typedef ssize_t fsSsizeT;
#endif
typedef int64_t fsOff64T;
typedef time_t fsTimeT;
typedef struct stat fsStat;

#define VMFS3_0    (0LLU)
#define VMFS3_KB   (1LLU * 1024LLU)
#define VMFS3_MB   (1LLU * 1024LLU * VMFS3_KB)
#define VMFS3_GB   (1LLU * 1024LLU * VMFS3_MB)
#define VMFS3_TB   (1LLU * 1024LLU * VMFS3_GB)

int sundaySearchGenNextTable(const unsigned char * pPattern, const unsigned int patternLen, char sundayTable[SUNDAY_SEARCH_TABLE_LEN]);

int sundaySearch(const unsigned char * pSrc, const unsigned int srcLen, 
    const unsigned char * pPattern, const unsigned int patternLen, char sundayTable[SUNDAY_SEARCH_TABLE_LEN]);

int getFileSize(string & filepath, unsigned int & filesize);

fsUint32 fsCfgCrc32(const fsUchar* pSrc, fsInt32 nSize);

fsUint32 fsCalcCheckSum(const fsChar * data, fsInt32 dataLen, fsInt32 skipBytes);

string convTimestampToLocalTime(const unsigned int timestamp);

#endif

