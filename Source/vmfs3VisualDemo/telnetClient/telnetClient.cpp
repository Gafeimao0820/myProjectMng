#include "telnetClient.h"
#include <QInputDialog>
#include <QDebug>

// construction
telnetClient::telnetClient(QWidget *par)
                : QWidget(par)
{
    m_pTelnet = new QtTelnet();

   connect(m_pTelnet, SIGNAL(message(const QString &)),
           this, SLOT(telnetMessage(const QString &)));
   connect(m_pTelnet, SIGNAL(loginRequired()),
           this, SLOT(telnetClient::telnetLoginRequired()));
   connect(m_pTelnet, SIGNAL(loginFailed()),
           this, SLOT(telnetClient::telnetLoginFailed()));
   connect(m_pTelnet, SIGNAL(loggedOut()),
           this, SLOT(telnetClient::telnetLoggedOut()));
   connect(m_pTelnet, SIGNAL(loggedIn()),
           this, SLOT(telnetClient::telnetLoggedIn()));
   connect(m_pTelnet, SIGNAL(connectionError(QAbstractSocket::SocketError)),
           this, SLOT(telnetClient::telnetConnectionError(QAbstractSocket::SocketError)));

    QString host = QInputDialog::getText(this,
                                         "Host name",
                                         "Host name of Telnet server",
                                         QLineEdit::Normal,
                                         "localhost");
    host = host.trimmed();
    if (!host.isEmpty())
        m_pTelnet->connectToHost(host);
}

// ALL SLOT
void telnetClient::telnetMessage(const QString &msg)
{
    qDebug() << "telnet mssage in : " << msg;
}
void telnetClient::telnetLoginRequired()
{
    qDebug() << "login required in";

}
void telnetClient::telnetLoggedOut()
{
    qDebug() << "logged out in";

}
void telnetClient::telnetLoggedIn()
{
    qDebug() << "logged in in";
}
void telnetClient::telnetConnectionError(QAbstractSocket::SocketError error)
{
    qDebug() << "telnet connect error in : error code:" << error;
}
void telnetClient::suspend()
{

}
void telnetClient::kill()
{

}
void telnetClient::lineReturnPressed()
{

}
void telnetClient::deleteCharOrLogout()
{

}
