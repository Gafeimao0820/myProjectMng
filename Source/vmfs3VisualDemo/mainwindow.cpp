#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTimer>
#include <QHeaderView>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建模型
    m_pTableModel = new CustomTableModel();
    connect(m_pTableModel, SIGNAL(rowChange()), this, SLOT(updateView()));

    // 创建 tableView
    m_pTableView = new QTableView();
    m_pTableView->setModel(m_pTableModel);
    m_pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //m_pTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);    //x先自适应宽度
    //m_pTableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);    //x先自适应宽度
//    m_pTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);     //然后设置要根据内容使用宽度的列
    m_pTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);     //然后设置要根据内容使用宽度的列
    m_pTableView->setUpdatesEnabled(true);
    QHeaderView *headerGoods = m_pTableView->horizontalHeader();
    //SortIndicator为水平标题栏文字旁边的三角指示器
    headerGoods->setSortIndicator(0, Qt::AscendingOrder);
    headerGoods->setSortIndicatorShown(true);
    connect(headerGoods, SIGNAL(sectionClicked(int)), m_pTableView, SLOT (sortByColumn(int)));
//    m_pTableView->show();

    // 添加实时预览视图
    //m_pTableView->setParent(ui->tab_realTime);
    ui->centreTabWidget->insertTab(0, m_pTableView, tr("实时预览"));

    // 添加录像导出视图
    //m_queryExportReordWidget.setParent(ui->tab_exportRecord);
    ui->centreTabWidget->insertTab(1, &m_queryExportReordWidget, tr("录像导出"));

    // 添加clip预览视图
    //m_clipParserWidget.setParent(ui->tab_clipPreview);
    ui->centreTabWidget->insertTab(2, &m_clipParserWidget, tr("clip解析"));

    // 添加bsr解析视图
    //m_bsrParserWidget.setParent(ui->tab_bsrParse);
    ui->centreTabWidget->insertTab(3, &m_bsrParserWidget, tr("bsr解析"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateClipInfo(FsClipDescribeStruct &clipDescStruct)
{
    m_pTableModel->updateClipInfo(clipDescStruct);
}

void MainWindow::updateView()
{
//    qDebug() << "updateView";
    m_pTableView->setModel(nullptr);
    m_pTableView->setModel(m_pTableModel);
}


void MainWindow::on_actiontelnetLogin_triggered()
{
    //if(m_pTelnetClient)
        return;

    //m_pTelnetClient = new telnetClient();
    qDebug() << "telnet login begin";

}
