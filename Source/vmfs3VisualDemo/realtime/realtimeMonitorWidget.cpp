#include "realtimeMonitorWidget.h"
#include "ui_realtimeMonitorWidget.h"

realTimeMoniterWidget::realTimeMoniterWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::realTimeMoniterWidget)
{
    ui->setupUi(this);
}

realTimeMoniterWidget::~realTimeMoniterWidget()
{
    delete ui;
}
