#include "receiveosc.h"
#include <QDebug>

#include "oscpkt.hh"
#include "udp.hh"
using namespace oscpkt;
using std::string;


void Listener::requestWork()
{
    emit workRequested();
}


void Listener::goOSC()
{
    extern int PORT;

    extern QString masterPlay;
    string msgMaster = ("/sub/" + masterPlay + "/level").toStdString();

    extern QString chVol;
    string msgCh = ("/circ/" + chVol).toStdString();


    sock.bindTo(PORT);
    if (!sock.isOk()) {
      qDebug() << "Error opening port " << PORT << ": " << endl;
    } else {
      qDebug() << "Server started, will listen to packets on port " << PORT << endl;
      PacketReader pr;
      while (sock.isOk()) {
        if (sock.receiveNextPacket(30 /* timeout, in ms */)) {
          pr.init(sock.packetData(), sock.packetSize());
          oscpkt::Message *msg;
          while (pr.isOk() && (msg = pr.popMessage()) != 0) {

//            float farg;
            int iarg;
//            std::string sarg;
//            QString qarg;


              if (msg->match(msgCh).popInt32(iarg).isOkNoMoreArgs()) {
             emit sndLevel(iarg/2.55);}
              if (msg->match(msgMaster).popInt32(iarg).isOkNoMoreArgs()) {
             emit trackNum((iarg+1)/2.55);}

          }
        }
      }
    }
}

void Listener::reStart()
{
 sock.close();
sleep(1);
emit reboot();
        return;
}
