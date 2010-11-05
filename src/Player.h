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

#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

#include "inetClient.h"
#include "class_list.h"
#include "dataDef.h"

struct boatData {
    QString name;
    QString pseudo;
    int idu;
    int isOwn;
    int engaged;
};

class Player : public QObject, public inetClient
{ Q_OBJECT
    public:
        Player(QString login, QString pass,int type, int id, QString name,
               Projection * proj,MainWindow * main,
               myCentralWidget * parent,inetConnexion * inet);
        ~Player(void);
        /* inetClient */
        void requestFinished(QByteArray res);
        QString getAuthLogin(bool * ok=NULL) {if(ok) *ok=true; return login;}
        QString getAuthPass(bool * ok=NULL) {if(ok) *ok=true; return pass;}
        MainWindow * getMain(void) { return main; }
        myCentralWidget * getParent(void) { return parent; }
        Projection * getProjection(void) { return proj; }

        boatReal * getRealBoat(void) { if(type==BOAT_VLM) return NULL; return realBoat; }

        void authFailed(void);
        void inetError(void);

        void updateData(void);

        /* Data access */
        QString getLogin(void)          { return login; }
        QString getPass(void)           { return pass; }
        QString getName(void)           { return name; }
        int     getType(void)           { return type; }
        int     getId(void)             { return player_id; }
        QList<boatVLM*> * getBoats(void)  { return &boats; }
        QList<boatData*> getBoatsData(void)  { return boatsData; }
        bool    isUpdating(void)        { return updating; }

        void setId(int id)              { player_id=id; }
        void setParam(QString login, QString pass) { this->login=login; this->pass=pass; }
        void setType(int type)          { this->type=type; }
        void setName(QString name)      { this->name=name; }

        /* boat list */
        void addBoat(boatVLM * boat)    { boats.append(boat); }

    signals:
        void addBoat_list(boatVLM* boat);
        void delBoat_list(boatVLM* boat);
        void playerUpdated(bool,Player *);

    public slots:

    private:
        myCentralWidget * parent;
        MainWindow * main;
        Projection * proj;

        QString login;
        QString pass;
        QString name;
        int type;
        int player_id;
        QList<boatVLM*> boats;
        QList<boatData*> boatsData;

        boatReal * realBoat;

        bool updating;

        void doRequest(int requestCmd);

};

#endif // PLAYER_H