#ifndef CLIPPARSER_WIDGET_H
#define CLIPPARSER_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTreeWidget>

#include "clipParser.h"

namespace Ui {
class ClipParserWidget;
}

class ClipParserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClipParserWidget(QWidget *parent = 0);
    ~ClipParserWidget();

private slots:
    void on_btnBrowerFile_clicked();

    void on_btnParse_clicked();

    void on_tableClipInfo_cellDoubleClicked(int row, int column);

    void scrollAuthorInfo();

    void on_clipInfoTreeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_tableClipInfo_doubleClicked(const QModelIndex &index);

private:
    /*
    * Set table format;
    * like : header content, background, algin-format, and so on;
    * like : each time just can select one line, when select its background being changed, and so on.
    *
    * being called when constructor;
    */
    virtual void setTableFormat();

    void setClipTableValue(vector < FsKeyFramInfo > & allKeyFrameInfos);

    virtual void setAllToolTips();

    virtual void setScrollAuthorSlot();

private:
    Ui::ClipParserWidget *ui;

    QString mCurFilepath;

    QTimer * mpAuthorScrollTimer;
    QString mAuthorInfo;
    int mAuthorInfoCursor;

    int mLastShowKeyFrameIndex;
};

#endif // BSRPARSER_WIDGET_H
