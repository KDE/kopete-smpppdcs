/*
	Kopete Oscar Protocol
	client.h - The main interface for the Oscar protocol

	Copyright (c) 2004-2005 by Matt Rogers <mattr@kde.org>

	Based on code Copyright (c) 2004 SuSE Linux AG <http://www.suse.com>
	Based on Iris, Copyright (C) 2003  Justin Karneges

	Kopete (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

	*************************************************************************
	*                                                                       *
	* This library is free software; you can redistribute it and/or         *
	* modify it under the terms of the GNU Lesser General Public            *
	* License as published by the Free Software Foundation; either          *
	* version 2 of the License, or (at your option) any later version.      *
	*                                                                       *
	*************************************************************************
*/


#ifndef LIBOSCAR_CLIENT_H
#define LIBOSCAR_CLIENT_H

#include <qobject.h>
#include <qstring.h>
#include <kopete_export.h>
#include "rtf2html.h"
#include "transfer.h"
#include "icquserinfo.h"
#include "userdetails.h"

class Connection;
class StageOneLoginTask;
class StageTwoLoginTask;
class SSIManager;
class UserDetails;
class QString;
class Task;

using namespace Oscar;

class KOPETE_EXPORT Client : public QObject
{
Q_OBJECT

public:

	enum ErrorCodes {
		NoError = 0,
		NotConnectedError = 1,
		NonFatalProtocolError = 2,
		FatalProtocolError = 3
	};
			
	/*************
	  EXTERNAL API 
	 *************/

	Client(QObject *parent=0);
	~Client();

	/**
	 * Start a connection to the server using the supplied @ref ClientStream.
	 * This is only a transport layer connection.
	 * @param s initialised connection object to use for the connection.
	 * @param server the server to connect to - but this is also set on the connector used to construct the clientstream??
	 * @param auth indicate whether we're connecting to the authorizer or the bos server
	 */
	void connectToServer( Connection *c, const QString& server, bool auth = true );

	/**
	 * Start the login process for Oscar
	 * @param host - probably could obtain this back from the connector - used for outgoing tasks to determine destination
	 * @param user The user name to log in as.
	 * @param pass The password to use when logging in
	 */ 
	void start( const QString &host, const uint port, const QString &userId, const QString &pass );

	/** Logout and disconnect */
	void close();

	enum AIMStatus { Online = 0, Away };	
	/** Set our status for AIM */
	void setStatus( AIMStatus status, const QString &message = QString::null );
	/** Set our status for ICQ */
 	void setStatus( DWORD status, const QString &message = QString::null );
	
	/** Retrieve our user info */
	UserDetails ourInfo() const;

	/**
	 * Remove a group to the contact list
	 * \param groupName the name of the group to remove
	 * \return true if the group removal was successful
	 */
	void removeGroup( const QString& groupName );
	
	/**
	 * Add a group from the contact list
	 * \param groupName the name of the group to add
	 * \return true if the group addition was successful
	 */
	void addGroup( const QString& groupName );
	
	/**
	 * Add a contact to the contact list
	 * \param contactName the screen name of the new contact to add
	 * \return true if the contact addition was successful
	 */
	void addContact( const QString& contactName, const QString& groupName );
	
	/**
	 * Remove a contact from the contact list
	 * \param contactName the screen name of the contact to remove
	 * \return true if the contact removal was successful
	 */
	void removeContact( const QString &contactName );
	
	/**
	 * Rename a group on the contact list
	 * \param oldGroupName the old group name
	 * \param newGroupName the new group name
	 */
	void renameGroup( const QString& oldGroupName, const QString& newGroupName );
	
	/**
	 * Change a contact's group on the server
	 * \param contact the contact to change
	 * \param newGroup the new group to move the contact to
	 */
	void changeContactGroup( const QString& contact, const QString& newGroupName );
	
	/** 
	 * Send a message to a contact
	 * \param msg the message to be sent
	 */
	void sendMessage( const Oscar::Message& msg );
	
	/**
	 * Request authorization from a contact
	 * \param contactid the id of the contact to request auth from
	 * \param reason the reason for this authorization request
	 */
	void requestAuth( const QString& contactid, const QString& reason );
	
	/**
	 * Grant or decline authorization to a contact
	 * \param contactid the id of the contact to grant/decline authorization
	 * \param reason the reason to grant/decline authorization
	 * \param auth grant or decline authorization
	 */
	void sendAuth( const QString& contactid, const QString& reason, bool auth=true );
	
	/**
	 * Request full user info from an ICQ contact
	 * \param contactId the UIN of the contact to get info for
	 */
	void requestFullInfo( const QString& contactId );
	
	/**
	 * Request short info for an ICQ contact
	 * \param contactId the UIN of the contact to get info for
	 */
	void requestShortInfo( const QString& contactId );
	
	/**
	 * Send a warning to the OSCAR servers about a contact
	 * \param contact the contact to send the warning to
	 * \param anon indicate whether to do it anonymously
	 */
	void sendWarning( const QString& contact, bool anonymous );
	
	/**
	 * Get the general ICQ info for a client
	 * \param contact the contact to get info for
	 */
	ICQGeneralUserInfo getGeneralInfo( const QString& contact );
	
	/**
	 * Get the work info for a contact
	 * \param contact the contact to get info for
	 */
	ICQWorkUserInfo getWorkInfo( const QString& contact );
	
	/**
	 * Get the email info for a contact
	 * \param contact the contact to get info for
	 */
	ICQEmailInfo getEmailInfo( const QString& contact );
	
	/**
	 * Get the additional info available for a contact
	 * \param contact the contact to get info for
	 */
	ICQMoreUserInfo getMoreInfo( const QString& contact );
	
	/**
	 * Get the short info available for an icq contact
	 * \param contact the contact to get info for
	 */
	ICQShortInfo getShortInfo( const QString& contact );
	
	/**
	 * Request the aim profile
	 * \param contact the contact to get info for
	 */
	void requestAIMProfile( const QString& contact );
	
	/**
	 * Request the aim away message
	 * \param contact the contact to get info for
	 */
	void requestAIMAwayMessage( const QString& contact );
	
	/** Request the extended status info */
	void requestStatusInfo( const QString& contact );
	
	void whitePagesSearch( const ICQWPSearchInfo& info );
	void uinSearch( const QString& uin );
	
	/** Accessors needed for login */
	QString host();
	int port();

	/*************
	  INTERNAL (FOR USE BY TASKS) METHODS 
	 *************/
	/**
	 * Print a debug statement
	 */
	void debug( const QString &str );

	/** Have we logged in yet? */
	bool isActive() const;

	/** Accessor for the SSI Manager */
	SSIManager* ssiManager() const;
	
	/** The current user's user ID */
	QString userId() const;

	/** The current user's password */
	QString password() const;

	/** ICQ Settings */
	bool isIcq() const;
	void setIsIcq( bool isIcq );
	
	/** Host's IP address */
	QCString ipAddress() const;
	
signals:
	/** CONNECTION EVENTS */

	/** Notifies that the login process has succeeded. */
	void loggedIn();

	/** Notifies that the login process has failed */
	void loginFailed();

	/** Notifies tasks and account so they can react properly */
	void disconnected();

	/** We were disconnected because we connected elsewhere */
	void connectedElsewhere();

	/**
	 * A protocol error happened
	 * \param code the generic error code for this error
	 * \param psError the protocol specific error code for this error
	 * \param error the i18n'ed message for the erro
	 */
	void error( int code, int psError, const QString& error );
	
	/** We have our own user info */
	void haveOwnInfo();
	
	/** We have our SSI list */
	void haveSSIList();
	
	/** a user is online. */
	void userIsOnline( const QString& );
	
	/** a user is offline. */
	void userIsOffline( const QString& );
	
	/** we've received a message */
	void messageReceived( const Oscar::Message& );
	
	/** we've received an authorization request */
	void authRequestReceived( const QString& contact, const QString& reason );
	
	/** we've received an authorization reply */
	void authReplyReceived( const QString& contact, const QString& reason, bool auth );
	
	void receivedIcqShortInfo( const QString& contact );
	void receivedIcqLongInfo( const QString& contact );
	
	void receivedProfile( const QString& contact, const QString& profile );
	void receivedAwayMessage( const QString& contact, const QString& message );
	void receivedUserInfo( const QString& contact, const UserDetails& details );
	
	/** We warned a user */
	void userWarned( const QString& contact, Q_UINT16 increase, Q_UINT16 newLevel );
	
	/** Search signals */
	void gotSearchResults( const ICQSearchResult& );
	void endOfSearch( int);
	
protected slots:
	// INTERNAL, FOR USE BY TASKS' finished() SIGNALS //

	/** Singleshot timer to start stage two login */
	void startStageTwo();

	/**
	 * A login task finished. For stage one, this means we've either errored
	 * out, or gotten a cookie. For stage two, this means we've either done
	 * something wrong, or we're successfully connected
	 */
	void lt_loginFinished();

	/** Stream connected for stage two login */
	void streamConnected();

	/**
	 * Used by the client stream to notify errors to upper layers.
	 */
	void streamError( int error );
	
	/** We have our own user info */
	void haveOwnUserInfo();
	
	/** Service setup finished */
	void serviceSetupFinished();
	
	/** we have icq info for a contact */
	void receivedIcqInfo( const QString& contact, unsigned int type );
	
	/** we have normal user info for a contact */
	void receivedInfo( Q_UINT16 sequence );
	
	void disconnectionError( int, const QString& );
	
	void offlineUser( const QString&, const UserDetails& );
	
private:
	/** Delete the connections */
	void deleteConnections();
	
	/** Initialize some static tasks */
	void initializeStaticTasks();
	
private:
	class ClientPrivate;
	ClientPrivate* d;

	StageOneLoginTask* m_loginTask;
	StageTwoLoginTask* m_loginTaskTwo;
};

#endif

//kate: tab-width 4; indent-mode csands;

