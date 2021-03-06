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
#include <QDebug>
#include <parser.h>
#include <serializer.h>

#include "boatVLM.h"

#include "MainWindow.h"
#include "vlmLine.h"
#include "parser.h"
#include "Util.h"
#include "Polar.h"
#include "settings.h"
#include "orthoSegment.h"
#include "Orthodromie.h"
#include "inetConnexion.h"
#include "DataManager.h"


boatVLM::boatVLM(QString        pseudo, bool activated, int boatId, int playerId,Player * player, int isOwn,
                 Projection *   proj,MainWindow * main, myCentralWidget * parent,
                 inetConnexion * inet):
   boat(pseudo,activated,proj,main,parent),
   inetClient(inet),
   pilotType (0)
{
    set_boatType(BOAT_VLM);
    setFont(QApplication::font());

    this->isOwn=isOwn;
    name="";
    alias="";
    useAlias=false;
    inetClient::setName(pseudo);
    this->player=player;
    boat_id=boatId;
    player_id=playerId;
    race_id=0;
    pilototo.clear();
    hasPilototo=false;
    polarVlm="";
    forcePolar=false;
    updating=false;
    vacLen=300;
    doingSync=false;
    gatesLoaded=false;
    nWP=0;
    this->porteHidden=parent->get_shPor_st();
    firstSynch=false;
    race_name="";
    playerName=player->getName();
    this->rank=1;
    connect(parent, SIGNAL(shPor(bool)),this,SLOT(slot_shPor(bool)));
    connect(parent,SIGNAL(resetTraceCache()),this,SLOT(slot_resetTraceCache()));
    myCreatePopUpMenu();
    connect(this->popup,SIGNAL(aboutToShow()),parent,SLOT(slot_resetGestures()));
    connect(this->popup,SIGNAL(aboutToHide()),parent,SLOT(slot_resetGestures()));
    this->initialized=false;
    own=QString();
    npd=QString();
    showNpd=false;
    this->logIndexLimit=12; //number of boatinfo records we keep
    loadBoatInfolog();
    //setStatus(activated);
}

boatVLM::~boatVLM(void)
{
    if(gatesLoaded)
    {
        vlmLine * porte;
        QListIterator<vlmLine*> i (gates);
        while(i.hasNext())
        {
            porte=i.next();
            //parent->getScene()->removeItem(porte);
            if(porte)
            {
                if(!parent->getAboutToQuit())
                    delete porte;
            }
        }
    }
    saveBoatInfolog();
}

void boatVLM::updateData(boatData * data)
{
//    this->boat_name=data->name;
//    this->playerName=data->playerName;
    this->boat_id=data->idu;
    this->isOwn=data->isOwn;
    this->pseudo=data->pseudo;
    this->name=data->name;
    /* Not updating activated status */
    //this->setStatus(data->engaged>0);
}

/***********************************/
/* VLM link                        */

void boatVLM::slot_resetTraceCache()
{
    this->trace_drawing->deleteAll();
}

void boatVLM::slot_getData(bool doingSync)
{
    updating=true;
    doRequest(VLM_REQUEST_BOAT);
    this->doingSync=doingSync;
}

void boatVLM::slot_getDataTrue()
{
    slot_getData(true);
}

void boatVLM::set_pilotHeading(double heading) {
    QVariantMap instruction;
    if(confirmChange(tr("Pilotage au cap ?"),tr("Mode de pilotage change en 'Cap'"))) {
        instruction.insert("pim",1);
        instruction.insert("pip",heading);
        sendPilotMode("pilot_set.php",instruction);
    }
}

void boatVLM::set_pilotAngle(double angle) {
    QVariantMap instruction;
    if(confirmChange(tr("Pilotage a angle constant par rapport au vent ?"),tr("Mode de pilotage change en 'Angle'"))) {
        instruction.insert("pim",2);
        instruction.insert("pip",angle);
        sendPilotMode("pilot_set.php",instruction);
    }
}

void boatVLM::set_pilotOrtho(void) {
    QVariantMap instruction;
    if(confirmChange(tr("Pilotage orthodromique ?"),tr("Mode de pilotage change en 'Pilot Ortho'"))) {
        instruction.insert("pim",3);
        sendPilotMode("pilot_set.php",instruction);
    }
}

void boatVLM::set_pilotVmg(void) {
    QVariantMap instruction;
    if(confirmChange(tr("Pilotage en VMG ?"),tr("Mode de pilotage change en 'VMG'"))) {
        instruction.insert("pim",4);
        sendPilotMode("pilot_set.php",instruction);
    }
}

void boatVLM::set_pilotVbvmg(void) {
    QVariantMap instruction;
    if(confirmChange(tr("Pilotage en VBVMG ?"),tr("Mode de pilotage change en 'VBVMG'"))) {
        instruction.insert("pim",5);
        sendPilotMode("pilot_set.php",instruction);
    }
}

void boatVLM::setWP(QPointF WP,double WPHd) {
    if(getLockStatus()) return;

    if(confirmChange(tr("Confirmer le changement du WP"),tr("WP change"))) {
        QVariantMap instruction;
        QVariantMap pip;
        pip.insert("targetlat",WP.y());
        pip.insert("targetlong",WP.x());
        pip.insert("targetandhdg",WPHd);
        instruction.insert("pip",pip);
        sendPilotMode("target_set.php",instruction);
    }
}

double boatVLM::getWPangle(void) {
    return Util::A360(getOrtho()-getWindDir());
}


bool boatVLM::confirmChange(QString question, QString info)
{
    if(Settings::getSetting("askConfirmation","0").toInt()==0)
        return true;

    if(QMessageBox::question(mainWindow,tr("Instruction pour ")+getBoatPseudo(),
                             getBoatPseudo()+": " + question,QMessageBox::Yes|QMessageBox::No,
                             QMessageBox::Yes)==QMessageBox::Yes) {
        emit showMessage(getBoatPseudo()+": " + info,2000);
        return true;
    }
    return false;
}

void boatVLM::sendPilotMode(QString phpScript,QVariantMap instruction) {

    //qWarning() << "sendPilotMode: " << phpScript << " - " << instruction;

    if(hasInet()) {
        if(hasRequest()) {
            qWarning() << "request already running for " << getBoatPseudo();
            return;
        }

        QString url;
        QString data;

        instruction.insert("idu",getId());

        QJson::Serializer serializer;
        QByteArray json = serializer.serialize(instruction);

        QTextStream(&url) << "/ws/boatsetup/" << phpScript;



        QTextStream(&data) << "parms=" << json;
        QTextStream(&data) << "&select_idu=" << getId();

        inetPost(VLM_REQUEST_SENDPILOT,url,data,QString(),true);
    }
}


void boatVLM::doRequest(int requestCmd)
{
    if(!activated)
    {
        //qWarning() << "Doing a synch on a non activated boat";
        while(!gates.isEmpty())
           delete gates.takeFirst();
        this->gatesLoaded=false;
    }
    if(!activated && initialized)
    {
        //qWarning() << "Doing a synch on a non activated and initialized boat";
        updating=false;
        emit hasFinishedUpdating();
        return;
    }

    if(hasInet())
    {
        if(hasRequest() )
        {
            qWarning() << "request already running for " << pseudo;
            return;
        }
        else
        {
            //qWarning() << "Doing request " << requestCmd << " for " << pseudo;
        }

        QString page;

        switch(requestCmd)
        {
            case VLM_REQUEST_BOAT:
                QTextStream(&page) << "/ws/boatinfo.php?forcefmt=json"
                            << "&select_idu="<<this->boat_id;
                //qWarning()<<"requesting VLM_REQUEST_BOAT"<<pseudo;
                break;
            case VLM_REQUEST_TRJ:
            {
                //trace_drawing->deleteAll();
                if(race_id==0)
                {
                    //qWarning() << "boat Acc no request TRJ for:" << pseudo << " id=" << boat_id;
                    emit hasFinishedUpdating();
                    return;
                }
                time_t et=QDateTime::currentDateTime().toUTC().toTime_t();
                time_t st=et-(Settings::getSetting("trace_length",12).toInt()*60*60);
                clearCurrentRequest();
                //qWarning()<<this->name<<"normal st"<<QDateTime::fromTime_t(st).toUTC();
                if(!trace_drawing->getPoints()->isEmpty())
                {
                    for(int i=trace_drawing->getPoints()->count()-1;i>=0;--i)
                    {
                        if(trace_drawing->getPoints()->at(i).timeStamp<st)
                            trace_drawing->getPoints()->removeAt(i);
                    }
                    if(!trace_drawing->getPoints()->isEmpty())
                        st=trace_drawing->getPoints()->last().timeStamp;
                }
                //qWarning()<<"after st"<<QDateTime::fromTime_t(st).toUTC();
                QTextStream(&page)
                        << "/ws/boatinfo/tracks_private.php?"
                        << "idu="
                        << boat_id
                        << "&starttime="
                        << st;
                //qWarning()<<"requesting VLM_REQUEST_TRJ"<<pseudo;
                break;
            }
            case VLM_REQUEST_GATE:
                QTextStream(&page) << "/ws/raceinfo.php?idrace="<<race_id;
                //qWarning()<<"requesting VLM_REQUEST_GATE";
                break;
            case VLM_REQUEST_POLAR:
                QTextStream(&page) << "/Polaires/"+this->polarVlm+".csv";
                //qWarning()<<"requesting VLM_REQUEST_POLAR"<<pseudo;
                break;
            default:
                qWarning() << "[boatVLM-doRequest] error: unknown request: " << requestCmd;
                break;
        }
        inetGet(requestCmd,page,true);
    }
    else
    {
         qWarning("no Inet => Creating dummy data");
         boat_id = 0;
         pseudo = "test";
         race_id = 0;
         lat = 0;
         lon = 0;
         vacLen=300;
         race_name = "Test race";
         updating=false;
         updateBoatData();
         emit hasFinishedUpdating();
    }
}

#define printData(JSON,DATA) qWarning() << DATA << JSON[DATA].toString();

void boatVLM::requestFinished (QByteArray res_byte)
{
    double latitude=0,longitude=0;

    //QString res(res_byte);

    //qWarning() << "Request finished: " << getCurrentRequest() << " boat: " << this->boat_id<<this->pseudo<<name;

    switch(getCurrentRequest())
    {
        case VLM_REQUEST_WS:
            break;

        case VLM_REQUEST_BOAT:
        {
            QJson::Parser parser;
            bool ok;

            QVariantMap result = parser.parse (res_byte, &ok).toMap();
            if (!ok) {
                qWarning() << "Error parsing json data " << res_byte;
                qWarning() << "Error: " << parser.errorString() << " (line: " << parser.errorLine() << ")";
            }

            if(1 /*checkWSResult(res_byte,"BoatVLM_boatf",mainWindow)*/) // pas de wsCheck car VLM ne renvoit pas "success"
            {
                hasPilototo=false;
                newRace=false;
                vacLen=300;
                pilototo.clear();
                for(int i=0;i<5;++i)
                    pilototo.append("none");


                if(race_id != result["RAC"].toInt())
                    newRace=true;
                if(result["error"].toMap()["code"].toString()=="XXXXXXX" || boat_id!=result["IDU"].toInt()) /* cas de la radiation du boatsit*/
                {
                    /* clearing trace */
                    trace_drawing->deleteAll();
                    /*updating everything*/
                    updateBoatData();
                    updating=false;
                    QMessageBox::warning(0,QObject::tr("Bateau au ponton"),
                                         QObject::tr("Le bateau ")+"'"+pseudo+"' "+QObject::tr("a ete desactive car vous n'etes plus boatsitter"));
                    setStatus(false);
                    while(!gates.isEmpty())
                       delete gates.takeFirst();
                    this->gatesLoaded=false;
                    emit boatUpdated(this,newRace,doingSync);
                    emit hasFinishedUpdating();
                    return;
                }

                boat_id     = result["IDU"].toInt();
                race_id     = result["RAC"].toInt();
                name        = result["IDB"].toString();
                own         = result["OWN"].toString();
                latitude    = result["LAT"].toDouble();
                longitude   = result["LON"].toDouble();
                race_name   = result["RAN"].toString();
                speed       = result["BSP"].toDouble();
                heading     = result["HDG"].toDouble();
                avg         = result["AVG"].toDouble();
                dnm         = result["DNM"].toDouble();
                loch        = result["LOC"].toDouble();
                loch        = qRound(loch*100)/100.00;
                ortho       = result["ORT"].toDouble();
                loxo        = result["LOX"].toDouble();
                vmg         = result["VMG"].toDouble();
                windDir     = result["TWD"].toDouble();
                country     = result["CNT"].toString();
                windSpeed   = result["TWS"].toDouble();
                WP.setY(      result["WPLAT"].toDouble());
                WP.setX(      result["WPLON"].toDouble());
                WPHd        = result["H@WP"].toDouble();
//                    QString debug;
//                    debug=debug.sprintf("receiving WPLon %.10f WPLat %.10f @WP %.10f",WPLon,WPLat,WPHd);
//                    qWarning()<<debug;
                pilotType   = result["PIM"].toInt();
                pilotString = result["PIP"].toString();
                TWA         = result["TWA"].toDouble();
                ETA         = result["ETA"].toString();
                score       = result["POS"].toString();
                prevVac     = result["LUP"].toUInt();
                nextVac     = result["NUP"].toUInt();
                nWP         = result["NWP"].toInt();
                rank        = result["RNK"].toInt();
                pilototo[0] = result["PIL1"].toString();
                pilototo[1] = result["PIL2"].toString();
                pilototo[2] = result["PIL3"].toString();
                pilototo[3] = result["PIL4"].toString();
                pilototo[4] = result["PIL5"].toString();
//                    qWarning()<<"pil0="<<pilototo[0];
//                    qWarning()<<"pil1="<<pilototo[1];
//                    qWarning()<<"pil2="<<pilototo[2];
//                    qWarning()<<"pil3="<<pilototo[3];
//                    qWarning()<<"pil4="<<pilototo[4];
                stopAndGo   = result["S&G"].toString();
                polarVlm = result["POL"].toString();
                email = result["EML"].toString();
                vacLen = result["VAC"].toInt();
                if(vacLen==0)
                    vacLen=1;
                if(npd!=result["NPD"].toString())
                {
                    //qWarning()<<"old npd"<<npd;
                    npd=result["NPD"].toString();
                    //qWarning()<<"new npd"<<npd;
                    showNpd=true;
                }
                //else
                    //showNpd=false;

                lat = latitude/1000;
                lon = longitude/1000;
                if(activated && !this->forcePolar && !polarVlm.isEmpty())
                {
                    if(!this->polarData || this->polarData->getName()!=polarName)
                    {
                        QFile file;
                        file.setFileName(appFolder.value("polar")+polarVlm+".csv");
                        if(!file.exists())
                        {
                            file.setFileName(appFolder.value("polar")+polarVlm+".pol");
                        }
                        if(!file.exists())
                        {
                            connect(this->getInet(),SIGNAL(errorDuringGet()),this,SLOT(slot_errorDuringGet()));
                            doRequest(VLM_REQUEST_POLAR);
                            break;
                        }
                    }
                }
                endOfUpdating();
            }
            else /*this is dead code, never occurs*/
            {
                /* clearing trace */
                trace_drawing->deleteAll();
                updateBoatData();
                updating=false;
                setStatus(false);
                emit boatUpdated(this,newRace,doingSync);
                emit hasFinishedUpdating();
            }
            rotatesBoatInfoLog(result);
            break;
        }
        case VLM_REQUEST_POLAR:
        {
            disconnect(this->getInet(),SIGNAL(errorDuringGet()),this,SLOT(slot_errorDuringGet()));
            if(res_byte.size()==0)
            {
            }
            else
            {
                QFile filePolar(appFolder.value("polar")+polarVlm+".csv");
                if(!filePolar.exists() && filePolar.open(QIODevice::WriteOnly))
                {
                    filePolar.write(res_byte);
                    filePolar.close();
                }
            }
            endOfUpdating();
            break;
        }
        case VLM_REQUEST_TRJ:
        {
            emit getTrace(res_byte,trace_drawing->getPoints());
            //qWarning()<<"TRJ trace size="<<trace_drawing->getPoints()->count();
            if(!trace_drawing->getPoints()->isEmpty() &&
              (qRound(trace_drawing->getPoints()->last().lon*1000)!=qRound(this->lon*1000) ||
               qRound(trace_drawing->getPoints()->last().lat*1000)!=qRound(this->lat*1000)))
            {
                //qWarning()<<"missing last point in trace???"<<trace_drawing->getPoints()->last().lon<<this->lon<<trace_drawing->getPoints()->last().lat<<this->lat<<QDateTime::fromTime_t(trace_drawing->getPoints()->last().timeStamp).toUTC();
                trace_drawing->getPoints()->append(vlmPoint(lon,lat));
            }
#if 0
            double estimatedSpeed=0;
            QList<vlmPoint> * t=this->trace_drawing->getPoints();
            if(t->count()>=2)
            {
                if(t->at(t->count()-1).timeStamp!=0 &&
                   t->at(t->count()-2).timeStamp!=0)
                {
                    Orthodromie oo(t->at(t->count()-1).lon,
                                   t->at(t->count()-1).lat,
                                   t->at(t->count()-2).lon,
                                   t->at(t->count()-2).lat);
                    estimatedSpeed=oo.getDistance()/
                            ((t->at(t->count()-1).timeStamp-t->at(t->count()-2).timeStamp)/3600.0);
                }
            }
            qWarning()<<"Estimated speed:"<<QString().sprintf("%.2f",estimatedSpeed)<<"kts";
#endif
            trace_drawing->slot_showMe();
            /* we can now update everything */
            updateBoatData();
            updateTraceColor();

            if(race_id!=0 && !gatesLoaded)
            {
                updating=false;
                drawEstime();
                updating=true;
                doRequest(VLM_REQUEST_GATE);
            }
            else
            {
                updating=false;
                this->getDistHdgGate();
                drawEstime();
                emit hasFinishedUpdating();
                emit boatUpdated(this,newRace,doingSync);
            }
            showNextGates();
            break;
        }
        case VLM_REQUEST_GATE:
            {
                QJson::Parser parser;
                bool ok;

                QVariantMap result = parser.parse (res_byte, &ok).toMap();
                if (!ok) {
                    qWarning() << "Error parsing json data " << res_byte;
                    qWarning() << "Error: " << parser.errorString() << " (line: " << parser.errorLine() << ")";
                }

                //qWarning() << "id Race: " << result["idraces"].toString();
                //qWarning() << "race name: " << result["racename"].toString();

                QVariantMap wps= result["races_waypoints"].toMap();
                for(int i=1;true;++i)
                {
                    QString str;
                    QVariantMap wp= wps[str.setNum(i)].toMap();
                    if(wp.isEmpty()) break;
                    //qWarning()<<"wp: "<<wp["libelle"].toString();
                    double lonPorte1=wp["longitude1"].toDouble()/1000.000;
                    double latPorte1=wp["latitude1"].toDouble()/1000.000;
                    double lonPorte2=wp["longitude2"].toDouble()/1000.000;
                    double latPorte2=wp["latitude2"].toDouble()/1000.000;
                    int wpformat=wp["wpformat"].toInt();
                    bool oneBuoy=false;
                    bool iceGateN=false;
                    bool iceGateS=false;
                    bool clockWise=false;
                    bool antiClockWise=false;
                    bool crossOnce=false;
                    if(wpformat>=1024)
                    {
                        crossOnce=true;
                        wpformat=wpformat-1024;
                    }
                    if(wpformat>=512)
                    {
                        antiClockWise=true;
                        wpformat=wpformat-512;
                    }
                    if(wpformat>=256)
                    {
                        clockWise=true;
                        wpformat=wpformat-256;
                    }
                    if(wpformat>=32)
                    {
                        iceGateS=true;
                        wpformat=wpformat-32;
                    }
                    if(wpformat>=16)
                    {
                        iceGateN=true;
                        wpformat=wpformat-16;
                    }
                    if(wpformat>=1)
                        oneBuoy=true;


                    vlmLine * porte=new vlmLine(proj,parent->getScene(),Z_VALUE_GATE);
                    connect(parent, SIGNAL(shLab(bool)),porte,SLOT(slot_shLab(bool)));
                    porte->setGateMode("WP"+str+": "+wp["libelle"].toString());
                    if(iceGateN)
                        porte->setIceGate(1);
                    else if(iceGateS)
                        porte->setIceGate(2);
                    else
                        porte->setIceGate(0);
                    porte->addPoint(latPorte1,lonPorte1);
                    if((qRound(lonPorte1*1000000)==qRound(lonPorte2*1000000) && qRound(latPorte1*1000000)==qRound(latPorte2*1000000))||oneBuoy)
                    {
                        porte->setPorteOnePoint();
                        Util::getCoordFromDistanceAngle(latPorte1, lonPorte1, 500,wp["laisser_au"].toDouble()+180,&latPorte2,&lonPorte2);
                        oneBuoy=true;
                    }
                    porte->addPoint(latPorte2,lonPorte2);
                    QPen penLine(Qt::black,1);
                    penLine.setWidthF(3);
                    porte->setLinePen(penLine);
                    porte->setHidden(true);
                    QString tip;
                    if(iceGateN)
                        tip=tr("Passer au moins une fois au Sud de cette ligne<br>")+
                            tr("Vous n'etes pas oblige de couper la ligne");
                    else if(iceGateS)
                        tip=tr("Passer au moins une fois au Nord de cette ligne<br>")+
                            tr("Vous n'etes pas oblige de couper la ligne");
                    else
                    {
                        if(((QVariantMap)wps[str.setNum(i+1)].toMap()).isEmpty())
                            tip=tr("Arrivee<br>");
                        if(oneBuoy)
                        {
                            QString a;
                            if(qRound(wp["laisser_au"].toDouble())==wp["laisser_au"].toDouble())
                                tip=tip+tr("Une seule bouee a laisser au ")+a.sprintf("%.0f",wp["laisser_au"].toDouble());
                            else
                                tip=tip+tr("Une seule bouee a laisser au ")+a.sprintf("%.2f",wp["laisser_au"].toDouble());
                        }
                        else
                            tip=tip+tr("Passage a deux bouees");
                        if(clockWise)
                            tip=tip+tr("<br>A passer dans le sens des aiguilles d'une montre");
                        else if (antiClockWise)
                            tip=tip+tr("<br>A passer dans le sens contraire des aiguilles d'une montre");
                        else
                            tip=tip+tr("<br>Sens du passage libre");
                        if(crossOnce)
                            tip=tip+tr("<br>Il est interdit de couper cette ligne plusieurs fois");
                        else
                            tip=tip+tr("<br>Il est autorise de couper cette ligne plusieurs fois");
                    }
                    porte->setTip(tip);
                    gates.append(porte);

                }
                double ngate=gates.count();
                foreach(vlmLine * gate,gates)
                {
                    gate->set_zValue((double)Z_VALUE_GATE+ngate/100.0);
                    --ngate;
                }

                gatesLoaded=true;
                this->getDistHdgGate();
                updating=false;
                emit hasFinishedUpdating();
                emit boatUpdated(this,newRace,doingSync);
                showNextGates();
            }
            break;
        case VLM_REQUEST_SENDPILOT:
            /* pilot data send => updating boat*/
            qWarning() << "Pilot data send, result= " << res_byte;
            slot_getDataTrue();
            break;
        default:
            qWarning() << "[boatVLM-requestFinished] error: unknown request: " << getCurrentRequest();
            break;
    }
#if 0 //unflag to check b-vmg
    double res_h,res_wangle;
    if(this->polarData && lon != 0 && lat != 0 && WPLon != 0 && WPLat != 0)
    {
        this->polarData->bvmg(this->lon,this->lat,this->WPLon,this->WPLat,this->windDir,this->windSpeed,0,&res_h,&res_wangle);
        qWarning()<<"Cap BVMG: "<<res_h<<" TWA BVMG: "<<res_wangle;
    }
#endif
}
void boatVLM::slot_errorDuringGet()
{
    disconnect(this->getInet(),SIGNAL(errorDuringGet()),this,SLOT(slot_errorDuringGet()));
    QMessageBox::warning(0,QObject::tr("Telechargement de la polaire"),
                         QObject::tr("La polaire ")+polarVlm+tr(" est introuvable sur le site VLM"));
    endOfUpdating();
}

void boatVLM::endOfUpdating()
{
    initialized=true;
    if(!activated)
    {
        updating=false;
        emit hasFinishedUpdating();
        return;
    }
    hasPilototo=true;
    if(newRace)
    {
        while(!gates.isEmpty())
            delete gates.takeFirst();
        this->gatesLoaded=false;
    }
    /* request trace points */
    if(race_id==0)
    {
        /* clearing trace */
        trace_drawing->deleteAll();
        /*updating everything*/
        updateBoatData();
        updating=false;
        QMessageBox::warning(0,QObject::tr("Bateau au ponton"),
                             QObject::tr("Le bateau ")+"'"+pseudo+"' "+QObject::tr("a ete desactive car hors course"));
        setStatus(false);
        while(!gates.isEmpty())
           delete gates.takeFirst();
        this->gatesLoaded=false;
        emit boatUpdated(this,newRace,doingSync);
        emit hasFinishedUpdating();
    }
    else
    {
        doRequest(VLM_REQUEST_TRJ);
    }
}

void boatVLM::authFailed(void)
{
    QMessageBox::warning(0,QObject::tr("Parametre bateau"),
                  QString("Erreur de parametrage du bateau '")+
                  pseudo+"'.\n Verifier le login et mot de passe puis reactivez le bateau");
    setStatus(false);
    emit boatUpdated(this,false,doingSync);
    inetClient::authFailed();
}

void boatVLM::inetError()
{
    updating=false;
    emit hasFinishedUpdating();
    //emit boatUpdated(this,newRace,doingSync);
}

/**************************/
/* Select / unselect      */
/**************************/

void boatVLM::slot_selectBoat(void)
{
    boat::slot_selectBoat();
    showNextGates();
}


void boatVLM::unSelectBoat(bool needUpdate)
{
    boat::unSelectBoat(needUpdate);
    showNextGates();
}

/*************************************/
/* Drawing                           */

void boatVLM::showNextGates()
{
    if(!gatesLoaded) return;
    vlmLine * porte;
    QListIterator<vlmLine*> i (gates);
    int j=0;
    while(i.hasNext())
    {
        porte=i.next();
        if(!selected || porteHidden)
        {
            porte->setHidden(true);
            porte->hide();
            continue;
        }
        porte->show();
        ++j;
        if(j<nWP)
        {
            porte->setHidden(true);
            porte->slot_showMe();
        }
        else if (j==nWP)
        {
            porte->setZValue(Z_VALUE_NEXT_GATE);
            QPen penLine(Settings::getSetting("nextGateLineColor", QColor(Qt::blue)).value<QColor>(),1);
            penLine.setWidthF(Settings::getSetting("nextGateLineWidth", 3.0).toDouble());
            porte->setLinePen(penLine);
            porte->setHidden(false);
            porte->slot_showMe();
        }
        else
        {
            porte->setZValue(Z_VALUE_GATE);
            QPen penLine(Settings::getSetting("gateLineColor", QColor(Qt::magenta)).value<QColor>(),1);
            penLine.setWidthF(Settings::getSetting("gateLineWidth", 3.0).toDouble());
            porte->setLinePen(penLine);
            porte->setHidden(false);
            porte->slot_showMe();
        }
    }

}

/**************************/
/* Data update            */
/**************************/

void boatVLM::updateBoatString()
{
    if(getAliasState() && !alias.isEmpty())
        my_str=alias;
    else
    {
        switch(Settings::getSetting("opp_labelType",0).toInt())
        {
            case SHOW_PSEUDO:
                my_str=pseudo;
                break;
            case SHOW_NAME:
                my_str=name;
                break;
            case SHOW_IDU:
                my_str=this->getBoatId();
                break;
        }
    }

     /* computing widget size */
    QFont myFont=QApplication::font();
    QFontMetrics fm(myFont);
    prepareGeometryChange();
    width = fm.width("_"+my_str+"_")+2;
    height = qMax(fm.height()+2,12);
}

void boatVLM::updateHint(void)
{
    if(!initialized) return;
    QString str;
    /* adding score */
    str = getScore() + " - Spd: " + QString().setNum(getSpeed()) + " - ";
    switch(getPilotType())
    {
        case 1: /*heading*/
            str += tr("Hdg") + ": " + getPilotString() + tr("deg");
            break;
        case 2: /*constant angle*/
            str += tr("Angle") + ": " + getPilotString()+ tr("deg");
            break;
        case 3: /*pilotortho*/
            str += tr("Ortho" ) + "-> " + getPilotString();
            break;
        case 4: /*VMG*/
            str += tr("BVMG") + "-> " + getPilotString();
            break;
       case 5: /*VBVMG*/
            str += tr("VBVMG") /*+ "-> " + getPilotString()*/;
            break;
    }
    QString desc;
    if(!polarData) desc=tr(" (pas de polaire chargee)");
    else if (polarData->getIsCsv()) desc=polarData->getName() + tr(" (format CSV)");
    else desc=polarData->getName() + tr(" (format POL)");
    if(stopAndGo!="0")
    {
        int secs=stopAndGo.toInt()-QDateTime().currentDateTimeUtc().toTime_t();
        desc=desc+"<br>"+tr("Bateau echoue, pour encore ")+QString().setNum(secs)+" "+tr("secondes");
    }
    double windSpeed,windAngle;
    DataManager * dataManager=parent->get_dataManager();
    if(dataManager && dataManager->isOk() &&
       dataManager->getInterpolatedWind(this->lon,this->lat,
                           this->getPrevVac(),&windSpeed,&windAngle,INTERPOLATION_DEFAULT))
    {
        windAngle=radToDeg(windAngle);
        desc=desc+"<br><b>"+tr("Donnees GRIB a la derniere vac:")+"</b><br>"+
             QString().sprintf("TWS: %.2f TWD: %.2f",windSpeed,windAngle)+tr("deg");
        double twa=this->heading-windAngle;
        if(qAbs(twa)>180)
        {
            if(twa<0)
                twa=360+twa;
            else
                twa=twa-360;
        }
        if(this->getPolarData())
        {
            double bs=this->getPolarData()->getSpeed(windSpeed,twa);
            desc=desc+"<br>"+tr("BS polaire: ")+QString().sprintf("%.2f",bs)+tr(" nds");
        }
        double previousTWA=twa;
        double previousTWS=windSpeed;
        dataManager->getInterpolatedWind(this->lon,this->lat,
                            this->getPrevVac()+this->getVacLen(),&windSpeed,&windAngle,INTERPOLATION_DEFAULT);
        windAngle=radToDeg(windAngle);
        desc=desc+"<br><b>"+tr("Donnees GRIB a la prochaine vac:")+"</b><br>"+
             QString().sprintf("TWS: %.2f TWD: %.2f",windSpeed,windAngle)+tr("deg");
        if(this->getPolarData())
        {
            double twa=this->heading-windAngle;
            if(qAbs(twa)>180)
            {
                if(twa<0)
                    twa=360+twa;
                else
                    twa=twa-360;
            }
            double bs=this->getPolarData()->getSpeed(windSpeed,twa);
            desc=desc+"<br>"+tr("BS polaire: ")+QString().sprintf("%.2f",bs)+tr(" nds");
            double bVmgUp=this->getPolarData()->getBvmgUp(windSpeed);
            double bVmgDown=this->getPolarData()->getBvmgDown(windSpeed);
            desc=desc+"<br>"+tr("BVMG vent (pres): ")+QString().sprintf("%.2f",bVmgUp)+tr("deg");
            desc=desc+"<br>"+tr("BVMG vent (portant): ")+QString().sprintf("%.2f",bVmgDown)+tr("deg");
            if(windEstimeSpeed!=-1)
            {
                desc=desc+"<br><b>"+tr("Tendance a l'estime: ")+"</b>";
                previousTWS=qRound(previousTWS*100);
                windSpeed=qRound(windEstimeSpeed*100);
                previousTWA=qAbs(qRound(previousTWA*100));
                twa=this->heading-this->windEstimeDir;
                twa=qAbs(qRound(twa*100));
                //qWarning()<<"previous:"<<previousTWS<<previousTWA<<"estime:"<<windSpeed<<twa;
                if(previousTWS==windSpeed && previousTWA==twa)
                    desc=desc+tr("stable.");
                else if(previousTWS==windSpeed)
                    desc=desc+tr("force stable");
                else if (previousTWS>windSpeed)
                    desc=desc+tr("mollissant");
                else
                    desc=desc+tr("forcissant");
                if(!(previousTWS==windSpeed && previousTWA==twa))
                {
                    if(twa==previousTWA)
                        desc=desc+tr(", direction stable");
                    else
                    {
                       if(previousTWA>twa)
                            desc=desc+tr(" en refusant");
                       else
                            desc=desc+tr(" en adonnant");
                    }
                }
            }
        }

    }
    if(gatesLoaded && qRound(closest.distArrival*100.0)!=0)
    {
        QString str2=tr("Prochaine porte: ")+QString().sprintf("%.2f",closest.capArrival)+tr("deg")+"/"+
                QString().sprintf("%.2f NM<br>",closest.distArrival);
        double vvmg=this->speed*(cos(degToRad(Util::myDiffAngle(heading,closest.capArrival))));
        str2+=QString().sprintf("VMG: %.2f ",vvmg)+tr("kts")+"<br>";
        str=str2+str;
    }
    str=str.replace(" ","&nbsp;");
    desc=desc.replace(" ","&nbsp;");
    setToolTip(desc+"<br>"+str);
}

void boatVLM::setStatus(bool activated)
{
    boat::setStatus(activated);
    if(!activated)
    {
        if(gatesLoaded)
        {
            while(!gates.isEmpty())
                delete gates.takeFirst();
        }
        gatesLoaded=false;
    }
}

void boatVLM::reloadPolar(bool forced)
{
    if(forcePolar)
        polarName=polarForcedName;
    else
        polarName=polarVlm;
    boat::reloadPolar(forced);
}

void boatVLM::setPolar(bool state,QString polar)
{
    this->polarForcedName=polar;
    forcePolar=state;
    if(activated)
        reloadPolar();
    else if(polarData)
    {
        emit releasePolar(polarData->getName());
        polarData=NULL;
    }

}

void boatVLM::setAlias(bool state,QString alias)
{
    useAlias=state;
    this->alias=alias;
}

QString boatVLM::getDispName(void)
{
    if(getAliasState() && !alias.isEmpty())
        return alias + " (" + pseudo + " - " + getBoatId() + ")";
    else
    {
        switch(Settings::getSetting("opp_labelType",0).toInt())
        {
            case SHOW_PSEUDO:
                return pseudo + " (" + name + " - " + getBoatId() +")";
                break;
            case SHOW_NAME:
                return name + " (" + pseudo + " - " + getBoatId() +")";
                break;
            case SHOW_IDU:
                return getBoatId() + " (" + pseudo + " - " + name +")";
                break;
        }
    }
    return "";
}

void boatVLM::rotatesBoatInfoLog(QVariantMap lastBoatInfo)
{
    if(!boatInfoLog.isEmpty()) {
        boatInfoLog.removeFirst();
        boatInfoLog.append(lastBoatInfo);
    }

}

void boatVLM::saveBoatInfolog()
{
    player->setBoatLog(boat_id,boatInfoLog);
}

void boatVLM::loadBoatInfolog()
{
    boatInfoLog.clear();
    boatInfoLog = player->getBoatLog(boat_id);
    QVariantMap emptyMap;
    //Adjusts the size if needed
    while (boatInfoLog.count()>logIndexLimit && !boatInfoLog.isEmpty()) {
        boatInfoLog.removeFirst();
    }
    //completes with empty maps if necessary
    while (boatInfoLog.count()<logIndexLimit) {
        boatInfoLog.prepend(emptyMap);
    }
}

void boatVLM::exportBoatInfoLog(QString fileName)
{
    QFileInfo info(fileName);
    QString textFileName = fileName.remove(info.suffix());
    QString summaryFileName = textFileName;
    textFileName.append("FullLog.txt");
    summaryFileName.append("TableLog.txt");
    QFile textFile(textFileName),summaryFile(summaryFileName);
    QTextStream stream(&textFile),tableStream(&summaryFile);
    QStringList tableKeys;
    tableKeys<<"NOW"<<"LUP"<<"IDU"<<"LAT"<<"LON"<<"PIM"<<"PIP"<<"HDG"<<"TWA"<<"WPLAT"<<"WPLON"<<"TWS"<<"TWD"<<"PIL1";
    tableStream<<"Time\t"<<"LastUp\t"<<"boatId\t"<<"Lat\t"<<"Lon\t"<<"PIM\t"<<"PIP\t"<<"Cap\t"<<"Angle\t"<<"WPLat\t"<<"WPLon\t"<<"WindSpeed\t"<<"WindDir\t"<<"PIL1";
    tableStream<<"\n";
    if(!textFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(0,QObject::tr("Exports VLM Syncs"),
             QString(QObject::tr("Impossible de creer le fichier %1")).arg(fileName));
        return;
    }
    if(!summaryFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(0,QObject::tr("Exports VLM Syncs Summary"),
             QString(QObject::tr("Impossible de creer le fichier %1")).arg(fileName));
        return;
    }
    QVariantMap boatInfoRecord;
    QList<QString> recordKeys;
    QStringList textOutput;
    QString key,gribFileName="";
    DataManager * dataManager=parent->get_dataManager();
    if(dataManager)
        gribFileName=dataManager->get_fileName(DataManager::GRIB_GRIB);
    if (!boatInfoLog.isEmpty()) {
        int logIndex;
        for( logIndex=0;logIndex<boatInfoLog.count();++logIndex) {
            boatInfoRecord=boatInfoLog[logIndex];
            recordKeys=boatInfoRecord.keys();
            if (!boatInfoLog[logIndex].isEmpty()) {
                int keysIndex;
                for(keysIndex=0;keysIndex<recordKeys.count();++keysIndex) {
                    textOutput=QStringList();
                    key=recordKeys[keysIndex];
                    textOutput.append(key);
                    textOutput.append(boatInfoRecord[key].toString());
                    stream<<textOutput.join("\t");
                    stream<<"\n";
                }
                stream<<"\n\n";
                QDateTime time;
                QString timeString;
                int tableIndex;
                for (tableIndex=0 ; tableIndex<2; ++tableIndex) {
                    time.setTime_t(boatInfoRecord[tableKeys[tableIndex]].toUInt());
                    timeString=time.toUTC().toString("yyyy/MM/dd hh:mm:ss");
                    tableStream <<  timeString <<"\t";
                }
                for (tableIndex=2 ; tableIndex<tableKeys.size(); ++tableIndex) {
                    tableStream <<  boatInfoRecord[tableKeys[tableIndex]].toString() <<"\t";
                }
                tableStream<<"\n";
            }
        }
    }
    stream<<"grib\t"<<gribFileName<<"\n";
    tableStream<<"\n"<<"grib\t"<<gribFileName<<"\n";
    textFile.close();
    summaryFile.close();
}
void boatVLM::getDistHdgGate()
{
    if(!gatesLoaded || gates.isEmpty() || nWP<=0 || nWP>gates.count())
    {
        closest=vlmPoint(0,0);
        return;
    }
#if 1
    vlmLine *porte=NULL;
    for (int i=nWP-1;i<gates.count();++i)
    {
        porte=gates.at(i);
        if (!porte->isIceGate()) break;
    }
#else
    vlmLine *porte=gates.at(nWP-1);
#endif
    double x,y;
    Util::distance_to_line_dichotomy_xing(lat,lon,
                                          porte->getPoints()->first().lat,porte->getPoints()->first().lon,
                                          porte->getPoints()->last().lat,porte->getPoints()->last().lon,
                                          &y,&x);
    closest=vlmPoint(x,y);
    Orthodromie oo(lon,lat,x,y);
    closest.distArrival=oo.getDistance();
    closest.capArrival=oo.getAzimutDeg();
    return;
}
