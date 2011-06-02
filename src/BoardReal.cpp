/**********************************************************************
qtVlm: Virtual Loup de mer GUI
Copyright (C) 2010 - Christophe Thomas aka Oxygen77

http://qtvlm.sf.net

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include <QMessageBox>
#include <QMenu>
#include <QDebug>
#include <QString>

#include "MainWindow.h"

#include "BoardReal.h"
#include "Board.h"
#include "settings.h"
#include "Polar.h"
#include "boatReal.h"

#include "dataDef.h"
#include "Util.h"
#include "Grib.h"

boardReal::boardReal(MainWindow * mainWin, board * parent) : QWidget(mainWin)
{
    setupUi(this);
    Util::setFontDialog(this);
    this->mainWin=mainWin;
    this->parent=parent;

    /* Contextual Menu */
    popup = new QMenu(this);
    ac_showHideCompass = new QAction(tr("Cacher compas"),popup);
    popup->addAction(ac_showHideCompass);
    connect(ac_showHideCompass,SIGNAL(triggered()),this,SLOT(slot_hideShowCompass()));
    imgInfo=QPixmap(230,70);
    pntImgInfo.begin(&imgInfo);
    pntImgInfo.setRenderHint(QPainter::Antialiasing,true);
    pntImgInfo.setRenderHint(QPainter::TextAntialiasing,true);
    pntImgInfo.setPen(Qt::black);
    QFont font=pntImgInfo.font();
    font.setPointSize(6);
    pntImgInfo.setFont(font);
    this->gpsInfo->hide();
    this->statusBtn->setEnabled(false);
//    /* Etat du compass */
//    if(Settings::getSetting("boardCompassShown", "1").toInt()==1)
//        windAngle->show();
//    else
//        windAngle->hide();
}

boatReal * boardReal::currentBoat(void)
{
    if(parent && parent->currentBoat())
    {
        if(parent->currentBoat()->getType()==BOAT_REAL)
            return (boatReal*)parent->currentBoat();
        else
            return NULL;
    }
    else
        return NULL;
}

void boardReal::setWp(float lat,float lon,float wph)
{
    boatReal * myBoat=currentBoat();
    if(myBoat)
        myBoat->setWp(lat,lon,wph);
}
void boardReal::boatUpdated(void)
{
    boatReal * myBoat=currentBoat();

    if(!myBoat)
    {
        qWarning() << "No current real boat";
        return;
    }
    /* boat position */
    latitude->setText(Util::pos2String(TYPE_LAT,myBoat->getLat()));
    longitude->setText(Util::pos2String(TYPE_LON,myBoat->getLon()));

    /* boat heading */
    //windAngle->setValues(myBoat->getHeading(),0,myBoat->getWindSpeed(), -1, -1);
    this->dir->display(myBoat->getHeading());

    /* boat speed*/
    this->speed->display(myBoat->getSpeed());

    /*WP*/
    if(myBoat->getWPLat()!=0 && myBoat->getWPLat()!=0)
    {
        dnm_2->setText(QString().setNum(myBoat->getDnm()));
        ortho->setText(QString().setNum(myBoat->getOrtho()));
        vmg_2->setText(QString().setNum(myBoat->getVmg()));
        angle->setText(QString().setNum(myBoat->getLoxo()));
    }
    else
    {
        dnm_2->setText("---");
        ortho->setText("---");
        vmg_2->setText("---");
        angle->setText("---");
    }
    this->windInfo->setText("--");
    if(mainWin->getGrib() && mainWin->getGrib()->isOk())
    {
        time_t now=QDateTime::currentDateTimeUtc().toTime_t();
        Grib * grib=mainWin->getGrib();
        QString s=QString();
        if(now>grib->getMinDate() && now<grib->getMaxDate())
        {
            double tws,twd;
            grib->getInterpolatedValue_byDates((double) myBoat->getLon(),myBoat->getLat(),
                                   now,&tws,&twd,INTERPOLATION_DEFAULT);
            twd=radToDeg(twd);
            if(twd>360)
                twd-=360;
            double twa=A180(myBoat->getWindDir()-twd);
            double Y=90-twa;
            double a=tws*cos(degToRad(Y));
            double b=tws*sin(degToRad(Y));
            double bb=b+myBoat->getSpeed();
            double aws=sqrt(a*a+bb*bb);
            double awa=90-radToDeg(atan(bb/a));
            s=s.sprintf("<BODY LEFTMARGIN=\"0\">TWS <FONT COLOR=\"RED\"><b>%.1fnds</b></FONT> TWD %.0fdeg TWA %.0fdeg<br>AWS %.1fnds AWA %.0fdeg",tws,twd,A180(twa),aws,awa);
            if(myBoat->getPolarData())
            {
                QString s1;
                float bvmgUp=myBoat->getBvmgUp(tws);
                float bvmgDown=myBoat->getBvmgDown(tws);
                s=s+s1.sprintf("<br>Pres %.0fdeg Portant %.0fdeg",bvmgUp,bvmgDown);
            }
            s=s.replace("nds",tr("nds"));
            s=s.replace("deg",tr("deg"));
            s=s.replace("Pres",tr("Pres"));
            s=s.replace("Portant",tr("Portant"));
            this->windInfo->setText(s);
        }
    }
    /* GPS status */
    QString status;
    if(myBoat->gpsIsRunning() && !this->gpsInfo->isHidden())
    {
        nmeaINFO info=myBoat->getInfo();
        imgInfo.fill(Qt::white);
        for(int n=0;n<12;n++)
        {
            if(info.satinfo.sat[n].in_use==0)
                pntImgInfo.setBrush(Qt::red);
            else
                pntImgInfo.setBrush(Qt::green);
            pntImgInfo.drawRect(1+n*19,50,16,-info.satinfo.sat[n].sig*.7);
            pntImgInfo.drawText(1+n*19,52,16,18,Qt::AlignHCenter | Qt::AlignVCenter,QString().setNum(info.satinfo.sat[n].id));
        }
        gpsInfo->setPixmap(imgInfo);
        status=tr("Running")+"<br>";
        if(myBoat->getSig()==0)
            status=status+tr("no signal")+"<br>";
        else if(myBoat->getSig()==1)
            status=status+tr("Fix quality")+"<br>";
        else if (myBoat->getSig()==2)
            status=status+tr("Differential quality")+"<br>";
        else if (myBoat->getSig()==3)
            status=status+tr("Sensitive quality")+"<br>";
        if(myBoat->getFix()==1)
            status=status+tr("no position");
        else if(myBoat->getFix()==2)
            status=status+tr("2D position");
        else
            status=status+tr("3D position");
        if(myBoat->getFix()>1)
        {
            status=status+"<br>";
            if(myBoat->getPdop()<=1)
                status=status+tr("Ideal accuracy");
            else if(myBoat->getPdop()<=2)
                status=status+tr("Excellent accuracy");
            else if(myBoat->getPdop()<=1)
                status=status+tr("Good accuracy");
            else if(myBoat->getPdop()<=5)
                status=status+tr("Moderate accuracy");
            else if(myBoat->getPdop()<=20)
                status=status+tr("Not so good accuracy");
            else
                status=status+tr("Very bad accuracy");
        }
        status=status.replace(" ","&nbsp;");
        gpsInfo->setToolTip(status);
#if 0 /*show the image as a tooltip for startBtn (experimental)*/
        imgInfo.save("tempTip.png");
        startBtn->setToolTip("<p>"+status+"</p><img src=\"tempTip.png\">");
#endif
    }
}
double boardReal::A180(double angle)
    {
        if(qAbs(angle)>180)
        {
            if(angle<0)
                angle=360+angle;
            else
                angle=angle-360;
        }
        return angle;
    }

void boardReal::setChangeStatus(bool /*status*/)
{

}

void boardReal::paramChanged()
{

}

void boardReal::disp_boatInfo()
{
    QMessageBox::information(this,tr("Information"),tr("Bientot des infos ici"));
}

void boardReal::chgBoatPosition(void)
{
    boatReal * myBoat=currentBoat();
    if(myBoat)
        myBoat->slot_chgPos();
}

void boardReal::startGPS(void)
{
    boatReal * myBoat=currentBoat();
    if(!myBoat)
    {
        qWarning() << "No real boat to start GPS";
        return;
    }
    if(myBoat->gpsIsRunning())
    {
        myBoat->stopRead();
        this->startBtn->setText(tr("Start GPS"));
        this->statusBtn->setEnabled(false);
        this->gpsInfo->hide();
    }
    else
    {
        myBoat->startRead();
        if(myBoat->gpsIsRunning())
        {
            this->startBtn->setText(tr("Stop GPS"));
            this->statusBtn->setEnabled(true);
        }
    }
}

 void boardReal::statusGPS(void)
 {
     if(this->gpsInfo->isHidden())
         gpsInfo->show();
     else
         gpsInfo->hide();
 }

void boardReal::slot_hideShowCompass()
{
    //setCompassVisible(~windAngle->isVisible());
}

void boardReal::setCompassVisible(bool /*status*/)
{
//    if(status)
//    {
//        Settings::setSetting("boardCompassShown",1);
//        windAngle->show();
//    }
//    else
//    {
//        Settings::setSetting("boardCompassShown",0);
//        windAngle->hide();
//    }
}

void boardReal::contextMenuEvent(QContextMenuEvent  *)
{
//    if(windAngle->isVisible())
//        ac_showHideCompass->setText(tr("Cacher le compas"));
//    else
//        ac_showHideCompass->setText(tr("Afficher le compas"));
//    popup->exec(QCursor::pos());
}
