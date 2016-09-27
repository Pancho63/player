#include "receiveosc.h"
#include <QDebug>

#include "oscpkt.hh"
using namespace oscpkt;
using std::string;

extern int PORT;

extern QString masterPlay;
extern QString chVol;



void Listener::requestWork()
{
    emit workRequested();
}


void Listener::goOSC()
{
    udpSocket = new QUdpSocket;
   bool bound = udpSocket->bind(QHostAddress::AnyIPv4, PORT);
    if (!bound)
    {emit portConnect(false, PORT);}
    else emit portConnect(true, PORT);

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
 }

void Listener::processPendingDatagrams()
{
          int iarg;
          int iarg2;
      PacketReader pr;
      while (udpSocket->hasPendingDatagrams()) {

          oscpkt::Message *msg;
          QByteArray datagram;
          datagram.resize(udpSocket->pendingDatagramSize());
             udpSocket->readDatagram(datagram.data(), datagram.size());
                 pr.init(datagram.data(), datagram.size());


          while (pr.isOk() && (msg = pr.popMessage()) != 0) {

QString msgMaster = ("/sub/level");
QString msgCh = ("/circ/level");
int play = masterPlay.toInt();
int vol = chVol.toInt();

              if (msg->match((msgCh).toStdString()).popInt32(iarg).popInt32(iarg2).isOkNoMoreArgs()) {
             if (iarg==vol) emit sndLevel(iarg2/2.55);}
              if (msg->match((msgMaster).toStdString()).popInt32(iarg).popInt32(iarg2).isOkNoMoreArgs()) {
             if (iarg==play) emit trackNum((iarg2+1)/2.55);}

          }
        }
}


void Listener::reStart()
{
 udpSocket->deleteLater();

emit reboot();
        return;
}
