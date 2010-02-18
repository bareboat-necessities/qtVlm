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
***********************************************************************/

#include <QDebug>
#include <QFileDialog>

#include "DialogVLM_grib.h"
#include "settings.h"

#define VLM_REQUEST_GET_FOLDER 0
#define VLM_REQUEST_GET_FILE   1

DialogVLM_grib::DialogVLM_grib(MainWindow * ,myCentralWidget * parent,inetConnexion * inet) :
        QDialog(parent),
        inetClient(inet)
{
    setupUi(this);
    listRadio[0]=radio1;
    listRadio[1]=radio2;
    listRadio[2]=radio3;
    listRadio[3]=radio4;

    waitBox = new QMessageBox(QMessageBox::Information,
                             tr("VLM Grib"),
                             tr("Chargement de la liste de grib"));

}

void DialogVLM_grib::done(int res)
{
    if(res == QDialog::Accepted)
    {        
        if(!doRequest(VLM_REQUEST_GET_FILE))
            QDialog::done(QDialog::Rejected);
    }

    QDialog::done(res);
}

void DialogVLM_grib::showDialog(void)
{
    /* disable all radio button */

    for(int i=0;i<4;i++)
        listRadio[i]->setEnabled(false);

    filename="";

    if(doRequest(VLM_REQUEST_GET_FOLDER))
        waitBox->exec();
   // if(waitBox->isVisible()) waitBox->hide();
}

int DialogVLM_grib::parseFolderListing(QString data)
{
    int pos=0;
    int i;

    QString gribName_str;
    QString date_str;
    QString size_str;

    i=0;
    while(1)
    {
        /* grib file name */
        pos = data.indexOf("gfs_NOAA",pos);
        if(pos==-1) break;
        gribName_str = data.mid(pos,23);

        pos+=23;

        /* grib date */
        pos = data.indexOf("<td align=\"right\">",pos);
        if(pos==-1) break;
        date_str = data.mid(pos+18,17);
        pos=pos+20;

        /* file size */
        pos = data.indexOf("<td align=\"right\">",pos);
        if(pos==-1) break;
        size_str = data.mid(pos+19,3);
        pos=pos+20;

        listRadio[i]->setText(gribName_str + ", modified " + date_str + ", size " + size_str );
        listRadio[i]->setEnabled(true);
        i++;
    }
    return i;
}

bool DialogVLM_grib::gribFileReceived(QByteArray * content)
{
    QString gribPath=Settings::getSetting("edtGribFolder","grib").toString();
    QDir dirGrib(gribPath);
    if(!dirGrib.exists())
    {
        gribPath=QApplication::applicationDirPath()+"/grib";
        Settings::setSetting("askGribFolder",1);
        Settings::setSetting("edtGribFolder",gribPath);
    }
    filename=gribPath+"/"+filename;
    if(Settings::getSetting("askGribFolder",1)==1)
    {
        filename = QFileDialog::getSaveFileName(this,
                         tr("Sauvegarde du fichier GRIB"), filename, "Grib (*.grb)");
    }

    if (filename != "")
    {
        QFile *saveFile = new QFile(filename);
        assert(saveFile);
        if (saveFile->open(QIODevice::WriteOnly))
        {
            int nb=saveFile->write(*content);
            if(nb>0)
                saveFile->close();
            qWarning() << nb << " bytes saved in " << filename;
            return true;
        }
        else
        {
            QMessageBox::critical (this,
                    tr("Erreur"),
                    tr("Ecriture du fichier impossible."));
        }
    }
    return false;
}

/*****************************************
* Inet request
****************************************/

bool DialogVLM_grib::doRequest(int reqType)
{
    if(!hasInet() || hasRequest())
    {
        qWarning("VLM dvnld grib:  bad state in inet");
        return false;
    }

    QString page;
    int i;

    switch(reqType)
    {
        case VLM_REQUEST_GET_FOLDER:
            inetGet(VLM_REQUEST_GET_FOLDER,"/","http://grib.virtual-loup-de-mer.org");
            break;
        case VLM_REQUEST_GET_FILE:
            /*search selected file*/
            for(i=0;i<4;i++)
                if(listRadio[i]->isChecked())
                    break;
            if(i==4)
                return false;
            filename=listRadio[i]->text().mid(0,23);
            page="/"+filename;
            inetGetProgress(VLM_REQUEST_GET_FILE,page,"http://grib.virtual-loup-de-mer.org");
            break;
    }
    return true;
}

void DialogVLM_grib::requestFinished (QByteArray data)
{
    int nb;
    switch(getCurrentRequest())
    {
        case VLM_REQUEST_GET_FOLDER:
            if(!waitBox->isVisible())
                return;
            waitBox->hide();
            nb=parseFolderListing(QString(data));
            if(nb==0)
                return;
            listRadio[nb-1]->setChecked(true);
            exec();
            break;
        case VLM_REQUEST_GET_FILE:
            if(filename.isEmpty())
            {
                qWarning() << "Empty file name in VLM grib save";
            }
            else
                if(!gribFileReceived(&data))
                    showDialog();
                else
                    emit signalGribFileReceived(filename);

            break;
    }
}
