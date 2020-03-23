#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "bsrParser.h"
#include "utils.h"

using namespace std;

#include <string>
#include <list>
#include <map>

#define BLOCK_SIZE  1024

static const unsigned char gBsfpMarkArray[4] = {0xdb, 0x1f, 0x9d, 0xbf};

BsfpInfo::BsfpInfo() : 
    mType(0), mSubType(0), mLen(0), mSeqNo(0), mTimestamp(0), mOffset(0), mChannel(0)
{
    memset(mFormat, 0x00, 4);
}
    
BsfpInfo::BsfpInfo(const BsfpInfo & other) : 
    mType(other.mType), mSubType(other.mSubType), mLen(other.mLen), 
    mSeqNo(other.mSeqNo), mTimestamp(other.mTimestamp), mOffset(other.mOffset),
    mTicks(other.mTicks), mChannel(other.mChannel)
{
    memcpy(mFormat, other.mFormat, 4);
}

BsfpInfo::~BsfpInfo()
{
    ;
}

BsfpInfo BsfpInfo::operator = (const BsfpInfo & other)
{
    if(this == &other)
        return *this;

    mType = other.mType;
    mSubType = other.mSubType;
    mLen = other.mLen;
    mSeqNo = other.mSeqNo;
    mTimestamp = other.mTimestamp;
    mTicks = other.mTicks;
    mOffset = other.mOffset;
    memcpy(mFormat, other.mFormat, 4);
    mChannel = other.mChannel;

    return *this;
}

bool BsfpInfo::operator == (const BsfpInfo & other)
{
    return (mType == other.mType && mSeqNo == other.mSeqNo) ? true : false;
}

BsrParser::BsrParser() : mFilepath(DEFAULT_BSR_FILEPATH), mFd(NULL), mIsRightFormat(false),
    mErrno(PARSEBSRFILE_ERR_OK), mFirstBsfpOffset(0), mLastBsfpOffset(0), mFileSize(0)
{
    memset(mSundayTable, 0x00, SUNDAY_SEARCH_TABLE_LEN);
    initErrStrMap();
    mBsfpInfoList.clear();
    mErrPartList.clear();
    sundaySearchGenNextTable(gBsfpMarkArray, 4);
}

BsrParser::BsrParser(const string &filepath, const  off_t &maxParserSize) : mFilepath(filepath), mFd(NULL), mIsRightFormat(false),
    mErrno(PARSEBSRFILE_ERR_OK), mFirstBsfpOffset(0), mLastBsfpOffset(0), mFileSize(0), mMaxParserSize(maxParserSize)
{
    memset(mSundayTable, 0x00, SUNDAY_SEARCH_TABLE_LEN);
    initErrStrMap();
    mBsfpInfoList.clear();
    mErrPartList.clear();
    sundaySearchGenNextTable(gBsfpMarkArray, 4);
}

BsrParser::BsrParser(const BsrParser &other)
{
    mFilepath = other.mFilepath;
    mFd = other.mFd;
    mIsRightFormat = other.mIsRightFormat;
    mErrno = other.mErrno;
    mBsfpInfoList = other.mBsfpInfoList;
    mErrStrMap = other.mErrStrMap;
    mFirstBsfpOffset = other.mFirstBsfpOffset;
    mLastBsfpOffset = other.mLastBsfpOffset;
    mFileSize = other.mFileSize;
    memcpy(mSundayTable, other.mSundayTable, SUNDAY_SEARCH_TABLE_LEN);
    mErrPartList = other.mErrPartList;
}

BsrParser::~BsrParser()
{
    mErrStrMap.clear();
    mBsfpInfoList.clear();
    mErrPartList.clear();
}

BsrParser & BsrParser::operator = (const BsrParser & other)
{
    if(this == &other)
    {
        return *this;
    }

    mFilepath = other.mFilepath;
    mFd = other.mFd;
    mIsRightFormat = other.mIsRightFormat;
    mErrno = other.mErrno;
    mBsfpInfoList = other.mBsfpInfoList;
    mErrStrMap = other.mErrStrMap;
    mFirstBsfpOffset = other.mFirstBsfpOffset;
    mLastBsfpOffset = other.mLastBsfpOffset;
    mFileSize = other.mFileSize;
    memcpy(mSundayTable, other.mSundayTable, SUNDAY_SEARCH_TABLE_LEN);
    mErrPartList = other.mErrPartList;
    
    return *this;
}

bool BsrParser::operator == (const BsrParser & other)
{
    return mFilepath == other.mFilepath ? true : false;
}

void BsrParser::setBsrFilepath(const string & filepath)
{
    mFilepath = filepath;
}

void BsrParser::setMaxParserSize(const off_t & maxParserSize)
{
    mMaxParserSize = maxParserSize;
}

bool BsrParser::isTooLargerFile()
{
    return (mFileSize > mMaxParserSize) ? true : false;
}

unsigned int BsrParser::getErrno()
{
    return mErrno;
}

string BsrParser::getErrStr(int err)
{
    /* check input err is valid key or not */
    map<int, string>::iterator it = mErrStrMap.find(err);
    if(it != mErrStrMap.end())
    {
        return it->second;
    }
    else
    {
        string ret("Input errno is invalid!\n");
        return ret;
    }
}

void BsrParser::initErrStrMap()
{
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_OK, "No error."));
    
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_FILE_OPEN_FAILED, "File open failed!"));
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_FILE_NOT_BSRFORMAT, "File not in bsr format(bsfpFormat)!"));
    
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED, 
        "Read data from src bsr file as bsfp+frame format failed!"));
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED, 
        "Write frame data to dst file failed!"));
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_SEEK_BSFP_FAILED, 
        "Seek bsfp from src bsr file failed!"));


    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_SUBDIR_EXIST_YET, 
        "Sub directory to save frame as pictures has been existd!"));
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_SUBDIR_CREATE_FAILED, 
        "Create sub directory to save frame as pictures failed!"));
    mErrStrMap.insert(map<int, string>::value_type(PARSEBSRFILE_ERR_PIC_OPEN_FAILED, 
        "Open picture file to write frame data failed!"));

    mErrStrMap.insert(map<int, string>::value_type(PARSERBSRFILE_ERR_INVALID_PARAM,
        "Input param is invalid!"));
}

/*
    In sunday algo., next pos has rules like : 
        If exist in @pPattern, moveBytes = @patternLen - rightPosOfThisChar;
        else, moveBytes = @patternLen + 1;
*/
int BsrParser::sundaySearchGenNextTable(const unsigned char * pPattern, const unsigned int patternLen)
{
    if(NULL == pPattern)
    {
        dbgError("Input param is NULL.\n");
        return -1;
    }
    unsigned int i = 0;
    for(; i < SUNDAY_SEARCH_TABLE_LEN; i++)
    {
        mSundayTable[i] = patternLen + 1;
    }

    for(i = 0; i < patternLen; i++)
    {
        mSundayTable[pPattern[i]] = patternLen - i;
    }

    return 0;
}

/*
    return :
        0+:find the postion, ret is the position;
        0-:param error, or donot find it.
*/
int BsrParser::sundaySearch(const unsigned char * pSrc, const unsigned int srcLen, 
    const unsigned char * pPattern, const unsigned int patternLen)
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
                    i += mSundayTable[pSrc[i + patternLen]];
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


/*
    If find the BSFP Mark, return 0, and the offset will be set to mFirstBsfpOffset;
    else, return 0-;
*/
long long int BsrParser::getNextBsfpOffset(const off_t startOffset)
{
    if(mFd == NULL)
    {
        dbgError("getFirstBsfpMarkOffset failed! mFd == NULL!\n");
        return -1;
    }

    //Jump to the start of this file
    rewind(mFd);
    off_t curOffset = startOffset;
    fseek(mFd, curOffset, SEEK_SET);
    
    bool isFind = true;

    //Check bsfp mark looply
    while(true)
    {
        //Read a block which has size 1024bytes each time;
        unsigned char curData[BLOCK_SIZE] = {0x00};
        memset(curData, 0x00, BLOCK_SIZE);
        size_t readSize = fread((void *)curData, 1, BLOCK_SIZE, mFd);
        if(readSize != BLOCK_SIZE)
        {
            dbgWarn("readSize = %d, BLOCK_SIZE = %d\n", readSize, BLOCK_SIZE);
            if(readSize < sizeof(BsfpHead))
            {
                dbgError("readSize = %d, length then bsfp length(%d), cannot find a bsfp surely!\n",
                    readSize, sizeof(BsfpHead));
                isFind = false;
                break;
            }
        }

        //search bsfp mark in this block
        int ret = sundaySearch(curData, BLOCK_SIZE, gBsfpMarkArray, 4);
        if(ret < 0)
        {
            dbgWarn("sundaySearch return %d, means donot find bsfp mark or other error, jump to next block.\n", ret);
            //jump to next block, we should not jump 1024bytes, just can jump 1021bytes, because MARK has 4bytes, 
            //this 4bytes donot checked in this block 
            curOffset += (BLOCK_SIZE - 3);
            fseek(mFd, curOffset, SEEK_SET);
            continue;
        }
        dbgDebug("Find the bsfp mark in this round, pos = %d\n", ret);

        //We should read 28bytes data, and check this 28bytes data is a valid bsfp format or not
        //If donot in right bsfp format, we should get next bsfp MARK
        BsfpHead curBsfpHead;
        memset(&curBsfpHead, 0x00, sizeof(BsfpHead));
        memcpy(&curBsfpHead, curData + ret, sizeof(BsfpHead));
        
        if(!isValidBsfp(curBsfpHead))
        {
            dbgError("The bsfp data donot a valid one! Will find the next bsfp mark now.\n");
            //should jump the MARK, then find bsfp mark again.
            curOffset += (ret + 4);
            fseek(mFd, curOffset, SEEK_SET);
            continue;
        }
        else
        {
            //This is valid bsfp, can be used as the first one.
            curOffset += ret;
            break;;
        }
    }
    
    return isFind ? curOffset : -2;
}

/*
    1.open file, should deal with some errores when open;
    2.should get the first valid bsfp, MARK check donot enough, should have other check options;
    3.If some err part appeared in the bsr file, should recgonize them, and get their info,
        this can help us to recover bsfp file to a valid format, although it will drop some data
        which maybe usable.
    4.generally speaking, this function should get all bsfp infomation in bsr file, this can help
        us to get data;
*/
int BsrParser::parse()
{
    mBsfpInfoList.clear();
    mErrPartList.clear();

    //Check file exist or not
    struct stat bsrFileStat;
    int ret = stat(mFilepath.c_str(), &bsrFileStat);
    if(ret != 0)
    {
        dbgError("src bsr file [%s] donot exist!\n", mFilepath.c_str());
        mIsRightFormat = false;
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return -1;
    }
    if(!S_ISREG(bsrFileStat.st_mode))
    {
        dbgError("src bsr file [%s] donot a file!\n", mFilepath.c_str());
        mIsRightFormat = false;
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return -2;
    }

    //Get file size in bytes
    mFileSize = bsrFileStat.st_size;

    //Open file
    if(mFd == NULL)
    {
        mFd = fopen(mFilepath.c_str(), "ab+");
        if(mFd == NULL)
        {
            dbgError("fopen failed! errno = %d, desc = [%s]\n", errno, strerror(errno));
            mIsRightFormat = false;
            mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
            return -3;
        }
        dbgError("fopen [%s] succeed.\n", mFilepath.c_str());
    }

    //Get the first bsfp mark offset, if donot exist bsfp mark, not a right bsr file format
    ret = getNextBsfpOffset();
    if(ret < 0)
    {
        dbgError("getFirstBsfpMarkOffset failed! ret = %d\n", ret);
        mIsRightFormat = false;
        mErrno = PARSEBSRFILE_ERR_FILE_NOT_BSRFORMAT;
        fclose(mFd);
        mFd = NULL;
        return -4;
    }

    mFirstBsfpOffset = ret;
    off_t curOffset = mFirstBsfpOffset;
    mIsRightFormat = true;
    mLastBsfpOffset = mFirstBsfpOffset;

    //parse file, Add all bsfp head to list
    rewind(mFd);
    fseek(mFd, mFirstBsfpOffset, SEEK_SET);
    while(true)
    {
        BsfpHead curBsfp;
        memset(&curBsfp, 0x00, sizeof(BsfpHead));
        int readSize = fread((void *)&curBsfp, 1, BSFP_HEAD_LENGTH, mFd);
        if(readSize != BSFP_HEAD_LENGTH)
        {
            if(0 == readSize)   //The end of file 
            {
                dbgDebug("File being checked over!\n");
                mErrno = PARSEBSRFILE_ERR_OK;
                break;
            }
            else    //Some bytes data left in the end, will ignore them
            {
                dbgError("readSize = %d, bsfpLength = %d, donot get a complete bsfp!\n",
                    readSize, BSFP_HEAD_LENGTH);
                mErrno = PARSEBSRFILE_ERR_OK;
                break;
            }
        }
        //Check this bsfp
        if(!isValidBsfp(curBsfp))
        {
            //TODO, get the next bsfp position
            dbgError("Current offset = %ld, bsfp mark = 0x%x, donot equal with 0XBF9D1FDB! will find the next valid bsfp\n",
                curOffset, curBsfp.mark);
            
            mIsRightFormat = false;

            //should get this err part info, and set to local memory
            ErrPart curErrPartInfo;
            curErrPartInfo.offset = curOffset;
            long long int nextBsfpPos = getNextBsfpOffset(curOffset);
            if(nextBsfpPos < 0)
            {
                //From this offset, the last data donot have bsfp, and length larger than sizeof(BsfpHead)
                //So, it should be a err part
                dbgError("Donot find bsfp in the last file!\n");

                curErrPartInfo.length = mFileSize - curOffset + 1;
                mErrPartList.push_back(curErrPartInfo);
                
                mErrno = PARSEBSRFILE_ERR_FILE_NOT_BSRFORMAT;
                break;
            }
            else
            {
                dbgDebug("From %ld, find a valid bsfp, length is %llu, next valid bsfp position is %llu\n",
                    curOffset, nextBsfpPos - curOffset, nextBsfpPos);

                curErrPartInfo.length = nextBsfpPos - curOffset;
                mErrPartList.push_back(curErrPartInfo);

                //refresh the pos of fp, and jump to that pos
                curOffset = nextBsfpPos;
                fseek(mFd, curOffset, SEEK_SET);
                
                mErrno = PARSEBSRFILE_ERR_FILE_NOT_BSRFORMAT;
                continue;
            }
        }

        //refresh the pos of fp
        if(curOffset + BSFP_HEAD_LENGTH + curBsfp.length > mFileSize)
        {
            dbgDebug("The last bsfp is not complete, but donot care about it.\n");
            mIsRightFormat = true;
            mErrno = PARSEBSRFILE_ERR_OK;
            break;
        }

        //In some OS, like windowsXP, and win7, and in some machine, its hardware is poor,
        //If the file is too large, we cannot set all infomation to memory,
        //So I limit a maxSize for file;
        if(curOffset > mMaxParserSize)
        {
            dbgDebug("Larger than the max parser size, we just stop parsing to save memory.\n");
            mIsRightFormat = true;
            mErrno = PARSEBSRFILE_ERR_OK;
            break;
        }
        
        //Add this bsfp info to list
        insertBsfp(curBsfp, curOffset);
        
        //refresh the last valid bsfp info
        mLastBsfpOffset = curOffset;
        //offset should jump to the next bsfp+data
        curOffset += BSFP_HEAD_LENGTH;
        curOffset += curBsfp.length;
        
        //goto next bsfp head
        fseek(mFd, curOffset, SEEK_SET);
    }

    fclose(mFd);
    mFd = NULL;

    //dump all errpart firstly
    dumpAllErrParts();

    //dump all seqence error then 
    dumpAllSequenceErr();

    return 0;
}

/*
    1.Check MARK;
    2.Check type;
    3.Check length, currently, an I frame can be 600+Kbytes, here we limit the length to 2000Kbytes is enough;
        if a bsfp has length larger than 2000Kbytes, means it is not valid;
    4.Check format;
*/
bool BsrParser::isValidBsfp(const BsfpHead & bsfpHead)
{
    //MARK
    if(bsfpHead.mark != BSFP_MARK)
    {
        dbgError("bsfpHead.mark = 0x%08x, donot equal with BSFP_MARK(0x%08x)\n",
            bsfpHead.mark, BSFP_MARK);
        return false;
    }

    //TYPE
    bool ret = true;
    switch(bsfpHead.type)
    {
        case 1: //audio
        case 2: //vedio
        case 3: //speech
        case 4: //intelligent
        case 5: //VDA
        case 9: //notification
        case 10:    //order request and response
        case 15:    //alarm
        case 21:    //vedio msg notification
        case 22:    //IPC AF
        case 24:    //vedio cipher frame
        case 25:    //vedio other
        case 255:   //time sync
            break;
        default:
            dbgError("bsfpHead.type = %d, donot in valid range.\n", bsfpHead.type);
            ret = false;
            break;
    }

    if(!ret)
        return false;

    //LENGTH
    if(bsfpHead.length > MAX_FRAME_LENGTH)
    {
        dbgError("bsfpHead.length = %u, MAX_FRAME_LENGTH = %d, donot valid value.\n",
            bsfpHead.length, MAX_FRAME_LENGTH);
        return false;
    }

    //FORMAT[4]
    //Vedio frame
    if(bsfpHead.type == 2)
    {
        if(bsfpHead.format[0] != 1/*mpeg4*/ && bsfpHead.format[0] != 2/*m-jpeg*/ && bsfpHead.format[0] != 4/*h264*/ && 
            bsfpHead.format[0] != 5/*h265*/ && bsfpHead.format[0] != 12/*cipherH264*/)
        {
            dbgError("bsfpHead.format[0] = %d, donot a valid one!\n", bsfpHead.format[0]);
            return false;
        }

        if(bsfpHead.format[3] != 1/*Iframe*/ && bsfpHead.format[3] != 2/*Bframe*/ && 
            bsfpHead.format[3] != 3/*Pframe*/)
        {
            dbgError("bsfpHead.format[3] = %d, donot a valid value!\n", bsfpHead.format[3]);
            return false;
        }
    }
    //Audio frame, donot check it now.
    ;
    
    return true;
}

void BsrParser::dumpAllErrParts()
{
    printf("\nShow all err parts in bsr file [%s] now : \n", mFilepath.c_str());

    if(mErrPartList.size() > 0)
    {
        int id = 0;
        for(list<ErrPart>::iterator it = mErrPartList.begin(); 
            it != mErrPartList.end(); it++)
        {
            printf("\tId : %d, startOffset = %lld, length = %lld\n",
                id++, it->offset, it->length);
        }
    }
    else
    {
        printf("\tDonot have any err parts in this file!\n");
    }
    
    printf("Show end.\n\n");
}

/*
    Just check sequece No is in order or not now.
    Just check audio and vedio frame seqence number now.
*/
void BsrParser::dumpAllSequenceErr()
{
    printf("\nShow all sequence err in bsr file [%s] now : \n", mFilepath.c_str());

    //FIXME, should optimize me! Can save all err sequence in a list.
    printf("\n========== Vedio frame sequece number ============\n");
    if(mBsfpInfoList.size() > 0)
    {
        bool isFirstBsfpNode = true;
        unsigned int lastSeqId = 0;
        for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); 
            it != mBsfpInfoList.end(); it++)
        {
            if(2 == it->mType)
            {
                if(isFirstBsfpNode)
                {
                    lastSeqId = it->mSeqNo;
                    isFirstBsfpNode = false;
                }
                else
                {
                    if(lastSeqId + 1 != it->mSeqNo)
                    {
                        printf("\tVideo frame : LastSeqNo = %u, CurSeqNo = %u, some error ocurred!\n",
                            lastSeqId, it->mSeqNo);
                    }
                    lastSeqId = it->mSeqNo;
                }
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        printf("Donot have any bsfp info! So donot have sequence err.\n");
    }
    printf("\n========== Vedio frame sequece number Showing over. ============\n");

    printf("\n========== Audio frame sequece number ============\n");
    if(mBsfpInfoList.size() > 0)
    {
        bool isFirstBsfpNode = true;
        unsigned int lastSeqId = 0;
        for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); 
            it != mBsfpInfoList.end(); it++)
        {
            if(1 == it->mType)
            {
                if(isFirstBsfpNode)
                {
                    lastSeqId = it->mSeqNo;
                    isFirstBsfpNode = false;
                }
                else
                {
                    if(lastSeqId + 1 != it->mSeqNo)
                    {
                        printf("\tAudio frame : LastSeqNo = %u, CurSeqNo = %u, some error ocurred!\n",
                            lastSeqId, it->mSeqNo);
                    }
                    lastSeqId = it->mSeqNo;
                }
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        printf("Donot have any bsfp info! So donot have sequence err.\n");
    }
    printf("\n========== Audio frame sequece number Showing over. ============\n");

    printf("Show end.\n\n");
}

int BsrParser::getBsfpsInfo(list<BsfpInfo> &infoList, const QUERY_TYPE type)
{
    if(type >= QUERY_TYPE_MAX)
    {
        dbgError("type = %d, invalid value! valid range is [0, %d)\n", type, QUERY_TYPE_MAX);
        return -1;
    }

    infoList.clear();
    if(QUERY_TYPE_ALL == type)
    {
        infoList = mBsfpInfoList;
        return 0;
    }

    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        switch(type)
        {
        case QUERY_TYPE_VIDEO:
            if(it->mType == 2)
            {
                BsfpInfo curVedioInfo = *it;
                infoList.push_back(curVedioInfo);
            }
            break;
        case QUERY_TYPE_AUDIO:
            if(it->mType == 1)
            {
                BsfpInfo curVedioInfo = *it;
                infoList.push_back(curVedioInfo);
            }
            break;
        case QUERY_TYPE_IFRAME:
            static bool isFirst = true;
            if(it->mType == 2 && it->mSubType == 1)
            {
                printf("wjl_test : the first Iframe offset=%llu\n", it->mOffset);
                isFirst = false;
                BsfpInfo curVedioInfo = *it;
                infoList.push_back(curVedioInfo);
            }
            break;
        default:
            dbgError("type = %d, invalid!\n", type);
            break;
        }
    }
    return 0;
}

/*
 * if maxNum<0, means get all bsfpInfo which has error sequence Id;
*/
int BsrParser::getErrSeqidBsfpsInfo(list<BsfpInfo> &errInfoList, const QUERY_TYPE type, const int maxNum)
{
    return 0;
}

int BsrParser::getDetail(const unsigned char type, const unsigned int seqId, BsfpInfo & info)
{
    int ret = -1;
    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        if(it->mType == type && it->mSeqNo == seqId)
        {
            dbgDebug("Find this bsfpInfo which type=%u, seqId=%u\n", type, seqId);
            info = *it;
            ret = 0;
            break;
        }
    }
    return ret;
}

off_t BsrParser::getFilesize()
{
    return mFileSize;
}

off_t BsrParser::getMaxParserSize()
{
    return mMaxParserSize;
}

void BsrParser::insertBsfp(const BsfpHead & headInfo, const off_t offset)
{
    BsfpInfo info;
    info.mType = headInfo.type;
    info.mSubType = headInfo.format[3];
    info.mLen = headInfo.length;
    info.mSeqNo = headInfo.sequence;
    info.mTimestamp = headInfo.timestamp;
    info.mTicks = headInfo.ticks;
    memcpy(info.mFormat, headInfo.format, 4);
    info.mChannel = headInfo.channel;
    info.mOffset = offset;

    mBsfpInfoList.push_back(info);
}

string BsrParser::getLocalTime(const unsigned int timestamp, const unsigned int tricks)
{
    struct tm localTm;
    localtime_r((time_t *)&timestamp, &localTm);
    char temp[32] = {0x00};
    sprintf(temp, "%04d/%02d/%02d - %02d:%02d:%02d - %04u", localTm.tm_year + 1900, localTm.tm_mon + 1,
        localTm.tm_mday, localTm.tm_hour, localTm.tm_min, localTm.tm_sec, tricks);
    dbgDebug("localTime is : [%s]\n", temp);

    string ret = temp;
    return ret;
}

int BsrParser::dumpAllFrame2File(const string &dstFilepath)
{
    //open the dst file
    FILE *dstFd = NULL;
    dstFd = fopen(dstFilepath.c_str(), "wb");
    if(NULL == dstFd)
    {
        dbgError("fopen failed! dstFilepath = [%s], errno = %d, desc = [%s]\n",
            dstFilepath.c_str(), errno, strerror(errno));
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return mErrno;
    }
    if(mFd == NULL)
    {
        mFd = fopen(mFilepath.c_str(), "rb");
        if(NULL == mFd)
        {
            dbgError("open src file [%s] for read failed! errno=%d, desc=[%s]\n",
                     mFilepath.c_str(), errno, strerror(errno));
            fclose(dstFd);
            dstFd = NULL;
            mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
            return mErrno;
        }
    }

    //Copy each data to dst file.
    int readNum = 0, writeNum = 0;
    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        dbgDebug("Current bsfp info : offset = %llu, length = %u\n",
            curBsfp.mOffset, curBsfp.mLen);
        //Jump to this bsfp, get its data, and write to dst file
        int ret = fseek(mFd, curBsfp.mOffset + BSFP_HEAD_LENGTH, SEEK_SET);
        if(ret != 0)
        {
            dbgError("fseek failed! ret = %d, errno = %d, desc = [%s]\n", ret, errno, strerror(errno));
            fflush(dstFd);
            fclose(dstFd);
            dstFd = NULL;
            fclose(mFd);
            mFd = NULL;
            mErrno = PARSEBSRFILE_ERR_SEEK_BSFP_FAILED;
            return mErrno;
        }

        char dataSection[1024] = {0x00};
        int curLen = curBsfp.mLen;
        while(curLen > 0)
        {
            memset(dataSection, 0x00, 1024);
            int lenNeedRead = (curLen > 1024) ? 1024 : curLen;
            readNum = fread((void *)dataSection, 1, lenNeedRead, mFd);
            if(readNum != lenNeedRead)
            {
                dbgError("readNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    readNum, lenNeedRead, errno, strerror(errno));
                fflush(dstFd);
                fclose(dstFd);
                dstFd = NULL;
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED;
                return mErrno;
            }

            writeNum = fwrite((void *)dataSection, 1, lenNeedRead, dstFd);
            if(writeNum != lenNeedRead)
            {
                dbgError("writeNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    writeNum, lenNeedRead, errno, strerror(errno));
                fclose(dstFd);
                dstFd = NULL;
                unlink(dstFilepath.c_str());
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED;
                return mErrno;
            }

            curLen -= 1024;
        }
    }

    fflush(dstFd);
    fclose(dstFd);
    dstFd = NULL;

    fclose(mFd);
    mFd = NULL;
    mErrno = PARSEBSRFILE_ERR_OK;
    return mErrno;
}

int BsrParser::dumpAllVedioFrame2File(const string &dstFilepath)
{
    //open the dst file
    FILE *dstFd = NULL;
    dstFd = fopen(dstFilepath.c_str(), "wb");
    if(NULL == dstFd)
    {
        dbgError("fopen failed! dstFilepath = [%s], errno = %d, desc = [%s]\n",
            dstFilepath.c_str(), errno, strerror(errno));
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return mErrno;
    }
    if(mFd == NULL)
    {
        mFd = fopen(mFilepath.c_str(), "rb");
        if(NULL == mFd)
        {
            dbgError("open src file [%s] for read failed! errno=%d, desc=[%s]\n",
                     mFilepath.c_str(), errno, strerror(errno));
            fclose(dstFd);
            dstFd = NULL;
            mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
            return mErrno;
        }
    }

    //Copy each data to dst file.
    int readNum = 0, writeNum = 0;
    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        if(curBsfp.mType != 2)
        {
            //Donot a vedio frame, donot deal with it.
            continue;
        }
        dbgDebug("Current bsfp info : offset = %llu, length = %u\n",
            curBsfp.mOffset, curBsfp.mLen);
        //Jump to this bsfp, get its data, and write to dst file
        int ret = fseek(mFd, curBsfp.mOffset + BSFP_HEAD_LENGTH, SEEK_SET);
        if(ret != 0)
        {
            dbgError("fseek failed! ret = %d, errno = %d, desc = [%s]\n", ret, errno, strerror(errno));
            fflush(dstFd);
            fclose(dstFd);
            dstFd = NULL;
            fclose(mFd);
            mFd = NULL;
            mErrno = PARSEBSRFILE_ERR_SEEK_BSFP_FAILED;
            return mErrno;
        }

        char dataSection[1024] = {0x00};
        int curLen = curBsfp.mLen;
        while(curLen > 0)
        {
            memset(dataSection, 0x00, 1024);
            int lenNeedRead = (curLen > 1024) ? 1024 : curLen;
            readNum = fread((void *)dataSection, 1, lenNeedRead, mFd);
            if(readNum != lenNeedRead)
            {
                dbgError("readNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    readNum, lenNeedRead, errno, strerror(errno));
                fflush(dstFd);
                fclose(dstFd);
                dstFd = NULL;
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED;
                return mErrno;
            }

            writeNum = fwrite((void *)dataSection, 1, lenNeedRead, dstFd);
            if(writeNum != lenNeedRead)
            {
                dbgError("writeNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    writeNum, lenNeedRead, errno, strerror(errno));
                fclose(dstFd);
                dstFd = NULL;
                unlink(dstFilepath.c_str());
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED;
                return mErrno;
            }

            curLen -= 1024;
        }
    }

    fflush(dstFd);
    fclose(dstFd);
    dstFd = NULL;

    fclose(mFd);
    mFd = NULL;
    mErrno = PARSEBSRFILE_ERR_OK;
    return mErrno;
}


int BsrParser::dumpAllAudioFrame2File(const string &dstFilepath)
{
    //open the dst file
    FILE *dstFd = NULL;
    dstFd = fopen(dstFilepath.c_str(), "wb");
    if(NULL == dstFd)
    {
        dbgError("fopen failed! dstFilepath = [%s], errno = %d, desc = [%s]\n",
            dstFilepath.c_str(), errno, strerror(errno));
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return mErrno;
    }
    if(mFd == NULL)
    {
        mFd = fopen(mFilepath.c_str(), "rb");
        if(NULL == mFd)
        {
            dbgError("open src file [%s] for read failed! errno=%d, desc=[%s]\n",
                     mFilepath.c_str(), errno, strerror(errno));
            fclose(dstFd);
            dstFd = NULL;
            mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
            return mErrno;
        }
    }

    //Copy each data to dst file.
    int readNum = 0, writeNum = 0;
    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        if(curBsfp.mType != 1)
        {
            //Donot an audio frame, donot deal with it.
            continue;
        }
        dbgDebug("Current bsfp info : offset = %llu, length = %u\n",
            curBsfp.mOffset, curBsfp.mLen);
        //Jump to this bsfp, get its data, and write to dst file
        int ret = fseek(mFd, curBsfp.mOffset + BSFP_HEAD_LENGTH, SEEK_SET);
        if(ret != 0)
        {
            dbgError("fseek failed! ret = %d, errno = %d, desc = [%s]\n", ret, errno, strerror(errno));
            fflush(dstFd);
            fclose(dstFd);
            dstFd = NULL;
            fclose(mFd);
            mFd = NULL;
            mErrno = PARSEBSRFILE_ERR_SEEK_BSFP_FAILED;
            return mErrno;
        }

        char dataSection[1024] = {0x00};
        int curLen = curBsfp.mLen;
        while(curLen > 0)
        {
            memset(dataSection, 0x00, 1024);
            int lenNeedRead = (curLen > 1024) ? 1024 : curLen;
            readNum = fread((void *)dataSection, 1, lenNeedRead, mFd);
            if(readNum != lenNeedRead)
            {
                dbgError("readNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    readNum, lenNeedRead, errno, strerror(errno));
                fflush(dstFd);
                fclose(dstFd);
                dstFd = NULL;
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED;
                return mErrno;
            }

            writeNum = fwrite((void *)dataSection, 1, lenNeedRead, dstFd);
            if(writeNum != lenNeedRead)
            {
                dbgError("writeNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                    writeNum, lenNeedRead, errno, strerror(errno));
                fclose(dstFd);
                dstFd = NULL;
                unlink(dstFilepath.c_str());
                fclose(mFd);
                mFd = NULL;
                mErrno = PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED;
                return mErrno;
            }

            curLen -= 1024;
        }
    }

    fflush(dstFd);
    fclose(dstFd);
    dstFd = NULL;

    fclose(mFd);
    mFd = NULL;
    mErrno = PARSEBSRFILE_ERR_OK;
    return mErrno;
}


int BsrParser::dumpOneFrameData2File(const unsigned char type, const unsigned int seqId, const string & dstFilepath)
{
    BsfpInfo curBsfpInfo;
    int ret = getDetail(type, seqId, curBsfpInfo);
    if(ret != 0)
    {
        dbgError("getDetail failed! type=%d, seqId=%d, ret=%d\n", type, seqId, ret);
        mErrno = PARSERBSRFILE_ERR_INVALID_PARAM;
        return mErrno;
    }

    //open the dst file
    FILE *dstFd = NULL;
    dstFd = fopen(dstFilepath.c_str(), "wb");
    if(NULL == dstFd)
    {
        dbgError("fopen failed! dstFilepath = [%s], errno = %d, desc = [%s]\n",
            dstFilepath.c_str(), errno, strerror(errno));
        mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
        return mErrno;
    }
    if(mFd == NULL)
    {
        mFd = fopen(mFilepath.c_str(), "rb");
        if(NULL == mFd)
        {
            dbgError("open src file [%s] for read failed! errno=%d, desc=[%s]\n",
                     mFilepath.c_str(), errno, strerror(errno));
            fclose(dstFd);
            dstFd = NULL;
            mErrno = PARSEBSRFILE_ERR_FILE_OPEN_FAILED;
            return mErrno;
        }
    }

    //Jump to this bsfp, get its data, and write to dst file
    ret = fseek(mFd, curBsfpInfo.mOffset + BSFP_HEAD_LENGTH, SEEK_SET);
    if(ret != 0)
    {
        dbgError("fseek failed! ret = %d, errno = %d, desc = [%s]\n", ret, errno, strerror(errno));
        fflush(dstFd);
        fclose(dstFd);
        dstFd = NULL;
        fclose(mFd);
        mFd = NULL;
        mErrno = PARSEBSRFILE_ERR_SEEK_BSFP_FAILED;
        return mErrno;
    }

    char dataSection[1024] = {0x00};
    int curLen = curBsfpInfo.mLen;
    int readNum = 0, writeNum = 0;
    while(curLen > 0)
    {
        memset(dataSection, 0x00, 1024);
        int lenNeedRead = (curLen > 1024) ? 1024 : curLen;
        readNum = fread((void *)dataSection, 1, lenNeedRead, mFd);
        if(readNum != lenNeedRead)
        {
            dbgError("readNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                readNum, lenNeedRead, errno, strerror(errno));
            fflush(dstFd);
            fclose(dstFd);
            dstFd = NULL;
            fclose(mFd);
            mFd = NULL;
            mErrno = PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED;
            return mErrno;
        }

        writeNum = fwrite((void *)dataSection, 1, lenNeedRead, dstFd);
        if(writeNum != lenNeedRead)
        {
            dbgError("writeNum = %d, lenNeedRead = %d, read failed! errno = %d, desc = [%s]\n",
                writeNum, lenNeedRead, errno, strerror(errno));
            fclose(dstFd);
            dstFd = NULL;
            unlink(dstFilepath.c_str());
            fclose(mFd);
            mFd = NULL;
            mErrno = PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED;
            return mErrno;
        }

        curLen -= 1024;
    }

    fflush(dstFd);
    fclose(dstFd);
    dstFd = NULL;

    fclose(mFd);
    mFd = NULL;
    mErrno = PARSEBSRFILE_ERR_OK;
    return mErrno;
}

unsigned long long int BsrParser::getAllFrameNum()
{
    return mBsfpInfoList.size();
}

int BsrParser::getAllVideoFrameNum()
{
    int ret = 0;

    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        if(curBsfp.mType == 2)
        {
            ret++;
        }
    }
    dbgDebug("all video frame number is %d\n", ret);
    return ret;
}

int BsrParser::getAllAudioFrameNum()
{
    int ret = 0;

    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        if(curBsfp.mType == 1)
        {
            ret++;
        }
    }
    dbgDebug("all audio frame number is %d\n", ret);
    return ret;
}

int BsrParser::getAllOtherTypeFrameNum()
{
    int ret = 0;

    for(list<BsfpInfo>::iterator it = mBsfpInfoList.begin(); it != mBsfpInfoList.end(); it++)
    {
        BsfpInfo curBsfp = *it;
        if(curBsfp.mType != 1 && curBsfp.mType != 2)
        {
            ret++;
        }
    }
    dbgDebug("all other type frame number is %d\n", ret);
    return ret;
}

unsigned long long int BsrParser::getAllLostFrameNum()
{
    return getAllLostAudioFrameNum() + getAllLostVideoFrameNum();
}

int BsrParser::getAllLostVideoFrameNum()
{
    return getLostFrameNum(QUERY_TYPE_VIDEO);
}

int BsrParser::getAllLostAudioFrameNum()
{
    return getLostFrameNum(QUERY_TYPE_AUDIO);
}

//type just support video or audio, others donot support now.
int BsrParser::getLostFrameNum(QUERY_TYPE type)
{
    if(mBsfpInfoList.size() <= 1)
        return 0;
    if(type != QUERY_TYPE_VIDEO && type != QUERY_TYPE_AUDIO)
        return 0;

    int ret = 0;

    unsigned int preSeqNo = 0;
    list<BsfpInfo>::iterator it = mBsfpInfoList.begin();
    while(it != mBsfpInfoList.end())
    {
        if(it->mType == (type == QUERY_TYPE_VIDEO ? 2 : 1))
        {
            preSeqNo = it->mSeqNo;
            it++;
            break;
        }
        it++;
    }

    for(; it != mBsfpInfoList.end(); it++)
    {
        if(it->mType != (type == QUERY_TYPE_VIDEO ? 2 : 1))  continue;

        if(it->mSeqNo - preSeqNo != 1)  //sequence numebr donot in order
        {
            ret++;
        }
        preSeqNo = it->mSeqNo;
    }

    return ret;
}

unsigned long long int BsrParser::getDirtyDataNum()
{
    return mErrPartList.size();
}

int BsrParser::getDirtyDataList(list<ErrPart> &dirtyDataList)
{
    dirtyDataList = mErrPartList;
    return 0;
}

unsigned int BsrParser::getFirstBsfpOffset()
{
    return mFirstBsfpOffset;
}

BsrParser * BsrParserSingleton::pParser = new BsrParser();

BsrParser * BsrParserSingleton::getInstance()
{
    return pParser;
}


