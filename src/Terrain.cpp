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

#include <iostream>
#include <cassert>

#include <QApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

#include "Terrain.h"
#include "settings.h"
#include "GisReader.h"
#include "Projection.h"
#include "mycentralwidget.h"
#include "GshhsReader.h"
#include "Grib.h"

//---------------------------------------------------------
// Constructeur
//---------------------------------------------------------
Terrain::Terrain(myCentralWidget *parent, Projection *proj_) : QGraphicsWidget()
{
    this->parent=parent;
    proj = proj_;
    connect(proj,SIGNAL(projectionUpdated()),this,SLOT(redrawAll()));
    connect(parent,SIGNAL(redrawAll()),this,SLOT(redrawAll()));
    connect(parent,SIGNAL(redrawGrib()),this,SLOT(redrawGrib()));
    connect(this,SIGNAL(mousePress(QGraphicsSceneMouseEvent*)),parent,SLOT(slot_mousePress(QGraphicsSceneMouseEvent*)));
    connect(this,SIGNAL(mouseRelease(QGraphicsSceneMouseEvent*)),parent,SLOT(slot_mouseRelease(QGraphicsSceneMouseEvent*)));

    setZValue(Z_VALUE_TERRE);
    setData(0,TERRE_WTYPE);

    width=50;
    height=50;
    setPos(0,0);
    //qWarning() << "Terre is at " << x() << "," << y() << ", size: " << size().width() << "," << size().height();

    quality = 0;

    //---------------------------------------------------------------------
    showCountriesBorders  = Settings::getSetting("showCountriesBorders", true).toBool();

    showRivers   = Settings::getSetting("showRivers", false).toBool();
    showCitiesNamesLevel = Settings::getSetting("showCitiesNamesLevel", 0).toInt();
    showCountriesNames = Settings::getSetting("showCountriesNames", false).toBool();
    showWindColorMap  = Settings::getSetting("showWindColorMap", true).toBool();

    colorMapSmooth = Settings::getSetting("colorMapSmooth", true).toBool();
    showWindArrows  = Settings::getSetting("showWindArrows", true).toBool();
    showBarbules = Settings::getSetting("showBarbules", true).toBool();

    showIsobars  = Settings::getSetting("showIsobars", true).toBool();
    showIsobarsLabels = Settings::getSetting("showIsobarsLabels", false).toBool();
    isobarsStep = Settings::getSetting("isobarsStep", 2).toDouble();
    //setIsobarsStep(Settings::getSetting("isobarsStep", 2).toDouble());
    showPressureMinMax = Settings::getSetting("showPressureMinMax", false).toBool();

    showIsotherms0  = Settings::getSetting("showIsotherms0", false).toBool();
    showIsotherms0Labels  = Settings::getSetting("showIsotherms0Labels", false).toBool();
    isotherms0Step = Settings::getSetting("isotherms0Step", 50).toDouble();
    //setIsotherms0Step(Settings::getSetting("isotherms0Step", 50).toDouble());

    colorMapMode = Settings::getSetting("colorMapMode", Terrain::drawWind).toInt();

    showTemperatureLabels = Settings::getSetting("showTemperatureLabels", false).toBool();
    //showGribGrid = Settings::getSetting("showGribGrid", false).toBool();
    //----------------------------------------------------------------------------

    imgEarth = NULL;
    imgSea = NULL;
    imgWind  = NULL;
    imgAll   = NULL;
    isEarthMapValid = false;
    isWindMapValid  = false;
    mustRedraw = true;
    isWaiting=false;

    gshhsReader = NULL;
    gisReader = new GisReader();
    assert(gisReader);

    setPalette(QPalette(backgroundColor));

    updateGraphicsParameters();    
}

//-------------------------------------------
void Terrain::updateGraphicsParameters()
{
    backgroundColor  = Settings::getSetting("backgroundColor", QColor(0,0,45)).value<QColor>();
    seaColor  = Settings::getSetting("seaColor", QColor(50,50,150)).value<QColor>();
    landColor = Settings::getSetting("landColor", QColor(200,200,120)).value<QColor>();
    landColor.setAlpha(Settings::getSetting("landOpacity","180").toInt());
    tranparent=QColor(0,0,0,0);

    seaBordersPen.setColor(Settings::getSetting("seaBordersLineColor", QColor(40,45,30)).value<QColor>());
    seaBordersPen.setWidthF(Settings::getSetting("seaBordersLineWidth", 1.8).toDouble());

    boundariesPen.setColor(Settings::getSetting("boundariesLineColor", QColor(40,40,40)).value<QColor>());
    boundariesPen.setWidthF(Settings::getSetting("boundariesLineWidth", 1.4).toDouble());

    riversPen.setColor(Settings::getSetting("riversLineColor", QColor(50,50,150)).value<QColor>());
    riversPen.setWidthF(Settings::getSetting("riversLineWidth", 1.0).toDouble());

    isobarsPen.setColor(Settings::getSetting("isobarsLineColor", QColor(80,80,80)).value<QColor>());
    isobarsPen.setWidthF(Settings::getSetting("isobarsLineWidth", 2.0).toDouble());

    isotherms0Pen.setColor(Settings::getSetting("isotherms0LineColor", QColor(200,120,100)).value<QColor>());
    isotherms0Pen.setWidthF(Settings::getSetting("isotherms0LineWidth", 1.6).toDouble());

    int v = 180;
    selectColor     = QColor(v,v,v);


    isEarthMapValid = false;
    mustRedraw = true;
    indicateWaitingMap();
}

//---------------------------------------------------------
// Cartes GSHHS
//---------------------------------------------------------
void Terrain::setGSHHS_map(GshhsReader *map)
{
    gshhsReader = map;
    isEarthMapValid = false;
}

//-------------------------------------------------------
void Terrain::draw_GSHHSandGRIB()
{
//    if(proj->getFrozen()) //routage
//    {
//        QPainter pnt(imgAll);
//        gshhsReader->drawSeaBorders(pnt, proj);
//        return;
//    }
    QCursor oldcursor = cursor();
    setCursor(Qt::WaitCursor);
    if (imgAll != NULL) {
        delete imgAll;
        imgAll = NULL;
    }
    imgAll = new QPixmap(width,height);
    assert(imgAll);
    QPainter pnt(imgAll);
    pnt.setRenderHint(QPainter::Antialiasing, true);

    tranparent=Qt::transparent;

    //===================================================
    // Dessin du fond de carte et des donnees GRIB
    //===================================================
    if (!isEarthMapValid)
    {
        if (imgEarth != NULL) {
            delete imgEarth;
            imgEarth = NULL;
        }
        if (imgSea != NULL) {
            delete imgSea;
            imgSea = NULL;
        }

        imgEarth = new QImage(width,height,QImage::Format_ARGB32_Premultiplied);
        assert(imgEarth);
        imgEarth->fill(Qt::transparent);
        imgSea = new QImage(width,height,QImage::Format_ARGB32_Premultiplied);
        assert(imgSea);

        if (gshhsReader != NULL)
        {
            QPainter pnt0(imgSea);
            QPainter pnt1(imgEarth);
            pnt0.setRenderHint(QPainter::Antialiasing, true);
            pnt1.setRenderHint(QPainter::Antialiasing, true);
            pnt1.setCompositionMode(QPainter::CompositionMode_Source);
            gshhsReader->drawBackground(pnt0, proj, seaColor, backgroundColor);
            gshhsReader->drawContinents(pnt1, proj, tranparent, landColor);
        }
    }
    pnt.drawImage(0,0, *imgSea);

    //===================================================
    // Dessin des données GRIB
    //===================================================

    Grib * grib=parent->getGrib();

    if(grib)
        drawGrib(pnt,grib);

    //===================================================
    // Dessin des bordures et frontières
    //===================================================

    if (gshhsReader != NULL)
    {
        pnt.setPen(seaBordersPen);
        gshhsReader->drawSeaBorders(pnt, proj);

        if (showCountriesBorders) {
            pnt.setPen(boundariesPen);
            gshhsReader->drawBoundaries(pnt, proj);
        }
        if (showRivers) {
            pnt.setPen(riversPen);
            gshhsReader->drawRivers(pnt, proj);
        }
    }

    QPainter pnt1(imgEarth);
    if (showCountriesNames)
    {

        gisReader->drawCountriesNames(pnt1, proj);
    }
    if (showCitiesNamesLevel > 0) {
        gisReader->drawCitiesNames(pnt1, proj, showCitiesNamesLevel);
    }
    //===================================================


    //===================================================

    pnt.drawImage(0,0, *imgEarth);
    /*int save=0;
    if(save==1) imgEarth->save("test.jpg","JPG",100);*/
    QString cartouche="";
    if(grib) cartouche=grib->drawCartouche(pnt)+". ";
    if(this->gshhsReader)
        cartouche=cartouche+tr("Niveau de detail des cotes: ")+QString().setNum(this->gshhsReader->getQuality()+1);
    int fSize=12;
    QFont fontbig("TypeWriter", fSize, QFont::Bold, false);
    fontbig.setStyleHint(QFont::TypeWriter);
    fontbig.setStretch(QFont::Condensed);
    QColor   transpcolor(255,255,255,120);
    QColor   textcolor(20,20,20,255);
    pnt.setBrush(transpcolor);
    pnt.setFont(fontbig);
    pnt.setPen(transpcolor);
    pnt.drawRect(3,3,400,fSize+3+4);
    pnt.setPen(textcolor);

    pnt.drawText(10, fSize+6, cartouche);// forecast validity date

    setCursor(oldcursor);
}

void Terrain::drawGrib(QPainter &pnt, Grib *gribPlot)
{
        //gribPlot->show_CoverZone(pnt,proj);

        //QTime t1 = QTime::currentTime();
        //qWarning() << "Grib mode: " << colorMapMode << " (grib=" << Terrain::drawWind << ")";
        // grib->draw_WIND_Color(pnt, proj, colorMapSmooth,showWindColorMap,showWindArrows,showBarbules);
        switch (colorMapMode)
        {
                case Terrain::drawWind :
                        windArrowsColor.setRgb(255, 255, 255);                        
                        gribPlot->draw_WIND_Color(pnt, proj, colorMapSmooth,showWindArrows,showBarbules);
                        break;
                case Terrain::drawRain :
                        windArrowsColor.setRgb(140, 120, 100);
                        gribPlot->draw_RAIN_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawCloud :
                        windArrowsColor.setRgb(180, 180, 80);
                        gribPlot->draw_CLOUD_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawHumid :
                        windArrowsColor.setRgb(180, 180, 80);
                        gribPlot->draw_HUMID_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawTemp :
                        windArrowsColor.setRgb(255, 255, 255);
                        gribPlot->draw_Temp_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawTempPot :
                        windArrowsColor.setRgb(255, 255, 255);
                        gribPlot->draw_TempPot_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawDewpoint :
                        windArrowsColor.setRgb(255, 255, 255);
                        gribPlot->draw_Dewpoint_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawDeltaDewpoint :
                        windArrowsColor.setRgb(180, 180, 80);
                        gribPlot->draw_DeltaDewpoint_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawSnowDepth :
                        windArrowsColor.setRgb(140, 120, 100);
                        gribPlot->draw_SNOW_DEPTH_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawSnowCateg :
                        windArrowsColor.setRgb(140, 120, 100);
                        gribPlot->draw_SNOW_CATEG_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawFrzRainCateg :
                        windArrowsColor.setRgb(140, 120, 100);
                        gribPlot->draw_FRZRAIN_CATEG_Color(pnt, proj, colorMapSmooth);
                        break;
                case Terrain::drawCAPEsfc :
                        windArrowsColor.setRgb(100, 80, 80);
                        gribPlot->draw_CAPEsfc(pnt, proj, colorMapSmooth);
                        break;
        }
        //printf("time show ColorMap = %d ms\n", t1.elapsed());

        //send gfs:40N,60N,140W,120W|2,2|24,48,72|PRESS,WIND,SEATMP,AIRTMP,WAVES

        if (showIsobars) {
                pnt.setPen(isobarsPen);
                gribPlot->draw_Isobars(pnt, proj);
        }

        if (showIsotherms0) {
                pnt.setPen(isotherms0Pen);
                gribPlot->draw_Isotherms0(pnt, proj);
        }

        if (showIsobarsLabels) {
                gribPlot->draw_IsobarsLabels(pnt, proj);
        }
        if (showIsotherms0Labels) {
                gribPlot->draw_Isotherms0Labels(pnt, proj);
        }
        if (showPressureMinMax) {
                gribPlot->draw_PRESSURE_MinMax (pnt, proj);
        }
        if (showTemperatureLabels) {
                gribPlot->draw_TEMPERATURE_Labels (pnt, proj);
        }

        //===================================================
        // Grille GRIB
        //===================================================
        /*if (showGribGrid) {
                gribPlot->draw_GribGrid(pnt, proj);
        }*/

        gribPlot->show_CoverZone(pnt,proj);

    //remettre la grille grib
}

//=========================================================
void Terrain::setDrawRivers(bool b) {
    if (showRivers != b) {
        showRivers = b;
        Settings::setSetting("showRivers", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::setDrawCountriesBorders(bool b) {
    if (showCountriesBorders != b) {
        showCountriesBorders = b;
        Settings::setSetting("showCountriesBorders", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------

//-------------------------------------------------------
void Terrain::setCountriesNames(bool b) {
    if (showCountriesNames != b) {
        showCountriesNames = b;
        Settings::setSetting("showCountriesNames", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setCitiesNamesLevel  (int level) {
    if (showCitiesNamesLevel != level) {
        showCitiesNamesLevel = level;
        Settings::setSetting("showCitiesNamesLevel", level);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::slot_setMapQuality(int q) {
    if (quality != q) {
        if (gshhsReader == NULL)
            return;
        quality = q;        
        gshhsReader->setUserPreferredQuality(q);
        isEarthMapValid = false;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::slot_setDrawWindColors (bool b) {
    if (showWindColorMap != b) {
        showWindColorMap = b;
        Settings::setSetting("showWindColorMap", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
void Terrain::slotTemperatureLabels(bool b) {
    if (showTemperatureLabels != b) {
        showTemperatureLabels = b;
        Settings::setSetting("showTemperatureLabels", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setColorMapMode(int mode)
{    
    if (colorMapMode != mode)
    {
        Settings::setSetting("colorMapMode", mode);
        colorMapMode=mode;
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::setColorMapSmooth (bool b) {
    if (colorMapSmooth != b) {
        colorMapSmooth = b;
        Settings::setSetting("colorMapSmooth", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setDrawWindArrows (bool b) {
    if (showWindArrows != b) {
        showWindArrows = b;
        Settings::setSetting("showWindArrows", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setBarbules (bool b) {
    if (showBarbules != b) {
        showBarbules = b;
        Settings::setSetting("showBarbules", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::setPressureMinMax (bool b) {
    if (showPressureMinMax != b) {
        showPressureMinMax = b;
        Settings::setSetting("showPressureMinMax", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::setDrawIsobars(bool b) {
    if (showIsobars != b) {
        showIsobars = b;
        Settings::setSetting("showIsobars", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setIsobarsStep(double step)
{
    if (isobarsStep != step) {
        Grib * grib = parent->getGrib();
        if(grib)
            grib->setIsobarsStep(step);
        else
            qWarning() << "No grib present";
        Settings::setSetting("isobarsStep", step);
        isobarsStep = step;
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setDrawIsobarsLabels(bool b) {
    if (showIsobarsLabels != b) {
        showIsobarsLabels = b;
        Settings::setSetting("showIsobarsLabels", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

//-------------------------------------------------------
void Terrain::setDrawIsotherms0(bool b) {
    if (showIsotherms0 != b) {
        showIsotherms0 = b;
        Settings::setSetting("showIsotherms0", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setIsotherms0Step(double step)
{
    if (isotherms0Step!=step) {
        Grib * grib = this->parent->getGrib();
        if(grib)
            grib->setIsotherms0Step(step);
        Settings::setSetting("isotherms0Step", step);
        isotherms0Step = step;
        mustRedraw = true;
        indicateWaitingMap();
    }
}
//-------------------------------------------------------
void Terrain::setDrawIsotherms0Labels(bool b) {
    if (showIsotherms0Labels != b) {
        showIsotherms0Labels = b;
        Settings::setSetting("showIsotherms0Labels", b);
        mustRedraw = true;
        indicateWaitingMap();
    }
}

void Terrain::redrawAll()
{
    isEarthMapValid = false;
    isWindMapValid = false;
    indicateWaitingMap();
}

void Terrain::redrawGrib()
{
    isWindMapValid = false;
    indicateWaitingMap();
}


//---------------------------------------------------------




//---------------------------------------------------------
// Events
//---------------------------------------------------------


void Terrain::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{    
    emit showContextualMenu(event);
}

void Terrain::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if(e->button()==Qt::LeftButton || e->button()==Qt::MidButton )
    {
        emit mousePress(e);
    }
}

void Terrain::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if(e->button()==Qt::LeftButton)
    {
        emit mouseRelease(e);
    }
}

void Terrain::updateSize(int width, int height)
{
    prepareGeometryChange();
    isEarthMapValid = false;
    isWindMapValid = false;
    this->width=width;
    this->height=height;
    update();
}

QRectF Terrain::boundingRect() const
{
    return QRectF(0,0,width,height);
}

QPainterPath Terrain::shape() const
 {
     QPainterPath path;
     path.addRect(0,0,width,height);
     return path;
 }

//---------------------------------------------------------
// paintEvent
//---------------------------------------------------------
void Terrain::paint(QPainter * pnt, const QStyleOptionGraphicsItem * , QWidget * )
{
    pnt->drawPixmap(0,0, *imgAll);
}

void Terrain::indicateWaitingMap()
{
    if(isWaiting) return;
    isWaiting=true;
    if(imgAll!=NULL)
    {
        QPainter pnt_1(imgAll);
        pnt_1.setRenderHint(QPainter::Antialiasing, true);
        QFont fontWait("Helvetica", 12, QFont::Bold, true);
        QFontMetrics fmet(fontWait);
        pnt_1.setPen(QColor(Qt::white));
        int r = 80;
        QColor transp = QColor(r,r,r, 80);
        pnt_1.setFont(fontWait);
        pnt_1.setBrush(transp);
        QString txt = tr("Calculs en cours...");
        QRect rect = fmet.boundingRect(txt);
        rect.moveTo(20,20);
        pnt_1.drawRect(rect);
        pnt_1.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter , txt);
        updateRoutine();
   }
   if (mustRedraw  ||  !isEarthMapValid  || !isWindMapValid)
   {
        draw_GSHHSandGRIB();

        isEarthMapValid = true;
        isWindMapValid = true;
        mustRedraw = false;
    }
    updateRoutine();
    isWaiting=false;
}
void Terrain::updateRoutine()
{
    update();
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}
