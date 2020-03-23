#include "queryAndExportRecordWidget.h"
#include "ui_queryAndExportRecordWidget.h"

queryAndExportRecord::queryAndExportRecord(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::queryAndExportRecord)
{
    ui->setupUi(this);
}

queryAndExportRecord::~queryAndExportRecord()
{
    delete ui;
}
