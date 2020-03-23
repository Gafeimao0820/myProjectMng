#ifndef BSRPARSER_WIDGET_H
#define BSRPARSER_WIDGET_H

#include <QWidget>
#include <QTimer>

#include "bsrParser.h"

namespace Ui {
class BsrParserWidget;
}

typedef enum
{
    RUN_STATE_IDLE,
    RUN_STATE_PARSING,
    RUN_STATE_PARSED,
}RUN_STATE;

class BsrParserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BsrParserWidget(QWidget *parent = 0);
    ~BsrParserWidget();

private slots:
    void on_btnBrowerFile_clicked();

    void on_btnParse_clicked();

    void on_tableBsfpInfo_cellDoubleClicked(int row, int column);

    void on_btnOnlyShowVideo_clicked();

    void on_btnOnlyShowIFrames_clicked();

    void on_btnOnlyShowAudio_clicked();

    void on_btnShowAll_clicked();

    void on_btnExportAllFrameData_clicked();

    void scrollAuthorInfo();

    void on_btnExportAllVideoData_clicked();

    void on_btnExportAllAudioData_clicked();

    void on_btnExportCurrentFrameData_clicked();

    void on_btnParseRetDesc_clicked();

    void on_btnParseRetErrorFrameVideo_clicked();

    void on_btnParseRetErrorFrameAudio_clicked();

    void on_btnParseRetDirtyData_clicked();

private:
    /*
    * Set table format;
    * like : header content, background, algin-format, and so on;
    * like : each time just can select one line, when select its background being changed, and so on.
    *
    * being called when constructor;
    */
    virtual void setTableFormat();

    /*
    * insert rows to bsfpTable;
    * each row, get content from infoList;
    * return the row number of bsfpTable;
    */
    virtual void setBsfpTableValue(list<BsfpInfo> & infoList, bool isCheckSeqId = true);

    virtual void setAllToolTips();

    virtual void setScrollAuthorSlot();

    //just a message box being showed, include :
    //  1.frame number, video frame number, audio frame number, other frame number;
    //  2.video err parts number, audio err parts number;
    //  3.dirty data number;
    virtual void dumpParseRetDesc();

private:
    Ui::BsrParserWidget *ui;

    QString mCurFilepath;
    RUN_STATE mState;

    QTimer * mpAuthorScrollTimer;
    QString mAuthorInfo;
    int mAuthorInfoCursor;
};

#endif // BSRPARSER_WIDGET_H
