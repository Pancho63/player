/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "player.h"

#include "playercontrols.h"
#include "playlistmodel.h"


#include <QMediaService>
#include <QMediaPlaylist>
#include <QMediaMetaData>
#include <QtWidgets>

/////////                                       entrees micros !!!!!!
//
#include <QHostAddress>
#include <QNetworkInterface>
#include <QSettings>
#include <QDebug>
#include "receiveosc.h"
#include "oscpkt.hh"
#include "udp.hh"
using namespace oscpkt;

using std::string;


QString ipAdrr = "127.0.0.1";

QString nomPlayer = "player";
QString PORT_NUMin =  "7001";
    int PORT = PORT_NUMin.toInt();
QString ipDlight = "127.0.0.1";
QString PORT_NUMsend =  "7000";
QString chVol = "41";
QString masterPlay = "241";
int indx = 0;

QString path;


Player::Player(QWidget *parent)
    : QWidget(parent)
    , slider(0)

{
//! [create-objs]
    player = new QMediaPlayer(this);
    player->setNotifyInterval(100);
    // owned by PlaylistModel
    playlist = new QMediaPlaylist();

    player->setPlaylist(playlist);
//
    thread = new QThread();
    listener = new Listener();
    listener->moveToThread(thread);

    connect(listener, SIGNAL(workRequested()), thread, SLOT(start()));
    connect(thread, SIGNAL(started()), listener, SLOT(goOSC()));
    connect(listener, SIGNAL(reboot()), listener, SLOT(goOSC()));

//

    connect(player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    connect(player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    connect(player, SIGNAL(metaDataChanged()), SLOT(metaDataChanged()));
    connect(playlist, SIGNAL(currentIndexChanged(int)), SLOT(playlistPositionChanged(int)));
    connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(statusChanged(QMediaPlayer::MediaStatus)));
    connect(player, SIGNAL(bufferStatusChanged(int)), this, SLOT(bufferingProgress(int)));
    connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(displayErrorMessage()));


    playlistModel = new PlaylistModel(this);
    playlistModel->setPlaylist(playlist);



    playlistView = new QListView(this);
    playlistView->setModel(playlistModel);
    playlistView->setCurrentIndex(playlistModel->index(playlist->currentIndex(), 0));

    connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(jump(QModelIndex)));

    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, player->duration() / 1000);

    labelDuration = new QLabel(this);
    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)));

    QPushButton *openButton = new QPushButton(tr("Add"), this);
    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));

//
    QPushButton *save = new QPushButton(tr("save"), this);
    connect(save, SIGNAL(clicked()), this, SLOT(save()));

    QPushButton *load = new QPushButton(tr("load"), this);
    connect(load, SIGNAL(clicked()), this, SLOT(load()));

    QPushButton *clear = new QPushButton(tr("Delete"), this);
    connect(clear, SIGNAL(clicked()), this, SLOT(cleanplaylist()));


    QLabel *portin = new QLabel(tr("Port IN"), this);
    linePortOscIn = new QLineEdit(tr(""), this);
    linePortOscIn->setText(PORT_NUMin);

    QPushButton *oscButton = new QPushButton(tr("Lancer serveur OSC"), this);
    connect(oscButton, SIGNAL(clicked()), this, SLOT(startOSC()));


    QLabel *portout = new QLabel(tr("Port Dlight"), this);
    linePortOscOut = new QLineEdit(tr(""), this);
    linePortOscOut->setText(PORT_NUMsend);

    QLabel *nomPlay = new QLabel(tr("nom conduite : "), this);
    lineNomPlayer = new QLineEdit(tr(""), this);
    lineNomPlayer->setText(nomPlayer);
    QLabel *chVolume = new QLabel(tr("N° Channel du volume : "), this);
    lineChVol = new QLineEdit(tr(""), this);
    lineChVol->setText(chVol);
    QLabel *masterP = new QLabel(tr("N° Master Play : "), this);
    lineMasterPlay = new QLineEdit(tr(""), this);
    lineMasterPlay->setText(masterPlay);
//

    PlayerControls *controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());

    connect(controls, SIGNAL(play()), this, SLOT(playif()));
    connect(controls, SIGNAL(pause()), player, SLOT(pause()));
    connect(controls, SIGNAL(stop()), player, SLOT(stop()));
    connect(controls, SIGNAL(next()), this, SLOT(nextif()));
    connect(controls, SIGNAL(previous()), this, SLOT(previousClicked()));
    connect(controls, SIGNAL(changeVolume(int)), player, SLOT(setVolume(int)));
    connect(controls, SIGNAL(changeMuting(bool)), player, SLOT(setMuted(bool)));


    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)),
            controls, SLOT(setState(QMediaPlayer::State)));
    connect(player, SIGNAL(volumeChanged(int)), controls, SLOT(setVolume(int)));
    connect(player, SIGNAL(mutedChanged(bool)), controls, SLOT(setMuted(bool)));

//
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(stopFin(qint64)));
    connect(linePortOscOut, SIGNAL(textEdited(QString)), this, SLOT(validePortSend(QString)));
    connect(linePortOscIn, SIGNAL(textEdited(QString)), this, SLOT(validePortIn(QString)));
    connect(lineNomPlayer, SIGNAL(textEdited(QString)), this, SLOT(valideNomPlayer(QString)));
    connect(lineChVol, SIGNAL(textEdited(QString)), this, SLOT(valideChVol(QString)));
    connect(lineMasterPlay, SIGNAL(textEdited(QString)), this, SLOT(valideMasterPlay(QString)));

    connect(listener, SIGNAL(sndLevel(int)), controls, SLOT(chgVolume(int)));
    connect(controls, SIGNAL(changeVolume(int)), this, SLOT(vol(int)));
    connect(listener, SIGNAL(trackNum(int)), this, SLOT(playtrack(int)));
    connect(playlistView, SIGNAL(pressed(QModelIndex)), SLOT(setindx(QModelIndex)));

    QBoxLayout *osclabelLayout = new QHBoxLayout;
    osclabelLayout->setMargin(10);
    osclabelLayout->addWidget(portin);
    portin->setFixedWidth(50);
    osclabelLayout->addSpacing(10);
    osclabelLayout->addWidget(portout);
    portout->setFixedWidth(100);
    osclabelLayout->addStretch(1);

    osclabelLayout->addStretch(1);
    osclabelLayout->setContentsMargins(0,-1,0,-1);;

    QBoxLayout *oscLayout = new QHBoxLayout;
    oscLayout->setMargin(10);
    oscLayout->addWidget(linePortOscIn);
     linePortOscIn->setFixedWidth(40);
    oscLayout->addSpacing(20);
    oscLayout->addWidget(linePortOscOut);
    linePortOscOut->setFixedWidth(40);
    oscLayout->addSpacing(60);
    oscLayout->addWidget(oscButton);
    oscLayout->addStretch(1);
    oscLayout->setContentsMargins(0,0,0,10);

    QBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->setMargin(0);
    nameLayout->addWidget(nomPlay);
    nameLayout->addStretch(0);
    nameLayout->addWidget(lineNomPlayer);
    lineNomPlayer->setFixedWidth(100);
    nameLayout->addWidget(save);
    nameLayout->addWidget(load);
    nameLayout->addStretch(1);

    QBoxLayout *chLayout = new QHBoxLayout;
    chLayout->addWidget(chVolume);
    chLayout->addStretch(0);
    chLayout->addWidget(lineChVol);
    lineChVol->setFixedWidth(30);
    chLayout->addSpacing(10);
    chLayout->addWidget(masterP);
    chLayout->addStretch(0);
    chLayout->addWidget(lineMasterPlay);
    lineMasterPlay->setFixedWidth(30);
    chLayout->addStretch(1);
//
    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(playlistView);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addWidget(clear);
    controlLayout->addWidget(controls);;

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(nameLayout);
    layout->addLayout(chLayout);
    layout->addLayout(displayLayout);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(slider);
    hLayout->addWidget(labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);

//
    layout->addLayout(osclabelLayout);
    layout->addLayout(oscLayout);
//

    setLayout(layout);

    if (!player->isAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"\
                                "Please check the media service plugins are installed."));

        controls->setEnabled(false);
        playlistView->setEnabled(false);
        openButton->setEnabled(false);
    }

    metaDataChanged();

    QStringList arguments = qApp->arguments();
    arguments.removeAt(0);
    addToPlaylist(arguments);
}
Player::~Player()
{
}

void Player::open()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Files"));
    addToPlaylist(fileNames);
}

void Player::addToPlaylist(const QStringList& fileNames)
{
    foreach (QString const &argument, fileNames) {
        QFileInfo fileInfo(argument);
        if (fileInfo.exists()) {
            QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
            if (fileInfo.suffix().toLower() == QLatin1String("m3u")) {
                playlist->load(url);
            } else
                playlist->addMedia(url);
        } else {
            QUrl url(argument);
            if (url.isValid()) {
                playlist->addMedia(url);
            }
        }
    }
}


void Player::durationChanged(qint64 duration)
{
    this->duration = duration/1000;
    slider->setMaximum(duration / 1000);
}

void Player::positionChanged(qint64 progress)
{
    if (!slider->isSliderDown()) {
        slider->setValue(progress / 1000);
    }
    updateDurationInfo(progress / 1000);
}

void Player::metaDataChanged()
{
    if (player->isMetaDataAvailable()) {
        setTrackInfo(QString("%1 - %2")
                .arg(player->metaData(QMediaMetaData::AlbumArtist).toString())
                .arg(player->metaData(QMediaMetaData::Title).toString()));
    }
}

void Player::previousClicked()
{    if (!playlist->isEmpty())
    {
    // Go to previous track if we are within the first 3 seconds of playback
    // Otherwise, seek to the beginning.
    if(player->position() <= 3000)
        playlist->previous();
    else
        player->setPosition(0);
    }
}

void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        playlist->setCurrentIndex(index.row());
        player->play();
    }
}

void Player::playlistPositionChanged(int currentItem)
{
    playlistView->setCurrentIndex(playlistModel->index(currentItem, 0));
    indx = currentItem;
}

void Player::seek(int seconds)
{
    player->setPosition(seconds * 1000);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);


    // handle status message
    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Media Stalled"));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(int progress)
{
    setStatusInfo(tr("Buffering %4%").arg(progress));
}


void Player::setTrackInfo(const QString &info)
{
    trackInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::setStatusInfo(const QString &info)
{
    statusInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::displayErrorMessage()
{
    setStatusInfo(player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || duration) {
        QTime currentTime((currentInfo/3600)%60, (currentInfo/60)%60, currentInfo%60, (currentInfo*1000)%1000);
        QTime totalTime((duration/3600)%60, (duration/60)%60, duration%60, (duration*1000)%1000);
        QString format = "mm:ss";
        if (duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    labelDuration->setText(tStr);
}

//
void Player::startOSC()
{
    if (thread->isRunning())
    {
        listener->reStart();
        return;
    }
    else
        thread->start();
}

void Player::load()
{
    path = (QCoreApplication::applicationDirPath() + "/saves/");
    QDir dir(path);
    if (!dir.exists()) {dir.mkpath(".");}

    QString file = QFileDialog::getOpenFileName(this, tr("Open Player"), path , tr("ini Files (*.ini)"));

    if(file.isEmpty()) { return; }

    QFileInfo fileInfo(file);
    nomPlayer = fileInfo.baseName();
    QString loadpath = fileInfo.absolutePath();

    playlist->load(QUrl::fromLocalFile(loadpath +"/"+ nomPlayer + ".m3u"), "m3u");

    QSettings settings(path + nomPlayer + ".ini", QSettings::IniFormat) ;
    chVol = settings.value("chVol").toString();
    masterPlay = settings.value("masterPlay").toString();
    PORT_NUMin = settings.value("portin", "7001").toString();
    PORT_NUMsend = settings.value("portsend", "7000").toString();

    lineNomPlayer->setText(nomPlayer);
    lineChVol->setText(chVol);
    lineMasterPlay->setText(masterPlay);
    linePortOscIn->setText(PORT_NUMin);
    linePortOscOut->setText(PORT_NUMsend);
}
void Player::save()
{
    path = (QCoreApplication::applicationDirPath() + "/saves/");
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");}
    if (!playlist->isEmpty())
    {
    playlist->save(QUrl::fromLocalFile(path + nomPlayer+ ".m3u"), "m3u");
    }
    QSettings settings(path + nomPlayer + ".ini", QSettings::IniFormat) ;
    settings.setValue("portin", PORT_NUMin);
    settings.setValue("portsend", PORT_NUMsend);
    settings.setValue("chVol", chVol);
    settings.setValue("masterPlay", masterPlay);
}

void Player::setindx(const QModelIndex &index)
{
   indx = index.row();
}

void Player::playif()
{
    if (!playlist->isEmpty())
    {
        player->play();
    }
}

void Player::nextif()
{
    if (!playlist->isEmpty())
    {
        playlist->next();
    }
}


void Player::cleanplaylist()
{
   if (!playlist->isEmpty())
        {
        player->stop();
        if (!(indx==0)){
        playlist->removeMedia(indx);
        playlist->setCurrentIndex(indx);
        }}
}

void Player::stopFin (qint64 pos)
{
    if (player->currentMedia() !=0)
      {
            if (pos /100 == player->duration()/100)
              {
               player->stop();
               playlist->next();
              };
      }
}
void Player::valideNomPlayer(QString nom)
{
    nomPlayer = nom;
}

void Player::valideChVol(QString nmb)
{
     chVol = nmb;
}

void Player::valideMasterPlay(QString nmb)
{
     masterPlay = nmb;
}

void Player::validePortSend(QString por)
{
    path = (QCoreApplication::applicationDirPath() + "/saves/");
    PORT_NUMsend = por;
    QDir dir(path);
        if (!dir.exists()) {
           dir.mkpath(".");}
    QSettings settings(path + nomPlayer + ".ini", QSettings::IniFormat) ;
    settings.setValue("portsend", por);                     
}

void Player::validePortIn(QString por)
{
    path = (QCoreApplication::applicationDirPath() + "/saves/");
    PORT_NUMin = por;
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");}
    QSettings settings(path + nomPlayer + ".ini", QSettings::IniFormat) ;
    settings.setValue("portin", por);    
        PORT = PORT_NUMin.toInt();
}


void Player::vol(int level)
{
UdpSocket sock;
int lvl = level*2.55+1;
 sock.connectTo(ipDlight.toStdString(), PORT_NUMsend.toStdString());
 if (!sock.isOk()) {
    qDebug() << "Error connection to port " << PORT_NUMsend << endl;
   } else {string message = ("/circ/" + chVol).toStdString();
    Message msg(message); msg.pushInt32(lvl);
     PacketWriter pw;
     pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
     sock.sendPacket(pw.packetData(), pw.packetSize());
     sock.close();
          }
}

void Player::playtrack(int index)
{if (index==0){player->stop();}
else if (index <= playlist->mediaCount()){
        playlist->setCurrentIndex(index-1);
        player->play();}

}
