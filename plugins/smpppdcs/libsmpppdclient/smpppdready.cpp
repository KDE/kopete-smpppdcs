/*
	smpppdready.cpp
 
	Copyright (c) 2006      by Heiko Schaefer        <heiko@rangun.de>
 
	Kopete    (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>
 
	*************************************************************************
	*                                                                       *
	* This program is free software; you can redistribute it and/or modify  *
	* it under the terms of the GNU General Public License as published by  *
	* the Free Software Foundation; version 2 of the License.               *
	*                                                                       *
	*************************************************************************
*/

#include <qregexp.h>

#include <kdebug.h>
#include <k3streamsocket.h>

#include "smpppdunsettled.h"
#include "smpppdclient.h"
#include "smpppdready.h"

using namespace SMPPPD;

Ready * Ready::m_instance = NULL;

Ready::Ready() {}

Ready::~Ready() {}

Ready * Ready::instance() {
    if(!m_instance) {
        m_instance = new Ready;
    }

    return m_instance;
}

void Ready::disconnect(Client * client) {
    kDebug(14312) ;
    if(socket(client)) {
        socket(client)->flush();
        socket(client)->close();

        delete socket(client);
        setSocket(client, NULL);

        setServerID(client, QString());
        setServerVersion(client, QString());
    }

    changeState(client, Unsettled::instance());
}

QStringList Ready::getInterfaceConfigurations(Client * client) {

    QStringList ifcfgs;

    // we want all ifcfgs
    kDebug(14312) << "smpppd req: list-ifcfgs";
    write(client, "list-ifcfgs");
    QStringList stream = read(client);
    kDebug(14312) << "smpppd ack: " << stream[0];
    if(stream[0].startsWith("ok")) {
        // we have now a QStringList with all ifcfgs
        // we extract them and put them in the global ifcfgs-list
        // stream[1] tells us how many ifcfgs are coming next
        QRegExp numIfcfgsRex("^BEGIN IFCFGS ([0-9]+).*");
        if(numIfcfgsRex.exactMatch(stream[1])) {
            int count_ifcfgs = numIfcfgsRex.cap(1).toInt();
            kDebug(14312) << "ifcfgs: " << count_ifcfgs;

            for(int i = 0; i < count_ifcfgs; i++) {
                QRegExp ifcfgRex("^i \"(ifcfg-[a-zA-Z]+[0-9]+)\".*");
                if(ifcfgRex.exactMatch(stream[i+2])) {
                    ifcfgs.push_back(ifcfgRex.cap(1));
                }
            }
        }
    }

    return ifcfgs;
}

bool Ready::statusInterface(Client * client, const QString& ifcfg) {
	
	QString cmd = "list-status " + ifcfg;
	
	write(client, cmd.toLatin1());
	socket(client)->waitForMore(0);
	
	QStringList stream = read(client);
	
	if(stream[0].startsWith("ok")) {
		if(stream[2].startsWith("status connected")) {
			return true;
		}
	}

	return false;
}
