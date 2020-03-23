#ifndef TELNETCLIENT_H
#define TELNETCLIENT_H


#include "qttelnet.h"
#include <QWidget>

class telnetClient : public QWidget
{
    Q_OBJECT

public:
    telnetClient(QWidget *par = 0);
    ~telnetClient();

private slots:
    void telnetMessage(const QString &msg);
    void telnetLoginRequired();
    void telnetLoginFailed();
    void telnetLoggedOut();
    void telnetLoggedIn();
    void telnetConnectionError(QAbstractSocket::SocketError error);
    void suspend();
    void kill();
    void lineReturnPressed();
    void deleteCharOrLogout();

private:
    QtTelnet *m_pTelnet;
};

#endif // TELNETCLIENT_H
