/*
    kopeteviewmanager.cpp - View Manager

    Copyright (c) 2003      by Jason Keirstead       <jason@keirstead.org>
    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/


#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <qptrlist.h>
#include <qstylesheet.h>
#include <kplugininfo.h>

#include "kopetenotifyclient.h"
#include "kopeteprefs.h"
#include "kopeteaccount.h"
#include "kopetepluginmanager.h"
#include "kopeteviewplugin.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "kopetemessageevent.h"
#include "kopeteview.h"
//#include "systemtray.h"

#include "kopeteviewmanager.h"

typedef QMap<Kopete::ChatSession*,KopeteView*> ManagerMap;
typedef QPtrList<Kopete::MessageEvent> EventList;

struct KopeteViewManagerPrivate
{
	ManagerMap managerMap;
	EventList eventList;
	KopeteView *activeView;

	bool useQueue;
	bool raiseWindow;
	bool foreignMessage;
};

KopeteViewManager *KopeteViewManager::s_viewManager = 0L;

KopeteViewManager *KopeteViewManager::viewManager()
{
	if( !s_viewManager )
		s_viewManager = new KopeteViewManager();
	return s_viewManager;
}

KopeteViewManager::KopeteViewManager()
{
	s_viewManager=this;
	d = new KopeteViewManagerPrivate;
	d->activeView = 0L;
	d->foreignMessage=false;

	connect( KopetePrefs::prefs(), SIGNAL( saved() ), this, SLOT( slotPrefsChanged() ) );

	connect( Kopete::ChatSessionManager::self() , SIGNAL( display( Kopete::Message &, Kopete::ChatSession *) ),
		this, SLOT ( messageAppended( Kopete::Message &, Kopete::ChatSession *) ) );

	connect( Kopete::ChatSessionManager::self() , SIGNAL( readMessage() ),
		this, SLOT ( nextEvent() ) );

	slotPrefsChanged();
}

KopeteViewManager::~KopeteViewManager()
{
	kdDebug(14000) << k_funcinfo << endl;

	//delete all open chatwindow.
	ManagerMap::Iterator it;
	for ( it = d->managerMap.begin(); it != d->managerMap.end(); ++it )
		it.data()->closeView( true ); //this does not clean the map, but we don't care

	delete d;
}

void KopeteViewManager::slotPrefsChanged()
{
	d->useQueue = KopetePrefs::prefs()->useQueue();
	d->raiseWindow = KopetePrefs::prefs()->raiseMsgWindow();
}

KopeteView *KopeteViewManager::view( Kopete::ChatSession* session, const QString &requestedPlugin )
{
	kdDebug(14000) << k_funcinfo << endl;

	if( d->managerMap.contains( session ) && d->managerMap[ session ] )
	{
		return d->managerMap[ session ];
	}
	else
	{
		Kopete::PluginManager *pluginManager = Kopete::PluginManager::self();
		Kopete::ViewPlugin *viewPlugin = 0L;

		if( !requestedPlugin.isNull() )
		{
			viewPlugin = (Kopete::ViewPlugin*)pluginManager->loadPlugin( requestedPlugin );

			if( !viewPlugin )
			{
				kdWarning(14000) << "Requested view plugin, " << requestedPlugin <<
				    ", was not found. Falling back to chat window plugin" << endl;
			}
		}

		if( !viewPlugin )
			viewPlugin = (Kopete::ViewPlugin*)pluginManager->loadPlugin( QString::fromLatin1("kopete_chatwindow") );

		if( viewPlugin )
		{
			KopeteView *newView = viewPlugin->createView(session);

			d->foreignMessage = false;
			d->managerMap.insert( session, newView );

			connect( session, SIGNAL( closing(Kopete::ChatSession *) ),
					this, SLOT(slotChatSessionDestroyed(Kopete::ChatSession*)) );

			return newView;
		}
		else
		{
			kdError(14000) << "Could not create a view, no plugins available!" << endl;
			return 0L;
		}
	}
}


void KopeteViewManager::messageAppended( Kopete::Message &msg, Kopete::ChatSession *manager)
{
	kdDebug(14000) << k_funcinfo << endl;

	bool outgoingMessage = ( msg.direction() == Kopete::Message::Outbound );

	if( !outgoingMessage || d->managerMap.contains( manager ) )
	{
		d->foreignMessage=!outgoingMessage; //let know for the view we are about to create
		manager->view(true)->appendMessage( msg );
		d->foreignMessage=false; //the view is created, reset the flag

		if ( d->useQueue && !view( manager )->isVisible()  )
		{
			if ( !outgoingMessage )
			{

				Kopete::MessageEvent *event=new Kopete::MessageEvent(msg,manager);
				d->eventList.append( event );
				connect(event, SIGNAL(done(Kopete::MessageEvent *)), this, SLOT(slotEventDeleted(Kopete::MessageEvent *)));
				Kopete::ChatSessionManager::self()->postNewEvent(event);
			}
		}
		else
		{
			readMessages( manager, outgoingMessage );
		}

		if ( !outgoingMessage && ( !manager->account()->isAway() || KopetePrefs::prefs()->soundIfAway() ) )
		{
			int winId = 0;  //KopeteSystemTray::systemTray() ? KopeteSystemTray::systemTray()->winId() : 0;
			QWidget *w=dynamic_cast<QWidget*>(manager->view(false));
			if(w) winId=w->topLevelWidget()->winId();

			KConfig *config = KGlobal::config();
			config->setGroup("General");
			if( !manager->view(false) || !w || manager->view() != d->activeView ||
						  config->readBoolEntry("EventIfActive", true) || !w->isActiveWindow() )
			{
				QString msgFrom = QString::null;
				if( msg.from()->metaContact() )
					msgFrom = msg.from()->metaContact()->displayName();
				else
					msgFrom = msg.from()->contactId();
	
				QString msgText = msg.plainBody();
				if( msgText.length() > 90 )
					msgText = msgText.left(88) + QString::fromLatin1("...");
	
				QString event;
				QString body =i18n( "<qt>Incoming message from %1<br>\"%2\"</qt>" );
	
				switch( msg.importance() )
				{
					case Kopete::Message::Low:
						event = QString::fromLatin1( "kopete_contact_lowpriority" );
						break;
					case Kopete::Message::Highlight:
						event = QString::fromLatin1( "kopete_contact_highlight" );
						body = i18n( "<qt>A highlighted message arrived from %1<br>\"%2\"</qt>" );
						break;
					default:
						event = QString::fromLatin1( "kopete_contact_incoming" );
				}
				KNotifyClient::event(winId,  event, body.arg( QStyleSheet::escape(msgFrom), QStyleSheet::escape(msgText) ), msg.from()->metaContact(),
					i18n("View") , const_cast<Kopete::Contact*>(msg.from()) , SLOT(execute()) );
			}
		}
	}
}

void KopeteViewManager::readMessages( Kopete::ChatSession *manager, bool outgoingMessage )
{
	kdDebug( 14000 ) << k_funcinfo << endl;
	d->foreignMessage=!outgoingMessage; //let know for the view we are about to create
	KopeteView *thisView = manager->view( true );
	d->foreignMessage=false; //the view is created, reset the flag
 	if( ( outgoingMessage && !thisView->isVisible() ) || d->raiseWindow )
		thisView->raise();

	else if( !thisView->isVisible() )
		thisView->makeVisible();

	QPtrListIterator<Kopete::MessageEvent> it( d->eventList );
	Kopete::MessageEvent* event;
	while ( ( event = it.current() ) != 0 )
	{
		++it;
		if ( event->message().manager() == manager )
		{
			event->apply();
			d->eventList.remove( event );
		}
	}
}

void KopeteViewManager::slotEventDeleted( Kopete::MessageEvent *event )
{
	kdDebug(14000) << k_funcinfo << endl;
	Kopete::ChatSession *kmm=event->message().manager();
	if(!kmm)
		return;

	d->eventList.remove( event );
	
	if ( event->state() == Kopete::MessageEvent::Applied )
	{
		readMessages( kmm, false );
	}
	else if ( event->state() == Kopete::MessageEvent::Ignored )
	{
		bool bAnotherWithThisManager = false;
		for( QPtrListIterator<Kopete::MessageEvent> it( d->eventList ); it; ++it )
		{
			Kopete::MessageEvent *event = it.current();
			if ( event->message().manager() == kmm )
				bAnotherWithThisManager = true;
		}
		if ( !bAnotherWithThisManager && kmm->view( false ) )
			kmm->view()->closeView( true );
	}
}

void KopeteViewManager::nextEvent()
{
	kdDebug( 14000 ) << k_funcinfo << endl;

	if( d->eventList.isEmpty() )
		return;

	Kopete::MessageEvent* event = d->eventList.first();

	if ( event )
		event->apply();
}

void KopeteViewManager::slotViewActivated( KopeteView *view )
{
	kdDebug( 14000 ) << k_funcinfo << endl;
	d->activeView = view;

	QPtrListIterator<Kopete::MessageEvent> it ( d->eventList );
	Kopete::MessageEvent* event;
	while ( ( event = it.current() ) != 0 )
	{
		++it;
		if ( event->message().manager() == view->msgManager() )
			event->deleteLater();
	}

}

void KopeteViewManager::slotViewDestroyed( KopeteView *closingView )
{
	kdDebug( 14000 ) << k_funcinfo << endl;

	if( d->managerMap.contains( closingView->msgManager() ) )
	{
		d->managerMap.remove( closingView->msgManager() );
//		closingView->msgManager()->setCanBeDeleted( true );
	}

	if( closingView == d->activeView )
		d->activeView = 0L;
}

void KopeteViewManager::slotChatSessionDestroyed( Kopete::ChatSession *manager )
{
	kdDebug( 14000 ) << k_funcinfo << endl;

	if( d->managerMap.contains( manager ) )
	{
		d->managerMap[ manager ]->closeView( true );
	}
}

KopeteView* KopeteViewManager::activeView() const
{
	return d->activeView;
}


#include "kopeteviewmanager.moc"

// vim: set noet ts=4 sts=4 sw=4:
