/*
    smpppdcsprefsimpl.cpp

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

#include <arpa/inet.h>
#include <netdb.h>

#include <qradiobutton.h>

#include <kstandarddirs.h>
#include <kpushbutton.h>
#include <k3resolver.h>
#include <knuminput.h>
#include <klocale.h>
#include <kdebug.h>

#include "kopetepluginmanager.h"

#include "smpppdcsplugin.h"

#include "smpppdlocationwidget.h"
#include "smpppdcsprefsimpl.h"
#include "smpppdsearcher.h"

SMPPPDCSPrefs::SMPPPDCSPrefs ( QWidget* parent, Qt::WindowFlags f )
		: QWidget (parent, f), m_plugin ( NULL ), m_scanProgressDlg ( NULL ), m_curSearcher ( NULL )
{
	// search for our main-plugin instance
	Kopete::Plugin * p = Kopete::PluginManager::self()->plugin ( "kopete_smpppdcs" );
	if ( p )
	{
		m_plugin = static_cast<SMPPPDCSPlugin *> ( p );
	}

	// signals and slots connections
	QObject::connect ( useNetstat, SIGNAL ( toggled ( bool ) ), this, SLOT ( disableSMPPPDSettings() ) );
	QObject::connect ( useSmpppd,  SIGNAL ( toggled ( bool ) ), this, SLOT ( enableSMPPPDSettings() ) );
	QObject::connect ( autoCSTest, SIGNAL ( clicked() ),     this, SLOT ( determineCSType() ) );

	if ( m_plugin )
	{
		Kopete::Plugin * pluginPlugin = m_plugin;
		QObject * pluginObj = pluginPlugin;
		connect ( SMPPPDLocation->server, SIGNAL ( textChanged ( const QString& ) ),
		          pluginObj, SLOT ( smpppdServerChanged ( const QString& ) ) );
	}

	// if netstat is NOT available, disable the option and set to SMPPPD
	if ( KStandardDirs::findExe ( "netstat" ).isNull() )
	{
		autoCSTest->setEnabled ( false );
		useNetstat->setEnabled ( false );
		useNetstat->setChecked ( false );
		useSmpppd->setChecked ( true );
	}
}

SMPPPDCSPrefs::~SMPPPDCSPrefs()
{
	delete m_scanProgressDlg;
}

void SMPPPDCSPrefs::determineCSType()
{
	// while we search, we'll disable the button
	autoCSTest->setEnabled ( false );
	//qApp->processEvents();

	/* broadcast network for a smpppd.
	   If one is available set to smpppd method */

	SMPPPDSearcher searcher;
	m_curSearcher = &searcher;

	connect ( &searcher, SIGNAL ( smpppdFound ( const QString& ) ), this, SLOT ( smpppdFound ( const QString& ) ) );
	connect ( &searcher, SIGNAL ( smpppdNotFound() ), this, SLOT ( smpppdNotFound() ) );
	connect ( &searcher, SIGNAL ( scanStarted ( uint ) ), this, SLOT ( scanStarted ( uint ) ) );
	connect ( &searcher, SIGNAL ( scanProgress ( uint ) ), this, SLOT ( scanProgress ( uint ) ) );
	connect ( &searcher, SIGNAL ( scanFinished() ), this, SLOT ( scanFinished() ) );

	searcher.searchNetwork();
	m_curSearcher = NULL;
}

void SMPPPDCSPrefs::scanStarted ( uint total )
{
	kDebug ( 14312 ) << "Scanning for a SMPPPD started. Will scan " << total << " IPs";

	// setup the scanProgress Dialog
	if ( !m_scanProgressDlg )
	{
		m_scanProgressDlg = new KProgressDialog ( this, i18n ( "Searching" ), i18n ( "Searching for a SMPPPD on the local network..." ));
		m_scanProgressDlg->setAutoClose ( true );
		m_scanProgressDlg->setAllowCancel ( true );
		m_scanProgressDlg->setMinimumDuration ( 2000 );

		connect ( m_scanProgressDlg, SIGNAL ( cancelClicked() ), this, SLOT ( cancelScanning() ) );
	}
	m_scanProgressDlg->progressBar()->setMaximum ( total );
	m_scanProgressDlg->progressBar()->setValue ( 0 );
	m_scanProgressDlg->show();
}

void SMPPPDCSPrefs::scanProgress ( uint cur )
{
	m_scanProgressDlg->progressBar()->setValue ( cur );
	qApp->processEvents();
}

void SMPPPDCSPrefs::cancelScanning()
{
	kDebug ( 14312 ) ;
	Q_ASSERT ( m_curSearcher );
	m_curSearcher->cancelSearch();
}

void SMPPPDCSPrefs::smpppdFound ( const QString& host )
{
	kDebug ( 14312 ) ;

	QString myHost = host;

	// try to get the domain name
	struct in_addr addr;
	if ( inet_aton ( host.toAscii(), &addr ) )
	{
		struct hostent * hostEnt = gethostbyaddr ( &addr.s_addr, sizeof ( addr.s_addr ), AF_INET );
		if ( hostEnt )
		{
			myHost = hostEnt->h_name;
		}
		else
		{
#ifndef NDEBUG
			switch ( h_errno )
			{
				case HOST_NOT_FOUND:
					kDebug ( 14312 ) << "No such host is known in the database.";
					break;
				case TRY_AGAIN:
					kDebug ( 14312 ) << "Couldn't contact DNS server.";
					break;
				case NO_RECOVERY:
					kDebug ( 14312 ) << "A non-recoverable error occurred.";
					break;
				case NO_ADDRESS:
					kDebug ( 14312 ) << "The host database contains an entry for the name, but it doesn't have an associated Internet address.";
					break;
			}
#endif

		}
	}

	SMPPPDLocation->setServer ( myHost );
	useNetstat->setChecked ( false );
	useSmpppd->setChecked ( true );
	autoCSTest->setEnabled ( true );
}

void SMPPPDCSPrefs::smpppdNotFound()
{
	kDebug ( 14312 ) ;
	useNetstat->setChecked ( true );
	useSmpppd->setChecked ( false );
	autoCSTest->setEnabled ( true );
}

#include "smpppdcsprefsimpl.moc"
