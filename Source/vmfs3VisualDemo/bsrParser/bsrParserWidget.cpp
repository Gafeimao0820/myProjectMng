#include "bsrParserWidget.h"
#include "ui_bsrParserWidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFont>
#include <QScrollBar>
#include <QDateTime>

#include "utils.h"
#include "bsrParser.h"

using namespace std;

#include <string>
#include <list>

//Type, SequenceId, Length, Time
#define BSFPINFO_TABLE_COLUMNNUM    4
#define BSFPINFO_TABLE_HEADER_COLUMN0_CONTENT   tr("类型")
#define BSFPINFO_TABLE_HEADER_COLUMN1_CONTENT   tr("序列号(seqId)")
#define BSFPINFO_TABLE_HEADER_COLUMN2_CONTENT   tr("长度")
#define BSFPINFO_TABLE_HEADER_COLUMN3_CONTENT   tr("时间")

#define BSFP_DETAIL_FORMAT  "type=%d\nsubType=%d\nlength=%u\nsequenceId=%u\nchannel=%d\ntimestamp=%u\nformat=[%d, %d, %d, %d]\nticks=%u\n"

#define VEDIO_NAME  "Vedio"
#define AUDIO_NAME  "Audio"

#define MAX_PARSER_SIZE 1024*1024*1024   //1GBytes

BsrParserWidget::BsrParserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BsrParserWidget),
    mState(RUN_STATE_IDLE)
{
    ui->setupUi(this);

    setScrollAuthorSlot();

    setAllToolTips();
    printf("isActive=%d\n", mpAuthorScrollTimer->isActive());

    //donot allowed user edit the filepathLineEdit manually
//    ui->filepathLineEdit->setFocusPolicy(Qt::NoFocus);
    ui->filepathLineEdit->setReadOnly(true);

    setTableFormat();
}

BsrParserWidget::~BsrParserWidget()
{
    delete ui;
}

void BsrParserWidget::setScrollAuthorSlot()
{
    QFont fontFormat;
    fontFormat.setBold(true);
    fontFormat.setPixelSize(18);
    fontFormat.setUnderline(true);
    ui->authorInfoLabel->setFont(fontFormat);

    QPalette pa;
    pa.setColor(QPalette::WindowText, Qt::blue);
    ui->authorInfoLabel->setPalette(pa);

    mAuthorInfo = tr(MY_AUTHOR_INFO);
    mAuthorInfoCursor = 0;
    mpAuthorScrollTimer = new QTimer(this);
    connect(mpAuthorScrollTimer, SIGNAL(timeout()), this, SLOT(scrollAuthorInfo()));
    mpAuthorScrollTimer->start(500);
}

/*
    show author infomation in bottom label looply.
*/
void BsrParserWidget::scrollAuthorInfo()
{
    if(mAuthorInfoCursor > mAuthorInfo.size())
    {
        //To the end of string, should loop it from the beginning.
        mAuthorInfoCursor = 0;
    }
    ui->authorInfoLabel->setText(mAuthorInfo.mid(mAuthorInfoCursor++));
}

void BsrParserWidget::setAllToolTips()
{
    QString browseComment(tr("点我点我~~点我就可以选择一个bsr文件去解析啦^_^"));
    ui->btnBrowerFile->setToolTip(browseComment);
    QString parseComment(tr("点我点我~~点我就可以开始分析文件了哦~~"));
    ui->btnParse->setToolTip(parseComment);

    QString onlyV(tr("点了我，就只能看到视频帧了哦~~快来试试看啊！"));
    ui->btnOnlyShowVideo->setToolTip(onlyV);
    QString onlyA(tr("点了我，就只能看到音频帧了哦~~快来试试看啊！"));
    ui->btnOnlyShowAudio->setToolTip(onlyA);
    QString onlyIFrame(tr("点了我，就只能看到I帧了哦~~快来试试看啊！"));
    ui->btnOnlyShowIFrames->setToolTip(onlyIFrame);
    QString seeAll(tr("点了我，就又能看到所有帧了哦~~快来试试看啊！"));
    ui->btnShowAll->setToolTip(seeAll);

    QString exportAll(tr("迷人绝技之：导出所有帧数据到本地文件"));
    ui->btnExportAllFrameData->setToolTip(exportAll);
    QString exportVideo(tr("迷人绝技之：导出所有视频帧数据到本地文件"));
    ui->btnExportAllVideoData->setToolTip(exportVideo);
    QString exportAudio(tr("迷人绝技之：导出所有音频帧数据到本地文件"));
    ui->btnExportAllAudioData->setToolTip(exportAudio);
    QString exportCurrentSelected(tr("迷人绝技之：导出当前选中的帧数据到本地文件"));
    ui->btnExportCurrentFrameData->setToolTip(exportCurrentSelected);
}

void BsrParserWidget::setTableFormat()
{
    //When init, just table header is needed
    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->setColumnCount(BSFPINFO_TABLE_COLUMNNUM);
    //do some initation to table header
    QStringList tblHeader;
    tblHeader << BSFPINFO_TABLE_HEADER_COLUMN0_CONTENT << BSFPINFO_TABLE_HEADER_COLUMN1_CONTENT <<
                 BSFPINFO_TABLE_HEADER_COLUMN2_CONTENT << BSFPINFO_TABLE_HEADER_COLUMN3_CONTENT;
    ui->tableBsfpInfo->setHorizontalHeaderLabels(tblHeader);
    ui->tableBsfpInfo->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    QFont font = ui->tableBsfpInfo->horizontalHeader()->font();
    font.setBold(true);
    ui->tableBsfpInfo->horizontalHeader()->setFont(font);
    ui->tableBsfpInfo->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tableBsfpInfo->horizontalHeader()->setFixedHeight(25);

    //we donot need row number in each row, so we hide it.
    QHeaderView * rowHeader = ui->tableBsfpInfo->verticalHeader();
    rowHeader->setHidden(true);

    //this table cannot being edit by user
    ui->tableBsfpInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //in this table, we should select a whole row in one time
    ui->tableBsfpInfo->setSelectionBehavior(QAbstractItemView::SelectRows);

    //each time just one row can be selected
    ui->tableBsfpInfo->setSelectionMode(QAbstractItemView::SingleSelection);

    //when a row being selected, set its background color
    ui->tableBsfpInfo->setStyleSheet("selection-background-color::lightbule;");

    //set height of each line
    ui->tableBsfpInfo->verticalHeader()->setDefaultSectionSize(15);

    //should expanding with our layout
    ui->tableBsfpInfo->horizontalHeader()->setStretchLastSection(true);

    //scroll bar format
    ui->tableBsfpInfo->horizontalScrollBar()->setStyleSheet("QScrollBar{background:transparent; height:10px}"
                                                            "QScrollBar::handle{background:lightgray; border:2px solid transparent; border-radius:5px}"
                                                            "QScrollBar::handle:hover{background:gray;}"
                                                            "QScrollBar::sub-line{background:transparent;}"
                                                            "QScrollBar::add-line{background:transparent;}");
    ui->tableBsfpInfo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->tableBsfpInfo->verticalScrollBar()->setStyleSheet("QScrollBar:vertical{"        //垂直滑块整体
                                                              "background:#FFFFFF;"  //背景色
                                                              "padding-top:20px;"    //上预留位置（放置向上箭头）
                                                              "padding-bottom:20px;" //下预留位置（放置向下箭头）
                                                              "padding-left:3px;"    //左预留位置（美观）
                                                              "padding-right:3px;"   //右预留位置（美观）
                                                              "border-left:1px solid #d7d7d7;}"//左分割线
                                                              "QScrollBar::handle:vertical{"//滑块样式
                                                              "background:#dbdbdb;"  //滑块颜色
                                                              "border-radius:6px;"   //边角圆润
                                                              "min-height:80px;}"    //滑块最小高度
                                                              "QScrollBar::handle:vertical:hover{"//鼠标触及滑块样式
                                                              "background:#d0d0d0;}" //滑块颜色
                                                              "QScrollBar::add-line:vertical{"//向下箭头样式
                                                              "background:url(:/images/resource/images/checkout/down.png) center no-repeat;}"
                                                              "QScrollBar::sub-line:vertical{"//向上箭头样式
                                                              "background:url(:/images/resource/images/checkout/up.png) center no-repeat;}");

    //相邻两行，一灰一白
    ui->tableBsfpInfo->setAlternatingRowColors(true);
}

void BsrParserWidget::on_btnBrowerFile_clicked()
{
    QFileDialog * fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle("SelectBsrFile");
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("BsrFiles(*.bsr)"));
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setViewMode(QFileDialog::Detail);

    if(fileDialog->exec())
    {
        QStringList filepaths = fileDialog->selectedFiles();
        mCurFilepath = filepaths[0];
        ui->filepathLineEdit->setText(mCurFilepath);

        mState = RUN_STATE_IDLE;
        ui->tableBsfpInfo->clearContents();
    }
}

void BsrParserWidget::on_btnParse_clicked()
{
    if(RUN_STATE_PARSING == mState)
    {
        //Now it is parsing, cannot parse again.
        QMessageBox::warning(this, tr("正在解析中！"), tr("我正在解析一个bsr，不要点来点去的哈，解析完了再点！"));
        return ;
    }

    mState = RUN_STATE_PARSING;
    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->clearContents();

//    string bsrFilepath = mCurFilepath.toStdString();
    string bsrFilepath(mCurFilepath.toLocal8Bit().data());
    BsrParserSingleton::getInstance()->setBsrFilepath(bsrFilepath);
    BsrParserSingleton::getInstance()->setMaxParserSize(MAX_PARSER_SIZE);
    int ret = BsrParserSingleton::getInstance()->parse();
    if(ret < 0)
    {
        QString errDesc(tr("解析失败了! 原因是 : "));
        errDesc += QString::fromStdString(BsrParserSingleton::getInstance()->getErrStr(
                                              BsrParserSingleton::getInstance()->getErrno()));
        errDesc += "\n" + tr("解析出错的文件是：") + QString::fromStdString(bsrFilepath);
        QMessageBox::warning(this, tr("解析失败"), errDesc);
        mState = RUN_STATE_IDLE;
        return ;
    }
    mState = RUN_STATE_PARSED;

    if(BsrParserSingleton::getInstance()->isTooLargerFile())
    {
        QString desc =
                tr("文件size=") + QString::number(BsrParserSingleton::getInstance()->getFilesize() / 1024 / 1024, 10) + tr("兆字节") + "\n" +
                tr("为了照顾公司的部分硬件性能差的机型,和一部分xp的操作系统，防止其内存被写爆，") + "\n" +
                tr("只解析前") + QString::number(BsrParserSingleton::getInstance()->getMaxParserSize()/1024/1024, 10) + tr("兆字节的数据！");
        QMessageBox::information(this, tr("文件太大"), desc);
    }

    //get all bsfp info
    list<BsfpInfo> curList;
    curList.clear();
    BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_ALL);

    //Set to table
    setBsfpTableValue(curList);

    //show parse ret description info here
    dumpParseRetDesc();
}

void BsrParserWidget::setBsfpTableValue(list<BsfpInfo> & infoList, bool isCheckSeqId)
{
    unsigned int lastVedioSeqId = 0;
    unsigned int lastAudioSeqId = 0;
    int rowCnt = 0;

    for(list<BsfpInfo>::iterator it = infoList.begin(); it != infoList.end(); it++)
    {
        ui->tableBsfpInfo->insertRow(rowCnt);

        QTableWidgetItem *itemType = new QTableWidgetItem();
        QTableWidgetItem *itemSeqId = new QTableWidgetItem();
        QTableWidgetItem *itemLength = new QTableWidgetItem();
        QTableWidgetItem *itemTime = new QTableWidgetItem();

        QString type;
        switch(it->mType)
        {
            case 1:
                type = AUDIO_NAME;
                break;
            case 2:
                type = VEDIO_NAME;
                if(it->mSubType == 1)
                    type += "--I";
                else if(it->mSubType == 2)
                    type += "--B";
                else if(it->mSubType == 3)
                    type += "--P";
                else
                    type += "N/A";
                break;
            default:
                type = "Others : " + QString::number(it->mType, 10);
                break;
        }
        itemType->setText(type);

        itemSeqId->setText(QString::number(it->mSeqNo, 10));

        itemLength->setText(QString::number(it->mLen, 10));

        string locTime = BsrParserSingleton::getInstance()->getLocalTime(it->mTimestamp, it->mTicks);
        QString qLocTime = QString::fromStdString(locTime);
        itemTime->setText(qLocTime);

        if(isCheckSeqId)
        {
            if(it->mType == 1)
            {
                if(lastAudioSeqId == 0)
                    lastAudioSeqId = it->mSeqNo;
                else
                {
                    if(lastAudioSeqId + 1 != it->mSeqNo)
                    {
                        dbgError("Have a sequence No error! lastVedioSeqId=%u, curSeqNo=%u!\n", lastAudioSeqId, it->mSeqNo);

                        itemType->setBackgroundColor(QColor(192, 0, 0));
                        itemSeqId->setBackgroundColor(QColor(192, 0, 0));
                        itemLength->setBackgroundColor(QColor(192, 0, 0));
                        itemTime->setBackgroundColor(QColor(192, 0, 0));
                    }
                    lastAudioSeqId = it->mSeqNo;
                }
            }
            if(it->mType == 2)
            {
                if(lastVedioSeqId == 0)
                    lastVedioSeqId = it->mSeqNo;
                else
                {
                    if(lastVedioSeqId + 1 != it->mSeqNo)
                    {
                        dbgError("Have a sequence No error! lastVedioSeqId=%u, curSeqNo=%u!\n", lastVedioSeqId, it->mSeqNo);

                        itemType->setBackgroundColor(QColor(192, 0, 0));
                        itemSeqId->setBackgroundColor(QColor(192, 0, 0));
                        itemLength->setBackgroundColor(QColor(192, 0, 0));
                        itemTime->setBackgroundColor(QColor(192, 0, 0));
                    }
                    lastVedioSeqId = it->mSeqNo;
                }
            }
        }

        ui->tableBsfpInfo->setItem(rowCnt, 0, itemType);
        ui->tableBsfpInfo->setItem(rowCnt, 1, itemSeqId);
        ui->tableBsfpInfo->setItem(rowCnt, 2, itemLength);
        ui->tableBsfpInfo->setItem(rowCnt, 3, itemTime);

        rowCnt++;
    }
}

void BsrParserWidget::on_tableBsfpInfo_cellDoubleClicked(int row, int column)
{
    QTableWidgetItem * itemType = ui->tableBsfpInfo->item(row, 0);
    QString strType = itemType->text();
    string strStd = strType.toStdString();
    char type = 0;
    if(strType.startsWith(VEDIO_NAME))
        type = 2;
    else if(strType.startsWith(AUDIO_NAME))
        type = 1;
    else
    {
        QString errDesc = tr("你选中的类型是：") + strType + tr("，暂时我不处理这个类型的数据哈，以后再说");
        QMessageBox::warning(this, tr("类型错误"), errDesc);
        return ;
    }

    QTableWidgetItem * itemSeqId = ui->tableBsfpInfo->item(row, 1);
    QString strSeqId = itemSeqId->text();
    unsigned int seqId = strSeqId.toUInt();

    BsfpInfo info;
    int ret = BsrParserSingleton::getInstance()->getDetail(type, seqId, info);
    if(ret != 0)
    {
        QMessageBox::warning(this, tr("读取失败"), tr("获取这个bsfp的细节信息出错啦~~快去找类戈查看原因啦~~"));
        return;
    }

    QString detailDesc = tr("类型：");

    QString typeStr;
    if(type == 1)
        typeStr = tr("音频帧");
    else if(type == 2)
        typeStr = tr("视频帧");
    else
        typeStr = tr("其他类型, type=") + QString::number(info.mType, 10);
    detailDesc += typeStr + "\n";

    QString lengthStr = tr("帧长度(不含bsfp头的长度)：") + QString::number(info.mLen, 10) + tr("字节") + "\n";
    detailDesc += lengthStr;

    QString chanStr = tr("通道号：") + QString::number(info.mChannel, 10) + "\n";
    detailDesc += chanStr;

    QString formatStr;
    if(info.mType == 1)   //Audio
    {
        formatStr = tr("音频帧信息") + "\n\t" + tr("编码--");

        switch(info.mFormat[0])
        {
        case 4: //aac
            formatStr += tr("AAC; ") + "\n\t";
            break;
        case 9: //g711u
            formatStr += tr("G711U; ") + "\n\t";
            break;
        case 11://g711a
            formatStr += tr("G711A; ") + "\n\t";
            break;
        case 109://hisi g711u
            formatStr += tr("带海思4字节头的G711U; ") + "\n\t";
            break;
        case 111://hisi g711a
            formatStr += tr("带海思4字节头的G711A; ") + "\n\t";
            break;
        case 26:    //g726
            formatStr += tr("G726; ") + "\n\t";
            break;
        case 126:   //hisi g726
            formatStr += tr("带海思4字节头的G726; ") + "\n\t";
            break;
        default:
            formatStr += tr("吼吼，format[0]=") + QString::number(info.mFormat[0], 10) + tr(",查查bsfp协议看看是啥吧^_^") + "\n\t";
            break;
        }

        formatStr += tr("分辨率--") + QString::number(info.mFormat[1], 10) + "\n\t";

        formatStr += tr("音频采样率--") + QString::number(info.mFormat[2], 10) + tr("千赫兹") + "\n\t";

        formatStr += tr("码率--") + QString::number(info.mFormat[3], 10) + "\n";
    }
    else if(info.mType == 2)    //video
    {
        formatStr += tr("视频帧信息") + "\n\t" + "编码--";

        switch(info.mFormat[0])
        {
        case 1:
            formatStr += tr("mpeg4;") + "\n\t";
            break;
        case 2:
            formatStr += tr("m-jpeg;") + "\n\t";
            break;
        case 4:
            formatStr += tr("h264;") + "\n\t";
            break;
        case 5:
            formatStr += tr("h265;") + "\n\t";
            break;
        default:
            formatStr += tr("吼吼，format[0]=") + QString::number(info.mFormat[0], 10) + tr(",查查bsfp协议看看是啥吧^_^") + "\n\t";
            break;
        }

        switch(info.mFormat[1])
        {
        case 1:
            formatStr += tr("分辨率--") + "QCIF(176*144)" + "\n\t";
            break;
        case 2:
            formatStr += tr("分辨率--") + "CIF(352*288)" + "\n\t";
            break;
        case 3:
            formatStr += tr("分辨率--") + "halfD1(704*288)" + "\n\t";
            break;
        case 4:
            formatStr += tr("分辨率--") + "D1(704*576)" + "\n\t";
            break;
        case 11:
            formatStr += tr("分辨率--") + "720I(1280*720)" + "\n\t";
            break;
        case 12:
            formatStr += tr("分辨率--") + "720P(1280*720)" + "\n\t";
            break;
        case 13:
            formatStr += tr("分辨率--") + "1080I(1920*1080)" + "\n\t";
            break;
        case 14:
            formatStr += tr("分辨率--") + "1080P(1920*1080)" + "\n\t";
            break;
        case 15:
            formatStr += tr("分辨率--") + "960H(960*576)" + "\n\t";
            break;
        case 18:
            formatStr += tr("分辨率--") + "5M(2592*1944)" + "\n\t";
            break;
        case 19:
            formatStr += tr("分辨率--") + "6M(3072*2048)" + "\n\t";
            break;
        case 20:
            formatStr += tr("分辨率--") + "8M(3840*2160)" + "\n\t";
            break;
        case 21:
            formatStr += tr("分辨率--") + "7M(3480*2160)" + "\n\t";
            break;
        case 22:
            formatStr += tr("分辨率--") + "12M(4000*3000)" + "\n\t";
            break;
        case 57:
            formatStr += tr("分辨率--") + "NA(2560*2048)" + "\n\t";
            break;
        default:
            formatStr += tr("分辨率实在是太多啦，类戈不愿意再打啦~~自己去翻文档吧, value=") + QString::number(info.mFormat[1], 10) + "\n\t";
            break;
        }

        formatStr += tr("I帧间隔--") + QString::number(info.mFormat[2], 10) + "\n\t";

        switch(info.mFormat[3])
        {
        case 1:
            formatStr += tr("帧类型--") + tr("I帧") + "\n";
            break;
        case 2:
            formatStr += tr("帧类型--") + tr("B帧") + "\n";
            break;
        case 3:
            formatStr += tr("帧类型--") + tr("P帧") + "\n";
            break;
        default:
            formatStr += tr("帧类型--") + tr("value=") + QString::number(info.mFormat[3], 10) + tr("，这个类型类戈不知道是个啥......") + "\n";
        }
    }

    detailDesc += formatStr;
    QMessageBox::information(this, tr("bsfp信息"), detailDesc);
}

void BsrParserWidget::on_btnOnlyShowVideo_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->clearContents();

    //get bsfp info
    list<BsfpInfo> curList;
    curList.clear();
    BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_VIDEO);

    //Set to table
    setBsfpTableValue(curList);
    int rowNum = ui->tableBsfpInfo->rowCount();
    QString rowNumInfo = tr("合计：") + QString::number(rowNum, 10) + tr("条bsfp信息");
    QMessageBox::information(this, tr("合计"), rowNumInfo);
}

void BsrParserWidget::on_btnOnlyShowIFrames_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->clearContents();

    //get bsfp info
    list<BsfpInfo> curList;
    curList.clear();
    BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_IFRAME);

    //Set to table
    setBsfpTableValue(curList, false);
    int rowNum = ui->tableBsfpInfo->rowCount();
    QString rowNumInfo = tr("合计：") + QString::number(rowNum, 10) + tr("条bsfp信息");
    QMessageBox::information(this, tr("合计"), rowNumInfo);
}

void BsrParserWidget::on_btnOnlyShowAudio_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->clearContents();

    //get bsfp info
    list<BsfpInfo> curList;
    curList.clear();
    BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_AUDIO);

    //Set to table
    setBsfpTableValue(curList);
    int rowNum = ui->tableBsfpInfo->rowCount();
    QString rowNumInfo = tr("合计：") + QString::number(rowNum, 10) + tr("条bsfp信息");
    QMessageBox::information(this, tr("合计"), rowNumInfo);
}

void BsrParserWidget::on_btnShowAll_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    ui->tableBsfpInfo->setRowCount(0);
    ui->tableBsfpInfo->clearContents();

    //get bsfp info
    list<BsfpInfo> curList;
    curList.clear();
    BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_ALL);

    //Set to table
    setBsfpTableValue(curList);
    int rowNum = ui->tableBsfpInfo->rowCount();
    QString rowNumInfo = tr("合计：") + QString::number(rowNum, 10) + tr("条bsfp信息");
    QMessageBox::information(this, tr("合计"), rowNumInfo);
}

void BsrParserWidget::on_btnExportAllFrameData_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何操作！"));
        return ;
    }

    QDateTime curDateTime = QDateTime::currentDateTime();
    int curTimestamp = curDateTime.toTime_t();
    char curTimeChars[16] = {0x00};
    sprintf(curTimeChars, "%d", curTimestamp);
    string filepath(curTimeChars);
    filepath += "_allAVFrame.data";
    int ret = BsrParserSingleton::getInstance()->dumpAllFrame2File(filepath);
    if(ret != PARSEBSRFILE_ERR_OK)
    {
        QString errDesc = tr("失败原因是:");
        errDesc += QString::fromStdString(BsrParserSingleton::getInstance()->getErrStr(ret));
        QMessageBox::warning(this, tr("导出失败"), errDesc);
    }
    else
    {
        QString expFilepath = QString::fromStdString(filepath);
        QString desc = tr("导出所有音视频文件成功！\n文件导出到了：");
        desc += expFilepath;
        desc += "\n";
        QMessageBox::information(this, tr("导出成功"), desc);
    }
}

void BsrParserWidget::on_btnExportAllVideoData_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何操作！"));
        return ;
    }

    QDateTime curDateTime = QDateTime::currentDateTime();
    int curTimestamp = curDateTime.toTime_t();
    char curTimeChars[16] = {0x00};
    sprintf(curTimeChars, "%d", curTimestamp);
    string filepath(curTimeChars);
    filepath += "_allVideoFrame.data";
    int ret = BsrParserSingleton::getInstance()->dumpAllVedioFrame2File(filepath);
    if(ret != PARSEBSRFILE_ERR_OK)
    {
        QString errDesc = tr("失败原因是:");
        errDesc += QString::fromStdString(BsrParserSingleton::getInstance()->getErrStr(ret));
        QMessageBox::warning(this, tr("导出失败"), errDesc);
    }
    else
    {
        QString expFilepath = QString::fromStdString(filepath);
        QString desc = tr("导出所有视频文件成功！\n文件导出到了：");
        desc += expFilepath;
        desc += "\n";
        QMessageBox::information(this, tr("导出成功"), desc);
    }
}

void BsrParserWidget::on_btnExportAllAudioData_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何操作！"));
        return ;
    }

    QDateTime curDateTime = QDateTime::currentDateTime();
    int curTimestamp = curDateTime.toTime_t();
    char curTimeChars[16] = {0x00};
    sprintf(curTimeChars, "%d", curTimestamp);
    string filepath(curTimeChars);
    filepath += "_allAudioFrame.data";
    int ret = BsrParserSingleton::getInstance()->dumpAllAudioFrame2File(filepath);
    if(ret != PARSEBSRFILE_ERR_OK)
    {
        QString errDesc = tr("失败原因是:");
        errDesc += QString::fromStdString(BsrParserSingleton::getInstance()->getErrStr(ret));
        QMessageBox::warning(this, tr("导出失败"), errDesc);
    }
    else
    {
        QString expFilepath = QString::fromStdString(filepath);
        QString desc = tr("导出所有音频文件成功！\n文件导出到了：");
        desc += expFilepath;
        desc += "\n";
        QMessageBox::information(this, tr("导出成功"), desc);
    }
}

void BsrParserWidget::on_btnExportCurrentFrameData_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何操作！"));
        return ;
    }

    //get which line being selected
    int curRow = ui->tableBsfpInfo->currentIndex().row();
    if(curRow < 0)
    {
        QMessageBox::warning(this, tr("没有选中行"), tr("必须先选中一行音视频帧，才能执行导出动作！"));
        return ;
    }
    QAbstractItemModel * pModel = ui->tableBsfpInfo->model();
    QModelIndex indexType = pModel->index(curRow, 0);   //the first column is "type"
    QModelIndex indexSeqId = pModel->index(curRow, 1);   //the second column is "seqId"
    QVariant varType = pModel->data(indexType);
    QVariant varSeqId = pModel->data(indexSeqId);

    QString typeStr = varType.toString();
    string strStd = typeStr.toStdString();
    char type = 0;
    if(typeStr.startsWith(VEDIO_NAME))
        type = 2;
    else if(typeStr.startsWith(AUDIO_NAME))
        type = 1;
    else
    {
        QString errDesc = tr("所选行的类型是：") + typeStr + tr("，暂时类戈是不处理这个类型的数据哈，以后再说~~");
        QMessageBox::warning(this, tr("类型错误"), errDesc);
        return ;
    }

    unsigned int seqId = varSeqId.toUInt();

    QDateTime curDateTime = QDateTime::currentDateTime();
    int curTimestamp = curDateTime.toTime_t();
    char curTimeChars[64] = {0x00};
    sprintf(curTimeChars, "%d_type%d_seqId%u", curTimestamp, type, seqId);
    string filepath(curTimeChars);
    filepath += "_oneFrame.data";
    int ret = BsrParserSingleton::getInstance()->dumpOneFrameData2File(type, seqId, filepath);
    if(ret != PARSEBSRFILE_ERR_OK)
    {
        QString errDesc = tr("失败原因是:");
        errDesc += QString::fromStdString(BsrParserSingleton::getInstance()->getErrStr(ret));
        QMessageBox::warning(this, tr("导出失败"), errDesc);
    }
    else
    {
        QString expFilepath = QString::fromStdString(filepath);
        QString desc = tr("导出所选帧文件成功！\n文件导出到了：");
        desc += expFilepath;
        desc += "\n";
        QMessageBox::information(this, tr("导出成功"), desc);
    }
}

void BsrParserWidget::dumpParseRetDesc()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    unsigned int firstBsfpOffset = BsrParserSingleton::getInstance()->getFirstBsfpOffset();

    unsigned long long int allFrameNum = BsrParserSingleton::getInstance()->getAllFrameNum();
    int allVideoFrameNum = BsrParserSingleton::getInstance()->getAllVideoFrameNum();
    int allAudioFrameNum = BsrParserSingleton::getInstance()->getAllAudioFrameNum();
    unsigned long long int allLostFrameNum = BsrParserSingleton::getInstance()->getAllLostFrameNum();
    int allLostVideoFrameNum = BsrParserSingleton::getInstance()->getAllLostVideoFrameNum();
    int allLostAudioFrameNum = BsrParserSingleton::getInstance()->getAllLostAudioFrameNum();
    unsigned long long int allDirtyDataNum = BsrParserSingleton::getInstance()->getDirtyDataNum();

    QString desc;

    desc += tr("第一个有效的bsfp offset：") + QString::number(firstBsfpOffset, 10);

    desc += tr("\n总帧数：") + QString::number(allFrameNum, 10);
    desc += tr("\n视频帧总帧数：") + QString::number(allVideoFrameNum, 10);
    desc += tr("\n音频帧总帧数：") + QString::number(allAudioFrameNum, 10);

    desc += tr("\n\n丢帧总数：") + QString::number(allLostFrameNum, 10);
    desc += tr("\n视频帧丢帧总数：") + QString::number(allLostVideoFrameNum, 10);
    desc += tr("\n音频帧丢帧总数：") + QString::number(allLostAudioFrameNum, 10);

    desc += tr("\n\n脏数据块总数：") + QString::number(allDirtyDataNum, 10);

    QMessageBox::information(this, tr("解析基本信息"), desc);
}

void BsrParserWidget::on_btnParseRetDesc_clicked()
{
    dumpParseRetDesc();
}

void BsrParserWidget::on_btnParseRetErrorFrameVideo_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    int allLostVideoFrameNum = BsrParserSingleton::getInstance()->getAllLostVideoFrameNum();
    if(allLostVideoFrameNum <= 0)
    {
        QMessageBox::information(this, tr("视频帧丢帧概览"), tr("不存在视频帧丢帧^_^"));
    }
    else
    {
        //get bsfp info
        list<BsfpInfo> curList;
        curList.clear();
        BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_VIDEO);

        //check for sequence id, if lost frame being find, just save it;
        QString desc;
        list<BsfpInfo>::iterator it = curList.begin();
        unsigned int preSeqId = it->mSeqNo;
        unsigned int preTimestamp = it->mTimestamp;
        it++;
        for(; it != curList.end(); it++)
        {
            if(it->mSeqNo > preSeqId && it->mSeqNo - preSeqId != 1)
            {
                desc += tr("上一帧：seqId=") + QString::number(preSeqId, 10) + tr(", timestamp=") + QString::number(preTimestamp, 10) +
                        tr("; 下一帧：seqId=") + QString::number(it->mSeqNo, 10) + tr(", timestamp=") + QString::number(it->mTimestamp, 10) +
                        tr("; 丢帧总数：") + QString::number(it->mSeqNo - preSeqId) + tr(";\n");
                preSeqId = it->mSeqNo;
                preTimestamp = it->mTimestamp;
            }
        }
        QMessageBox::information(this, tr("视频帧丢帧概览"), desc);
    }
}

void BsrParserWidget::on_btnParseRetErrorFrameAudio_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    int allLostVideoFrameNum = BsrParserSingleton::getInstance()->getAllLostVideoFrameNum();
    if(allLostVideoFrameNum <= 0)
    {
        QMessageBox::information(this, tr("音频帧丢帧概览"), tr("不存在音频帧丢帧^_^"));
    }
    else
    {
        //get bsfp info
        list<BsfpInfo> curList;
        curList.clear();
        BsrParserSingleton::getInstance()->getBsfpsInfo(curList, QUERY_TYPE_AUDIO);

        //check for sequence id, if lost frame being find, just save it;
        QString desc;
        list<BsfpInfo>::iterator it = curList.begin();
        unsigned int preSeqId = it->mSeqNo;
        unsigned int preTimestamp = it->mTimestamp;
        it++;
        for(; it != curList.end(); it++)
        {
            if(it->mSeqNo > preSeqId && it->mSeqNo - preSeqId != 1)
            {
                desc += tr("上一帧：seqId=") + QString::number(preSeqId, 10) + tr(", timestamp=") + QString::number(preTimestamp, 10) +
                        tr("; 下一帧：seqId=") + QString::number(it->mSeqNo, 10) + tr(", timestamp=") + QString::number(it->mTimestamp, 10) +
                        tr("; 丢帧总数：") + QString::number(it->mSeqNo - preSeqId) + tr(";\n");
                preSeqId = it->mSeqNo;
                preTimestamp = it->mTimestamp;
            }
        }
        QMessageBox::information(this, tr("音频帧丢帧概览"), desc);
    }
}

void BsrParserWidget::on_btnParseRetDirtyData_clicked()
{
    if(mState != RUN_STATE_PARSED)
    {
        QMessageBox::warning(this, tr("请先解析bsr"), tr("没有bsr文件被解析，无法执行任何显示操作！"));
        return ;
    }

    list<ErrPart> dirtyDataList;
    BsrParserSingleton::getInstance()->getDirtyDataList(dirtyDataList);
    QString desc;
    for(list<ErrPart>::iterator it = dirtyDataList.begin(); it != dirtyDataList.end(); it++)
    {
        desc += tr("脏数据开始位置:") + QString::number(it->offset, 10) + tr("， 脏数据长度:") + QString::number(it->length, 10) + tr(";\n");
    }
    QMessageBox::information(this, tr("脏数据概览"), desc);
}
