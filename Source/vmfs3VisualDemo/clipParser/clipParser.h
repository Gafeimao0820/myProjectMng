#ifndef __CLIP_PARSER_H__
#define __CLIP_PARSER_H__

using namespace std;

#include <string>
#include <array>
#include <map>
#include <vector>

#include "utils.h"

/* 
    In VMFS3, a clip is 512M, and its structure is :
        journal:        start from 0K,      size is 64K;
        clipHeder :     start from 64K,     size is 4K;
        KeyFrameHeader: start from 68K,     size is 4K;
        KeyFrameIndex:  start from 72K,     size is 4K;
        KeyFrameInfo:   start from 76K,     size is sizeof(FsKeyFramInfo) * 64K;
        Data:           all the last space in this clip to save data;
    All position and size being defined in macros below.
*/
#define FS_CLIP_JOURNAL_POS       (0)               //journal start position
#define FS_CLIP_JOURNAL_SIZE      (64 * VMFS3_KB)   //journal size

#define FS_CLIP_HEAD_POS          (FS_CLIP_JOURNAL_SIZE)
#define FS_CLIP_HEAD_SIZE         (4 * VMFS3_KB)

#define FS_CLIP_KEYFRAM_HEAD_POS   (FS_CLIP_HEAD_POS + FS_CLIP_HEAD_SIZE)
#define FS_CLIP_KEYFRAM_HEAD_SIZE   (4 * VMFS3_KB)

#define FS_CLIP_KEYFRAM_IDX_POS     (FS_CLIP_KEYFRAM_HEAD_POS + FS_CLIP_KEYFRAM_HEAD_SIZE)
#define FS_CLIP_KEYFRAM_IDX_SIZE    (4 * VMFS3_KB)

#define FS_CLIP_KEYFRAM_INFO_POS     (FS_CLIP_KEYFRAM_IDX_POS + FS_CLIP_KEYFRAM_IDX_SIZE)
#define FS_CLIP_KEYFRAM_INFO_SIZE    (sizeof(FsKeyFramInfo) * 64 * VMFS3_KB)

#define FS_CLIP_DATA_POS             (FS_CLIP_KEYFRAM_INFO_POS + FS_CLIP_KEYFRAM_INFO_SIZE)
#define FS_CLIP_DATA_SIZE            (FS_CLIP_SIZE - FS_CLIP_DATA_POS)

/*
    In a clip, At most, 64 KeyFrameIndex exist, and each KeyFrameIndex include at most 1000 KeyFrameInfo, 
*/
#define FS_CLIP_KEYFRAM_IDX_MAX_CNT  (64)
#define FS_CLIP_KEYFRAM_CNTPREIDXBLK  (1000)
#define FS_CLIP_KEYFRAMCNT_MAX      (FS_CLIP_KEYFRAM_CNTPREIDXBLK * FS_CLIP_KEYFRAM_IDX_MAX_CNT)
#define FS_JOURNAL_KEYFRAMINFO_COUNT  (20)


/* journal开始的标志 */
#define FS_JOURNAL_MARK               (0xabadcafe)


/* Clip块的头信息结构体 */
typedef struct
{
    fsUint32    checkSum;        /* 校验 */
    fsUint32    clipId;          /* 当前Clip编号，从1开始在本数据单元中的编号 */
    fsUint32    clipStatus;      /* 状态: 好、坏 */
    fsUint32    perClipId;       /* 上一个Clip编号 */
    fsUint32    nextClipId;      /* 下一个Clip编号 */
    fsUint32    streamId;        /* 属于哪个流，是streamTable中的strmtable表中的ID */
    fsUint32    minTime;         /* 块数据中帧最小时间 */
    fsUint32    maxTime;         /* 块数据中帧最大时间 */
    fsUint32    datSpLen;        /* 数据区总长度 */
    fsUint32    datSpUseLen;     /* 数据区已经使用的长度 */
    fsUint32    coverCnt;        /* 当前clip被复写的次数 */
    fsUint32    resv[5];         /* 保留 */
}FsClipHead;

/* 关键帧描述头结构 */
typedef struct
{
    fsUint32    checkSum;        /* 校验 */
    fsUint32    kFrInfoCnt;      /* 关键帧数量 */
    fsUint32    kFrInfoCurr;     /* 当前在用的frame info的编号 */
    fsUint32    resv[5];         /* 保留 */
}FsKeyFramHead;

/* 关键帧信息的索引结构 */
typedef struct
{
    fsUint32  checkSum;   /* 校验 */
    fsUint32  minTime;    /* 1000个关键帧信息的最小时间 */
    fsUint32  maxTime;    /* 1000个关键帧信息的最大时间 */
    fsUint32  resv[5];    /* 保留 */
}FsKeyFramInfoIdx;

/* 关键帧信息描述结构 */
typedef struct
{
    fsUint32    checkSum;    /* 校验 */
    fsUint32    datOffSet;   /* 偏移位置，相对clip数据区位置(FS_CLIP_DATA_POS)，fsUint32最大能表示4G的偏移 */
    fsUint32    time;        /* 数据时间 */
    fsUint32    dataLength;  /* 大小，这是原来保有位置的空间 */
}FsKeyFramInfo;

/* journal区结构 */
typedef struct
{
    fsUint32            checkSum;                                /* 校验 */
    fsUint32            mark;                                    /* 标志位 0xabadcafe */
    FsClipHead          clipHd;                                  /* ClipHead结构 */
    FsKeyFramHead       kFrHd;                                   /* KeyFramHead结构 */
    FsKeyFramInfoIdx    kFrInfoIdx;                              /* KeyFramInfoIdx结构 */
    fsUint32            kFrInfoUsedCnt;                          /* KeyFramInfo已使用个数 */
    FsKeyFramInfo       kFrInfo[FS_JOURNAL_KEYFRAMINFO_COUNT];   /* KeyFramInfo结构 */
    fsInt32             resv[5];                                 /* 保留 */
}FsClipJournalArea;

typedef enum
{
    VMFS3TOOLS_ERRNO_CLIPPARSER_OK = 0x00,
    VMFS3TOOLS_ERRNO_CLIPPARSER_INVALID_INPUT = 0x10001000,
    VMFS3TOOLS_ERRNO_CLIPPARSER_NULL_INPUT = 0x10001001,
    VMFS3TOOLS_ERRNO_CLIPPARSER_OPENFILE_FAILED = 0x10001002,
    VMFS3TOOLS_ERRNO_CLIPPARSER_SEEKFILE_FAILED = 0x10001003,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READJOURNAL_FAILED = 0x10001004,
    VMFS3TOOLS_ERRNO_CLIPPARSER_CHECKJOURNAL_FAILED = 0x10001005,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READHEAD_FAILED = 0x10001006,
    VMFS3TOOLS_ERRNO_CLIPPARSER_HEADERCHECK_FAILED = 0x10001007,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEHEADER_FAILED = 0x10001008,
    VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEHEADER_FAILED = 0x10001009,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINDEX_FAILED = 0x1000100a,
    VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINDEX_FAILED = 0x1000100b,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAMEINFO_FAILED = 0x1000100c,
    VMFS3TOOLS_ERRNO_CLIPPARSER_CHECK_KEYFRAMEINFO_FAILED = 0x1000100d,
    VMFS3TOOLS_ERRNO_CLIPPARSER_READ_KEYFRAME_FAILED = 0x1000100e,
}VMFS3TOOLS_ERRNO_CLIPPARSER;

class ClipParser
{
public:
    ClipParser();
    ClipParser(const string & filepath);
    ClipParser(const ClipParser & other);
    virtual ~ClipParser();

public:
    /* reset clip filepath, 
        if input @filepath equal with @mClipFilepath, do nothing; 
        else, change @mClipFilepath to @filepath;
       Do parse() if you need the info of @filepath;
       return 0 if succeed, 0- else;
    */
    virtual int resetFilepath(const string & filepath);

    /* parse the @mFilepath, and set all results to memory; 
       return 0 if succeed, 0- else;
       If return 0-, you can use getErrStr() to get the reason.
    */
    virtual int parse();

    /* If some function return error, this can help you to check for why. */
    virtual int getErrno();
    virtual string & getErrStr(const int errNo);

public:
    /* all get functions return 0 if succeed, 0- else; can get errno to check for why. */
    virtual int getClipHeader(FsClipHead &clipHeader);
    
    virtual int getKeyFrameHeader(FsKeyFramHead &keyFrameHeader);
#if 0
    virtual int getKeyFrameIndex(FsKeyFramInfoIdx keyFrameIndexArray[FS_CLIP_KEYFRAM_IDX_MAX_CNT]);
#else
    virtual int getKeyFrameIndex(array<FsKeyFramInfoIdx, FS_CLIP_KEYFRAM_IDX_MAX_CNT> &keyFrameIndexArray);
#endif
    virtual int getKeyFrameIndex(const int index, FsKeyFramInfoIdx *pKeyFrameIndexInfo);
    
    /* return true if keyFrameIndex is used yet, or if donot have valid keyframes in it, return false, means this keyFrameIndex donot being used yet. */
    virtual bool isUsedKeyFrameIndex(const int index);
    virtual bool isUsedKeyFrameIndex(const FsKeyFramInfoIdx & info);

#if 0
    virtual int getKeyFrameInfo(const int keyFrameIndex, FsKeyFramInfo keyFrameInfoArray[FS_CLIP_KEYFRAM_CNTPREIDXBLK]);
#else
    virtual int getKeyFrameInfo(const int keyFrameIndex, vector<FsKeyFramInfo> & keyFrameInfoVec);
#endif
    virtual int getKeyFrameInfo(const int keyFrameIndex, const int keyIndex, FsKeyFramInfo * pKeyFrameInfo);
    
    virtual int getJournal(FsClipJournalArea *& pJournal);

public:
    virtual int dumpAll();
    virtual int dumpAll(const string & dstFilepath);

    virtual int dumpJournalArea();
    virtual int dumpJournalArea(const string & dstFilepath);

    virtual int dumpClipHeader();
    virtual int dumpClipHeader(const string & dstFilepath);

    virtual int dumpKeyFrameHeader();
    virtual int dumpKeyFrameHeader(const string & dstFilepath);

    /*
        this is different with others:
            this will dump frame index info, and its key frame info once.
        If input @index as a param, will dump only this keyframeindex and info;
    */
    virtual int dumpKeyFrameIndex();
    virtual int dumpKeyFrameIndex(const int index);
    virtual int dumpKeyFrameIndex(const string & dstFilepath);
    virtual int dumpKeyFrameIndex(const int index, const string & dstFilepath);

private:
    virtual int dumpJournalArea(FILE * dstFp);
    virtual int dumpClipHeader(FILE * dstFp);
    virtual int dumpKeyFrameHeader(FILE * dstFp);
    virtual int dumpKeyFrameIndex(FILE * dstFp);
    virtual int dumpKeyFrameIndex(const int index, FILE * dstFp);

private:
    string getKeyFrameInfoOutputString(const int i, const FsKeyFramInfo & keyFrameInfo);
    string getKeyFrameIndexOutputString(const int index, const FsKeyFramInfoIdx & keyFrameIndex);
    string getKeyFrameIndexOutputString(const FsKeyFramInfoIdx & keyFrameIndex);
    string getKeyFrameHeaderOutputString(const FsKeyFramHead & keyFrameHeader);
    string getClipHeaderOutputString(const FsClipHead & clipHeader);

public:
    /* dump an I frame */
    virtual int dumpIframe(const FsKeyFramInfo & keyFrameInfo, const string & dstFilepath);
    /* dump a GOP */
    virtual int dumpGOP(const FsKeyFramInfo & keyFrameInfo, const FsKeyFramInfo & nextKeyFrameInfo, const string & dstFilepath);

    /* Append on @keyFrameIndex and @keyFrameInfoNo, get the keyFrameInfo */
    virtual int dumpIframe(const int keyFrameIndex, const int keyFrameInfoNo, const string & dstFilepath);
    /* ditto */
    virtual int dumpGOP(const int keyFrameIndex, const int keyFrameInfoNo, const string & dstFilepath);

private:
    string mClipFilepath;
    
    int mErrno;
    map<int, string> mErrStrMap;

    FsClipHead mClipHeader;
    FsKeyFramHead mKeyFrameHeader;
    array<FsKeyFramInfoIdx, FS_CLIP_KEYFRAM_IDX_MAX_CNT> mKeyFrameIndexArray;
    map<int, vector<FsKeyFramInfo>> mKeyFrameIndexInfoMap;
    FsClipJournalArea mClipJournal;

    FILE * mFp;
};



class ClipParserSingleton
{
public:
    static ClipParser * getInstance();

private:
    ClipParserSingleton() {}
    ~ClipParserSingleton() {}

    static ClipParser * pParser;
};

#endif
