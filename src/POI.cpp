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
#include <cassert>

#include <QTimer>
#include <QMessageBox>

#include "POI.h"
#include "Util.h"
#include "MainWindow.h"

//-------------------------------------------------------------------------------
POI::POI(QString name, float lon, float lat,
                 Projection *proj, QWidget *ownerMeteotable, QWidget *parentWindow, int type, float wph)
    : QWidget(parentWindow)
{
    this->parent = parentWindow;
    this->owner = ownerMeteotable;
    this->name = name;
    this->lon = lon;
    this->lat = lat;
    this->wph=wph;
    this->type = type;

    setProjection(proj);
    createWidget();
    connect(proj, SIGNAL(projectionUpdated(Projection * )), this, SLOT(projectionUpdated(Projection *)) );
    connect(this, SIGNAL(signalOpenMeteotablePOI(POI*)),
                            ownerMeteotable, SLOT(slotOpenMeteotablePOI(POI*)));
    connect(this,SIGNAL(chgWP(float,float,float)),ownerMeteotable,SLOT(slotChgWP(float,float,float)));

    connect(this,SIGNAL(addPOI_list(POI*)),ownerMeteotable,SLOT(addPOI_list(POI*)));
    connect(this,SIGNAL(delPOI_list(POI*)),ownerMeteotable,SLOT(delPOI_list(POI*)));

}

POI::~POI()
{
    //printf("delete POI_Editor\n");
}

//-------------------------------------------------------------------------------
void POI::createWidget()
{
    countClick = 0;

    fgcolor = QColor(0,0,0);
    int gr = 255;
    bgcolor = QColor(gr,gr,gr,150);

    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(10,0,2,0);
    layout->setSpacing(0);

    label = new QLabel(name, this);
    label->setFont(QFont("Helvetica",10));

    QPalette p;
    p.setBrush(QPalette::Active, QPalette::WindowText, fgcolor);
    p.setBrush(QPalette::Inactive, QPalette::WindowText, fgcolor);
    label->setPalette(p);
    label->setAlignment(Qt::AlignHCenter);

    layout->addWidget(label, 0,0, Qt::AlignHCenter|Qt::AlignVCenter);
    this->setLayout(layout);
    setAutoFillBackground (false);

    createPopUpMenu();
}

//-------------------------------------------------------------------------------
void POI::setName(QString name)
{
    this->name=name;
    label->setText(name);
    setToolTip(tr("Point d'intérêt : ")+name);
    adjustSize();
}

//-------------------------------------------------------------------------------
void POI::setProjection( Projection *proj)
{
    this->proj = proj;
    if (proj->isPointVisible(lon, lat)) {      // tour du monde ?
        proj->map2screen(lon, lat, &pi, &pj);
    }
    else if (proj->isPointVisible(lon-360, lat)) {
        proj->map2screen(lon-360, lat, &pi, &pj);
    }
    else {
        proj->map2screen(lon+360, lat, &pi, &pj);
    }

    int dy = height()/2;
    move(pi-3, pj-dy);
}

//-------------------------------------------------------------------------------
void POI::projectionUpdated(Projection * )
{
    setProjection(proj);
}

//-------------------------------------------------------------------------------
void  POI::paintEvent(QPaintEvent *)
{
    QPainter pnt(this);
    int dy = height()/2;

    pnt.fillRect(9,0, width()-10,height()-1, QBrush(bgcolor));

    QPen pen(Qt::black);
    pen.setWidth(4);
    pnt.setPen(pen);
    pnt.fillRect(0,dy-3,7,7, QBrush(Qt::black));

    int g = 60;
    pen = QPen(QColor(g,g,g));
    pen.setWidth(1);
    pnt.setPen(pen);
    pnt.drawRect(9,0,width()-10,height()-1);

}

//-------------------------------------------------------------------------------
void  POI::enterEvent (QEvent *)
{
    enterCursor = cursor();
    setCursor(Qt::PointingHandCursor);
    
}
//-------------------------------------------------------------------------------
void  POI::leaveEvent (QEvent *)
{
    setCursor(enterCursor);
}

//-------------------------------------------------------------------------------
void  POI::mousePressEvent(QMouseEvent *)
{
}
//-------------------------------------------------------------------------------
void  POI::mouseDoubleClickEvent(QMouseEvent *)
{
}
//-------------------------------------------------------------------------------
void  POI::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        setCursor(Qt::BusyCursor);
        if (countClick == 0) {
            QTimer::singleShot(200, this, SLOT(timerClickEvent()));
        }
        countClick ++;
        if (countClick==2)
        {
            // Double Clic : Edit this Point Of Interest
            new POI_Editor(this, owner, parent);
            countClick = 0;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // Right clic : Edit this Point Of Interest
        //new POI_Editor(this, parent);
        popup->exec(QCursor::pos());
    }
}
//-------------------------------------------------------------------------------
void  POI::timerClickEvent()
{
    if (countClick==1)
    {
        // Single clic : Meteotable for this Point Of Interest
        emit signalOpenMeteotablePOI(this);
    }
    countClick = 0;
}

void POI::doChgWP(float lat,float lon, float wph)
{
    emit chgWP(lat,lon,wph);
}

void POI::createPopUpMenu(void)
{
    popup = new QMenu(parent);

    ac_edit = new QAction("Editer",popup);
    popup->addAction(ac_edit);
    connect(ac_edit,SIGNAL(triggered()),this,SLOT(slot_editPOI()));

    ac_setWp = new QAction("POI->WP",popup);
    popup->addAction(ac_setWp);
    connect(ac_setWp,SIGNAL(triggered()),this,SLOT(slot_setWP()));
}

void POI::slot_editPOI()
{
    new POI_Editor(this, owner, parent);
}

void POI::slot_setWP()
{
    //showMessage(QString("changing WP %1,%2,%3").arg(lat).arg(lon).arg(wph));
    emit chgWP(lat,lon,wph);
}

void POI::slotDelPoi()
{

}

//=====================================================
// AngleEditor
//=====================================================
DegreeMinuteEditor::DegreeMinuteEditor(float val, QWidget *parent,
                        int degreMin, int degreMax)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;

    int   deg = (int) trunc(val);
    float min = 60.0*fabs(val-trunc(val));

    angDeg = new QSpinBox(this);
    angDeg->setMinimum(degreMin);
    angDeg->setMaximum(degreMax);
    angDeg->setValue(deg);
    angDeg->setSuffix(tr(" °"));
    layout->addWidget(angDeg);

    angMin = new QDoubleSpinBox(this);
    angMin->setMinimum(0);
    angMin->setMaximum(59.99);
    angMin->setValue(min);
    angMin->setSuffix(tr(" '"));
    layout->addWidget(angMin);

    setLayout(layout);
}
//---------------------------------------
float DegreeMinuteEditor::getValue()
{
    float deg = angDeg->value();
    float min = angMin->value()/60.0;
    if (deg < 0)
        return deg - min;
    else
        return deg + min;
}

void DegreeMinuteEditor::setValue(float val)
{
    int   deg = (int) trunc(val);
    float min = 60.0*fabs(val-trunc(val));
    angDeg->setValue(deg);
    angMin->setValue(min);
}

//=====================================================
// POI_Editor
//=====================================================

//-------------------------------------------------------
// POI_Editor: Constructor for edit and create a new POI
//-------------------------------------------------------
POI_Editor::POI_Editor(float lon, float lat,
                Projection *proj, QWidget *ownerMeteotable, QWidget *parentWindow)
    : QDialog(parentWindow)
{
    modeCreation = true;
    setWindowTitle(tr("Nouveau Point d'Intérêt"));
    this->poi = new POI(tr("POI"), lon, lat, proj, ownerMeteotable, parentWindow, POI_STD,-1);
    assert(this->poi);
    createInterface();
    setModal(false);
    connect(this,SIGNAL(addPOI_list(POI*)),ownerMeteotable,SLOT(addPOI_list(POI*)));
    connect(this,SIGNAL(delPOI_list(POI*)),ownerMeteotable,SLOT(delPOI_list(POI*)));
    show();
}
//-------------------------------------------------------
// POI_Editor: Constructor for edit an existing POI
//-------------------------------------------------------
POI_Editor::POI_Editor(POI *poi_, QWidget *ownerMeteotable, QWidget *parent)
    : QDialog(parent)
{
    this->poi = poi_;
    modeCreation = false;
    setWindowTitle(tr("Point d'Intérêt : ")+poi->getName());

    createInterface();
    setModal(false);
    connect(this,SIGNAL(addPOI_list(POI*)),ownerMeteotable,SLOT(addPOI_list(POI*)));
    connect(this,SIGNAL(delPOI_list(POI*)),ownerMeteotable,SLOT(delPOI_list(POI*)));
    show();
}
//---------------------------------------
POI_Editor::~POI_Editor()
{
    //printf("delete POI_Editor\n");
}
//---------------------------------------
void POI_Editor::reject()
{
    btCancelClicked();
}
//---------------------------------------
void POI_Editor::btOkClicked()
{
    poi->setName( (editName->text()).trimmed() );
    poi->setLongitude(editLon->getValue());
    poi->setLatitude (editLat->getValue());
    if(editWph->text().isEmpty())
        poi->setWph(-1);
    else
        poi->setWph(editWph->text().toFloat());
    poi->projectionUpdated(NULL);
    
    if (modeCreation) {
        poi->show();
        emit addPOI_list(poi);
    }
    delete this;
}
//---------------------------------------
void POI_Editor::btCancelClicked()
{
    if (modeCreation) {
        delete poi;
    }
    delete this;
}
//---------------------------------------
void POI_Editor::btDeleteClicked()
{
    if (! modeCreation) {
        int rep = QMessageBox::question (this,
            tr("Détruire le POI : %1").arg(poi->getName()),
            tr("La destruction d'un point d'intérêt est définitive.\n\nEtes-vous sûr ?"),
            QMessageBox::Yes | QMessageBox::No);
        if (rep == QMessageBox::Yes) {
            
            delPOI_list(poi);
            delete poi;
            delete this;
        }
    }
    else {
        delete poi;
        delete this;
    }
}

void POI_Editor::btPasteClicked()
{
    float lat,lon,wph;
    if(!Util::getWP(&lat,&lon,&wph))
        return;
    editLon->setValue(lon);
    editLat->setValue(lat);
    if(wph<0 || wph > 360)
        editWph->setText(QString());
    else
        editWph->setText(QString().setNum(wph));
}

void POI_Editor::btSaveWPClicked()
{
    float wph;
    if(editWph->text().isEmpty())
        wph=-1;
    else
        wph=editWph->text().toFloat();
    poi->doChgWP(editLat->getValue(),editLon->getValue(),wph);
    btOkClicked();
}

//---------------------------------------
void POI_Editor::createInterface()
{
    setMinimumWidth(400);
    QVBoxLayout *layout = new QVBoxLayout;
    QVBoxLayout *vbox;
    QGroupBox *gbox;
    // Input name
    gbox = new QGroupBox(tr("Nom"));
        vbox = new QVBoxLayout();
        editName = new QLineEdit(poi->getName(), this);
        editName->setMaxLength(48);
        vbox->addWidget(editName);
    gbox->setLayout(vbox);
    layout->addWidget(gbox);
    // Input latitude
    gbox = new QGroupBox(tr("Latitude Nord"));
        vbox = new QVBoxLayout();
        editLat = new DegreeMinuteEditor(poi->getLatitude(), this, -89, 89);
        vbox->addWidget(editLat);
    gbox->setLayout(vbox);
    layout->addWidget(gbox);
    // Input longitude
    gbox = new QGroupBox(tr("Longitude Est"));
        vbox = new QVBoxLayout();
        editLon = new DegreeMinuteEditor(poi->getLongitude(), this);
        vbox->addWidget(editLon);
    gbox->setLayout(vbox);
    layout->addWidget(gbox);
    gbox = new QGroupBox(tr("@ Wph"));
        vbox = new QVBoxLayout();
        if(poi->getWph()==-1)
            editWph = new QLineEdit(QString(),this);
        else
            editWph = new QLineEdit(QString().setNum(poi->getWph()),this);
        editWph->setMaxLength(3);
        vbox->addWidget(editWph);
    gbox->setLayout(vbox);
    layout->addWidget(gbox);
    //------------------------
    QWidget *btsbox = new QWidget(this);
    QHBoxLayout *laybts = new QHBoxLayout();

    btOk = new QPushButton(tr("Valider"), this);
    btCancel = new QPushButton(tr("Annuler"), this);
    btDelete = new QPushButton(tr("Supprimer ce POI"), this);
    btPaste = new QPushButton(tr("Coller"), this);
    btSaveWP = new QPushButton(tr("POI->WP"), this);
        laybts->addWidget(btOk);
        laybts->addWidget(btCancel);
        laybts->addWidget(btDelete);
        laybts->addWidget(btPaste);
        laybts->addWidget(btSaveWP);
    btsbox->setLayout(laybts);
    layout->addWidget(btsbox);

    connect(btOk, SIGNAL(clicked()), this, SLOT(btOkClicked()));
    connect(btCancel, SIGNAL(clicked()), this, SLOT(btCancelClicked()));
    connect(btDelete, SIGNAL(clicked()), this, SLOT(btDeleteClicked()));
    connect(btPaste, SIGNAL(clicked()), this, SLOT(btPasteClicked()));
    connect(btSaveWP, SIGNAL(clicked()), this, SLOT(btSaveWPClicked()));
    //------------------------
    setLayout(layout);
}
