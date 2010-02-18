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

#include <QStringList>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QClipboard>
#include "Util.h"
#include "POI.h"
#include "settings.h"


//======================================================================
QString Util::formatTemperature(float tempKelvin)
{
    QString tunit = Settings::getSetting("unitsTemp", "").toString();
    QString unit = (tunit=="") ? QObject::tr("°C") : tunit;
    QString r;
    if (unit == QObject::tr("°C")) {
        r.sprintf("%.1f ", tempKelvin-273.15);
    }
    else if (unit == QObject::tr("°F")) {
        r.sprintf("%.1f ", 1.8*(tempKelvin-273.15)+32.0);
    }
    else  {   // if (unit == QObject::tr("°K"))
        unit = QObject::tr("°K");
        r.sprintf("%.1f ", tempKelvin);
    }
    return r+unit;
}
//-------------------------------------------------------
QString Util::formatTemperature_short(float tempKelvin)
{
    QString tunit = Settings::getSetting("unitsTemp", "").toString();
    QString unit = (tunit=="") ? QObject::tr("°C") : tunit;
    QString r;
    if (unit == QObject::tr("°C")) {
        r.sprintf("%d", qRound(tempKelvin-273.15) );
    }
    else if (unit == QObject::tr("°F")) {
        r.sprintf("%d", qRound(1.8*(tempKelvin-273.15)+32.0) );
    }
    else  {   // if (unit == QObject::tr("°K"))
        unit = QObject::tr("°K");
        r.sprintf("%d", qRound(tempKelvin) );
    }
    return r; //+unit;
}
//----------------------------------------------------------------
QString Util::formatSpeed(float meterspersecond)
{
    QString tunit = Settings::getSetting("unitsWindSpeed", "").toString();
    QString unit = (tunit=="") ? QObject::tr("km/h") : tunit;
    QString r;
    if (unit == QObject::tr("m/s")) {
        r.sprintf("%.1f m/s", meterspersecond);
    }
    else if (unit == QObject::tr("km/h")) {
        r.sprintf("%.1f km/h", meterspersecond*3.6);
    }
    else  {   // if (unit == QObject::tr("nœuds"))
        r.sprintf("%.1f knt", meterspersecond*3.6/1.852);
    }
    return r;
}
//----------------------------------------------------------------
QString Util::formatDistance(float mille)
{
    QString tunit = Settings::getSetting("unitsDistance", "").toString();
    QString unit = (tunit=="") ? QObject::tr("km") : tunit;
    QString r, unite;
    float d;
    if (unit == QObject::tr("km")) {
        unite = "km";
        d= mille*1.852;
    }
    else  {
        unite = "NM";
        d = mille;
    }
    if (d<10)
        r.sprintf("%5.2f %s", d, qPrintable(unite));
    else if (d<100)
        r.sprintf("%5.1f %s", d, qPrintable(unite));
    else
        r.sprintf("%5.0f %s", d, qPrintable(unite));
    return r;
}
//----------------------------------------------------------------
QString Util::formatDegres(float x)     // 123.4 -> 123°24.00'
{
    QString tunit = Settings::getSetting("unitsPosition", "").toString();
    QString unit = (tunit=="") ? QObject::tr("dd°mm'ss\"") : tunit;

    QString r;
    if (unit == QObject::tr("dd°mm,mm'"))
    {
        int deg = (int) fabs(x);
        float min = (fabs(x) - deg)*60.0;
        char sign = (x<0) ? '-' : ' ';
        const char *cdeg = "°";
        r.sprintf("%c%03d%s%05.2f'", sign,deg,cdeg, min);
    }
    else if (unit == QObject::tr("dd°mm'ss\""))
    {
        int sec = (int) fabs(x*3600.0);  // total en secondes
        int min = sec / 60;              // nombre entier de minutes
        int deg = min / 60;              // nombre entier de degrés
        min = min % 60;                  // reste en minutes
        sec = sec % 60;                  // reste en secondes
        char sign = (x<0) ? '-' : ' ';
        const char *cdeg = "°";
        r.sprintf("%c%03d%s%02d'%02d\"", sign,deg,cdeg, min,sec);
    }
    else // if (unit == QObject::tr("dd,dd°"))
    {
        const char *cdeg = "°";
        r.sprintf("%06.2f%s",x,cdeg);
    }
    return r;
}
//---------------------------------------------------------------------
QString Util::formatPosition(float x, float y)  // 123°24.00'W 45°67.89'N
{
    return formatLongitude(x)+" "+formatLatitude(y);
}
//---------------------------------------------------------------------
QString Util::formatLongitude(float x)
{
    QString dir = Settings::getSetting("longitudeDirection", "").toString();
    if (dir == "Ouest positive")
        return formatDegres(-x)+"W";
    else if (dir == "Est positive")
        return formatDegres(x)+"E";
    else {
        // Mode automatique
        if (x > 0) {
            while (x > 360)
                x -= 360;
            if (x <= 180)
                return formatDegres(x)+"E";
            else
                return formatDegres(360-x)+"W";
        }
        else {
            while (x < -360)
                x += 360;
            if (x >= -180)
                return formatDegres(-x)+"W";
            else
                return formatDegres(x+360)+"E";
        }
    }
}
//---------------------------------------------------------------------
QString Util::formatLatitude(float y)
{
    QString dir = Settings::getSetting("latitudeDirection", "").toString();
    if (dir == "Sud positive")
        return formatDegres(-y)+"S";
    else if (dir == "Nord positive")
        return formatDegres(y)+"N";
    else {
        // Mode automatique
        if (y > 0)
            return formatDegres(y)+"N";
        else
            return formatDegres(-y)+"S";
    }
}
//---------------------------------------------------------------------
QString Util::formatPercentValue(float v)
{
    QString r;
    if (v<0)
        v=0;
    else if (v>100)
        v=100;
    r.sprintf("%d %%", (int)floor(v+0.5));
    return r;
}
//======================================================================
QString Util::formatDateLong(time_t t)
{
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setTime_t(t);
    QLocale loc;

    if (loc.language()==QLocale::French)
        return loc.toString(dt.date(), "ddd dd-MM-yyyy");
    else
        return loc.toString(dt.date(), "ddd yyyy-MM-dd");
}
//---------------------------------------------------------------------
QString Util::formatDateTimeLong(time_t t)
{
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setTime_t(t);
    QLocale loc;
    if (loc.language()==QLocale::French)
        return loc.toString(dt.date(), "ddd dd-MM-yyyy ") + dt.toString("HH:mm UTC");
    else
        return loc.toString(dt.date(), "ddd yyyy-MM-dd ") + dt.toString("HH:mm UTC");
}
//---------------------------------------------------------------------
QString Util::formatDateTimeShort(time_t t)
{
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setTime_t(t);
    QLocale loc;
    if (loc.language()==QLocale::French)
        return dt.toString("dd-MM-yyyy hh:mm UTC");
    else
        return dt.toString("yyyy-MM-dd hh:mm UTC");
}
//---------------------------------------------------------------------
QString Util::formatDateTime_date(time_t t)
{
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setTime_t(t);
    QLocale loc;
    if (loc.language()==QLocale::French)
        return dt.toString("dd-MM-yyyy");
    else
        return dt.toString("yyyy-MM-dd");
}
//---------------------------------------------------------------------
QString Util::formatDateTime_hour(time_t t)
{
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setTime_t(t);
    return dt.toString("hh:mm UTC");
}

#ifdef QT_4_5
void Util::paramProxy(QNetworkAccessManager *inetManager,QString host)
#else
void Util::paramProxy(QNetworkAccessManager *inetManager,QString)
#endif
{
    /* update connection */

    int proxyType = Settings::getSetting("httpUseProxy", 0).toInt();


    QNetworkProxy * inetProxy;

    switch(proxyType)
    {
        case 1:
            /* basic proxy */
            inetProxy = new QNetworkProxy(QNetworkProxy::DefaultProxy,
                    Settings::getSetting("httpProxyHostname", "").toString(),
                    Settings::getSetting("httpProxyPort", 0).toInt(),
                    Settings::getSetting("httpProxyUsername", "").toString(),
                    Settings::getSetting("httpProxyUserPassword", "").toString());
            inetManager->setProxy(*inetProxy);
            break;
#ifdef QT_4_5
        case 2:
            /* IE proxy*/
            QList<QNetworkProxy> proxyList =QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(QUrl(host)));
            inetProxy = &(proxyList.first());

            inetProxy->setUser(Settings::getSetting("httpProxyUsername", "").toString());
            inetProxy->setPassword(Settings::getSetting("httpProxyUserPassword", "").toString());

            inetManager->setProxy(*inetProxy);
            break;
#endif
    }
}

/* format: LAT,LON@WPH,TSTAMP */
bool Util::getWPClipboard(QString * name,float * lat,float * lon, float * wph, int * tstamp)
{
    QClipboard *clipboard = QApplication::clipboard();
    QString WP_txt = clipboard->text();
    QStringList lsval=WP_txt.split(QRegExp("\\s+"));
    for(int i=0;i<lsval.size();i++)
        if(!lsval[i].isEmpty())
        {
            if(convertPOI(lsval[i],name,lat,lon,wph,tstamp,0)) // default type of mark = POI
                return true;
            else
                qWarning() << "Bad string: " << i << " |" << lsval[i] << "|";
        }
    qWarning() << "No correct string found";
    return false;
}

bool Util::convertPOI(const QString & str,QString * name,float * lat,float * lon,float * wph,int * tstamp,
                      int type)
{
    QStringList lsval1,lsval2,lsval3;

    //qWarning() << "Converting: " << str;

    lsval1 = str.split("@");

    if(lsval1.size()==2)
    {
        lsval2 = lsval1[0].split(",");
        //qWarning() << "Sub 1: " << lsval1.at(0);
        lsval3 = lsval1[1].split(",");
        //qWarning() << "Sub 2: " << lsval1.at(1);

        switch(lsval2.size())
        {
            case 2:
                if(name)    *name=POI::getTypeStr(type);
                if(lat)     *lat=lsval2[0].toFloat();
                if(lon)     *lon=lsval2[1].toFloat();
                break;
            case 3:
                if(name)    *name=lsval2[0];
                if(lat)     *lat=lsval2[1].toFloat();
                if(lon)     *lon=lsval2[2].toFloat();
                break;
            default:
                return false; /* bad format */
        }

        switch(lsval3.size())
        {
            case 1:
                if(tstamp) *tstamp=-1;
                if(wph) *wph=lsval3[0].toDouble();
                break;
            case 2:
                if(tstamp) *tstamp=lsval3[1].toInt();
                if(wph) *wph=lsval3[0].toDouble();
                break;
            default:
                return false; /* bad format */
        }
        return true; /* all ok */
    }
    return false; /* bad format */
}

void Util::setWPClipboard(float lat,float lon, float wph)
{
    /*if(wph==-1)
        QApplication::clipboard()->setText(QString("%1,%2").arg(lat).arg(lon));
    else*/
        QApplication::clipboard()->setText(QString("%1,%2@%3").arg(lat).arg(lon).arg(wph));
}

void Util::getCoordFromDistanceAngle2(float latitude, float longitude,
             float distance,float heading, float * res_lat,float * res_lon)
{
    double ld, la;
    *res_lat = latitude + degToRad( (cos(heading)*distance)/60.0 );
    if (fabs(*res_lat - latitude) > degToRad(0.001))
    {
        ld = log(tan(M_PI_4 + (latitude/2.0)));
        la = log(tan(M_PI_4 + (*res_lat/2.0)));
        *res_lon = longitude + (la-ld)*tan(heading);
    }
    else
    {
        *res_lon = longitude
                +sin(heading)*degToRad(distance/(60.0*cos(latitude)));
    }
}

void Util::getCoordFromDistanceAngle(float latitude, float longitude,
             float distance,float heading, float * res_lat,float * res_lon)
{
    float lat,lon;
    float ratio;

    if(!res_lat || !res_lon)
        return;

    latitude = degToRad(latitude);
    longitude = fmod(degToRad(longitude), TWO_PI);
    heading = degToRad(heading);

    getCoordFromDistanceAngle2(latitude,longitude,distance,heading,&lat,&lon);

    if (fabs(lat) > degToRad(80.0))
    {
        ratio = (degToRad(80.0)-fabs(latitude)) / (fabs(lat)-fabs(latitude));
        distance *= ratio;
        getCoordFromDistanceAngle2(latitude,longitude,distance,heading,&lat,&lon);
    }

    if (lon > PI)
    {
        lon -= TWO_PI;
    }
    else if (lon < -PI)
    {
        lon += TWO_PI;
    }

    *res_lat=radToDeg(lat);
    *res_lon=radToDeg(lon);
}

QString Util::pos2String(int type,float value)
{
    QString str;
//    int d,m,s;
//    float l;
//    l=value<0?-value:value;
//    d=(int)l;
//    m=(int)((l-d)*60);
//    s=(int)((l-d-(float)m/60)*3600);

    if(type==TYPE_LON)
//        str.sprintf("%03d%c%02d'%02d\"%s",d,176,m,s,value<0?"W":"E");
        str=formatLongitude(value);
        else
//        str.sprintf("%02d%c%02d'%02d\"%s",d,176,m,s,value<0?"S":"N");
        str=formatLatitude(value);
    return str;
}

QString  url_name[NB_URL] = { "s10","s11"};
QString  url_str[NB_URL] = { "s10.virtual-loup-de-mer.org","s11.virtual-loup-de-mer.org"};

QString Util::getHost()
{
    QString host;
    int num = Settings::getSetting("vlm_url",0).toInt();

    host="http://";

    if(num>=NB_URL)
    {
        qWarning() << "Updating wrong config for VLM url";
        num=0;
        Settings::setSetting("vlm_url",0);
    }

    return host+url_str[num];
}

void Util::computePos(Projection * proj, float lat, float lon, int * x, int * y)
{
    if (proj->isPointVisible(lon, lat)) {      // tour du monde ?
        proj->map2screen(lon, lat, x, y);
    }
    else if (proj->isPointVisible(lon-360, lat)) {
        proj->map2screen(lon-360, lat, x, y);
    }
    else  if (proj->isPointVisible(lon+360,lat)) {
        proj->map2screen(lon+360, lat, x, y);
    }
    else
    {
        proj->map2screen(lon, lat, x, y);
    }
}

void Util::addAgent(QNetworkRequest & request)
{
    if(Settings::getSetting("forceUserAgent",0).toInt()==1
        && !Settings::getSetting("userAgent", "").toString().isEmpty())
        request.setRawHeader("User-Agent",Settings::getSetting("userAgent", "").toString().toAscii());
}
