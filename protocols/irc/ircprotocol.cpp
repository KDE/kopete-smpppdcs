/*
    ircprotocol - IRC Protocol

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>

    Kopete    (c) 2002      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "ircprotocol.h"

#include <qregexp.h>
#include <dom/html_element.h>
#include <kaction.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <ksimpleconfig.h>

#include "ircaccount.h"
#include "ircaddcontactpage.h"
#include "ircpreferences.h"
#include "ircchannelcontact.h"
#include "ircusercontact.h"
#include "kopeteaccountmanager.h"
#include "irceditaccountwidget.h"
#include "kopetecommandhandler.h"
#include "ksparser.h"

K_EXPORT_COMPONENT_FACTORY( kopete_irc, KGenericFactory<IRCProtocol> );

IRCProtocol *IRCProtocol::s_protocol = 0L;

KopeteOnlineStatus IRCProtocol::m_ChannelOnline;
KopeteOnlineStatus IRCProtocol::m_ChannelOffline;
KopeteOnlineStatus IRCProtocol::m_UserOnline;
KopeteOnlineStatus IRCProtocol::m_UserOp;
KopeteOnlineStatus IRCProtocol::m_UserVoice;
KopeteOnlineStatus IRCProtocol::m_UserOffline;
KopeteOnlineStatus IRCProtocol::m_UserAway;
KopeteOnlineStatus IRCProtocol::m_UserConnecting;
KopeteOnlineStatus IRCProtocol::m_Unknown;

IRCProtocol::IRCProtocol( QObject *parent, const char *name, const QStringList & /* args */ )
: KopeteProtocol( parent, name )
{
	s_protocol = this;
	mActions = 0L;

	kdDebug(14120) << k_funcinfo << endl;
	// Load all ICQ icons from KDE standard dirs

	m_ChannelOnline = KopeteOnlineStatus( KopeteOnlineStatus::Online,  50, this, 0, QString::null,   i18n( "Go O&nline" ),  i18n( "Online" ));
	m_ChannelOffline = KopeteOnlineStatus( KopeteOnlineStatus::Offline, 40, this, 1, QString::null, i18n( "Go O&ffline" ), i18n( "Offline" ));

	m_UserOp = KopeteOnlineStatus( KopeteOnlineStatus::Online,  30, this, 1, "irc_op",   QString::null,  i18n( "Online" ));
	m_UserVoice = KopeteOnlineStatus( KopeteOnlineStatus::Online,  20, this, 2, "irc_voice",   QString::null,  i18n( "Online" ));
	m_UserOnline = KopeteOnlineStatus( KopeteOnlineStatus::Online,  10, this, 0, QString::null,   i18n( "Go O&nline" ),  i18n( "Online" ));
	m_UserOffline = KopeteOnlineStatus( KopeteOnlineStatus::Offline, 5, this, 3, QString::null, i18n( "Go O&ffline" ), i18n( "Offline" ));
	m_UserAway = KopeteOnlineStatus( KopeteOnlineStatus::Away, 5, this, 5, "irc_away", i18n( "Go &Away" ), i18n( "Away" ));
	m_UserConnecting = KopeteOnlineStatus( KopeteOnlineStatus::Connecting, 1, this, 4, QString::null, i18n( "Connecting" ), i18n( "Connecting" ));
	m_Unknown = KopeteOnlineStatus( KopeteOnlineStatus::Unknown, 0, this, 6, "status_unknown", "FIXME: Make this unselectable", i18n( "Status not available") );

	//m_status = m_unknownStatus = m_Unknown;

	new IRCPreferences("irc_protocol", this);

	mParser = new KSParser();

	KConfig *cfg = KGlobal::config();
        cfg->setGroup("IRC");

	//Migration code
	if( cfg->hasKey("Nickname") )
	{
		createNewAccount( cfg->readEntry("Nickname") + "@" + cfg->readEntry("Server") + ":" + cfg->readEntry("Port") );

		cfg->deleteEntry("Nickname");
		cfg->deleteEntry("Server");
		cfg->deleteEntry("Port");
		cfg->deleteEntry("AutoConnect");
		cfg->sync();
	}

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("join"),
		SLOT( slotJoinCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /join <#channel> - Joins the sepecified channel.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("topic"),
		SLOT( slotTopicCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /topic [<topic>] - Sets and/or displays the topic for the active channel.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("whois"),
		SLOT( slotWhoisCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /whois <nickname> - Display whois info on this user.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("query"),
		SLOT( slotQueryCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /query <nickname> - Open a provate chat with this user.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("mode"),
		SLOT( slotModeCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /mode <channel> <modes> - Set modes on the given channel.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("nick"),
		SLOT( slotNickCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /nick <nickname> - Change your nickname to the given one.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("me"),
		SLOT( slotMeCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /me <action> - Do something.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("kick"),
		SLOT( slotKickCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /kick <nickname> [<reason>] - Kick someone from the channel (Requires operator status).") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("ban"),
		SLOT( slotBanCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /ban <nickname> - Add someone to this channel's ban list. Uses the hostmask nickname!*@* (Requires operator status).") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("op"),
		SLOT( slotOpCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /op <nickname> - Give channel operator status to someone (Requires operator status).") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("deop"),
		SLOT( slotDeopCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /deop <nickanme> - Remove channel operator status from someone (Requires operator status).") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("voice"),
		SLOT( slotVoiceCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /voice <nickanme> - Give channel voice status to someone (Requires operator status).") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("devoice"),
		SLOT( slotDevoiceCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /devoicep <nickanme> - Remove channel voice status from someone (Requires operator status).") );


	QObject::connect( KopeteMessageManagerFactory::factory(), SIGNAL(aboutToDisplay(KopeteMessage &)), this, SLOT(slotMessageFilter(KopeteMessage &)) );
}

IRCProtocol * IRCProtocol::protocol()
{
	return s_protocol;
}

IRCProtocol::~IRCProtocol()
{
	delete mParser;
}

void IRCProtocol::slotMessageFilter( KopeteMessage &msg )
{
	if( msg.from()->protocol() == this )
	{
		kdDebug(14120) << k_funcinfo << endl;
		QString messageText = msg.escapedBody();
		kdDebug(14120) << k_funcinfo << messageText << endl;

		//Add right click for channels, only replace text not in HTML tags
		messageText.replace( QRegExp( QString::fromLatin1("(?![^<]+>)(#[^#\\s]+)(?![^<]+>)") ), QString::fromLatin1("<span class=\"KopeteLink\" type=\"IRCChannel\">\\1</span>") );

		msg.setBody( messageText, KopeteMessage::RichText );
	}
}

KActionCollection *IRCProtocol::customChatWindowPopupActions( const KopeteMessage &m, DOM::Node &n )
{
	delete mActions;
	mActions = 0L;
	DOM::HTMLElement e = n;

	//isNull checks that the cast was successful
	if( !e.isNull() && !m.to().isEmpty() )
	{
		activeNode = n;
		activeAccount = static_cast<IRCAccount*>( m.from()->account() );
		mActions = new KActionCollection(this);
		if( e.getAttribute( QString::fromLatin1("type") ) == QString::fromLatin1("IRCChannel") )
			return activeAccount->findChannel( e.innerText().string() )->customContextMenuActions();
	}

	return mActions;
}

AddContactPage *IRCProtocol::createAddContactWidget(QWidget *parent, KopeteAccount *account)
{
	return new IRCAddContactPage(parent,static_cast<IRCAccount*>(account));
}

EditAccountWidget *IRCProtocol::createEditAccountWidget(KopeteAccount *account, QWidget *parent)
{
	return new IRCEditAccountWidget(this, static_cast<IRCAccount*>(account),parent);
}

KopeteAccount *IRCProtocol::createNewAccount(const QString &accountId)
{
	return new IRCAccount( accountId, this );
}

void IRCProtocol::deserializeContact( KopeteMetaContact *metaContact, const QMap<QString, QString> &serializedData,
	const QMap<QString, QString> & /* addressBookData */ )
{
	kdDebug(14120) << k_funcinfo << endl;

	QString contactId = serializedData[ "contactId" ];
	QString displayName = serializedData[ "displayName" ];

	if( displayName.isEmpty() )
		displayName = contactId;

	QDict<KopeteAccount> accounts = KopeteAccountManager::manager()->accounts( this );
	if( !accounts.isEmpty() )
	{
		KopeteAccount *a = accounts[ serializedData[ "accountId" ] ];
		if( a )
			a->addContact( contactId, displayName, metaContact );
		else
			kdDebug(14120) << k_funcinfo << serializedData[ "accountId" ] << " was a contact's account,"
				" but we dont have it in the accounts list" << endl;
	}
	else
		kdDebug(14120) << k_funcinfo << "No accounts loaded!" << endl;
}

void IRCProtocol::slotTopicCommand( const QString &args, KopeteMessageManager *manager )
{
	KopeteContactPtrList members = manager->members();
	IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
	if( chan )
	{
		if( !args.isEmpty() )
			chan->setTopic( args );
		else
		{
			KopeteMessage msg(manager->user(), manager->members(), i18n("Topic for %1 is %2").arg(chan->nickName()).arg(chan->topic()), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			msg.setImportance( KopeteMessage::Low); //set the importance manualy to low
			manager->appendMessage(msg);
		}
	}
}

void IRCProtocol::slotJoinCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		if( argsList.front().startsWith( QString::fromLatin1("#") ) )
			static_cast<IRCAccount*>( manager->account() )->findChannel( argsList.front() )->startChat();
		else
		{
			KopeteMessage msg(manager->user(), manager->members(), i18n("\"%1\" is an invalid channel. Channels must start with '#'.").arg(argsList.front()), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager->appendMessage(msg);
			//TODO: join the user
		}
	}
}

void IRCProtocol::slotQueryCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		if( !argsList.front().startsWith( QString::fromLatin1("#") ) )
			static_cast<IRCAccount*>( manager->account() )->findUser( argsList.front() )->startChat();
		else
		{
			KopeteMessage msg(manager->user(), manager->members(), i18n("\"%1\" is an invalid nickname. Nicknames must not start with '#'.").arg(argsList.front()), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager->appendMessage(msg);
		}
	}
}

void IRCProtocol::slotWhoisCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->whoisUser( argsList.first() );
	}
}

void IRCProtocol::slotNickCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->successfullyChangedNick( QString::null, argsList.front() );
	}
}

void IRCProtocol::slotModeCommand( const QString &args, KopeteMessageManager *manager )
{
	QStringList argsList = KopeteCommandHandler::parseArguments( args );
	if( argsList.count() > 1 )
	{
		static_cast<IRCAccount*>( manager->account() )->engine()->changeMode( argsList.front(),
			args.section( QRegExp(QString::fromLatin1("\\s+")), 1 ) );
	}
}

void IRCProtocol::slotMeCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		KopeteContactPtrList members = manager->members();
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->actionContact(
			static_cast<const IRCContact*>(members.first())->nickName(), args );
	}
}

void IRCProtocol::slotKickCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserOp )
	{
		QRegExp spaces(QString::fromLatin1("\\s+"));
		QString nick = args.section( spaces, 0, 1);
		QString reason = args.section( spaces, 1);
		KopeteContactPtrList members = manager->members();
		QString channel = static_cast<IRCContact*>( members.first() )->nickName();
		if( channel.startsWith( QString::fromLatin1("#")) )
			static_cast<IRCAccount*>( manager->account() )->engine()->kickUser( nick, channel, reason );
	}
}

void IRCProtocol::slotBanCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserOp )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		KopeteContactPtrList members = manager->members();
		IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
		if( chan && chan->locateUser( argsList.front() ) )
			chan->setMode( QString::fromLatin1("+b %1!*@*").arg( argsList.front() ) );
	}
}

void IRCProtocol::slotOpCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("+o") );
}

void IRCProtocol::slotDeopCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("-o") );
}

void IRCProtocol::slotVoiceCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("+v") );
}

void IRCProtocol::slotDevoiceCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("-v") );
}

void IRCProtocol::simpleModeChange( const QString &args, KopeteMessageManager *manager, const QString &mode )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserOp )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		KopeteContactPtrList members = manager->members();
		IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
		if( chan && chan->locateUser( argsList.front() ) )
			chan->setMode( QString::fromLatin1("%1 %2").arg( mode ).arg( argsList.front() ) );
	}
}

#include "ircprotocol.moc"

// vim: set noet ts=4 sts=4 sw=4:

