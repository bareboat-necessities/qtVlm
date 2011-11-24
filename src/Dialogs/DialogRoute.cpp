/**********************************************************************
qtVlm: Virtual Loup de mer GUI
Copyright (C) 2008 - Christophe Thomas aka Oxygen77

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

Original code: zyGrib: meteorological GRIB file viewer
Copyright (C) 2008 - Jacques Zaninetti - http://zygrib.free.fr

***********************************************************************/

#include <cmath>
#include <QMessageBox>
#include <QDebug>

#include "DialogRoute.h"
#include "Util.h"
#include "MainWindow.h"
#include "DialogGraphicsParams.h"
#include "mycentralwidget.h"
#include "route.h"
#include "boatVLM.h"
#include "Player.h"
#include <QDesktopWidget>
#include "settings.h"
#include <QStandardItemModel>
#include <QStringListModel>
#include "POI.h"
#include <QItemDelegate>
#include "Grib.h"
#include <QRadialGradient>

//-------------------------------------------------------
// ROUTE_Editor: Constructor for edit an existing ROUTE
//-------------------------------------------------------
DialogRoute::DialogRoute(ROUTE *route,myCentralWidget *parent)
    : QDialog(parent)
{
    this->route=route;
    this->parent=parent;
    tabWidthRatio=-1;
    setupUi(this);
    Util::setFontDialog(this);
    this->resize(this->tabWidget->width()+10,tabWidget->height()+10);
    tabWidget->setParent(0);
    scroll=new QScrollArea(this);
    scroll->resize(tabWidget->size());
    scroll->setWidget(tabWidget);
    QSize mySize=QSize(tabWidget->size().width()+20,tabWidget->size().height()+20);
    QSize screenSize=QApplication::desktop()->screenGeometry().size()*.8;
    if(mySize.height() > screenSize.height())
    {
        mySize.setHeight(screenSize.height());
    }
    if(mySize.width() > screenSize.width())
    {
        mySize.setWidth(screenSize.width());
    }
    this->resize(mySize);
    scroll->resize(mySize);
    inputTraceColor =new InputLineParams(route->getWidth(),route->getColor(),1.6,  QColor(Qt::red),this,0.1,5);
    verticalLayout->addWidget( inputTraceColor);
    setWindowTitle(tr("Parametres Route"));
    editName->setText(route->getName());
    editFrozen->setChecked(route->getFrozen());
    this->speedLossOnTack->setValue(qRound(route->getSpeedLossOnTack()*100.00));


    startFromBoat->setChecked(route->getStartFromBoat());
    startFromMark->setChecked(!route->getStartFromBoat());

    editDateBox->setDateTime(route->getStartTime());

    editCoasts->setChecked(route->getDetectCoasts());
    hidePois->setChecked(route->getHidePois());
    autoRemove->setChecked(route->getAutoRemove());
    autoAt->setChecked(route->getAutoAt());
    vacStep->setValue(route->getMultVac());
    hidden->setChecked(route->getHidden());
    connect(this->btOk,SIGNAL(clicked()),this,SLOT(accept()));
    connect(this->btCancel,SIGNAL(clicked()),this,SLOT(reject()));
    connect(this->btAppliquer,SIGNAL(clicked()),this,SLOT(slotApply()));
    connect(this->Envoyer,SIGNAL(clicked()),this,SLOT(slotEnvoyer()));
    if(route->getUseVbvmgVlm())
    {
        if(route->getNewVbvmgVlm())
        {
            this->useVbvmgVlm->setCheckState(Qt::PartiallyChecked);
        }
        else
        {
            this->useVbvmgVlm->setCheckState(Qt::Checked);
        }
    }
    else
        this->useVbvmgVlm->setCheckState(Qt::Unchecked);
    //this->useVbvmgVlm->setToolTip("");
    switch(route->getStartTimeOption())
    {
    case 1:
        editVac->setChecked(true);
        break;
    case 2:
        editGrib->setChecked(true);
        break;
    case 3:
        editDate->setChecked(true);
        editDateBox->setEnabled(true);
    }
    this->tabWidget->setCurrentIndex(0);
    int n=0;
    if(parent->getPlayer()->getType()!=BOAT_REAL)
    {
        if(parent->getBoats())
        {
            QListIterator<boatVLM*> i (*parent->getBoats());
            while(i.hasNext())
            {
                boatVLM * acc = i.next();
                if(acc->getStatus())
                {
                    if(acc->getAliasState())
                        editBoat->addItem(acc->getAlias() + "(" + acc->getBoatPseudo() + ")");
                    else
                        editBoat->addItem(acc->getBoatPseudo());
                    if(acc->getId()==route->getBoat()->getId()) editBoat->setCurrentIndex(n);
                    n++;
                }
            }
        }
    }
    else
    {
        this->editVac->setText(tr("Date de la derniere MAJ de la position"));
        editBoat->addItem(parent->getPlayer()->getName());
        editBoat->setEnabled(false);
        this->pilototo->hide();
        this->tabWidget->removeTab(1);
        this->autoRemove->setChecked(false);
        this->autoRemove->hide();
        this->autoAt->setChecked(false);
        this->autoAt->hide();
    }
    if(route->isImported())
    {
        this->tabWidget->setTabEnabled(1,false);
        this->tabWidget->setTabEnabled(2,false);
    }
#if 1
    this->pilototo->hide();
    //this->tabWidget->removeTab(1);
#endif
    model= new QStandardItemModel();
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("Date et heure"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("Aller vers"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("Cap a suivre apres"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("Mode"));
    DateBoxDelegate * delegate=new DateBoxDelegate();
    pilotView->setModel(model);
    pilotView->setItemDelegate(delegate);
    pilotView->header()->setAlternatingRowColors(true);
    pilotView->header()->setDefaultAlignment(Qt::AlignCenter|Qt::AlignVCenter);
    connect(this->defaultOrders,SIGNAL(clicked()),this,SLOT(slotLoadPilototo()));
    connect(this->customOrders,SIGNAL(clicked()),this,SLOT(slotLoadPilototoCustom()));
    this->pilotView->resizeColumnToContents(2);
    this->pilotView->setColumnWidth(1,pilotView->columnWidth(2));
    this->pilotView->resizeColumnToContents(0);
    this->pilotView->setColumnWidth(0,this->pilotView->columnWidth(0)+30);
    this->pilotView->setColumnWidth(3,pilotView->columnWidth(2));
    this->roadMapInterval->setValue(route->getRoadMapInterval());
    int min=route->getBoat()->getVacLen()/60;
    this->roadMapInterval->setMinimum(min);
    this->roadMapInterval->setSingleStep(min);
    this->roadMapInterval->setValue(route->getRoadMapInterval());
    intervalTimer=new QTimer(this);
    intervalTimer->setSingleShot(true);
    intervalTimer->setInterval(800);
    connect(this->intervalTimer,SIGNAL(timeout()),this,SLOT(slotInterval()));
    connect(this->roadMapInterval,SIGNAL(valueChanged(int)),this,SLOT(slotIntervalTimer(int)));
    rmModel = new QStandardItemModel();
    rmModel->setColumnCount(11);
    rmModel->setHeaderData(0,Qt::Horizontal,QObject::tr("Date heure"));
    rmModel->setHeaderData(1,Qt::Horizontal," ");
    rmModel->setHeaderData(2,Qt::Horizontal,QObject::tr("TWS"));
    rmModel->setHeaderData(3,Qt::Horizontal,QObject::tr("TWD"));
    rmModel->setHeaderData(4,Qt::Horizontal,QObject::tr("TWA"));
    rmModel->setHeaderData(5,Qt::Horizontal,QObject::tr("Vitesse"));
    rmModel->setHeaderData(6,Qt::Horizontal,QObject::tr("Cap"));
    rmModel->setHeaderData(7,Qt::Horizontal,QObject::tr("POI cible"));
    rmModel->setHeaderData(8,Qt::Horizontal,QObject::tr("Distance"));
    rmModel->setHeaderData(9,Qt::Horizontal,QObject::tr("Lon POI cible"));
    rmModel->setHeaderData(10,Qt::Horizontal,QObject::tr("Lat POI cible"));
    rmModel->setSortRole(Qt::UserRole);
    roadMap->setModel(rmModel);
    connect(this->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(slotTabChanged(int)));
    tabWidthRatio=this->size().width()-tabWidget->size().width();
    roadMapWidthRatio=this->size().width()-roadMap->size().width();
    roadMapWidth=roadMap->size().width();
    tabWidth=tabWidget->size().width();
    drawBoat.moveTo(20,10);
    drawBoat.quadTo(12,22,17,30);
    drawBoat.lineTo(23,30);
    drawBoat.quadTo(28,22,20,10);
}
DialogRoute::~DialogRoute()
{
}
void DialogRoute::slotTabChanged(int tab)
{
    if(tab!=tabWidget->count()-1) return;
    disconnect(this->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(slotTabChanged(int)));
    slotInterval();
}

void DialogRoute::resizeEvent ( QResizeEvent*)
{
    this->scroll->resize(this->size());
    if(tabWidthRatio>0.0)
    {
        tabWidget->resize(qMax(tabWidth,this->size().width()-tabWidthRatio),tabWidget->size().height());
        roadMap->resize(qMax(roadMapWidth,this->size().width()-roadMapWidthRatio),roadMap->size().height());
    }
}
void DialogRoute::slotIntervalTimer(int)
{
    intervalTimer->start();
}

void DialogRoute::slotInterval()
{
    this->roadMapInterval->blockSignals(true);
    int val=roadMapInterval->value();
    int step=roadMapInterval->minimum();
    val=qRound(val/step)*step;
    roadMapInterval->setValue(val);
    rmModel->removeRows(0,rmModel->rowCount());
    double dist=0;
    double speedMoy=0;
    double twsMoy=0;
    QPen pen(Qt::gray);
    pen.setWidthF(0.5);
    QRadialGradient radialGrad(QPointF(20, 20), 15);
    radialGrad.setColorAt(0, Qt::white);
    radialGrad.setColorAt(0.8, Qt::blue);
    for(int i=0;i<route->getRoadMap()->count();++i)
    {
        QList<double>roadItems=route->getRoadMap()->at(i);
        dist+=roadItems.at(5);
        speedMoy+=roadItems.at(4);
        twsMoy+=roadItems.at(7);
        QPixmap img(40,40);
        img.fill(Qt::white);
        QPainter pnt(&img);
        pnt.setRenderHint(QPainter::Antialiasing);
        pen.setColor(Qt::gray);
        pnt.setPen(pen);
        pnt.setBrush(QBrush(radialGrad));
        QMatrix mat=QMatrix().translate(20,20).rotate(roadItems.at(3)).translate(-20,-20);
        pnt.setMatrix(mat);
        pnt.drawPath(this->drawBoat);
        pnt.setMatrixEnabled(false);
        QColor rgb=Qt::white;
        if(parent->getGrib() && parent->getGrib()->isOk())
        {
            rgb=QColor(parent->getGrib()->getWindColor(roadItems.at(7),true));
            rgb.setAlpha(255);
        }
        pen.setColor(rgb);
        pen.setWidth(2);
        pnt.setPen(pen);
        pnt.setBrush(Qt::NoBrush);
        this->drawWindArrowWithBarbs(pnt,20,20,
                                     roadItems.at(7),roadItems.at(6),
                                     roadItems.at(2)<0);

        if(i%(val/step)==0 || i==route->getRoadMap()->count()-1)
        {
            QList<QStandardItem*> roadPoint;
            QColor c=Qt::white;
            if(roadItems.at(4)!=-1)
            {
                roadPoint.append(new QStandardItem(QDateTime().fromTime_t((int)roadItems.at(0)).toUTC().toString("dd MMM yyyy hh:mm")));
                roadPoint[0]->setData(roadItems.at(0),Qt::UserRole);
                roadPoint.append(new QStandardItem());
                roadPoint[1]->setData(img,Qt::DecorationRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",roadItems.at(7))+tr(" nds")));
                roadPoint[2]->setData(roadItems.at(7),Qt::UserRole);
                roadPoint[2]->setData(rgb,Qt::BackgroundRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",roadItems.at(6))+tr("deg")));
                roadPoint[3]->setData(roadItems.at(6),Qt::UserRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",qAbs(roadItems.at(8)))+tr("deg")));
                roadPoint[4]->setData(roadItems.at(8),Qt::UserRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",roadItems.at(4))+tr(" nds")));
                roadPoint[5]->setData(roadItems.at(4),Qt::UserRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",roadItems.at(3))+tr("deg")));
                roadPoint[6]->setData(roadItems.at(3),Qt::UserRole);
                roadPoint.append(new QStandardItem(route->getPoiList().at((int)roadItems.at(9))->getName()));
                roadPoint[7]->setData(route->getPoiList().at((int)roadItems.at(9))->getName(),Qt::UserRole);
                roadPoint.append(new QStandardItem(QString().sprintf("%.2f",roadItems.at(10))+tr(" NM")));
                roadPoint[8]->setData(roadItems.at(0),Qt::UserRole);
                roadPoint.append(new QStandardItem(Util::formatLongitude(roadItems.at(1))));
                roadPoint[9]->setData(roadItems.at(1),Qt::UserRole);
                roadPoint.append(new QStandardItem(Util::formatLatitude(roadItems.at(2))));
                roadPoint[10]->setData(roadItems.at(2),Qt::UserRole);
                if(roadItems.at(8)>0)
                    c=Qt::red;
                else
                    c=Qt::green;
            }
            else
            {
                roadPoint.append(new QStandardItem(QDateTime().fromTime_t((int)roadItems.at(0)).toUTC().toString("dd MMM yyyy hh:mm")));
                roadPoint[0]->setData(roadItems.at(0),Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[1]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[2]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[3]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[4]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[5]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[6]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[7]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[8]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[9]->setData(0,Qt::UserRole);
                roadPoint.append(new QStandardItem("-"));
                roadPoint[10]->setData(0,Qt::UserRole);
            }
            for(int n=0;n<11;++n)
            {
                if(n%2==0 && n!=2)
                    roadPoint[n]->setData(QColor(240,240,240),Qt::BackgroundRole);
                if(n==4)
                    roadPoint[n]->setData(c,Qt::BackgroundRole);
                roadPoint[n]->setEditable(false);
                if(n==0 || n==7 || n==10 || roadItems.at(4)==-1)
                    roadPoint[n]->setTextAlignment(Qt::AlignCenter| Qt::AlignVCenter);
                else
                    roadPoint[n]->setTextAlignment(Qt::AlignRight| Qt::AlignVCenter);
            }
            rmModel->appendRow(roadPoint);
        }
    }
    if(route->getRoadMap()->count()!=0)
    {
        speedMoy=speedMoy/route->getRoadMap()->count();
        twsMoy=twsMoy/route->getRoadMap()->count();
    }
    for (int n=0;n<9;++n)
    {
        roadMap->resizeColumnToContents(n);
    }
    roadMap->header()->setDefaultAlignment(Qt::AlignCenter|Qt::AlignVCenter);
    this->orthDist->setText(QString().sprintf("%.2f",route->getInitialDist())+tr(" NM"));
    this->orthDistParcourue->setText(QString().sprintf("%.2f",dist)+tr(" NM"));
    this->avgSpeed->setText(QString().sprintf("%.2f",speedMoy)+tr(" nds"));
    this->avgTWS->setText(QString().sprintf("%.2f",twsMoy)+tr(" nds"));
    this->roadMapInterval->blockSignals(false);
}
void DialogRoute::drawTransformedLine( QPainter &pnt,
        double si, double co,int di, int dj, int i,int j, int k,int l)
{
    int ii, jj, kk, ll;
    ii = (int) (i*co-j*si +0.5) + di;
    jj = (int) (i*si+j*co +0.5) + dj;
    kk = (int) (k*co-l*si +0.5) + di;
    ll = (int) (k*si+l*co +0.5) + dj;
    pnt.drawLine(ii, jj, kk, ll);
}
//-----------------------------------------------------------------------------
void DialogRoute::drawPetiteBarbule(QPainter &pnt, bool south,
                    double si, double co, int di, int dj, int b)
{
    if (south)
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+2, -5);
    else
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+2, 5);
}
//---------------------------------------------------------------
void DialogRoute::drawGrandeBarbule(QPainter &pnt, bool south,
                    double si, double co, int di, int dj, int b)
{
    if (south)
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+4,-10);
    else
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+4,10);
}
//---------------------------------------------------------------
void DialogRoute::drawWindArrowWithBarbs(QPainter &pnt, int i, int j, double vkn, double ang,
                        bool south)
{
    int windBarbuleSize = 30;     // longueur des fleches avec barbules
    ang = degToRad(ang);
    ang-=PI_2;
    double si=sin(ang),  co=cos(ang);


    if (vkn < 1)
    {
        int r = 5;     // vent tres faible, dessine un cercle
        pnt.drawEllipse(i-r,j-r,2*r,2*r);
    }
    else {
        // Fleche centree sur l'origine
        int dec = -windBarbuleSize/2;
        drawTransformedLine(pnt, si,co, i,j,  dec,0,  dec+windBarbuleSize, 0);   // hampe
        drawTransformedLine(pnt, si,co, i,j,  dec,0,  dec+5, 2);    // fleche
        drawTransformedLine(pnt, si,co, i,j,  dec,0,  dec+5, -2);   // fleche

                int b1 = dec+windBarbuleSize -4;  // position de la 1ere barbule
                if (vkn >= 7.5  &&  vkn < 45 ) {
                        b1 = dec+windBarbuleSize;  // position de la 1ere barbule si >= 10 noeuds
                }

        if (vkn < 7.5) {  // 5 ktn
            drawPetiteBarbule(pnt,south, si,co, i,j, b1);
        }
        else if (vkn < 12.5) { // 10 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
        }
        else if (vkn < 17.5) { // 15 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawPetiteBarbule(pnt,south, si,co, i,j, b1-4);
        }
        else if (vkn < 22.5) { // 20 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-4);
        }
        else if (vkn < 27.5) { // 25 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-4);
            drawPetiteBarbule(pnt,south, si,co, i,j, b1-8);
        }
        else if (vkn < 32.5) { // 30 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
        }
        else if (vkn < 37.5) { // 35 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
            drawPetiteBarbule(pnt,south, si,co, i,j, b1-12);
        }
        else if (vkn < 45) { // 40 ktn
            drawGrandeBarbule(pnt,south, si,co, i,j, b1);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-12);
        }
        else if (vkn < 55) { // 50 ktn
            drawTriangle(pnt,south, si,co, i,j, b1-4);
        }
        else if (vkn < 65) { // 60 ktn
            drawTriangle(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
        }
        else if (vkn < 75) { // 70 ktn
            drawTriangle(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-12);
        }
        else if (vkn < 85) { // 80 ktn
            drawTriangle(pnt,south, si,co, i,j, b1-4);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-8);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-12);
            drawGrandeBarbule(pnt,south, si,co, i,j, b1-16);
        }
        else { // > 90 ktn
            drawTriangle(pnt,south, si,co, i,j, b1-4);
            drawTriangle(pnt,south, si,co, i,j, b1-12);
        }
    }
}
void DialogRoute::drawTriangle(QPainter &pnt, bool south,
                    double si, double co, int di, int dj, int b)
{
    if (south) {
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+4,-10);
        drawTransformedLine(pnt, si,co, di,dj,  b+8,0,  b+4,-10);
    }
    else {
        drawTransformedLine(pnt, si,co, di,dj,  b,0,  b+4,10);
        drawTransformedLine(pnt, si,co, di,dj,  b+8,0,  b+4,10);
    }
}

void DialogRoute::done(int result)
{
    if(result == QDialog::Accepted || result==99)
    {
        if (!parent->freeRouteName((editName->text()).trimmed(),route))
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Ce nom est deja utilise, choisissez en un autre"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            return;
        }
        if(this->editFrozen->isChecked() && (!this->editDate->isChecked() || this->startFromBoat->isChecked()))
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Vous ne pouvez figer une route que<br>si elle part d'un POI et d'une date fixe"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            return;
        }
        if(this->editFrozen->isChecked() && route->getFrozen())
        {
            if(this->editDateBox->dateTime()!=route->getStartTime())
                route->shiftEtas(editDateBox->dateTime());
        }
        if(!this->editFrozen->isChecked())
        {
            this->tabWidget->setTabEnabled(1,true);
            this->tabWidget->setTabEnabled(2,true);
        }
        route->setSpeedLossOnTack((double)this->speedLossOnTack->value()/100.00);
        route->setBusy(true);
        route->setName((editName->text()).trimmed());
        route->setWidth(inputTraceColor->getLineWidth());
        route->setColor(inputTraceColor->getLineColor());
        route->setRoadMapInterval(this->roadMapInterval->value());
        if(this->useVbvmgVlm->checkState()==Qt::Unchecked)
        {
            route->setUseVbVmgVlm(false);
            route->setNewVbvmgVlm(false);
        }
        else if(this->useVbvmgVlm->checkState()==Qt::PartiallyChecked)
        {
            route->setUseVbVmgVlm(true);
            route->setNewVbvmgVlm(true);
        }
        else
        {
            route->setUseVbVmgVlm(true);
            route->setNewVbvmgVlm(false);
        }
        route->setAutoRemove(this->autoRemove->isChecked());
        route->setAutoAt(autoAt->isChecked());
        route->setPilototo(this->pilototo->isChecked());
        //Settings::setSetting("useVbvmgVlm",route->getUseVbvmgVlm()?"1":"0"  );
        //Settings::setSetting("useNewVbvmgVlm",route->getNewVbvmgVlm()?"1":"0"  );
        route->setMultVac(vacStep->value());
        if(editVac->isChecked())
            route->setStartTimeOption(1);
        if (editGrib->isChecked())
            route->setStartTimeOption(2);
        if (editDate->isChecked())
        {
            route->setStartTimeOption(3);
            route->setStartTime(editDateBox->dateTime());
        }
        route->setStartFromBoat(startFromBoat->isChecked());
        if(parent->getPlayer()->getType()!=BOAT_REAL)
        {
            if(parent->getBoats())
            {
                QListIterator<boatVLM*> i (*parent->getBoats());
                while(i.hasNext())
                {
                    boatVLM * acc = i.next();
                    if(acc->getBoatPseudo()==editBoat->currentText())
                    {
                        route->setBoat(acc);
                        break;
                    }
                }
            }
        }
        else
            route->setBoat((boat *) parent->getRealBoat());
        route->setHidden(hidden->isChecked());
        route->setFrozen(editFrozen->isChecked());
        route->setDetectCoasts(editCoasts->isChecked());
        route->getLine()->setCoastDetection(editCoasts->isChecked());
        if(hidePois->isChecked()!=route->getHidePois())
            route->setHidePois(hidePois->isChecked());
        if(this->Simplifier->isChecked())
            route->setSimplify(true);
        else
        {
            route->setSimplify(false);
            if(this->Optimiser->isChecked())
                route->setOptimize(true);
        }
        route->setBusy(false);
        if(result==99)
        {
            parent->treatRoute(route);
            this->slotInterval();
            return;
        }
    }
    if(result == QDialog::Rejected)
    {
    }
    QDialog::done(result);
}
void DialogRoute::slotApply()
{
    this->btAppliquer->setEnabled(false);
    this->btCancel->setEnabled(false);
    this->btOk->setEnabled(false);
    this->tabWidget->setEnabled(false);
    if(this->Simplifier->isChecked() || this->Optimiser->isChecked())
        this->hide();
    this->done(99);
    this->btAppliquer->setEnabled(true);
    this->btCancel->setEnabled(true);
    this->btOk->setEnabled(true);
    this->tabWidget->setEnabled(true);
    this->Simplifier->setChecked(false);
    this->Optimiser->setChecked(false);
    this->show();
}

void DialogRoute::GybeTack(int i)
{
    QFont font=this->labelTackGybe->font();
    if(i==100)
        font.setBold(false);
    else
        font.setBold(true);
    this->labelTackGybe->setFont(font);
}
void DialogRoute::slotLoadPilototo()
{
    if(!(this->startFromBoat->isChecked() &&
         this->useVbvmgVlm->isChecked()))
    {
        QMessageBox::critical(0,tr("Pilototo"),tr("Pour utiliser cette action il faut que:<br>- La route parte du bateau<br>- Le mode VBVMG-VLM soit actif"));
        return;
    }
    if(route->getPoiList().isEmpty())
    {
        return;
    }
    this->fillPilotView(true);
}
void DialogRoute::slotLoadPilototoCustom()
{
    if(!(this->startFromBoat->isChecked() &&
         this->useVbvmgVlm->isChecked()))
    {
        QMessageBox::critical(0,tr("Pilototo"),tr("Pour utiliser cette action il faut que:<br>- La route parte du bateau<br>- Le mode VBVMG-VLM soit actif"));
        return;
    }
    if(route->getPoiList().isEmpty())
    {
        return;
    }
    this->fillPilotView(false);
}
void DialogRoute::fillPilotView(bool def)
{
    if(!(route->getStartFromBoat() &&
         route->getUseVbvmgVlm()))
    {
        QMessageBox::critical(0,tr("Pilototo"),tr("Pour utiliser cette action il faut que:<br>- La route parte du bateau<br>- Le mode VBVMG-VLM soit actif"));
        return;
    }
    if(route->getPoiList().isEmpty())
    {
        return;
    }
    model->removeRows(0,model->rowCount());
    listPois.clear();
    for(int n=0;n<route->getPoiList().count();++n)
    {
        if(model->rowCount()==6) break;
        POI * poi=route->getPoiList().at(n);
        if(!def && !poi->getPiloteSelected()) continue;
        listPois.append(poi);
        time_t eta;
        QList<QStandardItem*> items;
        if(listPois.count()!=1)
        {
            eta=listPois.at(listPois.count()-2)->getRouteTimeStamp();
            eta=eta+20;
            if(eta==-1) break;
            items.append(new QStandardItem());
            QDateTime tt=QDateTime().fromTime_t(eta).toUTC();
            tt.setTimeSpec(Qt::UTC);
            items[0]->setData(tt.toString("dd MMM yyyy-hh:mm:ss"),Qt::EditRole);
            items[0]->setEditable(true);
        }
        else
        {
            items.append(new QStandardItem(tr("WP-VLM")));
            items[0]->setEditable(false);
        }
        items[0]->setData(QVariant(QMetaType::VoidStar, &poi ),Qt::UserRole);
        items[0]->setTextAlignment(Qt::AlignCenter| Qt::AlignVCenter);
        items.append(new QStandardItem(poi->getName()));
        items[1]->setTextAlignment(Qt::AlignCenter| Qt::AlignVCenter);
        items[1]->setEditable(false);
        items.append(new QStandardItem(QString().sprintf("%.2f",poi->getWph())));
        items[2]->setTextAlignment(Qt::AlignCenter| Qt::AlignVCenter);
        items[2]->setEditable(false);
        switch(poi->getNavMode())
        {
            case 0:
                items.append(new QStandardItem("VB-VMG"));
                break;
            case 1:
                items.append(new QStandardItem("VMG"));
                break;
            case 2:
                items.append(new QStandardItem("ORTHO"));
                break;
        }
        items[3]->setTextAlignment(Qt::AlignCenter| Qt::AlignVCenter);
        items[3]->setEditable(false);
        model->appendRow(items);
    }

    this->pilotView->resizeColumnToContents(2);
    this->pilotView->setColumnWidth(1,pilotView->columnWidth(2));
    this->pilotView->resizeColumnToContents(0);
    this->pilotView->setColumnWidth(0,this->pilotView->columnWidth(0)+30);
    this->pilotView->setColumnWidth(3,pilotView->columnWidth(2));
}
void DialogRoute::slotEnvoyer()
{
    QList<POI*> poiList;
    for (int n=0;n<model->rowCount();++n)
    {
        POI * poi=reinterpret_cast<struct POI *>(qvariant_cast<void*>(model->item(n,0)->data(Qt::UserRole)));
        QDateTime tt=QDateTime().fromString(model->item(n,0)->data(Qt::EditRole).toString(),"dd MMM yyyy-hh:mm:ss");
        tt.setTimeSpec(Qt::UTC);
        //qWarning()<<poi->getName()<<tt;
        poi->setPiloteDate(tt.toTime_t());
        poiList.append(poi);
    }
    parent->setPilototo(poiList);
    this->done(QDialog::Accepted);
}

//---------------------------------------
DateBoxDelegate::DateBoxDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}
QWidget *DateBoxDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/* option */,
    const QModelIndex &/* index */) const
{
    QDateTimeEdit *editor = new QDateTimeEdit(parent);
    editor->setTimeSpec(Qt::UTC);
    editor->setDisplayFormat("dd MMM yyyy-hh:mm:ss");
    return editor;
}
void DateBoxDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QDateTime value = QDateTime().fromString(index.model()->data(index, Qt::EditRole).toString(),"dd MMM yyyy-hh:mm:ss");
    value.setTimeSpec(Qt::UTC);
    //qWarning()<<"setEditorData"<<index<<value;
    QDateTimeEdit *editBox = static_cast<QDateTimeEdit*>(editor);
    editBox->setMinimumDateTime(QDateTime().currentDateTimeUtc());
    editBox->setDateTime(value);
}
void DateBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QDateTimeEdit *editBox = static_cast<QDateTimeEdit*>(editor);
    QDateTime value = editBox->dateTime().toUTC();
    value.setTimeSpec(Qt::UTC);
    model->setData(index,value.toString("dd MMM yyyy-hh:mm:ss"),Qt::EditRole);
}
void DateBoxDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
