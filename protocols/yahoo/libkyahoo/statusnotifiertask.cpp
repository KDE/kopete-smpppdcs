/*
    Kopete Yahoo Protocol
    Notifies about status changes of buddies

    Copyright (c) 2005 André Duffeck <andre.duffeck@kdemail.net>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "statusnotifiertask.h"
#include "transfer.h"
#include "ymsgtransfer.h"
#include "yahootypes.h"
#include "client.h"
#include <qstring.h>
#include <qstringlist.h>
#include <kdebug.h>

StatusNotifierTask::StatusNotifierTask(Task* parent) : Task(parent)
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
}

StatusNotifierTask::~StatusNotifierTask()
{

}

bool StatusNotifierTask::take( Transfer* transfer )
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	
	if ( !forMe( transfer ) )
		return false;
	
	YMSGTransfer *t = static_cast<YMSGTransfer*>(transfer);

	if( t->service() == Yahoo::ServiceStealthOffline )
		parseStealthStatus( transfer );
	else if( t->service() == Yahoo::ServiceAuthorization )
		parseAuthorization( transfer );
	else
		parseStatus( transfer );	

	return true;
}

bool StatusNotifierTask::forMe( Transfer* transfer ) const
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	YMSGTransfer *t = 0L;
	t = dynamic_cast<YMSGTransfer*>(transfer);
	if (!t)
		return false;


	if ( t->service() == Yahoo::ServiceLogon ||
		t->service() == Yahoo::ServiceLogoff ||
		t->service() == Yahoo::ServiceIsAway ||
		t->service() == Yahoo::ServiceIsBack ||
		t->service() == Yahoo::ServiceGameLogon ||
		t->service() == Yahoo::ServiceGameLogoff ||
		t->service() == Yahoo::ServiceIdAct ||
		t->service() == Yahoo::ServiceIddeAct ||
		t->service() == Yahoo::ServiceStatus ||
		t->service() == Yahoo::ServiceStealthOffline ||
		t->service() == Yahoo::ServiceAuthorization
	)
		return true;
	else
		return false;
}

void StatusNotifierTask::parseStatus( Transfer* transfer )
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	YMSGTransfer *t = 0L;
	t = dynamic_cast<YMSGTransfer*>(transfer);
	if (!t)
		return;

	if( t->status() == Yahoo::StatusDisconnected && 
		t->service() == Yahoo::ServiceLogoff )
	{
		emit loginResponse( Yahoo::LoginDupl, QString::null );
	}

	QString	myNick;		/* key = 1 */
	QString customError;	/* key = 16  */
	QString nick;		/* key = 7  */
	int state;		/* key = 10  */
	QString message;	/* key = 19  */
	int flags;		/* key = 13  */
	int away;		/* key = 47  */
	int idle;		/* key = 137 */

	customError = t->firstParam( 16 );
	if( !customError.isEmpty() )
		emit error( customError );

	myNick = t->firstParam( 1 );
	
	for( int i = 0; i < t->paramCount( 7 ); ++i)
	{
		nick = t->nthParam( 7, i );
		state = t->nthParamSeparated( 10, i, 7 ).toInt();
		flags = t->nthParamSeparated( 13, i, 7 ).toInt();
		message = t->nthParamSeparated( 19, i, 7 );
		away = t->nthParamSeparated( 47, i, 7 ).toInt();
		idle = t->nthParamSeparated( 137, i, 7 ).toInt();

		if( t->service() == Yahoo::ServiceLogoff || ( state != 0 && flags == 0 ) )
			emit statusChanged( nick, Yahoo::StatusOffline, QString::null, 0, 0 );
		else
			emit statusChanged( nick, state, message, away, idle );
	}
}

void StatusNotifierTask::parseAuthorization( Transfer* transfer )
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	YMSGTransfer *t = 0L;
	t = dynamic_cast<YMSGTransfer*>(transfer);
	if (!t)
		return;

	QString nick;		/* key = 4  */	
	QString msg;		/* key = 14  */
	int state;		/* key = 13  */
	bool utf;		/* key = 97 */

	utf = t->firstParam( 97 ).toInt() == 1;
	nick = t->firstParam( 4 );
	if( utf )
		msg = QString::fromUtf8( t->firstParam( 14 ) );
	else
		msg = t->firstParam( 14 );
	state = t->firstParam( 13 ).toInt();

	if( state == 1 )
	{
		emit( authorizationAccepted( nick ) );
	}
	else if( state == 2 )
	{
		emit( authorizationRejected( nick, msg ) );
	}
	else	// This is a request
	{
		QString fname = t->firstParam( 216 );
		QString lname = t->firstParam( 254 );
		QString name;
		if( !fname.isEmpty() || !lname.isEmpty() )
			name = QString("%1 %2").arg(fname).arg(lname);

		kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << "Emitting gotAuthorizationRequest( " << nick<< ", " << msg << ", " << name << " )" << endl;
		emit gotAuthorizationRequest( nick, msg, name );
	}
}

void StatusNotifierTask::parseStealthStatus( Transfer* transfer )
{
	kDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	YMSGTransfer *t = 0L;
	t = dynamic_cast<YMSGTransfer*>(transfer);
	if (!t)
		return;

	QString nick;		/* key = 7  */
	int state;		/* key = 31  */

	nick = t->firstParam( 7 );
	state = t->firstParam( 31 ).toInt();

	emit stealthStatusChanged( nick, ( state == 1 ) ? Yahoo::StealthActive : Yahoo::StealthNotActive );
}

#include "statusnotifiertask.moc"
