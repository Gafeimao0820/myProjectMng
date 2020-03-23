#ifndef QUERYANDEXPORTRECORD_H
#define QUERYANDEXPORTRECORD_H

#include <QWidget>

namespace Ui {
class queryAndExportRecord;
}

class queryAndExportRecord : public QWidget
{
    Q_OBJECT

public:
    explicit queryAndExportRecord(QWidget *parent = nullptr);
    ~queryAndExportRecord();

private:
    Ui::queryAndExportRecord *ui;
};

#endif // QUERYANDEXPORTRECORD_H
