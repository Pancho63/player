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


#include <QMediaService>
#include <QMediaMetaData>
#include <QtWidgets>
#include <QSettings>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QDebug>

#include "player.h"
#include "playercontrols.h"
#include "playlistmodel.h"
#include "receiveosc.h"
#include "oscpkt.hh"
using namespace oscpkt;

using std::string;


QString ipAdrr = "127.0.0.1";
QString nomsonig = "sonig";
int indx = -1;
QString chVol;
QString masterPlay;
int PORT;
QString path ;


Player::Player(QWidget *parent)
    : QWidget(parent)
    , slider(0)

{
path = (QCoreApplication::applicationDirPath() + "/saves/");
QSettings settings (path + "sonig.ini", QSettings::IniFormat) ;
chVol = settings.value("chVol", "41").toString();
masterPlay = settings.value("masterPlay", "241").toString();
PORT_NUMin = settings.value("portin", "7001").toString();
PORT = PORT_NUMin.toInt();
readSettings();

    player = new QMediaPlayer(this);
    player->setNotifyInterval(50);
    // owned by PlaylistModel
    playlist = new QMediaPlaylist();

    player->setPlaylist(playlist);
//
    thread = new QThread();
    listener = new Listener();
    listener->moveToThread(thread);

    connect(listener, SIGNAL(workRequested()), thread, SLOT(start()));
    connect(thread, SIGNAL(started()), listener, SLOT(goOSC()));
    connect(listener, SIGNAL(finished()), thread, SLOT(quit()));
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
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), playlistView);
    connect(shortcut, SIGNAL(activated()), this, SLOT(deleteItem()));

    autoFollow = new QCheckBox(this);
    autoFollow->setText("Auto-Follow");

    QPushButton *save = new QPushButton(tr("save"), this);
    connect(save, SIGNAL(clicked()), this, SLOT(save()));

    QPushButton *load = new QPushButton(tr("load"), this);
    connect(load, SIGNAL(clicked()), this, SLOT(load()));

    QPushButton *clear = new QPushButton(tr("Delete"), this);
    connect(clear, SIGNAL(clicked()), this, SLOT(cleanplaylist()));

    QLabel *portin = new QLabel(tr("Port IN"), this);
    linePortOscIn = new QLineEdit(tr(""), this);
    linePortOscIn->setText(PORT_NUMin);

    QPushButton *oscButton = new QPushButton(tr("relancer serveur OSC"), this);
    connect(oscButton, SIGNAL(clicked()), this, SLOT(startOSC()));

    QLabel *nomPlay = new QLabel(tr("nom conduite : "), this);
    lineNomPlayer = new QLineEdit(tr(""), this);
    lineNomPlayer->setText(nomsonig);
    QLabel *chVolume = new QLabel(tr("Ch du volume : "), this);
    lineChVol = new QLineEdit(tr(""), this);
    lineChVol->setText(chVol);
    QLabel *masterP = new QLabel(tr("Master Play : "), this);
    lineMasterPlay = new QLineEdit(tr(""), this);
    lineMasterPlay->setText(masterPlay);
    lineDialog = new QLabel(tr(""), this);
    lineDialog->setAlignment(Qt::AlignCenter);
/*
    mic1ChkBox = new QCheckBox(tr("Mic 1"), this);
    mic1Slider = new QSlider;
    mic1Slider->setMaximum(100);
    mic1Slider->setOrientation(Qt::Horizontal);
    mic1Slider->setVisible(false);
//
    QAudioFormat format;
    // Set up the desired format, for example:
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        lineDialog->setText("Default format on ch.1 not supported, trying to use the nearest.");
        format = info.nearestFormat(format);
        }

    audioIn1 = new QAudioInput(format, this);

*/
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


    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)), controls, SLOT(setState(QMediaPlayer::State)));
    connect(player, SIGNAL(volumeChanged(int)), controls, SLOT(setVolume(int)));
    connect(player, SIGNAL(mutedChanged(bool)), controls, SLOT(setMuted(bool)));

//
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(stopFin(qint64)));
    connect(linePortOscIn, SIGNAL(textChanged(QString)), this, SLOT(validePortIn(QString)));
    connect(lineNomPlayer, SIGNAL(textChanged(QString)), this, SLOT(valideNomPlayer(QString)));
    connect(lineChVol, SIGNAL(textChanged(QString)), this, SLOT(valideChVol(QString)));
    connect(lineMasterPlay, SIGNAL(textChanged(QString)), this, SLOT(valideMasterPlay(QString)));

    connect(listener, SIGNAL(sndLevel(int)), controls, SLOT(chgVolume(int)));
    connect(listener, SIGNAL(trackNum(int)), this, SLOT(playtrack(int)));

    connect(listener, SIGNAL(portConnect(bool, int)), this, SLOT(portConnectIn(bool, int)));
//    connect(mic1Slider, SIGNAL(valueChanged(int)), this, SLOT(volMic1(int)));
//    connect(mic1ChkBox, SIGNAL(toggled(bool)), mic1Slider, SLOT(setVisible(bool)));
//    connect(mic1ChkBox, SIGNAL(toggled(bool)), this, SLOT(openMic1(bool)));

    QBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(nomPlay);
    nameLayout->addWidget(lineNomPlayer);
    nameLayout->addStretch(0);
    lineNomPlayer->setFixedWidth(100);

    QBoxLayout *saveLayout = new QHBoxLayout;
    saveLayout->addWidget(save);
    saveLayout->addWidget(load);


    QBoxLayout *chLayout = new QHBoxLayout;
    chLayout->addWidget(chVolume);
    chLayout->addWidget(lineChVol);
    chLayout->addStretch(0);
    lineChVol->setFixedWidth(30);

    QBoxLayout *mstLayout = new QHBoxLayout;
    chLayout->addWidget(masterP);
    chLayout->addWidget(lineMasterPlay);
    chLayout->addStretch(0);
    lineMasterPlay->setFixedWidth(30);
//
    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(playlistView);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(autoFollow);
    hLayout->addWidget(slider);
    hLayout->addWidget(labelDuration);

    QBoxLayout *openLayout = new QHBoxLayout;
    openLayout->addWidget(openButton);
    openLayout->addWidget(clear);
    openLayout->addStretch(0);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(controls);

/*    QGridLayout *micsLayout = new QGridLayout;
    micsLayout->addWidget(mic1ChkBox, 0, 0);
    micsLayout->addWidget(mic1Slider, 1, 0);
*/
    QBoxLayout *oscLayout = new QHBoxLayout;
    oscLayout->addWidget(portin);
    portin->setFixedWidth(50);
    oscLayout->addWidget(linePortOscIn);
     linePortOscIn->setFixedWidth(40);
    oscLayout->addSpacing(20);
    oscLayout->addWidget(oscButton);
    oscLayout->addStretch(1);


    QBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(10, 10, 10, 5);
    layout->setSpacing(10);
    layout->addLayout(chLayout);
    layout->addLayout(mstLayout);
    layout->addLayout(displayLayout);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    layout->addLayout(openLayout);
//    layout->addLayout(micsLayout);
    layout->addLayout(oscLayout);
    layout->addLayout(nameLayout);
    layout->addLayout(saveLayout);
    layout->addWidget(lineDialog);
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

 playlist->load(QUrl::fromLocalFile(path +"sonig.m3u"), "m3u");
 setWindowTitle("sonig");
 startOSC();
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

    QString file = QFileDialog::getOpenFileName(this, tr("Open sonig"), path , tr("ini Files (*.ini)"));

    if(file.isEmpty()) { return; }
    playlist->clear();
    QFileInfo fileInfo(file);
    nomsonig = fileInfo.baseName();
    QString loadpath = fileInfo.absolutePath();

    playlist->load(QUrl::fromLocalFile(loadpath +"/"+ nomsonig + ".m3u"), "m3u");

    QSettings settings(path + nomsonig + ".ini", QSettings::IniFormat) ;
    chVol = settings.value("chVol").toString();
    masterPlay = settings.value("masterPlay").toString();
    PORT_NUMin = settings.value("portin", "7001").toString();


    lineNomPlayer->setText(nomsonig);
    lineChVol->setText(chVol);
    lineMasterPlay->setText(masterPlay);
    linePortOscIn->setText(PORT_NUMin);
     setWindowTitle(nomsonig);
    startOSC();
}

void Player::save()
{
    path = (QCoreApplication::applicationDirPath() + "/saves/");
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");}

              QFile f( path + nomsonig + ".m3u" );
 if ((playlist->mediaCount()==0)){QFile::remove(path + nomsonig + ".m3u");}
 else{

     if( f.open( QIODevice::WriteOnly ) ){
         QTextStream ts( &f );
            ts.setCodec("ISO 8859-1");
         for( int r = 0; r < playlist->mediaCount(); ++r ){

                 ts <<  (playlist->media(r).canonicalUrl()).toString()+"\n";
         }
         f.close();
     }

 }
 ////
    QSettings settings(path + nomsonig + ".ini", QSettings::IniFormat) ;
    settings.setValue("portin", PORT_NUMin);
    settings.setValue("chVol", chVol);
    settings.setValue("masterPlay", masterPlay);

    QMessageBox msgBox;
    QString defaut = "";
         if (nomsonig=="sonig") {defaut = "This will be your default OSCsonig";}
    msgBox.setWindowTitle("OSCsonig");
     msgBox.setText("Saved as \"" + nomsonig + "\""+"\n" + defaut );


     msgBox.exec();
    setWindowTitle(nomsonig);
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
    indx=((playlistView->currentIndex()).row());
   if (!(indx==-1))
        {
        player->stop();
        playlist->removeMedia(indx);
        }
}


void Player::stopFin (qint64 pos)
{
pos = pos /50;
qint64 max = player->duration()/50;

if (!(autoFollow->isChecked()))
{if(pos != 0)
    {
    if ((pos) == (max))
        {
        player->stop();
        nextif();
        }

    }
}
}
void Player::valideNomPlayer(QString nom)
{
    nomsonig = nom;
}

void Player::valideChVol(QString nmb)
{
     chVol = nmb;
}

void Player::valideMasterPlay(QString nmb)
{
     masterPlay = nmb;
}

void Player::validePortIn(QString por)
{
    PORT_NUMin = por;  
    PORT = PORT_NUMin.toInt();
}


void Player::playtrack(int index)
{if (index==0){player->stop();}
else if (index <= playlist->mediaCount()){
        playlist->setCurrentIndex(index-1);
        player->play();}

}

void Player::portConnectIn(bool ok, int por)
{
if (!ok)    {
    lineDialog->setText("!!! Error connecting to Port : " + QString::number(por)+ "      ");
    lineDialog->setStyleSheet("QLabel{color:red}");
    QMessageBox msgBoxNA;
msgBoxNA.setWindowTitle("OSCours!!");
 msgBoxNA.setText("Port non accessible");
 msgBoxNA.exec();

}
if (ok)    {
     lineDialog->setStyleSheet("QLabel{color:black}");
     lineDialog->setText("Listening on Port : " + QString::number(por) + "      ");
            }
}

void Player::deleteItem()
{
    indx=((playlistView->currentIndex()).row());
   if (!(indx==-1))
        {
       player->stop();
       playlist->removeMedia(indx);
        }
}


void Player::writeSettings()
{
    QSettings settings(path + "sonig.ini", QSettings::IniFormat);

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

void Player::readSettings()
{
    QSettings settings(path + "sonig.ini", QSettings::IniFormat);

    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(400, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    settings.endGroup();
}

void Player::closeEvent(QCloseEvent *)
{
        writeSettings();
}


/*
void Player::volMic1(int vol)
{
    audioIn1->setVolume(vol);
}

void Player::openMic1(bool on)
{if (on)
    {
    audioIn1->start();
    }
 if (!on)
 {
     audioIn1->stop();
 }
}
*/
