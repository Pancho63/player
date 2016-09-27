#ifndef RECEIVEOSC_H
#define RECEIVEOSC_H

#include <QDebug>
#include "oscpkt.hh"
#include <QUdpSocket>
using namespace oscpkt;



class Listener : public QObject
        {
            Q_OBJECT
     void requestWork();

signals:
     void workRequested();
     void finished();
     void reboot();
     void portConnect(bool, int);


     void sndLevel(int);
     void trackNum(int);
public slots:
    void goOSC();
    void reStart();

private slots :
     void processPendingDatagrams();

private :

    QUdpSocket *udpSocket;
};

#endif // RECEIVEOSC_H

