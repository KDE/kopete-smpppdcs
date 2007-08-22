/*
    smpppdcsplugin.cpp
 
    Copyright (c) 2002-2003 by Chris Howells         <howells@kde.org>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
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

#include "onlineinquiry.h"
#include "smpppdcsplugin.h"

#include <qtimer.h>
//Added by qt3to4:
#include <QByteArray>

#include <kdebug.h>
#include <kgenericfactory.h>

#include "kopeteprotocol.h"
#include "networkstatuscommon.h"
#include "kopetepluginmanager.h"
#include "kopeteaccountmanager.h"

#include "detectornetworkstatus.h"
#include "detectornetstat.h"
#include "detectorsmpppd.h"
#include "smpppdcsconfig.h"

typedef KGenericFactory<SMPPPDCSPlugin> SMPPPDCSPluginFactory;
K_EXPORT_COMPONENT_FACTORY(kopete_smpppdcs, SMPPPDCSPluginFactory("kopete_smpppdcs"))

SMPPPDCSPlugin::SMPPPDCSPlugin(QObject *parent, const char * name, const QStringList& /* args */)
        : DCOPObject("SMPPPDCSIface"), Kopete::Plugin(SMPPPDCSPluginFactory::componentData(), parent, name),
        m_detectorSMPPPD(NULL), m_detectorNetstat(NULL), m_detectorNetworkStatus(NULL), m_timer(NULL),
m_onlineInquiry(NULL) {

    kDebug(14312) ;

    m_pluginConnected = false;

    m_onlineInquiry   = new OnlineInquiry();
    m_detectorSMPPPD  = new DetectorSMPPPD(this);
    m_detectorNetstat = new DetectorNetstat(this);

    // experimental, not used yet
    m_detectorNetworkStatus = new DetectorNetworkStatus(this);

    // we wait for the allPluginsLoaded signal, to connect
    // as early as possible after startup, but not before
    // all accounts are ready
    connect(Kopete::PluginManager::self(), SIGNAL(allPluginsLoaded()),
            this, SLOT(allPluginsLoaded()));

    // if kopete was already running and the plugin
    // was loaded later, we check once after 15 secs
    // if all other plugins have been loaded
    QTimer::singleShot(15000, this, SLOT(allPluginsLoaded()));
}

SMPPPDCSPlugin::~SMPPPDCSPlugin() {

    kDebug(14312) ;

    delete m_timer;
    delete m_detectorSMPPPD;
    delete m_detectorNetstat;
    delete m_detectorNetworkStatus;
    delete m_onlineInquiry;
}

void SMPPPDCSPlugin::allPluginsLoaded() {

    if(Kopete::PluginManager::self()->isAllPluginsLoaded()) {
        m_timer = new QTimer();
        connect(m_timer, SIGNAL(timeout()), this, SLOT(slotCheckStatus()));

		if(SMPPPDCSConfig::self()->useSmpppd()) {
            m_timer->start(30000);
        } else {
            // we use 1 min interval, because it reflects
            // the old connectionstatus plugin behaviour
            m_timer->start(60000);
        }

        slotCheckStatus();
    }
}

bool SMPPPDCSPlugin::isOnline() const {
	return m_onlineInquiry->isOnline(SMPPPDCSConfig::self()->useSmpppd());
}

void SMPPPDCSPlugin::slotCheckStatus() {
	
	// reread config to get changes
	SMPPPDCSConfig::self()->readConfig();
	
	if(SMPPPDCSConfig::self()->useSmpppd()) {
        m_detectorSMPPPD->checkStatus();
    } else {
        m_detectorNetstat->checkStatus();
    }
}

void SMPPPDCSPlugin::setConnectedStatus( bool connected ) {
    kDebug(14312) << connected;

    // We have to handle a few cases here. First is the machine is connected, and the plugin thinks
    // we're connected. Then we don't do anything. Next, we can have machine connected, but plugin thinks
    // we're disconnected. Also, machine disconnected, plugin disconnected -- we
    // don't do anything. Finally, we can have the machine disconnected, and the plugin thinks we're
    // connected. This mechanism is required so that we don't keep calling the connect/disconnect functions
    // constantly.

    if ( connected && !m_pluginConnected ) {
        // The machine is connected and plugin thinks we're disconnected
        kDebug(14312) << "Setting m_pluginConnected to true";
        m_pluginConnected = true;
        connectAllowed();
        kDebug(14312) << "We're connected";
    } else if ( !connected && m_pluginConnected ) {
        // The machine isn't connected and plugin thinks we're connected
        kDebug(14312) << "Setting m_pluginConnected to false";
        m_pluginConnected = false;
        disconnectAllowed();
        kDebug(14312) << "We're offline";
    }
}

void SMPPPDCSPlugin::connectAllowed()
{
	QStringList list = SMPPPDCSConfig::self()->ignoredAccounts();
	
	Kopete::AccountManager * m = Kopete::AccountManager::self();
	foreach(Kopete::Account *account, m->accounts())
	{
#ifndef NDEBUG
        if(account->inherits("Kopete::ManagedConnectionAccount")) {
            kDebug(14312) << "Account " << account->protocol()->pluginId() + '_' + account->accountId() << " is an managed account!";
        } else {
            kDebug(14312) << "Account " << account->protocol()->pluginId() + '_' +account->accountId() << " is an unmanaged account!";
        }
#endif

		if(!list.contains(account->protocol()->pluginId() + '_' + account->accountId())) {
			account->connect();
		}
	}
}

void SMPPPDCSPlugin::disconnectAllowed() {
	QStringList list = SMPPPDCSConfig::self()->ignoredAccounts();
	
	Kopete::AccountManager * m = Kopete::AccountManager::self();
	foreach(Kopete::Account *account, m->accounts())
	{
#ifndef NDEBUG
        if(account->inherits("Kopete::ManagedConnectionAccount")) {
            kDebug(14312) << "Account " << account->protocol()->pluginId() + '_' + account->accountId() << " is an managed account!";
        } else {
            kDebug(14312) << "Account " << account->protocol()->pluginId() + '_' +account->accountId() << " is an unmanaged account!";
        }
#endif
		if(!list.contains(account->protocol()->pluginId() + '_' + account->accountId())) {
			account->disconnect();
		}
	}
}

QString SMPPPDCSPlugin::detectionMethod() const {
	if(SMPPPDCSConfig::self()->useSmpppd()) {
        return "smpppd";
    } else {
        return "netstat";
    }
}

/*!
    \fn SMPPPDCSPlugin::smpppdServerChanged(const QString& server)
 */
void SMPPPDCSPlugin::smpppdServerChanged(const QString& server)
{
	QString oldServer = SMPPPDCSConfig::self()->server().utf8();
	
	if(oldServer != server) {
		kDebug(14312) << "Detected a server change";
		m_detectorSMPPPD->smpppdServerChange();
	}
}

void SMPPPDCSPlugin::aboutToUnload() {

    kDebug(14312) ;

    if(m_timer) {
        m_timer->stop();
    }

    emit readyForUnload();
}

#include "smpppdcsplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:
