/*
    detectordcop.cpp
 
    Copyright (c) 2004-2006 by Heiko Schaefer        <heiko@rangun.de>
 
    Kopete    (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; version 2 of the License.               *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>

#include "detectordcop.h"
#include "iconnector.h"

#include <QByteArray>

QByteArray DetectorDCOP::m_kinternetApp = "";

DetectorDCOP::DetectorDCOP(IConnector * connector)
	: Detector(connector) {}

DetectorDCOP::~DetectorDCOP() {}

/*!
    \fn DetectorDCOP::getKInternetDCOP()
 */
QByteArray DetectorDCOP::getKInternetDCOP() const {
#ifndef NOKINTERNETDCOP
    DCOPClient * client = kapp->dcopClient();
    if(m_kinternetApp.isEmpty() && client && client->isAttached()) {
        // get all registered dcop apps and search for kinternet
        DCOPCStringList apps = client->registeredApplications();
        DCOPCStringList::iterator iter;
        for(iter = apps.begin(); iter != apps.end(); ++iter) {
            if((*iter).left(9) == "kinternet") {
                return *iter;
            }
        }
    }
#endif

    return m_kinternetApp;
}

/*!
    \fn DetectorDCOP::getConnectionStatusDCOP()
 */
DetectorDCOP::KInternetDCOPState DetectorDCOP::getConnectionStatusDCOP() const {
    kDebug(14312) << "Start inquiring " << m_kinternetApp << " via DCOP";
	
	
#ifndef NOKINTERNETDCOP
	KInternetIface_stub stub = KInternetIface_stub(kapp->dcopClient(), m_kinternetApp, "KInternetIface");
	
	bool status = stub.isOnline();
	
	if(stub.ok()) {
		if(status) {
			kDebug(14312) << "isOnline() returned true";
			return CONNECTED;
		} else {
			kDebug(14312) << "isOnline() returned false";
			return DISCONNECTED;
		}
	} else {
		kWarning(14312) << "DCOP call to " << m_kinternetApp << " failed!";
	}
#endif

	return ERROR;
}

