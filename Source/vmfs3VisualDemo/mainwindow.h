#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>

#include "clipParserWidget.h"
#include "bsrParserWidget.h"
#include "queryAndExportRecordWidget.h"
#include "realtimeMonitorWidget.h"
#include "customtablemodel.h"

// remote interaction tool
//#include "telnetClient.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateClipInfo(FsClipDescribeStruct &clipDescStruct);

    // slot
private slots:
    void updateView();

    void on_actiontelnetLogin_triggered();

private:
    Ui::MainWindow *ui;
    CustomTableModel *m_pTableModel;

    // ui widget view
    QTableView *m_pTableView;
    queryAndExportRecord m_queryExportReordWidget;
    ClipParserWidget m_clipParserWidget;
    BsrParserWidget m_bsrParserWidget;

    // remote interaction tool
    //telnetClient *m_pTelnetClient;
};

#endif // MAINWINDOW_H
