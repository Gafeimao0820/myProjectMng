#ifndef __BSR_PARSER_H__
#define __BSR_PARSER_H__

#include <stdio.h>

using namespace std;

#include <string>
#include <map>
#include <list>
#include <vector>

#include "utils.h"

#define MAX_FRAME_LENGTH    0X1f4000    //2048000bytes, 2000Kbytes, a frame cannot larger than this size
//If donot give a bsr filepath by user, this will be used as default.
#define DEFAULT_BSR_FILEPATH "./bsrFile"

/* Defined in DVR; */
typedef struct
{
  unsigned int mark;
  unsigned char type;
  char    channel;
  unsigned short device;
  unsigned int length;
  unsigned int sequence;
  unsigned int timestamp;
  unsigned int ticks;
  char    format[4];
  char    data[0];
}BsfpHead;

/*
    Save the err part info in middle of bsr file;
*/
typedef struct
{
    unsigned long long int offset;
    unsigned long long int length;
}ErrPart;

/*
    The basic info of bsfps in file;
*/
class BsfpInfo
{
public:
    BsfpInfo();
    BsfpInfo(const BsfpInfo & other);
    virtual ~BsfpInfo();

public:
    virtual BsfpInfo operator = (const BsfpInfo & other);
    virtual bool operator == (const BsfpInfo & other);

public:
    /*
        The type of this bsfp, 1--audio, 2--vedio, other--not sure;
    */
    unsigned char mType;
    /*
        The subType of this bsfp:
            type == 2: 1--I frame; 2--B; 3--P; other--not sure;
            type == else : not sure;
    */
    unsigned char mSubType;
    /* The length of data; */
    unsigned int mLen;
    /* Sequence number */
    unsigned int mSeqNo;
    /* The timestamp of this frame */
    unsigned int mTimestamp;
    /* 
        The format being get from BsfpHeader, to video and audio, this is important attributes 
        To a video frame, mFormat[3] is the frame type:
            1--I frame;
            2--B frame;
            3--P frame;
    */
    char mFormat[4];
    /* The offset in the bsr file, this can help us to find them in file directly; */
    unsigned long long int mOffset;
    /* The micro seconds of this frame */
    unsigned int mTicks;

    char mChannel;
};

typedef enum
{
    PARSEBSRFILE_ERR_OK = 0x0,

    PARSEBSRFILE_ERR_FILE_OPEN_FAILED = 0x10000000,  /* filepath is OK, but cannot open it, maybe cannot write or other reason. */
    PARSEBSRFILE_ERR_FILE_NOT_BSRFORMAT,    /* file donot in bsr format, bsr format means "bsfp + data + bsfp + data ..." */

    PARSEBSRFILE_ERR_READ_FRAME_DATA_FAILED = 0x10001000,
    PARSEBSRFILE_ERR_WRITE_FRAME_DATA_FAILED,
    PARSEBSRFILE_ERR_SEEK_BSFP_FAILED,

    PARSEBSRFILE_ERR_SUBDIR_EXIST_YET = 0x10002000,
    PARSEBSRFILE_ERR_SUBDIR_CREATE_FAILED,
    PARSEBSRFILE_ERR_PIC_OPEN_FAILED,

    PARSERBSRFILE_ERR_INVALID_PARAM = 0x10003000,
}PARSEBSRFILE_ERR;

typedef enum
{
    QUERY_TYPE_ALL,
    QUERY_TYPE_VIDEO,
    QUERY_TYPE_AUDIO,
    QUERY_TYPE_IFRAME,
    QUERY_TYPE_MAX
}QUERY_TYPE;

class BsrParser
{
public:
    BsrParser();
    BsrParser(const string &filepath, const off_t &maxParserSize);
    BsrParser(const BsrParser &other);
    virtual ~BsrParser();

public:
    /* This is necessary because we have pointer in our private params;
    */
    virtual BsrParser & operator = (const BsrParser & other);
    virtual bool operator == (const BsrParser & other);

public:
    virtual void setBsrFilepath(const string & filepath);
    virtual void setMaxParserSize(const off_t & maxParserSize);
    /* If bsr filesize large than max parser size, return true; else, return false; */
    virtual bool isTooLargerFile();

    /*
     * start parse the bsrfile which being defined by mFilepath;
    */
    virtual int parse();

    /* Set current errno to mErrno, and return it when call this function;
    */
    virtual unsigned int getErrno();

    /* Get the description of @errno;
    */
    virtual string getErrStr(int err);
    
public:
    /* Check the input file in right bsr file format or not;
       BSFP header will be the rule.
    */
    bool isRightFormat();

    /* get all of the bsfp info in the type beindg defined;
     * If return 0-, means some error ocurred when parsing this bsr file.
     */
    virtual int getBsfpsInfo(list<BsfpInfo> & infoList, const QUERY_TYPE type);
    /*
     * get all of the bsfp error sequence id info;
     * if maxNum==-1, means get all of them,
     * else, get at most @maxNum;
     *
     * return 0 if succeed, 0- when error;
    */
    virtual int getErrSeqidBsfpsInfo(list<BsfpInfo> & errInfoList, const QUERY_TYPE type, const int maxNum = -1);

    virtual int getDetail(const unsigned char type, const unsigned int seqId, BsfpInfo & info);

    /* convert a timestamp to localtime, its format like : 2017/08/23 - 15:49:50
    */
    string getLocalTime(const unsigned int timestamp, const unsigned int tricks);

    virtual off_t getFilesize();
    virtual off_t getMaxParserSize();

public:
    /*
     * Dump the frame data to files;
    */
    virtual int dumpAllFrame2File(const string &dstFilepath);
    virtual int dumpAllVedioFrame2File(const string & dstFilepath);
    virtual int dumpAllAudioFrame2File(const string & dstFilepath);
    virtual int dumpOneFrameData2File(const unsigned char type, const unsigned int seqId, const string & dstFilepath);

public:
    virtual unsigned long long int getAllFrameNum();
    virtual int getAllVideoFrameNum();
    virtual int getAllAudioFrameNum();
    virtual int getAllOtherTypeFrameNum();

    virtual unsigned long long int getAllLostFrameNum();
    virtual int getAllLostVideoFrameNum();
    virtual int getAllLostAudioFrameNum();

    virtual unsigned long long int getDirtyDataNum();
    virtual int getDirtyDataList(list<ErrPart> & dirtyDataList);

    virtual unsigned int getFirstBsfpOffset();

private:
    virtual int getLostFrameNum(QUERY_TYPE type);

private:
    /* Do init to mErrStrMap */
    void initErrStrMap();

    /* When find BSFP_MARK, use sunday algo., this will be faster */
    int sundaySearchGenNextTable(const unsigned char * pPattern, const unsigned int patternLen);
    int sundaySearch(const unsigned char * pSrc, const unsigned int srcLen, 
        const unsigned char * pPattern, const unsigned int patternLen);

    /* insert a bsfp head to mBsfpInfoList */
    void insertBsfp(const BsfpHead & headInfo, const off_t offset);

    void dumpAllErrParts();
    void dumpAllSequenceErr();

    /*
        From @startOffset in bsr file, find a bsfp;
        When we find the first bsfp head, we use this method, too.
        Just startOffset = 0 when find the first bsfp head;

        return :
            0+ : the offset start from @startOffset;
            0- : error, donot find next bsfp head;
    */
    long long int getNextBsfpOffset(const off_t startOffset = 0);

    /*
        Do more checking to @bsfpHead;
        Except MARK, other values being checked, too.
    */
    bool isValidBsfp(const BsfpHead & bsfpHead);
    
private:
    string mFilepath;
    FILE *mFd;
    bool mIsRightFormat;
    /* Set all bsfp info to this list, data will not set to it for saving memory */
    list<BsfpInfo> mBsfpInfoList;
    /* Records all err description in this map.
    */
    map<int, string> mErrStrMap;
    unsigned int mErrno;
    /* The first bsfp mark offset */
    unsigned int mFirstBsfpOffset;
    /* 
        The last bsfp offset;
        After this bsfp+data, maybe some data in file, but we will ignore them,
        because they may be not a complete bssp+data;
    */
    off_t mLastBsfpOffset;
    /* The size of input file, in bytes */
    off_t mFileSize;
    /* The max size we parsed, if mFilesize larger than it, we just parse the file in this size
     * We do this just because we should set all info to memory, in some poor hardware machine,
     * too large file maybe use too much memory, maybe cause some error.
    */
    off_t mMaxParserSize;

    /* sunday algo. need a table */
    char mSundayTable[SUNDAY_SEARCH_TABLE_LEN];

    /* The list which save all err part in middle of file; */
    list<ErrPart> mErrPartList;
};

class BsrParserSingleton
{
public:
    static BsrParser * getInstance();

private:
    BsrParserSingleton() {}
    ~BsrParserSingleton() {}

    static BsrParser * pParser;
};
#endif
