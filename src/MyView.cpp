#include "MyView.h"
#include <QDebug>
#include <QScrollBar>
#include <QGestureEvent>
#include "settings.h"
#include "StatusBar.h"

MyView::MyView(Projection *proj, myScene *scene, myCentralWidget * mcw) :
    QGraphicsView(scene,mcw)
{
    this->scene=scene;
    this->mcw=mcw;
    this->proj=proj;
    this->setTransformationAnchor(QGraphicsView::NoAnchor);
    this->setRenderHints(QPainter::Antialiasing |
                         QPainter::SmoothPixmapTransform |
                         QPainter::HighQualityAntialiasing);
    viewPix=new QGraphicsPixmapItem();
    viewPix->setZValue(100);
    viewPix->setPos(0,0);
    viewPix->setTransformationMode(Qt::SmoothTransformation);
    paning=false;
    pinching=false;
    px=0;
    py=0;
    backPix=new QGraphicsPixmapItem();
    backPix->setPos(0,0);
    backPix->setZValue(90);
    viewPix->setActive(false);
    backPix->setActive(false);
}
void MyView::startPaning(const QGraphicsSceneMouseEvent *e)
{
    if(pinching) return;
    if(proj->getFrozen()) return;
    qWarning()<<"inside start paning";
    px=e->scenePos().x();
    py=e->scenePos().y();
    QPixmap pix(proj->getW(),proj->getH());
    pix.fill(Qt::blue);
    backPix->setPixmap(pix);
    backPix->setPos(0,0);
    QPainter pnt(&pix);
    pnt.setRenderHints(QPainter::Antialiasing |
                         QPainter::SmoothPixmapTransform |
                         QPainter::HighQualityAntialiasing);
    this->render(&pnt);
    pnt.end();
    viewPix->setPixmap(pix);
    viewPix->setPos(0,0);
    if(!paning && !pinching)
    {
        scene->addItem(backPix);
        scene->addItem(viewPix);
    }
    paning=true;
}
void MyView::pane(const int &x, const int &y)
{
    viewPix->setPos(x-px,y-py);
}
void MyView::stopPaning(const int &x, const int &y)
{
    if(pinching)
    {
        paning=false;
        return;
    }
    if(px-x!=0 && py-y!=0)
    {
        QRectF r(QPointF(0,0),QSize(proj->getW(),proj->getH()));
        r.translate(px-x,py-y);
        proj->setCentralPixel(r.center());
    }
    else
    {
        qWarning()<<"anomaly 1 in myView";
        hideViewPix();
    }
}
void MyView::hideViewPix()
{
    qWarning()<<"inside hideViewPix";
    if(paning || pinching)
    {
        scene->removeItem(backPix);
        scene->removeItem(viewPix);
    }
    viewPix->setScale(1);
    paning=false;
    pinching=false;
}

void MyView::myScale(const double &scale, const double &lon, const double &lat, const bool &screenCoord)
{
    if(paning) return;
    if(!pinching)
    {
        pinching=true;
        QPixmap pix(proj->getW(),proj->getH());
        pix.fill(Qt::blue);
        backPix->setPixmap(pix);
        QPainter pnt(&pix);
        pnt.setRenderHints(QPainter::Antialiasing |
                             QPainter::SmoothPixmapTransform |
                             QPainter::HighQualityAntialiasing);
        this->render(&pnt);
        pnt.end();
        viewPix->resetTransform();
        viewPix->setPixmap(pix);
        viewPix->setPos(0,0);
        qWarning()<<"inside start pinching";
        scene->addItem(backPix);
        scene->addItem(viewPix);
    }
    viewPix->resetTransform();
    double X1,Y1;
    if(!screenCoord)
    {
        proj->map2screenDouble(lon,lat,&X1,&Y1);
        viewPix->setTransformOriginPoint(X1,Y1);
    }
    else
        viewPix->setTransformOriginPoint(lon,lat);
    viewPix->setScale(scale);
}
