#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#include "utils.h"

using namespace std;

#include <string>
#include <vector>
#include <list>

/*
    In sunday algo., next pos has rules like : 
        If exist in @pPattern, moveBytes = @patternLen - rightPosOfThisChar;
        else, moveBytes = @patternLen + 1;
*/
int sundaySearchGenNextTable(const unsigned char * pPattern, const unsigned int patternLen,
    char sundayTable[SUNDAY_SEARCH_TABLE_LEN])
{
    if(NULL == pPattern)
    {
        dbgError("Input param is NULL.\n");
        return -1;
    }
    unsigned int i = 0;
    for(i = 0; i < SUNDAY_SEARCH_TABLE_LEN; i++)
    {
        sundayTable[i] = patternLen + 1;
    }

    for(i = 0; i < patternLen; i++)
    {
        sundayTable[pPattern[i]] = patternLen - i;
    }

    return 0;
}

/*
    return :
        0+:find the postion, ret is the position;
        0-:param error, or donot find it.
*/
int sundaySearch(const unsigned char * pSrc, const unsigned int srcLen, 
    const unsigned char * pPattern, const unsigned int patternLen, char sundayTable[SUNDAY_SEARCH_TABLE_LEN])
{
    if(NULL == pSrc || NULL == pPattern)
    {
        dbgError("Input param is NULL!\n");
        return -1;
    }

    if(srcLen < patternLen)
    {
        dbgError("Input srcLen = %d, patternLen = %d, cannot find pattern surely.\n",
            srcLen, patternLen);
        return -2;
    }

    if(0 == patternLen || 0 == srcLen)
    {
        dbgError("patternLen = %d, srcLen = %d, they all cannot be 0!\n",
            patternLen, srcLen);
        return -3;
    }

    //Start matching
    bool isPatFind = false;
    unsigned int i = 0;
    for(i = 0; i <= srcLen - patternLen; )
    {
        bool isMatched = true;
        unsigned int j = 0;
        for(j = 0; j < patternLen; j++)
        {
            //pattern donot matched
            if(pSrc[i + j] != pPattern[j])
            {
                isMatched = false;
                //Should jump several bytes, append on @pNext
                if(i + patternLen < srcLen) //Must cmp with @srcLen!
                {
                    i += sundayTable[pSrc[i + patternLen]];
                    break;
                }
                //To the end of @pSrc, donot find @pPattern yet
                else
                {
                    //This increase is meaningless, just can break forLoop to i
                    i++;
                }
            }
        }
        //If we matched pattern, break is OK
        if(isMatched)
        {
            isPatFind = true;
            break;
        }
    }

    if(isPatFind)
    {
        dbgDebug("Pattern being find! offset = %d\n", i);
        return i;
    }
    else
    {
        dbgWarn("Pattern donot being find!\n");
        return -4;
    }
}

/* return 0 if succeed, else 0-; */
int getFileSize(string & filepath, unsigned int & filesize)
{
    struct stat curStat;
    int ret = stat(filepath.c_str(), &curStat);
    if(ret != 0)
    {
        dbgError("stat failed! filepath=[%s], ret=%d, errno=%d, desc=[%s]\n", filepath.c_str(), ret, errno, strerror(errno));
        return -1;
    }
    filesize = curStat.st_size;
    dbgDebug("file [%s] has size %uBytes\n", filepath.c_str(), filesize);
    return 0;
}

/**
 *@fn                fsCfgCrc32
 *@brief             计算32位冗余校验
 *@param[in] pSrc:   需要校验的字节数组
 *@param[in] nSize:  @pSrc的字节数
 *@return:           校验值
 */
fsUint32 fsCfgCrc32(const fsUchar* pSrc, fsInt32 nSize)
{
    fsInt32 i = 0, j = 0, nIdx = 0;
    fsUint32 byte = 0, crc = 0, mask = 0;
    static fsUint32 table[256] = {0};

    /**
     * 第一次运算需要初始化查询表
     */
    if (table[1] == 0)
    {
        for (byte = 0; byte <= 255; byte++)
        {
            crc = byte;
            for (j = 7; j >= 0; j--)
            {
                mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            table[byte] = crc;
        }
    }

    /** 查询表初始化完毕, 计算CRC. */
    i = 0;
    crc = 0xFFFFFFFF;

    for(nIdx = 0; nIdx < nSize; nIdx++)
    {
        crc = (crc >> 8) ^ table[(crc ^ pSrc[nIdx]) & 0xFF];
        i = i + 1;
    }

    return ~crc;
}

fsUint32 fsCalcCheckSum(const fsChar * data, fsInt32 dataLen, fsInt32 skipBytes)
{
    fsUint32 result = 0;
    fsInt32 i = 0;

    /**
     * 入参检测
     */
    //VMFS3_CHECK_NULL(data, VMFS3_EN_PARAM, result, fsDbgAssert(0));

    for(i = 0; i < dataLen; i+=skipBytes)
    {
        result += data[i];
    }

    return result;
}

/*
    Input @timestamp, output localtime like:2019/05/16 - 20:25:36;
*/
string convTimestampToLocalTime(const unsigned int timestamp)
{
    struct tm localTm;
    localtime_r((time_t *)&timestamp, &localTm);
    char temp[32] = {0x00};
    sprintf(temp, "%04d/%02d/%02d - %02d:%02d:%02d", localTm.tm_year + 1900, localTm.tm_mon + 1,
        localTm.tm_mday, localTm.tm_hour, localTm.tm_min, localTm.tm_sec);
    dbgDebug("localTime is : [%s]\n", temp);

    string ret = temp;
    return ret;
}


