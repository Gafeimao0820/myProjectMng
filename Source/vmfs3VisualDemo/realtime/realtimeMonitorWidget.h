#ifndef REALTIMEMONITERWIDGET_H
#define REALTIMEMONITERWIDGET_H

#include <QWidget>

namespace Ui {
class realTimeMoniterWidget;
}

class realTimeMoniterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit realTimeMoniterWidget(QWidget *parent = nullptr);
    ~realTimeMoniterWidget();

private:
    Ui::realTimeMoniterWidget *ui;
};

#endif // REALTIMEMONITERWIDGET_H
