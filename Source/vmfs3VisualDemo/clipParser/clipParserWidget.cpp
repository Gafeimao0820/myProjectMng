#include "clipParserWidget.h"
#include "ui_clipParserWidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFont>
#include <QScrollBar>
#include <QDateTime>

#include "utils.h"
#include "clipParser.h"

using namespace std;

#include <string>
#include <list>

//ClipHeader, 10 attributes
#define CLIPHEADER_ITEM_NAME      "ClipHeader"
#define CLIPHEADER_ATTR_NUM       10
#define CLIPHEADER_ATTR0_NAME     "ClipId"
#define CLIPHEADER_ATTR1_NAME     "ClipStatus"
#define CLIPHEADER_ATTR2_NAME     "PreClipId"
#define CLIPHEADER_ATTR3_NAME     "NextClipId"
#define CLIPHEADER_ATTR4_NAME     "StreamId"
#define CLIPHEADER_ATTR5_NAME     "MinTime"
#define CLIPHEADER_ATTR6_NAME     "MaxTime"
#define CLIPHEADER_ATTR7_NAME     "DataSpaceLen"
#define CLIPHEADER_ATTR8_NAME     "DataSpaceUsedLen"
#define CLIPHEADER_ATTR9_NAME     "CoverCount"
//KeyFrameHeader, 2 attributes
#define KEYFRAMEHEADER_ITEM_NAME    "KeyFrameHeader"
#define KEYFRAMEHEADER_ATTR_NUM     2
#define KEYFRAMEHEADER_ATTR0_NAME   "KeyFrameInfoCount"
#define KEYFRAMEHEADER_ATTR1_NAME   "KeyFrameInfoCurrent"
//KeyFrameIndex, 2 columns, first is "minTime", second is "maxTime"; at most 64 rows
#define KEYFRAMEINDEX_ITEM_NAME     "KeyFrameIndex"
#define KEYFRAMEINDEX_SUBITEM_NAME  "Index_"
#define KEYFRAMEINDEX_ATTR_NUM      2
#define KEYFRAMEINDEX_ATTR0_NAME   "MinTime"
#define KEYFRAMEINDEX_ATTR1_NAME   "MaxTime"
//KeyFrameInfo, 3 columns, first is "DataOffset", second is "Time", third is "DataLength"; at most 1000 rows
#define KEYFRAMEINFO_ITEM_NAME              "KeyFrameInfo"
#define KEYFRAMEINFO_TABLE_COLUMN_NUM       4
#define KEYFRAMEINFO_TABLE_COLUMN0_NAME     "Id"
#define KEYFRAMEINFO_TABLE_COLUMN1_NAME     "DataOffset"
#define KEYFRAMEINFO_TABLE_COLUMN2_NAME     "Time"
#define KEYFRAMEINFO_TABLE_COLUMN3_NAME     "DataLength"

ClipParserWidget::ClipParserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClipParserWidget),
    mLastShowKeyFrameIndex(-1)
{
    ui->setupUi(this);

    setScrollAuthorSlot();

    setAllToolTips();
    ui->filepathLineEdit->setReadOnly(true);

    setTableFormat();

    ui->clipInfoTreeWidget->clear();
}

ClipParserWidget::~ClipParserWidget()
{
    ui->clipInfoTreeWidget->clear();
    delete ui;
}

void ClipParserWidget::setScrollAuthorSlot()
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
void ClipParserWidget::scrollAuthorInfo()
{
    if(mAuthorInfoCursor > mAuthorInfo.size())
    {
        //To the end of string, should loop it from the beginning.
        mAuthorInfoCursor = 0;
    }
    ui->authorInfoLabel->setText(mAuthorInfo.mid(mAuthorInfoCursor++));
}

void ClipParserWidget::setAllToolTips()
{
    QString browseComment(tr("点我点我~~点我就可以选择一个clip文件去解析啦^_^"));
    ui->btnBrowerFile->setToolTip(browseComment);
    QString parseComment(tr("点我点我~~点我就可以开始分析clip文件了哦~~"));
    ui->btnParse->setToolTip(parseComment);
}

void ClipParserWidget::setTableFormat()
{
    //When init, just table header is needed
    ui->tableClipInfo->clear();
    ui->tableClipInfo->setRowCount(0);
    ui->tableClipInfo->setColumnCount(KEYFRAMEINFO_TABLE_COLUMN_NUM);
    //do some initation to table header
    QStringList tblHeader;
    tblHeader << KEYFRAMEINFO_TABLE_COLUMN0_NAME << KEYFRAMEINFO_TABLE_COLUMN1_NAME << KEYFRAMEINFO_TABLE_COLUMN2_NAME << KEYFRAMEINFO_TABLE_COLUMN3_NAME;
    ui->tableClipInfo->setHorizontalHeaderLabels(tblHeader);
    ui->tableClipInfo->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    QFont font = ui->tableClipInfo->horizontalHeader()->font();
    font.setBold(true);
    ui->tableClipInfo->horizontalHeader()->setFont(font);
    ui->tableClipInfo->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tableClipInfo->horizontalHeader()->setFixedHeight(45);

    ui->tableClipInfo->setColumnWidth(0, 40);
    ui->tableClipInfo->setColumnWidth(1, 80);
    ui->tableClipInfo->setColumnWidth(2, 240);
    ui->tableClipInfo->setColumnWidth(3, 80);

    //we donot need row number in each row, so we hide it.
    QHeaderView * rowHeader = ui->tableClipInfo->verticalHeader();
    rowHeader->setHidden(true);

    //this table cannot being edit by user
    ui->tableClipInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //in this table, we should select a whole row in one time
    ui->tableClipInfo->setSelectionBehavior(QAbstractItemView::SelectRows);

    //each time just one row can be selected
    ui->tableClipInfo->setSelectionMode(QAbstractItemView::SingleSelection);

    //when a row being selected, set its background color
    ui->tableClipInfo->setStyleSheet("selection-background-color::lightbule;");

    //set height of each line
    ui->tableClipInfo->verticalHeader()->setDefaultSectionSize(15);

    //should expanding with our layout
    ui->tableClipInfo->horizontalHeader()->setStretchLastSection(true);

    //scroll bar format
    ui->tableClipInfo->horizontalScrollBar()->setStyleSheet("QScrollBar{background:transparent; height:10px}"
                                                            "QScrollBar::handle{background:lightgray; border:2px solid transparent; border-radius:5px}"
                                                            "QScrollBar::handle:hover{background:gray;}"
                                                            "QScrollBar::sub-line{background:transparent;}"
                                                            "QScrollBar::add-line{background:transparent;}");
    ui->tableClipInfo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->tableClipInfo->verticalScrollBar()->setStyleSheet("QScrollBar:vertical{"        //垂直滑块整体
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
    ui->tableClipInfo->setAlternatingRowColors(true);
}

void ClipParserWidget::on_btnBrowerFile_clicked()
{
    QFileDialog * fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle("SelectBsrFile");
    fileDialog->setDirectory(".");
    fileDialog->setNameFilter(tr("ClipFiles(*.clip)"));
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setViewMode(QFileDialog::Detail);

    if(fileDialog->exec())
    {
        QStringList filepaths = fileDialog->selectedFiles();
        mCurFilepath = filepaths[0];
        ui->filepathLineEdit->setText(mCurFilepath);

        ui->tableClipInfo->clearContents();
    }
}

void ClipParserWidget::on_btnParse_clicked()
{
    string clipFilepath(mCurFilepath.toLocal8Bit().data());
    if(clipFilepath.size() == 0)
    {
        QMessageBox::warning(this, tr("解析异常"), tr("选择需要解析的clip文件！"));
        return ;
    }

    //should clear treeWidget and tableWidget first
    ui->clipInfoTreeWidget->clear();
    setTableFormat();
    mLastShowKeyFrameIndex = -1;

    //parse the file, and set results to treeWidget
    ClipParserSingleton::getInstance()->resetFilepath(clipFilepath);
    int ret = ClipParserSingleton::getInstance()->parse();
    if(ret < 0)
    {
        QString errDesc(tr("解析失败了! 原因是 : "));
        errDesc += QString::fromStdString(ClipParserSingleton::getInstance()->getErrStr(
                                              ClipParserSingleton::getInstance()->getErrno()));
        errDesc += "\n" + tr("解析出错的文件是：") + QString::fromStdString(clipFilepath);
        QMessageBox::warning(this, tr("解析失败"), errDesc);
        return ;
    }
    dbgDebug("parse clipfile[%s] succeed.\n", clipFilepath.c_str());
    QString retDesc(tr("解析clip文件："));   retDesc += mCurFilepath;    retDesc += tr("成功！");
    QMessageBox::information(this, tr("解析完成"), retDesc);

    //set values to treeWidget
    ui->clipInfoTreeWidget->setColumnCount(1);
    ui->clipInfoTreeWidget->setHeaderLabel(tr("clip信息"));

    //ClipHeader is the first item being showed to user
    FsClipHead clipHeader;
    ClipParserSingleton::getInstance()->getClipHeader(clipHeader);
    QTreeWidgetItem * clipHeaderItem = new QTreeWidgetItem(ui->clipInfoTreeWidget, QStringList(QString(CLIPHEADER_ITEM_NAME)));
//    clipHeaderItem->setIcon("xxx.ico");
    QString curAttrValueStr(CLIPHEADER_ATTR0_NAME);   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.clipId);    //clipId
    QTreeWidgetItem * clipHeaderIdSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(curAttrValueStr));
    clipHeaderItem->addChild(clipHeaderIdSubItem);
    curAttrValueStr = CLIPHEADER_ATTR1_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.clipStatus);    //clipStatus
    QTreeWidgetItem * clipHeaderStatusSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderStatusSubItem);
    curAttrValueStr = CLIPHEADER_ATTR2_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.perClipId);    //preClipId
    QTreeWidgetItem * clipHeaderPreClipIdSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderPreClipIdSubItem);
    curAttrValueStr = CLIPHEADER_ATTR3_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.nextClipId);    //nextClipId
    QTreeWidgetItem * clipHeaderNextClipIdSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderNextClipIdSubItem);
    curAttrValueStr = CLIPHEADER_ATTR4_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.streamId);    //streamId
    QTreeWidgetItem * clipHeaderStreamIdSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderStreamIdSubItem);
    curAttrValueStr = CLIPHEADER_ATTR5_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.minTime);    curAttrValueStr += "("; curAttrValueStr += QString::fromStdString(convTimestampToLocalTime(clipHeader.minTime));    curAttrValueStr += ")";    //Mintime
    QTreeWidgetItem * clipHeaderMinTimeSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderMinTimeSubItem);
    curAttrValueStr = CLIPHEADER_ATTR6_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.maxTime);   curAttrValueStr += "("; curAttrValueStr += QString::fromStdString(convTimestampToLocalTime(clipHeader.maxTime));    curAttrValueStr += ")";    //maxTime
    QTreeWidgetItem * clipHeaderMaxTimeSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderMaxTimeSubItem);
    curAttrValueStr = CLIPHEADER_ATTR7_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.datSpLen);    //dataSpaceLen
    QTreeWidgetItem * clipHeaderDataSpaceLenSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderDataSpaceLenSubItem);
    curAttrValueStr = CLIPHEADER_ATTR8_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.datSpUseLen);    //dataSpaceUsedLen
    QTreeWidgetItem * clipHeaderDataSpaceUsedLenSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderDataSpaceUsedLenSubItem);
    curAttrValueStr = CLIPHEADER_ATTR9_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(clipHeader.coverCnt);    //coverCount
    QTreeWidgetItem * clipHeaderCoverCountSubItem = new QTreeWidgetItem(clipHeaderItem, QStringList(QString(curAttrValueStr)));
    clipHeaderItem->addChild(clipHeaderCoverCountSubItem);

    //KeyFrameHeader
    FsKeyFramHead keyFrameHeader;
    ClipParserSingleton::getInstance()->getKeyFrameHeader(keyFrameHeader);
    QTreeWidgetItem * keyFrameHeaderItem = new QTreeWidgetItem(ui->clipInfoTreeWidget, QStringList(QString(KEYFRAMEHEADER_ITEM_NAME)));
    curAttrValueStr = KEYFRAMEHEADER_ATTR0_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(keyFrameHeader.kFrInfoCnt);    //keyFrameinfoCount
    QTreeWidgetItem * keyFrameHeaderInfoCountSubItem = new QTreeWidgetItem(keyFrameHeaderItem, QStringList(curAttrValueStr));
    keyFrameHeaderItem->addChild(keyFrameHeaderInfoCountSubItem);
    curAttrValueStr = KEYFRAMEHEADER_ATTR1_NAME;   curAttrValueStr += ": ";  curAttrValueStr += QString::number(keyFrameHeader.kFrInfoCurr);    //keyFrameinfoCount
    QTreeWidgetItem * keyFrameHeaderInfoCurrSubItem = new QTreeWidgetItem(keyFrameHeaderItem, QStringList(curAttrValueStr));
    keyFrameHeaderItem->addChild(keyFrameHeaderInfoCurrSubItem);

    //KeyFrameIndex, at most 64
    array<FsKeyFramInfoIdx, FS_CLIP_KEYFRAM_IDX_MAX_CNT> keyFrameIndexArray;
    ClipParserSingleton::getInstance()->getKeyFrameIndex(keyFrameIndexArray);
    QTreeWidgetItem * keyFrameIndexItem = new QTreeWidgetItem(ui->clipInfoTreeWidget, QStringList(QString(KEYFRAMEINDEX_ITEM_NAME)));   //first level, KeyFrameIndex
    int i = 0;
    for(i = 0; i < FS_CLIP_KEYFRAM_IDX_MAX_CNT; i++)
    {
        QString curSubItemStr = KEYFRAMEINDEX_SUBITEM_NAME;     curSubItemStr += QString::number(i);
        if(keyFrameIndexArray[i].minTime == 0 && keyFrameIndexArray[i].maxTime == 0)    curSubItemStr += tr(" - 未使用");
        QTreeWidgetItem * keyFrameIndexSubItem = new QTreeWidgetItem(keyFrameIndexItem, QStringList(curSubItemStr));    //second level, Index_N
        if(keyFrameIndexArray[i].minTime == 0 && keyFrameIndexArray[i].maxTime == 0)    keyFrameIndexSubItem->setBackground(0, QBrush(QColor("#808069")));

        curAttrValueStr = KEYFRAMEINDEX_ATTR0_NAME; curAttrValueStr += ": ";    curAttrValueStr += QString::number(keyFrameIndexArray[i].minTime);   curAttrValueStr += "("; curAttrValueStr += QString::fromStdString(convTimestampToLocalTime(keyFrameIndexArray[i].minTime));    curAttrValueStr += ")";    //third level, minTime and maxTime of it
        QTreeWidgetItem * minTimeSubItem = new QTreeWidgetItem(keyFrameIndexSubItem, QStringList(curAttrValueStr));
        keyFrameIndexSubItem->addChild(minTimeSubItem);
        curAttrValueStr = KEYFRAMEINDEX_ATTR1_NAME; curAttrValueStr += ": ";    curAttrValueStr += QString::number(keyFrameIndexArray[i].maxTime);   curAttrValueStr += "("; curAttrValueStr += QString::fromStdString(convTimestampToLocalTime(keyFrameIndexArray[i].maxTime));    curAttrValueStr += ")";    //third level, minTime and maxTime of it
        QTreeWidgetItem * maxTimeSubItem = new QTreeWidgetItem(keyFrameIndexSubItem, QStringList(curAttrValueStr));
        keyFrameIndexSubItem->addChild(maxTimeSubItem);

        keyFrameIndexItem->addChild(keyFrameIndexSubItem);
    }

    ui->clipInfoTreeWidget->expandItem(clipHeaderItem);  //expandAll() can expand all items
}

void ClipParserWidget::setClipTableValue(vector < FsKeyFramInfo > & allKeyFrameInfos)
{
    setTableFormat();

    int rowCnt = 0;
    for(vector < FsKeyFramInfo >::iterator it = allKeyFrameInfos.begin(); it != allKeyFrameInfos.end(); it++)
    {
        ui->tableClipInfo->insertRow(rowCnt);

        QTableWidgetItem * itemId = new QTableWidgetItem();
        itemId->setText(QString::number(rowCnt));

        QTableWidgetItem *itemDataOffset = new QTableWidgetItem();
        itemDataOffset->setText(QString::number(it->datOffSet));

        QTableWidgetItem *itemTime = new QTableWidgetItem();
        QString timeStr = QString::number(it->time);
        timeStr += " - ";
        timeStr += QString::fromStdString(convTimestampToLocalTime(it->time));
        itemTime->setText(timeStr);

        QTableWidgetItem *itemDataLength = new QTableWidgetItem();
        itemDataLength->setText(QString::number(it->dataLength));

        if(it->datOffSet == 0 && it->time == 0 && it->dataLength == 0)
        {
            itemId->setBackgroundColor(QColor("#808069"));
            itemDataOffset->setBackgroundColor(QColor("#808069"));
            itemTime->setBackgroundColor(QColor("#808069"));
            itemDataLength->setBackgroundColor(QColor("#808069"));
        }

        ui->tableClipInfo->setItem(rowCnt, 0, itemId);
        ui->tableClipInfo->setItem(rowCnt, 1, itemDataOffset);
        ui->tableClipInfo->setItem(rowCnt, 2, itemTime);
        ui->tableClipInfo->setItem(rowCnt, 3, itemDataLength);

        rowCnt++;
    }
}

/*
    When double clicked, a messageBox appeared, can export I frame and GOP to local file;
*/
void ClipParserWidget::on_tableClipInfo_cellDoubleClicked(int row, int column)
{
    QTableWidgetItem * itemId = ui->tableClipInfo->item(row, 0);
    int keyFrameInfoNo = itemId->text().toInt();
    QString exportDesc = tr("骚年，是要导出这个I帧了吗？") + "\n";
    exportDesc += tr("要导出I帧的话，类戈会附送GOP哦~~~") + "\n";
    exportDesc += ("\tKeyFrameIndex=");   exportDesc += QString::number(mLastShowKeyFrameIndex);  exportDesc += ("\n");
    exportDesc += ("\tKeyFrameInfoNo=");   exportDesc += QString::number(keyFrameInfoNo);  exportDesc += ("\n");
    QMessageBox::StandardButton retBtn = QMessageBox::information(this, tr("导出数据"), exportDesc, QMessageBox::Yes | QMessageBox::No);
    if(retBtn == QMessageBox::Yes)
    {
        //should export now.
        QDateTime curDateTime = QDateTime::currentDateTime();
        int curTimestamp = curDateTime.toTime_t();

        string IframeExportFilepath = to_string(curTimestamp) + "_Iframe_Index" + to_string(mLastShowKeyFrameIndex) + "_InfoNo" + to_string(keyFrameInfoNo) + ".file";
        string GopExportFilepath = to_string(curTimestamp) + "_GOP_Index" + to_string(mLastShowKeyFrameIndex) + "_InfoNo" + to_string(keyFrameInfoNo) + ".file";
        dbgDebug("Iframe export filepath is:[%s], GOP filepath is:[%s]\n", IframeExportFilepath.c_str(), GopExportFilepath.c_str());
        int ret = ClipParserSingleton::getInstance()->dumpIframe(mLastShowKeyFrameIndex, keyFrameInfoNo, IframeExportFilepath);
        if(ret != 0)
        {
            //dbgError("export I frame failed! ret=%d, errDesc=[%s], dstFilepath=[%s]\n", ret, ClipParserSingleton::getInstance()->getErrStr(ClipParserSingleton::getInstance()->getErrno()), IframeExportFilepath.c_str());
            QString errDesc = tr("导出I帧失败!! 原因是：") + QString::fromStdString(ClipParserSingleton::getInstance()->getErrStr(ClipParserSingleton::getInstance()->getErrno()));
            QMessageBox::information(this, tr("导出失败"), errDesc);
        }
        else
        {
            ret = ClipParserSingleton::getInstance()->dumpGOP(mLastShowKeyFrameIndex, keyFrameInfoNo, GopExportFilepath);
            if(ret != 0)
            {
                //dbgError("export GOP failed! ret=%d, errDesc=[%s], dstFilepath=[%s]\n", ret, ClipParserSingleton::getInstance()->getErrStr(ClipParserSingleton::getInstance()->getErrno()), GopExportFilepath.c_str());
                QString errDesc = tr("导出GOP失败!! 原因是：") + QString::fromStdString(ClipParserSingleton::getInstance()->getErrStr(ClipParserSingleton::getInstance()->getErrno()));
                QMessageBox::information(this, tr("导出失败"), errDesc);
            }
        }
        //I frame and GOP being exported yet
        QString overDesc = tr("导出成功！") + "\n";
        overDesc += "\tIframeExportFilepath=[" + QString::fromStdString(IframeExportFilepath) + "]\n";
        overDesc += "\tGopExportFilepath=[" + QString::fromStdString(GopExportFilepath) + "]\n";
        QMessageBox::information(this, tr("导出成功"), overDesc);
    }
}

/*
    When click the item in treeWidget, should :
    1.check click keyFrameIndex or not, if yes, step2; else, do nothing;
    2.get the keyFrameIndex info, and get its keyFrameInfos;
    3.set corresponding keyFrameInfos to tableWidget;
*/
void ClipParserWidget::on_clipInfoTreeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    QString curItemText = item->text(column);
//    dbgDebug("column=%d, item->text()=[%s]\n", column, curItemText.toLatin1().data());

    int selectIndex = -1;
    if(curItemText.startsWith(QString(KEYFRAMEINDEX_SUBITEM_NAME)))
    {
        int startSybmPos = strlen(KEYFRAMEINDEX_SUBITEM_NAME);
        //get the index number
        if(curItemText.length() == startSybmPos + 1)
        {
            QChar firstChar = curItemText.at(startSybmPos);
            selectIndex = firstChar.toLatin1() - '0';
        }
        else
        {
            QChar firstChar = curItemText.at(startSybmPos);
            QChar secondChar = curItemText.at(startSybmPos + 1);
            if(secondChar.isDigit())
            {
                selectIndex = (firstChar.toLatin1() - '0') * 10 + (secondChar.toLatin1() - '0');
            }
            else
            {
                selectIndex = (firstChar.toLatin1() - '0');
            }
        }
    }
    dbgDebug("curItemText=[%s], selectIndex=%d\n", curItemText.toLatin1().data(), selectIndex);

    if(selectIndex < 0) /* donot select the keyFrameIndex, will do nothing */   return;

    if(mLastShowKeyFrameIndex == selectIndex)   /* do nothing to make CPU happy */  return;

    vector < FsKeyFramInfo > allKeyFrameInfos;
    ClipParserSingleton::getInstance()->getKeyFrameInfo(selectIndex, allKeyFrameInfos);
    //add all key frame infos to table
    setClipTableValue(allKeyFrameInfos);
    mLastShowKeyFrameIndex = selectIndex;
}

void ClipParserWidget::on_tableClipInfo_doubleClicked(const QModelIndex &index)
{

}
