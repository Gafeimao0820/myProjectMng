#ifndef FSDATACTRL_H
#define FSDATACTRL_H

/**
 *  @file fsDataCtrl.h
 *  @breif 文件系统数据管理模块定义文件
 *  @version 0.0.1
 *  @since 0.0.1
 *  @author
 *  @date
 */
/******************************************************************************
*@note
    Copyright 2016, BeiJing Bluestar Corporation, Limited
                 ALL RIGHTS RESERVED
Permission is hereby granted to licensees of BeiJing Bluestar, Inc. products
to use or abstract this computer program for the sole purpose of implementing
a product based on BeiJing Bluestar, Inc. products. No other rights to
reproduce, use, or disseminate this computer program,whether in part or in
whole, are granted. BeiJing Bluestar, Inc. makes no representation or
warranties with respect to the performance of this computer program, and
specifically disclaims any responsibility for any damages, special or
consequential, connected with the use of this program.
For details, see http://www.bstar.com.cn/
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 文件系统使用的类型重定义
 */
typedef unsigned char fsUchar;
typedef unsigned short int fsUint16;
typedef unsigned int fsUint32;
typedef unsigned long long int fsUint64;
typedef float fsFloat;

typedef char fsChar;
typedef short int fsInt16;
typedef int fsInt32;
typedef long long int fsInt64;

/////////////////////////////////////////////////////////////
////////////      全局信息目录管理宏和结构      /////////////
/////////////////////////////////////////////////////////////

/**
 * 子目录文件夹名
 */
#ifndef FS_BSTARREC_DIR
#define FS_BSTARREC_DIR  "record_storage"
#endif

/**
 * 全局信息目录名
 */
#ifndef FS_GLOBAL_DIR
#define FS_GLOBAL_DIR   "GlobalDir"
#endif

/**
 * 全局信息文件，主文件和备份文件
 */
#ifndef FS_GLOBAL_FILE
#define FS_GLOBAL_FILE          "GlobalInfo.dat"
#endif

#ifndef FS_GLOBAL_BAK_FILE
#define FS_GLOBAL_BAK_FILE      "GlobalBakInfo.dat"
#endif

/**
 * 全局信息文件的标志位
 */
#ifndef FS_GLOBAL_MAGIC
#define FS_GLOBAL_MAGIC     (0x62736673)
#endif

/**
 * 全局信息文件的空间健康状态位
 */
typedef enum
{
    FS_DUH_HEALTH = 0,  /* 空间健康状况良好 */
    FS_DUH_BAD,         /* 空间已坏 */
}EFsDatUnitHealth;

typedef struct
{
    fsUint32     checksum;               /* 校验位 */
    fsUint32     magic;                  /* 标志位，用于检测当前文件是否正常 magic = bsfs = 0x62736673 */

    /**
     * 使能位，用于检测当前目录空间是否生效，为1时生效，为0时失效
     * 此参数最后在整体初始化完成后才会在次修改成1，使能目录空间，
     * 以防初始化期间出现意外造成目录空间异常
     */
    fsUint32     enable;
    fsUint64     totalSize;              /* 目录空间的总大小，单位MB */
    fsUint64     leftSize;               /* 目录空间剩余容量大小，单位MB */
    fsUint32     dataUnitCnt;            /* 数据单元总数 */
    fsUint32     dataUnitCurr;           /* 正在使用的数据单元 */
    fsChar       mac[32];                /* 目录空间所属的设备mac地址 */
    fsUint32     healthState;            /* 目录空间状态 */
    fsUint32     version;                /* 文件系统主版本号 高低字节分布:0~7主版本号， 8~15次版本号 16~31修订号 */
    fsUint32     resv[5];                /* 保留 */
}FsGlobalInfo;

///////////////////////////////////////////////////////////////////////////
/////////////////////          交换目录管理用宏和结构   ///////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * 交换目录名
 */
#ifndef FS_SWAP_DIR
#define FS_SWAP_DIR                 "SwapDir"
#endif

/**
 * 交换文件名
 */
#ifndef FS_SWAP_FILE
#define FS_SWAP_FILE                "SwapInfo.dat"
#endif


///////////////////////////////////////////////////////////////////////////
/////////////////////          系统日志管理用宏和结构   ///////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * 日志数据文件目录名
 */
#ifndef FS_LOG_DB_DIR
#define FS_LOG_DB_DIR                    "SysLogDir"
#endif

/**
 * 日志数据库文件名
 */
#ifndef FS_LOG_DB_FILE
#define FS_LOG_DB_FILE                   "SysLog.db"
#endif

/**
 * 日志数据库备份文件
 */
#ifndef FS_LOG_DB_BAK_FILE
#define FS_LOG_DB_BAK_FILE              "SysLogBak.db"
#endif

/**
 *
 */
#ifndef FS_LOG_RESULTS_MAX_ITEMS
#define FS_LOG_RESULTS_MAX_ITEMS        (50)
#endif

/**
 * 日志最大最小级别
 */
#ifndef FS_LOG_MAX_LEVEL
#define FS_LOG_MAX_LEVEL                  (7)
#endif

#ifndef FS_LOG_MIN_LEVEL
#define FS_LOG_MIN_LEVEL                  (1)
#endif

/**
 * 提交日志的用户名最长值
 */
#ifndef FS_LOG_USER_NAME_LEN
#define FS_LOG_USER_NAME_LEN             (20)
#endif

/**
 * 日志消息长度最长值
 */
#ifndef FS_LOG_MSG_INFO_LEN
#define FS_LOG_MSG_INFO_LEN              (56)
#endif

/**
 * 文件系统日志结构
 */
typedef struct
{
    fsUint32 time;                      /* 时间戳 */
    fsChar type;                        /* 日志类型:全部、系统日志、报警日志、管理日志、操作日志、视频日志 */
    fsChar level;                       /* 日志级别:小于5为紧急、5-6为正常、7为调试 */
    fsChar action;                      /* 操作名称:开机、本地登录、本地注销等； */
    fsChar result;                      /*  */
    fsInt32 target;                     /*  */
    fsChar user[FS_LOG_USER_NAME_LEN];  /* 提交日志的用户名 */
    fsChar msg[FS_LOG_MSG_INFO_LEN];    /* 日志内容 */
} FsLog;

/**
 * 应用层查询条件
 */
typedef struct
{
    fsUint32      startTime;                     /* 开始时间 */
    fsUint32      endTime;                       /* 结束时间 */
    fsChar        type;                          /* 日志类别 */
    fsChar        level;                         /* 日志级别 */
    fsUchar       disk;                          /* 硬盘盘符 */
    fsUchar       disk2;                         /* ???????? */
    fsChar        user[FS_LOG_USER_NAME_LEN];    /* 指定用户 */
}FsLogQuery;

/**
 * 存储返回的结果集
 */
typedef struct
{
    fsUint32     total;                                 /* 总记录数 total records*/
    fsInt32        rowId;                               /* 行ID row id in the page.*/
    fsUint32     rowCount;                              /* 行数量 total records in the page.*/
    FsLog         rowList[FS_LOG_RESULTS_MAX_ITEMS];    /* 结果 */
} FsLogResult;


/**
 *
 */
typedef struct
{
    fsInt32           count;
    fsInt32           total;
    FsLogResult *     result;
}SqlLogQueryPara;

///////////////////////////////////////////////////////////////////////////
/////////////////////         数据单元管理用宏和结构   ///////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * 数据单元目录名
 */
#ifndef FS_DATAUNIT_DIR_PERFIX
#define FS_DATAUNIT_DIR_PERFIX         "DataUnitDir_"
#endif

/**
 * 数据单元信息文件名
 */
#ifndef FS_DATAUNIT_FILE
#define FS_DATAUNIT_FILE          "DataUnitInfo.dat"
#endif

/**
 * 数据单元信息备份文件名
 */
#ifndef FS_DATAUNIT_BAK_FILE
#define FS_DATAUNIT_BAK_FILE      "DataUnitInfoBak.dat"
#endif

/**
 * 事件数据库
 */
#ifndef FS_EVTDB_FILE
#define FS_EVTDB_FILE             "Event.db"
#endif

/**
 * 事件备份数据库
 */
#ifndef FS_EVTDB_BAK_FILE
#define FS_EVTDB_BAK_FILE         "EventBak.db"
#endif

/**
 * 流信息数据库
 */
#ifndef FS_STRMDB_FILE
#define FS_STRMDB_FILE            "Stream.db"
#endif

/**
 * 流信息备份数据库
 */
#ifndef FS_STRMDB_BAK_FILE
#define FS_STRMDB_BAK_FILE        "StreamBak.db"
#endif

/**
 * 元数据信息数据库
 */
#ifndef FS_METADB_FILE
#define FS_METADB_FILE            "Meta.db"
#endif

/**
 * 元数据信息备份数据库
 */
#ifndef FS_METADB_BAK_FILE
#define FS_METADB_BAK_FILE        "MetaBak.db"
#endif

/**
 * 图片数据库
 */
#ifndef FS_PICTUREDB_FILE
#define FS_PICTUREDB_FILE        "Picture.db"
#endif

/**
 * 图片备份数据库
 */
#ifndef FS_PICTUREDB_BAK_FILE
#define FS_PICTUREDB_BAK_FILE    "PictureBak.db"
#endif

/**
 * 数据单元信息主备文件大小
 */
#ifndef FS_DATAUNIT_SIZE
#define FS_DATAUNIT_SIZE           (256 * VMFS3_MB)
#endif

/**
 * Clip数据块文件名
 */
#ifndef FS_CLIP_PER_NAME
#define FS_CLIP_PER_NAME        "ClipBlock_"
#endif

/**
 * 数据单元中clip的最大个数
 */
#ifndef VMFS3_CLIPCNT_MAX
#define VMFS3_CLIPCNT_MAX ((VMFS3_DATAUNIT_DIR_SIZE - 200*(FS_DATAUNIT_SIZE + FS_EVTDB_SIZE + FS_STRMDB_SIZE + FS_METADB_SIZE))/FS_CLIP_SIZE)
#endif

/**
 * Clip数据块大小
 */
#ifndef FS_CLIP_SIZE
#define FS_CLIP_SIZE           (512 * VMFS3_MB)
#endif

/**
 * 事件数据库,事件数据库备份占用空间长度
 */
#ifndef FS_EVTDB_SIZE
#define FS_EVTDB_SIZE     (1 * VMFS3_GB)
#endif

/**
 * 流信息数据库文件大小，主备文件占用空间长度
 */
#ifndef FS_STRMDB_SIZE
#define FS_STRMDB_SIZE     (256 * VMFS3_MB)
#endif

/**
 * 元数据信息数据库文件大小，主备文件占用空间长度
 */
#ifndef FS_METADB_SIZE
#define FS_METADB_SIZE     (256 * VMFS3_MB)
#endif

 /**
  * 图片数据库文件大小，主备文件占用空间长度
  */
#ifndef FS_PICTUREDB_SIZE
#define FS_PICTUREDB_SIZE     (5 * VMFS3_GB)
#endif

/**
 * journal区长度和开始位置
 */
#ifndef FS_CLIP_JOURNAL_POS
#define FS_CLIP_JOURNAL_POS       (0)
#endif

#ifndef FS_CLIP_JOURNAL_SIZE
#define FS_CLIP_JOURNAL_SIZE      (64 * VMFS3_KB)
#endif

/**
 * Clip Head区长度和开始位置
 */
#ifndef FS_CLIP_HEAD_POS
#define FS_CLIP_HEAD_POS          (FS_CLIP_JOURNAL_SIZE)
#endif

#ifndef FS_CLIP_HEAD_SIZE
#define FS_CLIP_HEAD_SIZE         (4 * VMFS3_KB)
#endif

/**
 * key frame Head 区长度和开始位置
 */
#ifndef FS_CLIP_KEYFRAM_HEAD_POS
#define FS_CLIP_KEYFRAM_HEAD_POS   (FS_CLIP_HEAD_POS + FS_CLIP_HEAD_SIZE)
#endif

#ifndef FS_CLIP_KEYFRAM_HEAD_SIZE
#define FS_CLIP_KEYFRAM_HEAD_SIZE   (4 * VMFS3_KB)
#endif

/**
 * key frame index区长度和开始位置
 */
#ifndef FS_CLIP_KEYFRAM_IDX_POS
#define FS_CLIP_KEYFRAM_IDX_POS     (FS_CLIP_KEYFRAM_HEAD_POS + FS_CLIP_KEYFRAM_HEAD_SIZE)
#endif

#ifndef FS_CLIP_KEYFRAM_IDX_SIZE
#define FS_CLIP_KEYFRAM_IDX_SIZE    (4 * VMFS3_KB)
#endif

/**
 * key frame index的最大数量
 */
#ifndef FS_CLIP_KEYFRAM_IDX_MAX_CNT
#define FS_CLIP_KEYFRAM_IDX_MAX_CNT  (64)
#endif

/**
 * key frame info cnt pre index block每个INDEX块代表的key frame info的个数
 */
#ifndef FS_CLIP_KEYFRAM_CNTPREIDXBLK
#define FS_CLIP_KEYFRAM_CNTPREIDXBLK  (1000)
#endif

/**
 * key frame head中的I帧总数
 */
#ifndef FS_CLIP_KEYFRAMCNT_MAX
#define FS_CLIP_KEYFRAMCNT_MAX      (FS_CLIP_KEYFRAM_CNTPREIDXBLK * FS_CLIP_KEYFRAM_IDX_MAX_CNT)
#endif

/**
 * key frame info区长度和开始位置
 */
#ifndef FS_CLIP_KEYFRAM_INFO_POS
#define FS_CLIP_KEYFRAM_INFO_POS     (FS_CLIP_KEYFRAM_IDX_POS + FS_CLIP_KEYFRAM_IDX_SIZE)
#endif

#ifndef FS_CLIP_KEYFRAM_INFO_SIZE
#define FS_CLIP_KEYFRAM_INFO_SIZE    (sizeof(FsKeyFramInfo) * 64 * VMFS3_KB)
#endif

/**
 * data区开始位置
 */
 #ifndef FS_CLIP_DATA_POS
 #define FS_CLIP_DATA_POS             (FS_CLIP_KEYFRAM_INFO_POS + FS_CLIP_KEYFRAM_INFO_SIZE)
 #endif

 #ifndef FS_CLIP_DATA_SIZE
 #define FS_CLIP_DATA_SIZE            (FS_CLIP_SIZE - FS_CLIP_DATA_POS)
 #endif

/**
 * Journal保护的KeyFramInfo得数量(根据内存中缓存的多少进行调节10~20个)
 */
#ifndef FS_JOURNAL_KEYFRAMINFO_COUNT
#define FS_JOURNAL_KEYFRAMINFO_COUNT  (20)
#endif

#ifndef FS_KEYFRAMINFOIDX_MNGCNT
#define FS_KEYFRAMINFOIDX_MNGCNT      (1000)
#endif

/**
 * journal开始的标志位
 */
#ifndef FS_JOURNAL_MARK
#define FS_JOURNAL_MARK               (0xabadcafe)
#endif

/**
 *
 */
typedef enum {
    FS_CS_WELL = 0,         /** clip好状态 */
    FS_CS_BAD,              /** clip坏状态 */
}EFsClipStatus;

/**
 * 位置信息
 */
typedef struct
{
    fsUint32  dirNo;        /* 目录号 */
    fsUint32  subDirNo;     /* 子目录号 */
    fsUint32  dataUnit;     /* 数据单元号 */
    fsUint32  clip;         /* 数据块号 */
    fsUint32   pos;         /* 位置 */
}FsLctnInfo;

/**
 * 位置信息
 */
typedef struct
{
    fsUint32  dirNo;        /* 目录号 */
    fsUint32  dataUnit;     /* 数据单元号 */
}FsDirLctn;




/**
 * 时间范围
 */
typedef struct
{
    fsUint32    sTm;    /* 开始时间 */
    fsUint32    eTm;    /* 结束时间 */
}FsTmRange;

/**
 * 数据单元信息结构
 */
typedef struct
{
    fsUint32 checkSum;      /* 校验值 */
    fsChar   enable;        /* 当前信息是否有效 */
    fsChar   res[3];        /* 保留 */
    fsUint32 dataUnitLen;   /* 数据单元的长度 */
    fsInt32  clipCnt;       /* 数据单元中Clip块的数量 */
    fsInt32  clipCurr;      /* 正在使用的Clip */
    fsUint32 badClipCnt;    /* 损坏Clip数量 */
    fsInt32  coverCnt;      /* 当前数据单元被复写的次数 */
    fsUint32 resv[5];       /* 保留 */
}FsDatUnitInfo;

/**
 * Clip块的头信息结构体
 */
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
}_FsClipHead;

/**
 * 关键帧描述头结构
 */
typedef struct
{
    fsUint32    checkSum;        /* 校验 */
    fsUint32    kFrInfoCnt;      /* 关键帧数量 */
    fsUint32    kFrInfoCurr;     /* 当前在用的frame info的编号 */
    fsUint32    resv[5];         /* 保留 */
}_FsKeyFramHead;

/**
 * 关键帧信息的索引结构
 */
typedef struct
{
    fsUint32  checkSum;   /* 校验 */
    fsUint32  minTime;    /* 1000个关键帧信息的最小时间 */
    fsUint32  maxTime;    /* 1000个关键帧信息的最大时间 */
    fsUint32  resv[5];    /* 保留 */
}_FsKeyFramInfoIdx;

/**
 * 关键帧信息描述结构
 */
typedef struct
{
    fsUint32    checkSum;    /* 校验 */
    fsUint32    datOffSet;   /* 偏移位置，相对clip数据区位置fsUint32最大能表示4G的偏移 */
    fsUint32    time;        /* 数据时间 */
    fsUint32    dataLength;  /* 大小，这是原来保有位置的空间 */
}_FsKeyFramInfo;

/**
 * journal区结构
 */
typedef struct
{
    fsUint32            checkSum;                                /* 校验 */
    fsUint32            mark;                                    /* 标志位 0xabadcafe */
    _FsClipHead          clipHd;                                  /* ClipHead结构 */
    _FsKeyFramHead       kFrHd;                                   /* KeyFramHead结构 */
    _FsKeyFramInfoIdx    kFrInfoIdx;                              /* KeyFramInfoIdx结构 */
    fsUint32            kFrInfoUsedCnt;                          /* KeyFramInfo已使用个数 */
    _FsKeyFramInfo       kFrInfo[FS_JOURNAL_KEYFRAMINFO_COUNT];   /* KeyFramInfo结构 */
    fsInt32             resv[5];                                 /* 保留 */
}_FsClipJournalArea;

/**
 *  clip头部描述结构
 */
typedef struct
{
    _FsClipJournalArea journal;
    _FsClipHead        clipHead;
    _FsKeyFramHead     keyFrameHead;
    _FsKeyFramInfoIdx  keyFrameIndex;
    _FsKeyFramInfo     keyFrameInfo;
}FsClipDescribeStruct;

#ifdef __cplusplus
}
#endif

#endif//__FSDATACTRL__H__
