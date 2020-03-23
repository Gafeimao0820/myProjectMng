#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "utils.h"
#include "clipParser.h"

using namespace std;

#include <map>
#include <vector>
#include <string>
#include <array>

ClipParser::ClipParser() : mFp(NULL), mClipFilepath("")
{
    memset(&mClipHeader, 0x00, sizeof(FsClipHead));
    memset(&mKeyFrameHeader, 0x00, sizeof(FsKeyFramHead));
    memset(&mClipJournal, 0X00, sizeof(FsClipJournalArea));
    mKeyFrameIndexInfoMap.clear();

    int i = 0;  for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)    memset(&mKeyFrameIndexArray[i], 0x00, sizeof(FsKeyFramInfoIdx));

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    mErrStrMap.clear();
}

ClipParser::ClipParser(const string & filepath) : mFp(NULL), mClipFilepath(filepath)
{
    memset(&mClipHeader, 0x00, sizeof(FsClipHead));
    memset(&mKeyFrameHeader, 0x00, sizeof(FsKeyFramHead));
    memset(&mClipJournal, 0X00, sizeof(FsClipJournalArea));
    mKeyFrameIndexInfoMap.clear();

    int i = 0;  for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)    memset(&mKeyFrameIndexArray[i], 0x00, sizeof(FsKeyFramInfoIdx));

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    mErrStrMap.clear();
}

ClipParser::ClipParser(const ClipParser & other)
{
    mClipFilepath = other.mClipFilepath;

    memcpy(&mClipHeader, &other.mClipHeader, sizeof(FsClipHead));
    memcpy(&mKeyFrameHeader, &other.mKeyFrameHeader, sizeof(FsKeyFramHead));
    memcpy(&mClipJournal, &other.mClipJournal, sizeof(FsClipJournalArea));

    mKeyFrameIndexArray = other.mKeyFrameIndexArray;
    mKeyFrameIndexInfoMap = other.mKeyFrameIndexInfoMap;

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    mErrStrMap.clear();

    //mFp set to NULL, we will reParse them when needed
    mFp = NULL;
}


ClipParser::~ClipParser()
{
    mKeyFrameIndexInfoMap.clear();

    if(mFp)
    {
        fclose(mFp);
        mFp = NULL;
    }
}

int ClipParser::resetFilepath(const string & filepath)
{
    if(mFp != NULL) {fclose(mFp);   mFp = NULL;}

    dbgDebug("change filepath: mFilepath=[%s], filepath=[%s]\n", mClipFilepath.c_str(), filepath.c_str());
    if(filepath != mClipFilepath)   mClipFilepath = filepath;
    mKeyFrameIndexInfoMap.clear();
    return 0;
}

int ClipParser::parse()
{
    if(mFp != NULL) {dbgInfo("mFp != NULL, has parse once time, will not parse again.\n");  mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;    return 0;}

    dbgDebug("start to parse clipfile [%s] now.\n", mClipFilepath.c_str());
    mFp = fopen(mClipFilepath.c_str(), "rb");
    if(mFp == NULL) {dbgError("open file [%s] for read failed! errno=%d, desc=[%s]\n", mClipFilepath.c_str(), errno, strerror(errno));  mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;   return -1;}
    dbgDebug("parse step 1 >>>>>> open file [%s] for read succeed.\n", mClipFilepath.c_str());

    //read journal firstly
    int ret = fseek(mFp, FS_CLIP_JOURNAL_POS, SEEK_SET);    if(ret != 0){dbgError("seek to JournalAreafailed! ret=%d, errno=%d, desc=[%s]\n", ret, errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;   return -2;}
    ret = fread((char *)&mClipJournal, 1, sizeof(FsClipJournalArea), mFp);
    if(ret != sizeof(FsClipJournalArea))    {dbgError("read clip journal failed! readLen=%d, journalAreaSize=%d, errno=%d, desc=[%s]\n", ret, sizeof(FsClipJournalArea), errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READJOURNAL_FAILED;    return -3;}
    //check journal area valid or not, include mark check and checksum check;
    if(mClipJournal.mark != FS_JOURNAL_MARK)    {dbgError("the mark being read from journal of this clip is 0x%x, FS_JOURNAL_MARK=0x%x, donot equal! parse failed!\n", mClipJournal.mark, FS_JOURNAL_MARK); fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_CHECKJOURNAL_FAILED;   return -4;}
    fsUint32 checkSumValue = fsCalcCheckSum((char*)&mClipJournal + sizeof(fsUint32), sizeof(FsClipJournalArea) - sizeof(fsUint32), 1);
    if(checkSumValue != mClipJournal.checkSum)  {dbgError("JournalArea checksum check failed! checkSumInClipJournal=%u, checkSumBeingCalc=%u, donot equal.! parse failed!\n", mClipJournal.checkSum, checkSumValue); fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_CHECKJOURNAL_FAILED;   return -5;}
    dbgDebug("parse step 2 >>>>>> journal area being read and checked succeed.\n");

    //read clip header now.
    ret = fseek(mFp, FS_CLIP_HEAD_POS, SEEK_SET);   if(ret != 0){dbgError("seek to ClipHeader failed! ret=%d, errno=%d, desc=[%s]\n", ret, errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;   return -6;}
    ret = fread((char *)&mClipHeader, 1, sizeof(FsClipHead), mFp);  if(ret != sizeof(FsClipHead)){dbgError("read clip header failed! readLen=%d, clipHeaderSize=%d, errno=%d, desc=[%s]\n", ret, sizeof(FsClipHead), errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READHEAD_FAILED;   return -7;}
    //check the header valid or not, checksum being checked here
    checkSumValue = fsCalcCheckSum((char *)&mClipHeader + sizeof(fsUint32), sizeof(FsClipHead) - sizeof(fsUint32), 1);
    if(checkSumValue != mClipHeader.checkSum){dbgError("ClipHeader checksum check failed! checkSumInClipHeader=%u, checkSumBeingCalc=%u, donot equal. parse failed!\n", mClipHeader.checkSum, checkSumValue);   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_HEADERCHECK_FAILED;    return -8;}
    dbgDebug("parse step 3 >>>>>> clip header being read and checked succeed.\n");

    //read keyframe header now.
    ret = fseek(mFp, FS_CLIP_KEYFRAM_HEAD_POS, SEEK_SET);   if(ret != 0){dbgError("seek to keyFrameHeader failed! ret=%d, errno=%d, desc=[%s]\n", ret, errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;   return -9;}
    ret = fread((char *)&mKeyFrameHeader, 1, sizeof(FsKeyFramHead), mFp);   if(ret != sizeof(FsKeyFramHead)){dbgError("read keyFrameHeader failed! readLen=%d, keyFrameHeaderSize=%d, errno=%d, desc=[%s]\n", ret, sizeof(FsKeyFramHead), errno, strerror(errno));  fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEHEADER_FAILED;    return -10;}
    //check keyFrameHeader, only checksum value being checked
    checkSumValue = fsCalcCheckSum((char *)&mKeyFrameHeader + sizeof(fsUint32), sizeof(FsKeyFramHead) - sizeof(fsUint32), 1);
    if(checkSumValue != mKeyFrameHeader.checkSum)   {dbgError("KeyFrameHeader checksum check failed! checkSumInClip=%u, checkSumBeingCalc=%u, donot equal! parse failed!\n", mKeyFrameHeader.checkSum, checkSumValue);    fclose(mFp);    mFp = NULL; mErrno =VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEHEADER_FAILED;    return -11;}
    dbgDebug("parse step 4 >>>>>> Key frame header being read and checked succeed.\n");

    //read keyframe index now.
    ret = fseek(mFp, FS_CLIP_KEYFRAM_IDX_POS, SEEK_SET);    if(ret != 0){dbgError("seek to keyFrameIndex failed! ret=%d, errno=%d, desc=[%s]\n", ret, errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;   return -12;}
    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        ret = fread((char *)&mKeyFrameIndexArray[i], 1, sizeof(FsKeyFramInfoIdx), mFp);
        if(ret != sizeof(FsKeyFramInfoIdx)) {dbgError("read KeyframeIndex failed! i=%d, readLen=%d, keyFrameIndexSize=%d, errno=%d, desc=[%s]\n", i, ret, sizeof(FsKeyFramInfoIdx), errno, strerror(errno));    fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINDEX_FAILED; return -13;}
        //check this keyFrameIndex
        checkSumValue = fsCalcCheckSum((char *)&mKeyFrameIndexArray[i] + sizeof(fsUint32), sizeof(FsKeyFramInfoIdx) - sizeof(fsUint32), 1);
        if(checkSumValue != mKeyFrameIndexArray[i].checkSum)    {dbgError("KeyFrameIndex checksum check failed! checkSumInClip=%u, checkSumBeingCalc=%u, donot equal! parse failed!\n", mKeyFrameIndexArray[i].checkSum, checkSumValue);    fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINDEX_FAILED;    return -14;}
        dbgDebug("i=%d, keyframeindex being read and checked succeed.\n", i);
    }
    dbgDebug("parse step 5 >>>>>> KeyFrameIndex being read and checked succeed.\n");

    //read keyFrameInfo now.
    ret = fseek(mFp, FS_CLIP_KEYFRAM_INFO_POS, SEEK_SET);   if(ret != 0){dbgError("seek to KeyFrameInfo failed! ret=%d, errno=%d, desc=[%s]\n", ret, errno, strerror(errno));   fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;   return -15;}
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        //each time, read 1000 keyframe info for one index
        vector<FsKeyFramInfo>   curKeyFrameInfoVec;
        FsKeyFramInfo curKeyInfo;
        int j = 0;
        for(j = 0; j < FS_CLIP_KEYFRAM_CNTPREIDXBLK; j++)
        {
            ret = fread((char *)&curKeyInfo, 1, sizeof(FsKeyFramInfo), mFp);
            if(ret != sizeof(FsKeyFramInfo))    {dbgError("read KeyFrameInfo failed! readLen=%d, FsKeyFrameInfoSize=%d, errno=%d, desc=[%s]\n", ret, sizeof(FsKeyFramInfo), errno, strerror(errno));    fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINFO_FAILED;  return -16;}
            checkSumValue = fsCalcCheckSum((char *)&curKeyInfo + sizeof(fsUint32), sizeof(FsKeyFramInfo) - sizeof(fsUint32), 1);
            if(checkSumValue != curKeyInfo.checkSum)    {dbgError("KeyFrameInfo checksum check failed! checkSumInClip=%u, checksumBeingCalc=%u, donot equal! parse failed!\n", curKeyInfo.checkSum, checkSumValue); fclose(mFp);    mFp = NULL; mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINFO_FAILED; return -17;}
            dbgDebug("i=%d, j=%d, keyFrameInfo being read and checked succeed.\n", i, j);
            //add this keyframe info into vector
            curKeyFrameInfoVec.push_back(curKeyInfo);
        }
        mKeyFrameIndexInfoMap.insert(pair<int, vector<FsKeyFramInfo>>(i, curKeyFrameInfoVec));
    }
    dbgDebug("parse step 6 >>>>>> KeyFrameInfo being read and checked succeed.\n");

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

int ClipParser::getErrno()
{
    return mErrno;
}

string & ClipParser::getErrStr(const int errNo)
{
    if(mErrStrMap.empty())
    {
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_OK, "No error."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT, "Input param is invalid."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT, "Input param is NULL."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED, "Open clip file for read failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED, "Seek in clip file failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READJOURNAL_FAILED, "read journal area failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_CHECKJOURNAL_FAILED, "check journal area failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READHEAD_FAILED, "read clip header failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_HEADERCHECK_FAILED, "check clip header failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEHEADER_FAILED, "read keyframe header failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEHEADER_FAILED, "check keyframe header failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINDEX_FAILED, "read key frame index failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINDEX_FAILED, "check key frame index failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINFO_FAILED, "read key frame info failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINFO_FAILED, "check key frame info failed."));
        mErrStrMap.insert(map<int, string>::value_type(VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAME_FAILED, "read key frame failed."));
    }

    string ret("Input err is invalid!");
    map<int, string>::iterator it = mErrStrMap.find(errNo);
    if(it != mErrStrMap.end())
    {
        ret = it->second;
    }
    return ret;
}

int ClipParser::getClipHeader(FsClipHead &clipHeader)
{
    memcpy(&clipHeader, &mClipHeader, sizeof(FsClipHead));
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

int ClipParser::getKeyFrameHeader(FsKeyFramHead &keyFrameHeader)
{
    memcpy(&keyFrameHeader, &mKeyFrameHeader, sizeof(FsKeyFramHead));
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

/* 
    get all key frame index info;
    param @keyFrameIndexArray must have enough memory to save all the info;
*/
#if 0
int ClipParser::getKeyFrameIndex(FsKeyFramInfoIdx keyFrameIndexArray[FS_CLIP_KEYFRAM_IDX_MAX_CNT])
{
    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        memcpy(&keyFrameIndexArray[i], &mKeyFrameIndexArray[i], sizeof(FsKeyFramInfoIdx));
    }
    return 0;
}
#else
int ClipParser::getKeyFrameIndex(array< FsKeyFramInfoIdx, FS_CLIP_KEYFRAM_IDX_MAX_CNT > & keyFrameIndexArray)
{
    keyFrameIndexArray = mKeyFrameIndexArray;
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
#endif

/*
    get one keyFrameIndex info which being defined by param @index;
    param @pKeyFrameIndexInfo must have enough memory to save one FsKeyFrameInfoIdx;
*/
int ClipParser::getKeyFrameIndex(const int index, FsKeyFramInfoIdx *pKeyFrameIndexInfo)
{
    if(index < 0 || index >= FS_CLIP_KEYFRAM_IDX_MAX_CNT)
    {
        dbgError("Input index=%d, valid range is [0, %d]\n", index, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }

    if(pKeyFrameIndexInfo == NULL)
    {
        dbgError("Input param is NULL.\n");
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT;
        return -2;
    }

    memcpy(pKeyFrameIndexInfo, &mKeyFrameIndexArray[index], sizeof(FsKeyFramInfoIdx));
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

/*
    get all keyFrameInfo which being pointed by @keyFrameIndex;
    param @keyFrameInfoArray must have enough space to save all of keyFrameInfo in a block;
*/
#if 0
int ClipParser::getKeyFrameInfo(const int keyFrameIndex, FsKeyFramInfo keyFrameInfoArray[FS_CLIP_KEYFRAM_CNTPREIDXBLK])
{
    if(keyFrameIndex < 0 || keyFrameIndex >= FS_CLIP_KEYFRAM_IDX_MAX_CNT)
    {
        dbgError("Input keyFrameIndex=%d, valid range is [0, %d]\n", keyFrameIndex, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }

    map<int, vector<FsKeyFramInfo>> it = mKeyFrameIndexInfoMap.find(keyFrameIndex);
    if(it == mKeyFrameIndexInfoMap.end())
    {
        dbgError("Input keyFrameIndex=%d, donot exist in mKeyFrameIndexInfoMap.\n", keyFrameIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -2;
    }
    vector<FsKeyFramInfo> infoVec = it->second;

    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_CNTPREIDXBLK; i++)
    {
        memcpy(&keyFrameInfoArray[i], &infoVec[i], sizeof(FsKeyFramInfo));
    }
    
    return 0;
}
#else
int ClipParser::getKeyFrameInfo(const int keyFrameIndex, vector < FsKeyFramInfo > & keyFrameInfoVec)
{
    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(keyFrameIndex);
    if(it == mKeyFrameIndexInfoMap.end())
    {
        dbgError("Input key frame index value is %d, cannot find it in mKeyFrameIndexInfoMap!\n", keyFrameIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }
    keyFrameInfoVec = it->second;
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
#endif

int ClipParser::getKeyFrameInfo(const int keyFrameIndex, const int keyIndex, FsKeyFramInfo * pKeyFrameInfo)
{
    if(keyFrameIndex < 0 || keyFrameIndex >= FS_CLIP_KEYFRAM_IDX_MAX_CNT)
    {
        dbgError("Input keyFrameIndex=%d, valid range is [0, %d]\n", keyFrameIndex, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }
    if(keyIndex < 0 || keyIndex >= FS_CLIP_KEYFRAM_CNTPREIDXBLK)
    {
        dbgError("Input keyIndex=%d, valid range is [0, %d]\n", keyIndex, FS_CLIP_KEYFRAM_CNTPREIDXBLK - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -2;
    }

    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(keyFrameIndex);
    if(it == mKeyFrameIndexInfoMap.end())
    {
        dbgError("Input keyFrameIndex=%d, donot exist in mKeyFrameIndexInfoMap.\n", keyFrameIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -3;
    }
    vector<FsKeyFramInfo> infoVec = mKeyFrameIndexInfoMap[keyFrameIndex];

    if(infoVec.size() <= keyIndex)
    {
        dbgError("infoVec.size=%d, keyIndex=%d, cannot get its value in this vector.\n", infoVec.size(), keyIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -4;
    }
    memcpy(pKeyFrameInfo, &infoVec[keyIndex], sizeof(FsKeyFramInfo));
    
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

int ClipParser::getJournal(FsClipJournalArea *& pJournal)
{
    memcpy(pJournal, &mClipJournal, sizeof(FsClipJournalArea));
    return 0;
}

bool ClipParser::isUsedKeyFrameIndex(const FsKeyFramInfoIdx & info)
{
    if(info.minTime >= info.maxTime || info.maxTime == 0 || info.minTime == 0 || info.maxTime == 0x7fffffff)
        return false;
    else
        return true;
}

bool ClipParser::isUsedKeyFrameIndex(const int index)
{
    //get the indexInfo by @index
    if(index < 0 || index >= FS_CLIP_KEYFRAM_IDX_MAX_CNT)
    {
        dbgError("index=%d, donot invalid, valid range is [0, %d]\n", index, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return false;
    }

    return isUsedKeyFrameIndex(mKeyFrameIndexArray[index]);
}

#define DUMP_LEVEL_1_PRESTRING  "->->->->->->"
#define DUMP_LEVEL_2_PRESTRING  "->->->->"
#define DUMP_LEVEL_3_PRESTRING  "->->"
#define DUMP_START_SYMBOL       "dump start -- "
#define DUMP_OVER_SYMBOL        "dump over -- "

int ClipParser::dumpAll()
{
    int ret = 0;
    ret = dumpClipHeader(); if(ret != 0)    {dbgError("dumpAll--dumpClipHeader failed! ret=%d\n", ret); return ret;}
    ret = dumpKeyFrameHeader(); if(ret != 0)    {dbgError("dumpAll--dumpKeyFrameHeader failed! ret=%d\n", ret); return ret;}
    ret = dumpKeyFrameIndex(); if(ret != 0)    {dbgError("dumpAll--dumpKeyFrameIndex failed! ret=%d\n", ret); return ret;}
    ret = dumpJournalArea();   if(ret != 0)    {dbgError("dumpAll--dumpJournalArea failed! ret=%d\n", ret); return ret;}
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpAll(const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL)   {dbgError("open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));   mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;  return -1;}

    int ret = dumpClipHeader(dstFp); if(ret != 0)    {dbgError("dumpAll--dumpClipHeader failed! ret=%d\n", ret);    fclose(dstFp);  dstFp = NULL; return ret;}
    ret = dumpKeyFrameHeader(dstFp); if(ret != 0)    {dbgError("dumpAll--dumpKeyFrameHeader failed! ret=%d\n", ret);    fclose(dstFp);  dstFp = NULL; return ret;}
    ret = dumpKeyFrameIndex(dstFp); if(ret != 0)    {dbgError("dumpAll--dumpKeyFrameIndex failed! ret=%d\n", ret);    fclose(dstFp);  dstFp = NULL; return ret;}
    ret = dumpJournalArea(dstFp);   if(ret != 0)    {dbgError("dumpAll--dumpJournalArea failed! ret=%d\n", ret);    fclose(dstFp);  dstFp = NULL; return ret;}

    fclose(dstFp);
    dstFp = NULL;

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

string ClipParser::getClipHeaderOutputString(const FsClipHead & clipHeader)
{
    string outStr("\tclipHeader info:\n");
    outStr += string("\t\tclipId=") + to_string(clipHeader.clipId) + string("(start from 1)\n");
    outStr += string("\t\tclipStatus=") + to_string(clipHeader.clipStatus) + string("(good/bad)\n");   //TODO, check value meanings
    outStr += string("\t\tpreClipId=") + to_string(clipHeader.perClipId) + string("\n");
    outStr += string("\t\tnextClipId=") + to_string(clipHeader.nextClipId) + string("\n");
    outStr += string("\t\tstreamId=") + to_string(clipHeader.streamId) + string("\n");
    outStr += string("\t\tminTime=") + to_string(clipHeader.minTime) + string("\n"); //TODO, get format time and dump it
    outStr += string("\t\tmaxTime=") + to_string(clipHeader.maxTime) + string("\n"); //TODO, get format time and dump it
    outStr += string("\t\tdataSpaceLen=") + to_string(clipHeader.datSpLen) + string("\n");
    outStr += string("\t\tdataSpaceUsedLen=") + to_string(clipHeader.datSpUseLen) + string("\n");
    outStr += string("\t\tcoverCount=") + to_string(clipHeader.coverCnt) + string("\n");
    return outStr;
}

string ClipParser::getKeyFrameHeaderOutputString(const FsKeyFramHead & keyFrameHeader)
{
    string outStr("\tKeyFrameHeader info:\n");
    outStr += string("\t\tKeyFrameInfoCount=") + to_string(keyFrameHeader.kFrInfoCnt) + string("\n");
    outStr += string("\t\tKeyFrameInfoCurrent=") + to_string(keyFrameHeader.kFrInfoCurr) + string("\n");
    return outStr;
}

string ClipParser::getKeyFrameIndexOutputString(const FsKeyFramInfoIdx & keyFrameIndex)
{
    string outStr("\tKeyFrameInfoIndex info:\n");
    outStr += string("\t\tminTime=") + to_string(keyFrameIndex.minTime) + string("()\n"); //TODO, get format time and dump it
    outStr += string("\t\tmaxTime=") + to_string(keyFrameIndex.maxTime) + string("()\n"); //TODO, get format time and dump it
    return outStr;
}

string ClipParser::getKeyFrameIndexOutputString(const int index, const FsKeyFramInfoIdx & keyFrameIndex)
{
    string outStr("\tKeyFrameInfoIndex info:\n");
    outStr += string("\t\tcurIndex=") + to_string(index) + string("\n");
    outStr += string("\t\tminTime=") + to_string(keyFrameIndex.minTime) + string("()\n"); //TODO, get format time and dump it
    outStr += string("\t\tmaxTime=") + to_string(keyFrameIndex.maxTime) + string("()\n"); //TODO, get format time and dump it
    return outStr;
}

string ClipParser::getKeyFrameInfoOutputString(const int i, const FsKeyFramInfo & keyFrameInfo)
{
    string outStr("\t\t\tKeyFrameInfo info:\n");
    outStr += string("\t\t\t\tcurIndex=") + to_string(i) + string("\n");
    outStr += string("\t\t\t\tdataOffset=") + to_string(keyFrameInfo.datOffSet) + string("()\n");   //TODO, offset start from clip or clip_data_area
    outStr += string("\t\t\t\ttime=") + to_string(keyFrameInfo.time) + string("\n");    //TODO, get format time and dump it
    outStr += string("\t\t\t\tdataLength=") + to_string(keyFrameInfo.dataLength) + string("\n");
    return outStr;
}

int ClipParser::dumpJournalArea()
{
    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string(" journal area\n");
    printf("%s", outStr.c_str());

    outStr = getClipHeaderOutputString(mClipJournal.clipHd);
    printf("%s", outStr.c_str());

    outStr = getKeyFrameHeaderOutputString(mClipJournal.kFrHd);
    printf("%s", outStr.c_str());

    outStr = getKeyFrameIndexOutputString(mClipJournal.kFrInfoIdx);
    printf("%s", outStr.c_str());

    outStr = string("\tKeyFrameInfoUsedCount=") + to_string(mClipJournal.kFrInfoUsedCnt) + string("(maxAllowedValue=") + to_string(FS_JOURNAL_KEYFRAMINFO_COUNT) + string(")\n");
    printf("%s", outStr.c_str());

    int i = 0;
    for(i = 0; i < (FS_JOURNAL_KEYFRAMINFO_COUNT < mClipJournal.kFrInfoUsedCnt ? FS_JOURNAL_KEYFRAMINFO_COUNT : mClipJournal.kFrInfoUsedCnt); i++)
    {
        outStr = getKeyFrameInfoOutputString(i, mClipJournal.kFrInfo[i]);
        printf("%s", outStr.c_str());
    }
    
    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string(" journal area\n");
    printf("%s", outStr.c_str());

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpJournalArea(const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL){dbgError("Open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));  mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;  return -1;}
    dbgDebug("dstFilepath=[%s], open it for write succeed.\n", dstFilepath.c_str());

    int ret = dumpJournalArea(dstFp);

    fclose(dstFp);  dstFp = NULL;
    return ret;
}
int ClipParser::dumpJournalArea(FILE * dstFp)
{
    if(dstFp == NULL)   {dbgError("Input param is NULL.\n");    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT;    return -1;}

    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string(" journal area\n");
    fputs(outStr.c_str(), dstFp);

    outStr = getClipHeaderOutputString(mClipJournal.clipHd);
    fputs(outStr.c_str(), dstFp);

    outStr = getKeyFrameHeaderOutputString(mClipJournal.kFrHd);
    fputs(outStr.c_str(), dstFp);

    outStr = getKeyFrameIndexOutputString(mClipJournal.kFrInfoIdx);
    fputs(outStr.c_str(), dstFp);

    outStr = string("\tKeyFrameInfoUsedCount=") + to_string(mClipJournal.kFrInfoUsedCnt) + string("(maxAllowedValue=") + to_string(FS_JOURNAL_KEYFRAMINFO_COUNT) + string(")\n");
    fputs(outStr.c_str(), dstFp);

    int i = 0;
    for(i = 0; i < (FS_JOURNAL_KEYFRAMINFO_COUNT < mClipJournal.kFrInfoUsedCnt ? FS_JOURNAL_KEYFRAMINFO_COUNT : mClipJournal.kFrInfoUsedCnt); i++)
    {
        outStr = getKeyFrameInfoOutputString(i, mClipJournal.kFrInfo[i]);
        fputs(outStr.c_str(), dstFp);
    }

    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string(" journal area\n");
    fputs(outStr.c_str(), dstFp);
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

int ClipParser::dumpClipHeader()
{
    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("Clip header\n");
    printf("%s", outStr.c_str());

    outStr = getClipHeaderOutputString(mClipHeader);
    printf("%s", outStr.c_str());
    
    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("Clip header\n");
    printf("%s", outStr.c_str());
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpClipHeader(const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL){dbgError("Open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;  return -1;}
    dbgDebug("dstFilepath=[%s], open it for write succeed.\n", dstFilepath.c_str());

    int ret = dumpClipHeader(dstFp);

    fclose(dstFp);  dstFp = NULL;
    return ret;
}
int ClipParser::dumpClipHeader(FILE * dstFp)
{
    if(dstFp == NULL)   {dbgError("Input param is NULL.\n");    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT;    return -1;}

    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("Clip header\n");
    fputs(outStr.c_str(), dstFp);

    outStr = getClipHeaderOutputString(mClipHeader);
    fputs(outStr.c_str(), dstFp);
    
    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("Clip header\n");
    fputs(outStr.c_str(), dstFp);
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

int ClipParser::dumpKeyFrameHeader()
{
    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("Key frame header\n");
    printf("%s", outStr.c_str());

    outStr = getKeyFrameHeaderOutputString(mKeyFrameHeader);
    printf("%s", outStr.c_str());
    
    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("Key frame header\n");
    printf("%s", outStr.c_str());
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpKeyFrameHeader(const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL){dbgError("Open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));  mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;    return -1;}
    dbgDebug("dstFilepath=[%s], open it for write succeed.\n", dstFilepath.c_str());

    int ret = dumpKeyFrameHeader(dstFp);

    fclose(dstFp);  dstFp = NULL;
    return ret;
}
int ClipParser::dumpKeyFrameHeader(FILE * dstFp)
{
    if(dstFp == NULL)   {dbgError("Input param is NULL.\n");    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT;    return -1;}

    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("Key frame header\n");
    fputs(outStr.c_str(), dstFp);

    outStr = getKeyFrameHeaderOutputString(mKeyFrameHeader);
    fputs(outStr.c_str(), dstFp);
    
    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("Key frame header\n");
    fputs(outStr.c_str(), dstFp);
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

/* dump all keyframe index and all of their key frame info once. */
int ClipParser::dumpKeyFrameIndex()
{
    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("All key frame index and info\n");
    printf("%s", outStr.c_str());

    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        outStr = getKeyFrameIndexOutputString(i, mKeyFrameIndexArray[i]);
        printf("%s", outStr.c_str());
        
        map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(i);
        if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */ 
        {
            outStr = string("cur key frame index=") + to_string(i) + string(", cannot find it in map, maybe donot exist.\n");
            printf("%s", outStr.c_str());
            continue;
        }
        else
        {
            vector<FsKeyFramInfo> tmp = it->second;
            int j = 0;
            for(j = 0; j < tmp.size(); j++)
            {
                outStr = getKeyFrameInfoOutputString(j, tmp[j]);
                printf("%s", outStr.c_str());
            }
        }
    }

    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("All key frame index and info\n");
    printf("%s", outStr.c_str());

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpKeyFrameIndex(const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL){dbgError("Open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;  return -1;}
    dbgDebug("dstFilepath=[%s], open it for write succeed.\n", dstFilepath.c_str());

    int ret = dumpKeyFrameIndex(dstFp);

    fclose(dstFp);  dstFp = NULL;

    return ret;
}
int ClipParser::dumpKeyFrameIndex(FILE * dstFp)
{
    if(dstFp == NULL)   {dbgError("Input param is NULL.\n");    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT;    return -1;}

    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("All key frame index and info\n");
    fputs(outStr.c_str(), dstFp);

    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        outStr = getKeyFrameIndexOutputString(i, mKeyFrameIndexArray[i]);
        fputs(outStr.c_str(), dstFp);
        
        map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(i);
        if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */ 
        {
            outStr = string("cur key frame index=") + to_string(i) + string(", cannot find it in map, maybe donot exist.\n");
            fputs(outStr.c_str(), dstFp);
            continue;
        }
        else
        {
            vector<FsKeyFramInfo> tmp = it->second;
            int j = 0;
            for(j = 0; j < tmp.size(); j++)
            {
                outStr = getKeyFrameInfoOutputString(j, tmp[j]);
                fputs(outStr.c_str(), dstFp);
            }
        }
    }

    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("All key frame index and info\n");
    fputs(outStr.c_str(), dstFp);

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}


/* just dump one key frame index and its key frame info which being defined by @index, */
int ClipParser::dumpKeyFrameIndex(const int index)
{
    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("One key frame index and info, index=") + to_string(index) + string("\n");
    printf("%s", outStr.c_str());

    outStr = getKeyFrameIndexOutputString(index, mKeyFrameIndexArray[index]);
    printf("%s", outStr.c_str());
    
    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(index);
    if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */ 
    {
        outStr = string("cur key frame index=") + to_string(index) + string(", cannot find it in map, maybe donot exist.\n");
        printf("%s", outStr.c_str());
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
        return 0;
    }
    else
    {
        vector<FsKeyFramInfo> tmp = it->second;
        int j = 0;
        for(j = 0; j < tmp.size(); j++)
        {
            outStr = getKeyFrameInfoOutputString(j, tmp[j]);
            printf("%s", outStr.c_str());
        }
    }

    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("One key frame index and info, index=") + to_string(index) + string("\n");
    printf("%s", outStr.c_str());

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}
int ClipParser::dumpKeyFrameIndex(const int index, const string & dstFilepath)
{
    FILE * dstFp = NULL;    dstFp = fopen(dstFilepath.c_str(), "w");    if(dstFp == NULL){dbgError("Open file [%s] for write failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;  return -1;}
    dbgDebug("dstFilepath=[%s], open it for write succeed.\n", dstFilepath.c_str());

    int ret = dumpKeyFrameIndex(index, dstFp);

    fclose(dstFp);  dstFp = NULL;

    return ret;
}
int ClipParser::dumpKeyFrameIndex(const int index, FILE * dstFp)
{
    if(dstFp == NULL)   {dbgError("Input param is NULL.\n");    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT; return -1;}

    string outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_START_SYMBOL) + string("One key frame index and info, index=") + to_string(index) + string("\n");
    fputs(outStr.c_str(), dstFp);

    outStr = getKeyFrameIndexOutputString(index, mKeyFrameIndexArray[index]);
    fputs(outStr.c_str(), dstFp);
    
    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(index);
    if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */ 
    {
        outStr = string("cur key frame index=") + to_string(index) + string(", cannot find it in map, maybe donot exist.\n");
        fputs(outStr.c_str(), dstFp);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
        return 0;
    }
    else
    {
        vector<FsKeyFramInfo> tmp = it->second;
        int j = 0;
        for(j = 0; j < tmp.size(); j++)
        {
            outStr = getKeyFrameInfoOutputString(j, tmp[j]);
            fputs(outStr.c_str(), dstFp);
        }
    }

    outStr = string(DUMP_LEVEL_1_PRESTRING) + "" + string(DUMP_OVER_SYMBOL) + string("One key frame index and info, index=") + to_string(index) + string("\n");
    fputs(outStr.c_str(), dstFp);

    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    return 0;
}

/*
    @keyFrameIndex + @keyFrameInfoNo -> keyFrameInfo;
*/
int ClipParser::dumpIframe(const int keyFrameIndex, const int keyFrameInfoNo, const string &dstFilepath)
{
    if( (keyFrameIndex < 0 || keyFrameIndex >= FS_CLIP_KEYFRAM_IDX_MAX_CNT) ||
            (keyFrameInfoNo < 0 || keyFrameInfoNo >= FS_CLIP_KEYFRAM_CNTPREIDXBLK))
    {
        dbgError("Input param invalid! keyFrameIndex=%d(valid range is [0, %d]); keyFrameInfoNo=%d(valid range is [0, %d]);\n",
                 keyFrameIndex, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1, keyFrameInfoNo, FS_CLIP_KEYFRAM_CNTPREIDXBLK - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }
    dbgDebug("keyFrameIndex=%d, keyFrameInfoNo=%d\n", keyFrameIndex, keyFrameInfoNo);

    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(keyFrameIndex);
    if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */
    {
        dbgError("keyFrameIndex=%d, cannot find it!\n", keyFrameIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -2;
    }

    vector<FsKeyFramInfo> tmp = it->second;
    if(keyFrameInfoNo >= tmp.size())
    {
        dbgError("keyFrameInfoNo=%d, keyFrameInfoVectorSize=%d, larger than it^_^! check for why!\n", keyFrameInfoNo, tmp.size());
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -3;
    }

    return dumpIframe(tmp[keyFrameInfoNo], dstFilepath);
}

/* dump an I frame */
int ClipParser::dumpIframe(const FsKeyFramInfo & keyFrameInfo, const string & dstFilepath)
{
    FILE * dstFp = NULL;
    dstFp = fopen(dstFilepath.c_str(), "wb");
    if(dstFp == NULL)   
    {
        dbgError("open dst file [%s] for saving I frame failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));  
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;
        return -1;
    }
    dbgDebug("open file [%s] for write an key frame succeed.\n", dstFilepath.c_str());

    //seek to the key frame now
    rewind(mFp);
    int ret = fseek(mFp, keyFrameInfo.datOffSet + FS_CLIP_DATA_POS, SEEK_SET);
    if(ret != 0)
    {
        dbgError("seek failed! offset=%d, errno=%d, desc=[%s]\n", keyFrameInfo.datOffSet, errno, strerror(errno));
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;
        fclose(dstFp);  dstFp = NULL;
        return -2;
    }
    dbgDebug("seek to offset(%d) succeed.\n", keyFrameInfo.datOffSet);

    //get the key frame by @keyFrameInfo, each time read a block which size 1024bytes, read block looply util to the end of this frame
    char block[1024] = {0x00};
    int leftLen = keyFrameInfo.dataLength + BSFP_HEAD_LENGTH;
    while(leftLen > 0)
    {
        int curNeedReadlen = (leftLen > 1024 ? 1024 : leftLen);
        int curReadLen = fread(block, 1, curNeedReadlen, mFp);
        if(curReadLen != curNeedReadlen)
        {
            dbgError("read failed! ret=%d, needReadlen=%d, errno=%d, desc=[%s]\n", curReadLen, curNeedReadlen, errno, strerror(errno));
            fclose(dstFp);  dstFp = NULL;   unlink(dstFilepath.c_str());
            mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAME_FAILED;
            return -3;
        }
        dbgDebug("read a block succeed. curReadLen=%d\n", curReadLen);

        fwrite(block, 1, curReadLen, dstFp);
        fflush(dstFp);
        dbgDebug("write the block to dstFile over.\n");

        leftLen -= curNeedReadlen;
    }

    dbgDebug("get keyframe to dst file over.\n");
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    fclose(dstFp);  dstFp = NULL;
    return 0;
}

int ClipParser::dumpGOP(const int keyFrameIndex, const int keyFrameInfoNo, const string &dstFilepath)
{
    if( (keyFrameIndex < 0 || keyFrameIndex >= FS_CLIP_KEYFRAM_IDX_MAX_CNT) ||
            (keyFrameInfoNo < 0 || keyFrameInfoNo >= FS_CLIP_KEYFRAM_CNTPREIDXBLK))
    {
        dbgError("Input param invalid! keyFrameIndex=%d(valid range is [0, %d]); keyFrameInfoNo=%d(valid range is [0, %d]);\n",
                 keyFrameIndex, FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1, keyFrameInfoNo, FS_CLIP_KEYFRAM_CNTPREIDXBLK - 1);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -1;
    }
    dbgDebug("keyFrameIndex=%d, keyFrameInfoNo=%d\n", keyFrameIndex, keyFrameInfoNo);

    map<int, vector<FsKeyFramInfo>>::iterator it = mKeyFrameIndexInfoMap.find(keyFrameIndex);
    if(it == mKeyFrameIndexInfoMap.end())   /* donot find it */
    {
        dbgError("keyFrameIndex=%d, cannot find it!\n", keyFrameIndex);
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -2;
    }

    vector<FsKeyFramInfo> tmp = it->second;
    if(keyFrameInfoNo >= tmp.size())
    {
        dbgError("keyFrameInfoNo=%d, keyFrameInfoVectorSize=%d, larger than it^_^! check for why!\n", keyFrameInfoNo, tmp.size());
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;
        return -3;
    }

    FsKeyFramInfo nextKeyFrameInfo;
    if(keyFrameIndex == FS_CLIP_KEYFRAM_IDX_MAX_CNT - 1 && keyFrameInfoNo == tmp.size() - 1)
    {
        dbgInfo("0000\n");
        //to the last valid keyframeinfo, should refresh its nextKeyFrameInfo.dataOffset to clip->dataSpaceUsedLen;
        nextKeyFrameInfo.datOffSet = mClipHeader.datSpUseLen;
    }
    else
    {
        if(keyFrameInfoNo == tmp.size() - 1)
        {
            dbgInfo("1111\n");
            //to the last key frame info of this index, should get the first info of next index
            map<int, vector<FsKeyFramInfo>>::iterator itNext = mKeyFrameIndexInfoMap.find(keyFrameIndex + 1);
            vector<FsKeyFramInfo> tmpNext = itNext->second;
            nextKeyFrameInfo.datOffSet = tmpNext[0].datOffSet == 0 ? mClipHeader.datSpUseLen : tmpNext[0].datOffSet;
        }
        else
        {
            nextKeyFrameInfo.datOffSet = tmp[keyFrameInfoNo + 1].datOffSet == 0 ? mClipHeader.datSpUseLen : tmp[keyFrameInfoNo + 1].datOffSet;
        }
    }

    return dumpGOP(tmp[keyFrameInfoNo], nextKeyFrameInfo, dstFilepath);
}

/* dump a GOP */
int ClipParser::dumpGOP(const FsKeyFramInfo & keyFrameInfo, const FsKeyFramInfo & nextKeyFrameInfo, const string & dstFilepath)
{
    if(nextKeyFrameInfo.datOffSet < keyFrameInfo.datOffSet) {dbgError("nextKeyFrameInfo.datOffSet=%u, keyFrameInfo.datOffSet=%u, invalid values.\n", nextKeyFrameInfo.datOffSet, keyFrameInfo.datOffSet);   mErrno=VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT;   return -1;}

    FILE * dstFp = NULL;
    dstFp = fopen(dstFilepath.c_str(), "wb");
    if(dstFp == NULL)   
    {
        dbgError("open dst file [%s] for saving I frame failed! errno=%d, desc=[%s]\n", dstFilepath.c_str(), errno, strerror(errno));  
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED;
        return -1;
    }
    dbgDebug("open file [%s] for write a GOP succeed.\n", dstFilepath.c_str());
    
    //seek to the key frame now
    rewind(mFp);
    int ret = fseek(mFp, keyFrameInfo.datOffSet + FS_CLIP_DATA_POS, SEEK_SET);
    if(ret != 0)
    {
        dbgError("seek failed! offset=%d, errno=%d, desc=[%s]\n", keyFrameInfo.datOffSet, errno, strerror(errno));
        mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED;
        fclose(dstFp);  dstFp = NULL;
        return -2;
    }
    dbgDebug("seek to offset(%d) succeed.\n", keyFrameInfo.datOffSet);

    //get GOP length
    int leftLen = nextKeyFrameInfo.datOffSet - keyFrameInfo.datOffSet;
    char block[1024] = {0x00};
    while(leftLen > 0)
    {
        int curNeedReadlen = (leftLen > 1024 ? 1024 : leftLen);
        int curReadLen = fread(block, 1, curNeedReadlen, mFp);
        if(curReadLen != curNeedReadlen)
        {
            dbgError("read failed! ret=%d, needReadlen=%d, errno=%d, desc=[%s]\n", curReadLen, curNeedReadlen, errno, strerror(errno));
            fclose(dstFp);  dstFp = NULL;   unlink(dstFilepath.c_str());
            mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAME_FAILED;
            return -3;
        }
        dbgDebug("read a block succeed. curReadLen=%d\n", curReadLen);

        fwrite(block, 1, curReadLen, dstFp);
        fflush(dstFp);
        dbgDebug("write the block to dstFile over.\n");

        leftLen -= curNeedReadlen;
    }

    dbgDebug("get keyframe to dst file over.\n");
    mErrno = VMFS3TOOLS_ERRNO_CLIPPARSER_OK;
    fclose(dstFp);  dstFp = NULL;
    return 0;
}


ClipParser * ClipParserSingleton::pParser = new ClipParser();

ClipParser * ClipParserSingleton::getInstance()
{
    return pParser;
}

