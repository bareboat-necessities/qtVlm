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
#ifdef QT_V5
#include <QtWidgets/QMessageBox>
#else
#include <QMessageBox>
#endif
#include <QDebug>

#include "DialogRoutage.h"
#include "Util.h"
#include "POI.h"
#include "MainWindow.h"
#include "DialogGraphicsParams.h"
#include "mycentralwidget.h"
#include "routage.h"
#include "boatVLM.h"
#include "Player.h"
#include <QThread>
#include <QDesktopWidget>
#include "settings.h"
#include "Terrain.h"


//-------------------------------------------------------
// ROUTAGE_Editor: Constructor for edit an existing ROUTAGE
//-------------------------------------------------------
DialogRoutage::DialogRoutage(ROUTAGE *routage,myCentralWidget *parent, POI *endPOI)
    : QDialog(parent)
{
    QString m;
    this->routage=routage;
    this->parent=parent;

    if(endPOI)
        routage->setToPOI(endPOI);
    setupUi(this);
    Util::setFontDialog(this);
    connect(this->Default,SIGNAL(clicked()),this,SLOT(slot_default()));
    this->i_iso->setChecked(routage->getI_done());
    this->i_iso->setDisabled((!routage->isDone() || routage->getI_done()));
    this->isoRoute->setDisabled(!routage->isDone());
    if(routage->isDone())
    {
        this->isoRoute->setMaximum(routage->getTimeStepMore24());
        this->isoRoute->setValue(routage->getIsoRouteValue());
    }
    m.sprintf("%d",QThread::idealThreadCount());
    m=tr("Calculer en parallele (")+m+tr(" processeurs disponibles)");
    multi->setText(m);
    if(QThread::idealThreadCount()<=1)
    {
        routage->setUseMultiThreading(false);
        multi->setEnabled(false);
    }
    multi->setChecked(routage->getUseMultiThreading());
    editName->setFocus();
    inputTraceColor =new InputLineParams(routage->getWidth(),routage->getColor(),1.6,  QColor(Qt::red),this,0.1,5);
    colorBox->layout()->addWidget(inputTraceColor);
    setWindowTitle(tr("Parametres Routage"));
    editName->setText(routage->getName());
    editDateBox->setDateTime(routage->getStartTime());
    editDateBox->setEnabled(true);
    whatIfDate->setDateTime(routage->getWhatIfDate());
    whatIfUse->setChecked(routage->getWhatIfUsed());
    whatIfWind->setValue(routage->getWhatIfWind());
    whatIfTime->setValue(routage->getWhatIfTime());
    autoZoom->setChecked(routage->getAutoZoom());
    this->zoomLevel->setValue(routage->getZoomLevel());
    visibleOnly->setChecked(routage->getVisibleOnly());
    this->poiPrefix->setText(routage->getPoiPrefix());
    this->startFromBoat->setChecked(routage->getRouteFromBoat());
    this->maxPortant->setValue(routage->getMaxPortant());
    this->maxPres->setValue(routage->getMaxPres());
    this->minPortant->setValue(routage->getMinPortant());
    this->minPres->setValue(routage->getMinPres());
    this->maxWaveHeight->setValue(routage->get_maxWaveHeight());
    if(!routage->isDone())
        this->convRoute->setChecked(Settings::getSetting("autoConvertToRoute","0").toInt()==1);
    if(routage->getFinalEta().isNull())
        this->groupBox_eta->setHidden(true);
    else
    {
        this->groupBox_eta->setHidden(false);
        this->editDateBox_2->setDateTime(routage->getFinalEta());
    }
    int n=0;
    if(parent->getPlayer()->getType()!=BOAT_REAL)
    {
        this->speedLossOnTack->setValue(Settings::getSetting("speedLossOnTackVlm","100").toInt());
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
                    if(acc==routage->getBoat()) editBoat->setCurrentIndex(n);
                    n++;
                }
            }
        }
    }
    else
    {
        this->speedLossOnTack->setValue(Settings::getSetting("speedLossOnTackReal","100").toInt());
        editBoat->addItem(parent->getPlayer()->getName());
        editBoat->setEnabled(false);
    }
    n=0;
    if(routage->isDone())
    {
        if(routage->getFromPOI())
        {
            fromPOI->addItem(routage->getFromPOI()->getName(),parent->getPois().indexOf(routage->getFromPOI()));
            fromPOI->setCurrentIndex(0);
        }
        if(routage->getToPOI())
        {
            toPOI->addItem(routage->getToPOI()->getName(),parent->getPois().indexOf(routage->getToPOI()));
            toPOI->setCurrentIndex(0);
        }
    }
    else
    {
        for(int m=0;m<parent->getPois().count();m++)
        {
            POI * poi = parent->getPois().at(m);
            if(poi->getType()!= POI_TYPE_BALISE && poi->getRoute()==NULL)
            {
                fromPOI->addItem(poi->getName(),m);
                if(poi==routage->getFromPOI()) fromPOI->setCurrentIndex(n);

                toPOI->addItem(poi->getName(),m);
                if(poi==routage->getToPOI()) toPOI->setCurrentIndex(n);
                n++;
            }
        }
    }
    this->range->setValue(routage->getAngleRange());
    this->step->setValue(routage->getAngleStep());
    this->dureeLess24->setValue(routage->getTimeStepLess24());
    this->dureeMore24->setValue(routage->getTimeStepMore24());
    this->showIso->setChecked(routage->getShowIso());
    this->explo->setValue(routage->getExplo());
    this->useVac->setChecked(routage->getUseRouteModule());
    this->log->setChecked(routage->useConverge);
    this->pruneWakeAngle->setValue(routage->pruneWakeAngle);
    this->colorIso->setChecked(routage->getColorGrib());
    this->RoutageOrtho->setChecked(routage->getRoutageOrtho());
    this->showBestLive->setChecked(routage->getShowBestLive());
    this->checkCoast->setChecked(routage->getCheckCoast());
    this->checkLines->setChecked(routage->getCheckLine());
    this->nbAlter->setValue(routage->getNbAlternative());
    this->diver->setValue(routage->getThresholdAlternative());
    if(routage->isDone() || routage->getIsNewPivot())
    {
        this->speedLossOnTack->setValue(qRound(routage->getSpeedLossOnTack()*100));
        this->speedLossOnTack->setDisabled(true);
        this->editName->setDisabled(false);
        this->autoZoom->setDisabled(true);
        this->zoomLevel->setDisabled(true);
        this->visibleOnly->setDisabled(true);
        this->editBoat->setDisabled(true);
        this->editDateBox->setDisabled(true);
        this->fromPOI->setDisabled(true);
        this->toPOI->setDisabled(true);
        this->dureeMore24->setDisabled(true);
        this->dureeLess24->setDisabled(true);
        this->range->setDisabled(true);
        this->step->setDisabled(true);
        this->explo->setDisabled(true);
        this->useVac->setDisabled(true);
        this->log->setDisabled(true);
        this->pruneWakeAngle->setDisabled(true);
        this->RoutageOrtho->setDisabled(true);
        this->showBestLive->setDisabled(true);
        this->checkCoast->setDisabled(true);
        this->checkLines->setDisabled(true);
        //this->diver->setDisabled(true);
        //this->nbAlter->setDisabled(true);
        if(routage->isConverted())
            this->convRoute->setDisabled(true);

        this->startFromBoat->setDisabled(true);
        this->whatIfUse->setDisabled(true);
        this->whatIfDate->setDisabled(true);
        this->whatIfWind->setDisabled(true);
        this->whatIfTime->setDisabled(true);
        this->multi->setDisabled(true);
        this->maxPortant->setDisabled(true);
        this->maxPres->setDisabled(true);
        this->maxWaveHeight->setDisabled(true);
        this->minPortant->setDisabled(true);
        this->minPres->setDisabled(true);
        this->Default->setDisabled(true);
        this->multi_routage->setDisabled(true);
    }
    if(routage->getIsNewPivot() && !routage->isDone())
    {
        this->speedLossOnTack->setDisabled(false);
        this->autoZoom->setDisabled(false);
        this->zoomLevel->setDisabled(false);
        this->visibleOnly->setDisabled(false);
        this->dureeMore24->setDisabled(false);
        this->dureeLess24->setDisabled(false);
        this->range->setDisabled(false);
        this->step->setDisabled(false);
        this->explo->setDisabled(false);
        this->toPOI->setDisabled(false);
        this->log->setDisabled(false);
        this->pruneWakeAngle->setDisabled(false);
        this->RoutageOrtho->setDisabled(false);
        this->showBestLive->setDisabled(false);
        this->checkCoast->setDisabled(false);
        this->checkLines->setDisabled(false);
        this->diver->setDisabled(false);
        this->nbAlter->setDisabled(false);
        this->whatIfUse->setDisabled(false);
        this->whatIfDate->setDisabled(false);
        this->whatIfWind->setDisabled(false);
        this->whatIfTime->setDisabled(false);
        this->multi->setDisabled(false);
        this->maxPres->setDisabled(false);
        this->maxWaveHeight->setDisabled(false);
        this->maxPortant->setDisabled(false);
        this->minPres->setDisabled(false);
        this->minPortant->setDisabled(false);
        this->Default->setDisabled(false);
        this->multi_routage->setDisabled(true);
    }
    if(routage->isDone() && !routage->getArrived())
        i_iso->setEnabled(false);
    this->multi_routage->setChecked(routage->get_multiRoutage());
    this->multi_nb->setValue(routage->get_multiNb()+1);
    this->multi_days->setValue(routage->get_multiDays());
    this->multi_hours->setValue(routage->get_multiHours());
    this->multi_min->setValue(routage->get_multiMin());
}
DialogRoutage::~DialogRoutage()
{
    Settings::setSetting(this->objectName()+".height",this->height());
    Settings::setSetting(this->objectName()+".width",this->width());
}

void DialogRoutage::slot_default()
{
    this->maxPres->setValue(70);
    this->maxWaveHeight->setValue(100);
    this->minPres->setValue(0);
    this->maxPortant->setValue(70);
    this->minPortant->setValue(0);
    this->step->setValue(3);
    this->dureeLess24->setValue(30);
    this->dureeMore24->setValue(60);
    this->range->setValue(180);
    this->showIso->setChecked(true);
    this->speedLossOnTack->setValue(100);
    this->useVac->setChecked(true);
    this->multi->setChecked(true);
    this->visibleOnly->setChecked(true);
    this->autoZoom->setChecked(true);
    this->zoomLevel->setValue(2);
    this->pruneWakeAngle->setValue(30);
    this->showBestLive->setChecked(true);
    this->RoutageOrtho->setChecked(true);
    this->colorIso->setChecked(false);
    this->explo->setValue(40);
    this->log->setChecked(true);
    this->whatIfUse->setChecked(false);
    this->checkCoast->setChecked(true);
    this->checkLines->setChecked(true);
    this->nbAlter->setValue(3);
    this->diver->setValue(75);
}

//---------------------------------------
void DialogRoutage::done(int result)
{
    if(result == QDialog::Accepted)
    {
        if (!parent->freeRoutageName((editName->text()).trimmed(),routage))
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Ce nom est deja utilise, choisissez en un autre"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            return;
        }
        routage->setUseMultiThreading(this->multi->isChecked());
        routage->setPoiPrefix(this->poiPrefix->text());
        routage->setName((editName->text()).trimmed());
        routage->setWidth(inputTraceColor->getLineWidth());
        routage->setColor(inputTraceColor->getLineColor());
        QDateTime dd=editDateBox->dateTime();
        if(routage->getBoat()->get_boatType()==BOAT_VLM)
        {
            time_t ddd=dd.toTime_t();
            ddd=floor((double)ddd/(double)routage->getBoat()->getVacLen())*routage->getBoat()->getVacLen();
            dd=dd.fromTime_t(ddd);
        }
        routage->setStartTime(dd);
        routage->setCheckCoast(checkCoast->isChecked());
        routage->setCheckLine(checkLines->isChecked());
        bool reCalculateAlternative=false;
        if(routage->isDone() && (routage->getNbAlternative()!=nbAlter->value()||
                                 routage->getThresholdAlternative()!=diver->value()))
            reCalculateAlternative=true;
        routage->setNbAlternative(this->nbAlter->value());
        routage->setThresholdAlternative(this->diver->value());
        routage->useConverge=log->isChecked();
        routage->pruneWakeAngle=pruneWakeAngle->value();
        routage->setColorGrib(this->colorIso->isChecked());
        routage->setRoutageOrtho(this->RoutageOrtho->isChecked());
        routage->setShowBestLive(this->showBestLive->isChecked());
        routage->setAutoZoom(autoZoom->isChecked());
        routage->setZoomLevel(this->zoomLevel->value());
        routage->setVisibleOnly(visibleOnly->isChecked());
        routage->setRouteFromBoat(this->startFromBoat->isChecked());
        routage->setSpeedLossOnTack((double)this->speedLossOnTack->value()/100.00);
        routage->setMaxPortant(this->maxPortant->value());
        routage->setMaxPres(this->maxPres->value());
        routage->set_maxWaveHeight(this->maxWaveHeight->value());
        routage->setMinPortant(this->minPortant->value());
        routage->setMinPres(this->minPres->value());
        routage->set_multiRoutage(this->multi_routage->isChecked());
        routage->set_multiNb(this->multi_nb->value()-1);
        routage->set_multiDays(this->multi_days->value());
        routage->set_multiHours(this->multi_hours->value());
        routage->set_multiMin(this->multi_min->value());
        if(parent->getPlayer()->getType()!=BOAT_REAL)
        {
            if(parent->getBoats())
            {
                if(editBoat->currentText().isEmpty() || editBoat->count()==0)
                {
                    QMessageBox::critical(0,tr("Creation d'un routage"),
                                          tr("Pas de bateau selectionne, pas de routage possible"));
                    return;
                }
                QListIterator<boatVLM*> i (*parent->getBoats());
                while(i.hasNext())
                {
                    boatVLM * acc = i.next();
                    if(acc->getBoatPseudo()==editBoat->currentText())
                    {
                        routage->setBoat(acc);
                        break;
                    }
                }
                if(this->useVac->isChecked())
                {
                    if(this->dureeLess24->value()<4*routage->getBoat()->getVacLen()/60)
                    {
                        QMessageBox::critical(0,tr("Creation d'un routage"),
                                              tr("Vous ne pouvez pas utiliser le module route<br>avec moins de 4 vacations entre les isochrones.<br>Vous devez donc desactiver cette option ou rallonger la duree"));
                        return;
                    }
                    if(this->dureeMore24->value()<4*routage->getBoat()->getVacLen()/60)
                    {
                        QMessageBox::critical(0,tr("Creation d'un routage"),
                                              tr("Vous ne pouvez pas utiliser le module route<br>avec moins de 4 vacations entre les isochrones.<br>Vous devez donc desactiver cette option ou rallonger la duree"));
                        return;
                    }
                }
            }
        }
        else
            routage->setBoat((boat *) parent->getRealBoat());
        if(!routage->isDone())
        {
            routage->setFromPOI(NULL);
            routage->setToPOI(NULL);
            if(toPOI->currentIndex()==-1)
            {
                QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le POI de destination est invalide")));
                return;
            }
            routage->setToPOI(parent->getPois().at(toPOI->itemData(toPOI->currentIndex(),Qt::UserRole).toInt()));
            if(!startFromBoat->isChecked() && fromPOI->currentIndex()==-1)
            {
                QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le POI de depart est invalide")));
                return;
            }
            else
            {
                routage->setFromPOI(parent->getPois().at(fromPOI->itemData(fromPOI->currentIndex(),Qt::UserRole).toInt()));
            }
            if(routage->getToPOI()==NULL)
            {
                QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le POI de destination est invalide")));
                return;
            }
            if(!routage->getRouteFromBoat())
            {
                if(routage->getFromPOI()==NULL)
                {
                    QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le POI de depart est invalide")));
                    return;
                }
                if(routage->getFromPOI()->getLongitude()==routage->getToPOI()->getLongitude() &&
                   routage->getFromPOI()->getLatitude()==routage->getToPOI()->getLatitude())
                {
                    if(!routage->getIsNewPivot() && !routage->getIsPivot())
                    {
                        QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le POI de depart et d'arrivee sont les memes, vous etes deja arrive...")));
                        return;
                    }
                }
            }
            else
            {
                if(this->multi_routage->isChecked())
                {
                    QMessageBox::critical(0,QString(QObject::tr("Routage")),QString(QObject::tr("Le routage ne peut pas partir du bateau si la fonction<br>multi-routage est utilisee")));
                    return;
                }
            }
        }
        routage->setWhatIfUsed(whatIfUse->isChecked());
        routage->setWhatIfDate(whatIfDate->dateTime());
        routage->setWhatIfWind(whatIfWind->value());
        routage->setWhatIfTime(whatIfTime->value());
        routage->setAngleRange(this->range->value());
        routage->setAngleStep(this->step->value());
        routage->setTimeStepMore24(this->dureeMore24->value());
        routage->setTimeStepLess24(this->dureeLess24->value());
        routage->setExplo(this->explo->value());
        routage->setShowIso(this->showIso->isChecked());
        routage->setUseRouteModule(this->useVac->isChecked());
        if(!routage->isDone())
            Settings::setSetting("autoConvertToRoute",convRoute->isChecked()?1:0);
        if(this->convRoute->isChecked() || this->multi_routage->isChecked())
        {
            if(!routage->isConverted())
            {
                if (!parent->freeRouteName(editName->text().trimmed(),NULL))
                {
                    QMessageBox msgBox;
                    msgBox.setText(tr("Ce nom de route est deja utilise, veuillez changer le nom du routage"));
                    msgBox.setIcon(QMessageBox::Critical);
                    msgBox.exec();
                    return;
                }
                if(routage->isDone())
                {
                    int rep=QMessageBox::Yes;
                    if(routage->getWhatIfUsed() && (routage->getWhatIfTime()!=0 || routage->getWhatIfWind()!=100))
                    {
                        rep = QMessageBox::question (0,
                                tr("Convertir en route"),
                                tr("Ce routage a ete calcule avec une hypothese modifiant les donnees du grib<br>La route ne prendra pas ce scenario en compte<br>Etes vous sur de vouloir le convertir en route?"),
                                QMessageBox::Yes | QMessageBox::No);
                    }
                    if(rep==QMessageBox::Yes)
                    {
                        routage->convertToRoute();
                    }
                }
                else
                    routage->setConverted();
            }
        }
        else
        {
            if(i_iso->isChecked() && routage->getTimeStepLess24()!=routage->getTimeStepMore24())
            {
                QMessageBox::critical(0,tr("Isochrones inverses"),
                                      tr("Pour l'instant, on ne peut pas calculer les isochrones inverses<br>si la duree des isochrones est variable"));
                return;
            }
            routage->setIsoRouteValue(this->isoRoute->value());
            routage->setI_iso(i_iso->isChecked());
        }
        if(!routage->isDone())
            routage->setIsoRouteValue(routage->getTimeStepMore24());
        else if(reCalculateAlternative)
            routage->calculateAlternative();
    }
    if(routage->isDone())
    {
//        if(routage->getColorGrib())
//            routage->setShowIso(false);
        if(!routage->getColorGrib() && parent->getTerre()->getRoutageGrib()==routage)
            parent->getTerre()->setRoutageGrib(NULL);
        else if(routage->getColorGrib() && parent->getTerre()->getRoutageGrib()!=routage)
            parent->getTerre()->setRoutageGrib(routage);
    }
    if(result == QDialog::Rejected)
    {
    }
    QDialog::done(result);
}

//---------------------------------------

void DialogRoutage::GybeTack(int i)
{
    QFont font=this->labelTackGybe->font();
    if(i==100)
        font.setBold(false);
    else
        font.setBold(true);
    this->labelTackGybe->setFont(font);
}

