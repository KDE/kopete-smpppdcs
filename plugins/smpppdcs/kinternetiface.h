// -*- c++ -*-
/***************************************************************************
 *									   *
 *   Copyright: SuSE Linux AG, Nuernberg				   *
 *									   *
 *   Author: Arvin Schnell <arvin@suse.de>				   *
 *									   *
 ***************************************************************************/

/***************************************************************************
 *									   *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or	   *
 *   (at your option) any later version.				   *
 *									   *
 ***************************************************************************/


#ifndef KINTERNETIFACE_H
#define KINTERNETIFACE_H

#include <QObject>

class KInternetIface : public QObject
{
		Q_OBJECT
		Q_CLASSINFO ( "D-Bus Interface", "org.kde.kopete.plugin.smpppdcs /KInternetIface" )
	public:

		explicit KInternetIface ( const QString& name ) { QDBusConnection::sessionBus().registerObject(name, this, QDBusConnection::ExportScriptableSlots); }
	public slots:
		// query function for susewatcher
		Q_SCRIPTABLE bool isOnline () { return kinternet && kinternet->get_status () == KInternet::CONNECTED; }

	private:
		Q_DISABLE_COPY ( KInternetIface )
};


#endif
