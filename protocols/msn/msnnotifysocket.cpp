/*
    msnnotifysocket.cpp - Notify Socket for the MSN Protocol

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart        <ogoffart@tiscalinet.be>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    Portions taken from
    KMerlin   (c) 2001      by Olaf Lueg              <olueg@olsd.de>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "msnnotifysocket.h"
#include "msndispatchsocket.h"
#include "msncontact.h"
#include "msnaccount.h"

#include <qregexp.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kmdcodec.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <krun.h>
#include <kio/job.h>

#include "kopetenotifyclient.h"

#include <ctime>

MSNNotifySocket::MSNNotifySocket( MSNAccount *account, const QString& msnId, const QString &password )
: MSNAuthSocket( msnId, account )
{
	m_newstatus = MSNProtocol::protocol()->NLN;

	m_account = account;
	m_password=password;
	QObject::connect( this, SIGNAL( blockRead( const QString & ) ),
		this, SLOT( slotReadMessage( const QString & ) ) );

	m_dispatchSocket = 0L;

	m_keepaliveTimer = new QTimer( this, "m_keepaliveTimer" );
	QObject::connect( m_keepaliveTimer, SIGNAL( timeout() ), SLOT( slotSendKeepAlive() ) );

	QObject::connect( this, SIGNAL( commandSent() ), SLOT( slotResetKeepAlive() ) );
}

MSNNotifySocket::~MSNNotifySocket()
{
	kdDebug(14140) << "MSNNotifySocket::~MSNNotifySocket" << endl;
}

void MSNNotifySocket::connect()
{
	dispatchOK=false;
	m_isHotmailAccount=false;
	m_ping=false;

	m_dispatchSocket = new MSNDispatchSocket( msnId() ,this);
	QObject::connect( m_dispatchSocket,
		SIGNAL( receivedNotificationServer( const QString &, uint ) ),
		this,
		SLOT( slotReceivedServer( const QString &, uint ) ) );

	//QObject::connect( m_dispatchSocket, SIGNAL( connectionFailed( ) ), SLOT( slotDispatchFailed( ) ) );
	QObject::connect( m_dispatchSocket, SIGNAL( socketClosed( int ) ), SLOT( slotDispatchClosed( ) ) );

	m_dispatchSocket->connect();
}

void MSNNotifySocket::slotReceivedServer( const QString &server, uint port )
{
	dispatchOK=true;
	MSNAuthSocket::connect( server, port );
}

void MSNNotifySocket::disconnect()
{
	if( onlineStatus() == Connected )
		sendCommand( "OUT", QString::null, false );

	m_keepaliveTimer->stop();

	MSNAuthSocket::disconnect();
}

void MSNNotifySocket::handleError( uint code, uint id )
{
	// See http://www.hypothetic.org/docs/msn/basics.php for a
	// description of all possible error codes.
	// TODO: Add support for all of these!
	switch( code )
	{
	case 201:
	case 205:
	{
		QString msg = i18n( "Invalid user! \n"
			"This MSN user does not exist: %1 \n Please check the MSN ID." ).arg(m_tmpLastHandle);
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		break;
	}
	case 209:
	{
		if(m_tmpLastHandle==msnId())
		{
			QString msg = i18n( "Your nickname has not been changed, maybe it contains incorrect words or it was too long" );
			KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		}
		/*else
		 {
			QString msg = i18n( "You are trying to change the display name of an user who has not "
				"confirmed his or her email address.\n"
				"The contact was not renamed on the server." );
			KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		}*/
		break;
	}
	case 215:
	{
		QString msg = i18n( "The user %1 already exists in this group!\n"
			"If this is not the case, please send us a detailed bug report "
			"at kopete-devel@kde.org containing the raw output on the "
			"console (in gzipped format, as it is probably a lot of output!)" ).arg(m_tmpLastHandle);
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		break;
	}
	case 216:
	{
		//This might happends is you rename an user if he is not in the contactlist
		//currently, we just iniore;
		//TODO: try to don't rename user not in the list
		//actualy, the bug is in MSNMessageManager::slotUserJoined()
		break;
	}
	case 223:
	{
		QString msg = i18n( "The maximum number of group is reached.\n"
			"You can't have more than 30 groups" );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		break;
	}
	case 710:
	{
		QString msg = i18n( "You can't open a hotmail inbox because you haven't a valid hotmail/msn account." );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		break;
	}
	case 800:
	{
		QString msg = i18n( "You are trying to change your status, or your nickname too rapidely.\n"
		 		"This might happends if you added yourself on your contactlist" );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		//FIXME: try to fix this problem
		break;
	}
	case 913:
	{
		QString msg = i18n( "You cannot send messages when you are offline or when you appear offline." );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin" ) );
		break;
	}
	case 910:
	case 921:
		KMessageBox::error( 0, i18n( "MSN Server is busy or temporary unavailable. Try to reconnect later." ) , i18n( "MSN Plugin" ) );
		break;
	default:
		MSNAuthSocket::handleError( code, id );
		break;
	}
}

void MSNNotifySocket::parseCommand( const QString &cmd, uint id,
	const QString &data )
{
	//kdDebug(14140) << "MSNNotifySocket::parseCommand: Command: " << cmd << endl;

	//// here follow the auth processus
	if( cmd == "USR" )
	{
		if( data.section( ' ', 1, 1 ) == "S" )
		{
			m_authData=data.section( ' ' , 2 , 2 );
			m_kv=QString::null;

			if( m_account->accountId().contains("@hotmail.com") )
				m_sid="loginnet.passport.com";
			else if( m_account->accountId().contains("@msn.com") ||  m_account->accountId().contains("@compaq.net") ||  m_account->accountId().contains("@webtv.net") )
				m_sid="msnialogin.passport.com";
			else
				m_sid="login.passport.com";

			QString authURL="https://"+m_sid+"/login.srf?" + m_authData;
			authURL.replace("," , "&" ) ;

			kdDebug(14140) << "MSNNotifySocket::parseCommand: " << authURL << endl;

			KIO::Job *job = KIO::get( authURL , false, false );
			job->addMetaData("cookies", "manual");
			QObject::connect( job, SIGNAL(result( KIO::Job *)), this, SLOT(slotAuthJobDone( KIO::Job *)) );
		}
		else
		{
			// Successful auth
			m_badPassword=false;
			// sync contact list
			QString serial=m_account->pluginData(m_account->protocol() , "serial" );
			if(serial.isEmpty())
				serial= "0";
			sendCommand( "SYN", serial );

			// this is our current user and friendly name
			// do some nice things with it  :-)
			QString publicName = unescape( data.section( ' ', 2, 2 ) );
			emit publicNameChanged( publicName );
		}
	}
	else if( cmd == "LST" )
	{
		//The hanlde is used if we receive some BRP
		m_tmpLastHandle=data.section( ' ', 0, 0 );

		// handle, publicName, lists, group
		emit contactList(  m_tmpLastHandle ,
			unescape( data.section( ' ', 1, 1 ) ), data.section( ' ', 2, 2 ).toUInt(),
			data.section( ' ', 3, 3 ) );
	}
	else if( cmd == "MSG" )
	{
		readBlock( data.section( ' ', 2, 2 ).toUInt() );
	}
	else if( cmd == "ILN" ||  cmd == "NLN" )
	{
		// status handle publicName strangeNumber MSNOBJECT
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ data.section( ' ', 1, 1 ) ] );
		if( c )
		{
			c->setOnlineStatus( convertOnlineStatus( data.section( ' ', 0, 0 ) ) );
			QString publicName=unescape( data.section( ' ', 2, 2 ) );
			if (publicName!=c->displayName())
				changePublicName(publicName,c->contactId());
			c->setObject( unescape(data.section( ' ', 4, 4 )) );
		}
	}
	else if( cmd == "FLN" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ data.section( ' ', 0, 0 ) ] );
		if( c )
			c->setOnlineStatus( MSNProtocol::protocol()->FLN );
	}
	else if( cmd == "XFR" )
	{
		// Address, AuthInfo
		emit startChat( data.section( ' ', 1, 1 ), data.section( ' ', 3, 3 ) );
	}
	else if( cmd == "RNG" )
	{
		// SessionID, Address, AuthInfo, handle, publicName
		emit invitedToChat( QString::number( id ), data.section( ' ', 0, 0 ), data.section( ' ', 2, 2 ),
			data.section( ' ', 3, 3 ), unescape( data.section( ' ', 4, 4 ) ) );
	}
	else if( cmd == "ADD" )
	{
		QString msnId = data.section( ' ', 2, 2 );
		uint group;
		if( data.section( ' ', 0, 0 ) == "FL" )
			group = data.section( ' ', 4, 4 ).toUInt();
		else
			group = 0;

		// handle, publicName, List,  group
		emit contactAdded( msnId, unescape( data.section( ' ', 2, 2 ) ),
			data.section( ' ', 0, 0 ), group );
		m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 1, 1 ) );
	}
	else if( cmd == "REM" ) // someone is removed from a list
	{
		uint group;
		if( data.section( ' ', 0, 0 ) == "FL" )
			group = data.section( ' ', 3, 3 ).toUInt();
		else
			group = 0;

		// handle, list, group
		emit contactRemoved( data.section( ' ', 2, 2 ), data.section( ' ', 0, 0 ), group );
		m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 1, 1 ) );
	}
	else if( cmd == "OUT" )
	{
		disconnect();
		if( data.section( ' ', 0, 0 ) == "OTH" )
		{
			KMessageBox::information( 0, i18n( "You have connected from another client." ) , i18n ("MSN Plugin") );
		}
	}
	else if( cmd == "CHG" )
	{
		QString status = data.section( ' ', 0, 0 );
		setOnlineStatus( Connected );
		emit statusChanged( convertOnlineStatus( status ) );
	}
	else if( cmd == "REA" )
	{
		QString handle=data.section( ' ', 1, 1 );
		if( handle == msnId() )
			emit publicNameChanged( unescape( data.section( ' ', 2, 2 ) ) );
		else
		{
			MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ handle ] );
			if( c )
				c->setDisplayName( unescape( data.section( ' ', 2, 2 ) ) );
		}
		m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0,0) );
	}
	else if( cmd == "LSG" )
	{
		//the LSG syntax depends if it is called from SYN or from LSG
		if(data.contains(' ') > 4) //FROM LSG
		{
			emit groupListed( unescape( data.section( ' ', 4, 4 ) ), data.section( ' ', 3, 3 ).toUInt() );
		}
		else //from SYN
		{
			//this command has not id, and the first param is a number, so MSNSocket think it is the ID
			emit groupListed( unescape( data.section( ' ', 0, 0 ) ), id );
		}
	}
	else if( cmd == "ADG" )
	{
		// groupName, group
		emit groupAdded( unescape( data.section( ' ', 1, 1 ) ),
			data.section( ' ', 2, 2 ).toUInt() );
		m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0, 0 ) );
	}
	else if( cmd == "REG" )
	{
		// groupName, group
		emit groupRenamed( unescape( data.section( ' ', 2, 2 ) ),
			data.section( ' ', 1, 1 ).toUInt() );
		m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0, 0 ) );
	}
	else if( cmd == "RMG" )
	{
		// group
		emit groupRemoved( data.section( ' ', 1, 1 ).toUInt() );
		m_account->setPluginData(m_account->protocol() , "serial" ,  data.section( ' ', 0, 0 ) );
	}
	else if( cmd  == "CHL" )
	{
		kdDebug(14140) << "Sending final Authentication" << endl;
		KMD5 context( ( data.section( ' ', 0, 0 ) + "Q1P7W2E4J9R8U3S5" ).utf8() );
		sendCommand( "QRY", "msmsgs@msnmsgr.com", true,
			context.hexDigest());
	}
	else if( cmd == "SYN" )
	{
		// this is the current serial on the server, if its different with the own we can get the user list
		QString serial = data.section( ' ', 0, 0 );
		if( serial != m_account->pluginData(m_account->protocol() , "serial") )
		{
			emit newContactList();  // remove all contacts datas, msn sends a new contact list
			m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0, 0 ) );
		}
		else //we have already the contactlist
		{
			//receive even the grouplist, since it is not correctly saved
			sendCommand( "LSG" );
		}

		// set the status
		setStatus( m_newstatus );
	}
	else if( cmd == "BPR" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ m_tmpLastHandle ] );
		if( c )
			c->setInfo(data.section( ' ', 0, 0 ),unescape(data.section( ' ', 1, 1 )));
	}
	else if( cmd == "PRP" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->myself() );
		if( c )
		{
			if( id > 0 ) //FROM PRP
			{
				m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0, 0 ) );
				m_account->setPluginData(m_account->protocol() ,data.section( ' ', 1, 1 ),unescape(data.section( ' ', 2, 2 ) ));
				c->setInfo(data.section( ' ', 1, 1 ),unescape(data.section( ' ', 2, 2 )));
			}
			else //FROM SYN
			{
				c->setInfo(data.section( ' ', 0, 0 ),unescape(data.section( ' ', 1, 1 )));
				m_account->setPluginData(m_account->protocol() ,data.section( ' ', 0, 0 ),unescape(data.section( ' ', 1, 1 ) ));
			}
		}
	}
	else if( cmd == "BLP" )
	{
		if( id > 0 ) //FROM BLP
		{
			m_account->setPluginData(m_account->protocol() , "serial" , data.section( ' ', 0, 0 ) );
			m_account->setPluginData(m_account->protocol() , "BLP" , data.section( ' ', 1, 1 ) );
		}
		else //FROM SYN
			m_account->setPluginData(m_account->protocol() , "BLP" , data.section( ' ', 0, 0 ) );
	}
	else if( cmd == "QRY" )
	{
		//do nothing
	}
	else if( cmd == "QNG" )
	{
		//this is a reply from a ping
		m_ping=false;
	}
	else if( cmd == "URL" )
	{
		//example of reply: URL 10 /cgi-bin/HoTMaiL https://msnialogin.passport.com/ppsecure/md5auth.srf?lc=1036 3
		QString from_action_url = data.section( ' ', 1, 1 );
		QString rru = data.section( ' ', 0, 0 );
		QString id = data.section( ' ', 2, 2 );

		//write the tmp file
		QString UserID=m_account->accountId();

		time_t actualTime;
		time(&actualTime);
		QString sl = QString::number( ( unsigned long ) actualTime - m_loginTime.toULong() );

		QString md5this( m_MSPAuth + sl + m_password );
		KMD5 md5( md5this.utf8() );

		QString hotmailRequest = "<html>\n"
			"<head>\n"
				"<noscript>\n"
					"<meta http-equiv=Refresh content=\"0; url=http://www.hotmail.com\">\n"
				"</noscript>\n"
			"</head>\n"
			"<body onload=\"document.pform.submit(); \">\n"
				"<form name=\"pform\" action=\"" + from_action_url  + "\" method=\"POST\">\n"
					"<input type=\"hidden\" name=\"mode\" value=\"ttl\">\n"
					"<input type=\"hidden\" name=\"login\" value=\"" + UserID.left( UserID.find('@') ) + "\">\n"
					"<input type=\"hidden\" name=\"username\" value=\"" + UserID + "\">\n"
					"<input type=\"hidden\" name=\"sid\" value=\"" + m_sid + "\">\n"
					"<input type=\"hidden\" name=\"kv\" value=\"" + m_kv + "\">\n"
					"<input type=\"hidden\" name=\"id\" value=\""+ id +"\">\n"
					"<input type=\"hidden\" name=\"sl\" value=\"" + sl +"\">\n"
					"<input type=\"hidden\" name=\"rru\" value=\"" + rru + "\">\n"
					"<input type=\"hidden\" name=\"auth\" value=\"" + m_MSPAuth + "\">\n"
					"<input type=\"hidden\" name=\"creds\" value=\"" + QString::fromLatin1( md5.hexDigest() ) + "\">\n"
					"<input type=\"hidden\" name=\"svc\" value=\"mail\">\n"
					"<input type=\"hidden\" name=\"js\" value=\"yes\">\n"
				"</form></body>\n</html>\n";


		KTempFile tmpFile( locateLocal( "tmp", "kopetehotmail-" ), ".html" );
		*tmpFile.textStream() << hotmailRequest;

		// In KDE 3.1 and older this will leave a stale temp file lying
		// around. There's no easy way for us to detect the browser exiting
		// though, so we can't do anything about it. For KDE 3.2 and newer
		// we use the improved KRun that auto-deletes the temp file when done.
		#if KDE_IS_VERSION(3,1,90)
		KRun::runURL( tmpFile.name(), "text/html", true );
		#else
		KRun::runURL( tmpFile.name(), "text/html" );
		#endif
	}
	else
	{
		// Let the base class handle the rest
		MSNAuthSocket::parseCommand( cmd, id, data );
	}
}


void MSNNotifySocket::slotAuthJobDataReceived ( KIO::Job */*job*/,const  QByteArray &data)
{
	m_authData += QCString( data, data.size()+1 );
//	kdDebug(14140) << "MSNNotifySocket::slotAuthJobDataReceived: " << data << endl;
}

void MSNNotifySocket::slotAuthJobDone ( KIO::Job *job)
{
	kdDebug(14140) << "MSNNotifySocket::slotAuthJobDone: "<< m_authData << endl;

	if(job->error())
	{
		//FIXME: Shouldn't we say that we are the MSN plugin?
		job->showErrorDialog();
		disconnect();
	}

	if(m_kv.isNull())
	{
		QStringList cookielist=QStringList::split("\n", job->queryMetaData("setcookies") );
		QString cookies="Cookie: ";
		for ( QStringList::Iterator it = cookielist.begin(); it != cookielist.end(); ++it )
		{
			kdDebug(14140) << "MSNNotifySocket::slotAuthJobDone: cl: " << *it << endl;
			QRegExp rx("Set-Cookie: ([^;]*)");
			rx.search(*it);
			cookies+=rx.cap(1)+";";
		}
		kdDebug(14140) << "MSNNotifySocket::slotAuthJobDone: cookie: " << cookies << endl;

		//QRegExp rx("lc=([1-9]*),id=([1-9]*),tw=([1-9]*),fs=[1-9]*,ru=[1-9a-zA-Z%]*,ct=[1-9]*,kpp=[1-9]*,kv=([1-9]*),");
		QRegExp rx("lc=([0-9]*),id=([0-9]*),tw=([0-9]*),.*kv=([0-9]*),");
		rx.search(m_authData);

		QString authURL = "https://" + m_sid + "/ppsecure/post.srf?lc=" + rx.cap( 1 ) + "&id=" +
			rx.cap( 2 ) + "&tw=" + rx.cap( 3 ) + "&cbid=" + rx.cap( 2 ) + "&da=passport.com&login=" +
			m_account->accountId() + "&domain=passport.com&passwd=";

		kdDebug( 14140 ) << "MSNNotifySocket::slotAuthJobDone: " << authURL << "(*******)" << endl;

		m_authData = QString::null;
		m_kv=rx.cap(4);
		if(m_kv.isNull()) m_kv="";

		authURL += escape( m_password );
		job = KIO::get( authURL, false, false );
		job->addMetaData("cookies", "manual");
		job->addMetaData("setcookies", cookies);

		QObject::connect( job, SIGNAL(data( KIO::Job *,const QByteArray&)), this, SLOT(slotAuthJobDataReceived( KIO::Job *,const QByteArray&)) );
		QObject::connect( job, SIGNAL(result( KIO::Job *)), this, SLOT(slotAuthJobDone( KIO::Job *)) );
	}
	else
	{
		if(m_authData.contains("CookiesDisabled"))
		{
			disconnect();
			KMessageBox::error( 0, "Impossible to connect to MSN Network.\n"
							"Your Web browser options are currently set to disable cookies.\n"
							"To use .NET Passport, you must enable cookies at least for the passport.com domain", i18n( "MSN Plugin" ) );
			return;
		}

		QRegExp rx(/*URL=http://memberservices.passport.net/memberservice.srf*/"\\?did=[0-9]*&(t=[0-9A-Za-z!$*]*&p=[0-9A-Za-z!$*]*)\"");
		rx.search(m_authData);

		m_badPassword=true;  //if this disconnect, that mean the password was bad
		sendCommand("USR" , "TWN S " + rx.cap(1));
	}
}


void MSNNotifySocket::slotOpenInbox()
{
	sendCommand("URL", "INBOX" );
}

void MSNNotifySocket::sendMail(const QString &email)
{
	sendCommand("URL", QString("COMPOSE " + email).utf8() );
}

void MSNNotifySocket::slotReadMessage( const QString &msg )
{
	if(msg.contains("text/x-msmsgsinitialemailnotification"))
	{
		 //this sends the server if we are going online, contains the unread message count
		QString m = msg.right(msg.length() - msg.find("Inbox-Unread:") );
		m = m.left(msg.find("\r\n"));
		mailCount = m.right(m.length() -m.find(" ")-1).toUInt();

		if(mailCount > 0 )
		{
			KNotifyClient::event( 0 , "msn_mail" , i18n( "You have one unread message in your MSN inbox.", "You have %n unread messages in your MSN inbox." , mailCount),
				i18n("Open &inbox") , this , SLOT(slotOpenInbox()) );
		}

	}
	else if(msg.contains("text/x-msmsgsactivemailnotification"))
	{
		 //this sends the server if mails are deleted
		 QString m = msg.right(msg.length() - msg.find("Message-Delta:") );
		 m = m.left(msg.find("\r\n"));
		 mailCount = mailCount - m.right(m.length() -m.find(" ")-1).toUInt();
	}
	else if(msg.contains("text/x-msmsgsemailnotification"))
	{
		 //this sends the server if a new mail has arrived
		QRegExp rx("From-Addr: ([A-Za-z0-9@._\\-]*)");
		rx.search(msg);
		QString m=rx.cap(1);

		mailCount++;

		KNotifyClient::event( 0 , "msn_mail" , i18n( "You have one new email from %1 in your MSN inbox." ).arg(m) ,
			i18n("Open &inbox") , this , SLOT(slotOpenInbox()) );
	}
	else if(msg.contains("text/x-msmsgsprofile"))
	{
		//Hotmail profile
		if(msg.contains("MSPAuth:"))
		{
			QRegExp rx("MSPAuth: ([A-Za-z0-9$!*]*)");
			rx.search(msg);
			m_MSPAuth=rx.cap(1);
		}
		if(msg.contains("sid:"))
		{
			QRegExp rx("sid: ([0-9]*)");
			rx.search(msg);
			m_sid=rx.cap(1);
		}
		if(msg.contains("kv:"))
		{
			QRegExp rx("kv: ([0-9]*)");
			rx.search(msg);
			m_kv=rx.cap(1);
		}
		if(msg.contains("LoginTime:"))
		{
			QRegExp rx("LoginTime: ([0-9]*)");
			rx.search(msg);
			m_loginTime=rx.cap(1);
		}
			else //IN MSNP9  there are no logintime it seems, so set it manualy
			{
				time_t actualTime;
				time(&actualTime);
				m_loginTime=QString::number((unsigned long)actualTime);
			}
		if(msg.contains("EmailEnabled:"))
		{
			QRegExp rx("EmailEnabled: ([0-9]*)");
			rx.search(msg);
			m_isHotmailAccount = (rx.cap(1).toUInt() == 1);
			emit hotmailSeted(m_isHotmailAccount);
		}
	}
}

void MSNNotifySocket::addGroup(const QString& groupName)
{
	// escape spaces
	sendCommand( "ADG", escape( groupName ) + " 0" );
}

void MSNNotifySocket::renameGroup( const QString& groupName, uint group )
{
	// escape spaces
	sendCommand( "REG", QString::number( group ) + " " +
		escape( groupName ) + " 0" );
}

void MSNNotifySocket::removeGroup( uint group )
{
	sendCommand( "RMG", QString::number( group ) );
}

void MSNNotifySocket::addContact( const QString &handle, const QString& publicName, uint group, int list )
{
	m_tmpLastHandle=handle;
	QString args;
	switch( list )
	{
	case MSNProtocol::FL:
		args = "FL " + handle + " " + escape( publicName ) + " " + QString::number( group );
		break;
	case MSNProtocol::AL:
		args = "AL " + handle + " " + escape( publicName );
		break;
	case MSNProtocol::BL:
		args = "BL " + handle + " " + escape( publicName );
		break;
	default:
		kdDebug(14140) << "MSNNotifySocket::addContact: WARNING! Unknown list " <<
			list << "!" << endl;
		return;
	}
	sendCommand( "ADD", args );
}

void MSNNotifySocket::removeContact( const QString &handle, uint group,	int list )
{
	m_tmpLastHandle=handle;
	QString args;
	switch( list )
	{
	case MSNProtocol::FL:
		args = "FL " + handle + " " + QString::number( group );
		break;
	case MSNProtocol::AL:
		args = "AL " + handle;
		break;
	case MSNProtocol::BL:
		args = "BL " + handle;
		break;
	default:
		kdDebug(14140) << "MSNNotifySocket::removeContact: " <<
			"WARNING! Unknown list " << list << "!" << endl;
		return;
	}
	sendCommand( "REM", args );
}

void MSNNotifySocket::setStatus( const KopeteOnlineStatus &status )
{
	kdDebug( 14140 ) << k_funcinfo << statusToString( status ) << endl;

	if( onlineStatus() == Disconnected )
		m_newstatus = status;
	else
		sendCommand( "CHG", statusToString( status ) + " 268435492 " + escape(m_account->pictureObject()) );
}

void MSNNotifySocket::changePublicName(  QString publicName, const QString &handle )
{
	if( escape(publicName).length() > 387 )
	{
		publicName=publicName.left(387);
	}

	m_tmpLastHandle=handle;
	if( handle.isNull() )
	{
		sendCommand( "REA", msnId() + " " + escape ( publicName ) );
		m_tmpLastHandle=msnId();
	}
	else
		sendCommand( "REA", handle + " " + escape ( publicName ) );
}

void MSNNotifySocket::changePhoneNumber( const QString &key, const QString &data )
{
	sendCommand( "PRP", key + " " + escape ( data ) );
}


void MSNNotifySocket::createChatSession()
{
	sendCommand( "XFR", "SB" );
}

QString MSNNotifySocket::statusToString( const KopeteOnlineStatus &status ) const
{
	if( status == MSNProtocol::protocol()->NLN )
		return "NLN";
	else if( status == MSNProtocol::protocol()->BSY )
		return "BSY";
	else if( status == MSNProtocol::protocol()->BRB )
		return "BRB";
	else if( status == MSNProtocol::protocol()->AWY )
		return "AWY";
	else if( status == MSNProtocol::protocol()->PHN )
		return "PHN";
	else if( status == MSNProtocol::protocol()->LUN )
		return "LUN";
	else if( status == MSNProtocol::protocol()->FLN )
		return "FLN";
	else if( status == MSNProtocol::protocol()->HDN )
		return "HDN";
	else if( status == MSNProtocol::protocol()->IDL )
		return "IDL";
	else
	{
		kdWarning( 14140 ) << k_funcinfo << "Unknown status " << status.internalStatus() << "!" << endl;
		return "UNK";
	}
}

void MSNNotifySocket::slotDispatchClosed()
{
	m_badPassword = m_dispatchSocket->badPassword();
	delete m_dispatchSocket;
	m_dispatchSocket = 0L;
	if(!dispatchOK)
	{
		if(!badPassword())
			KMessageBox::error( 0, i18n( "Connection failed.\nTry again later." ) , i18n ("MSN Plugin") );
		//because "this socket" isn't already connected, doing this manualy
		emit onlineStatusChanged( Disconnected );
		emit socketClosed(-1);
	}
}

void MSNNotifySocket::slotSendKeepAlive()
{
	//we did not received the previous QNG
	if(m_ping)
	{
		disconnect();
		KMessageBox::information( 0, i18n( "Connection with the MSN network has been lost" ) , i18n ("MSN Plugin") );
		return;
	}
	else
	{
		// Send a dummy command to fake activity. This makes sure MSN doesn't
		// disconnect you when the notify socket is idle.
		sendCommand( "PNG" , QString::null , false );
		m_ping=true;
	}
}

void MSNNotifySocket::slotResetKeepAlive()
{
	// Fire the timer every 60 seconds. QTimer will reset a running timer
	// on a subsequent call if there has been activity again.
	m_keepaliveTimer->start( 60000 );
}

KopeteOnlineStatus MSNNotifySocket::convertOnlineStatus( const QString &status )
{
	if( status == "NLN" )
		return MSNProtocol::protocol()->NLN;
	else if( status == "FLN" )
		return MSNProtocol::protocol()->FLN;
	else if( status == "HDN" )
		return MSNProtocol::protocol()->HDN;
	else if( status == "PHN" )
		return MSNProtocol::protocol()->PHN;
	else if( status == "LUN" )
		return MSNProtocol::protocol()->LUN;
	else if( status == "BRB" )
		return MSNProtocol::protocol()->BRB;
	else if( status == "AWY" )
		return MSNProtocol::protocol()->AWY;
	else if( status == "BSY" )
		return MSNProtocol::protocol()->BSY;
	else if( status == "IDL" )
		return MSNProtocol::protocol()->IDL;
	else
		return MSNProtocol::protocol()->UNK;
}

#include "msnnotifysocket.moc"

// vim: set noet ts=4 sts=4 sw=4:

