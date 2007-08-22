/*
	smpppdunsettled.cpp

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

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <openssl/md5.h>

#include <qregexp.h>

#include <kdebug.h>
#include <k3streamsocket.h>

#include "smpppdready.h"
#include "smpppdunsettled.h"

using namespace SMPPPD;

Unsettled * Unsettled::m_instance = NULL;

Unsettled::Unsettled() {}

Unsettled::~Unsettled() {}

Unsettled * Unsettled::instance()
{
	if ( !m_instance )
	{
		m_instance = new Unsettled();
	}

	return m_instance;
}

bool Unsettled::connect ( Client * client, const QString& server, uint port )
{
	if ( !socket ( client ) ||
	        socket ( client )->state() != KNetwork::KStreamSocket::Connected ||
	        socket ( client )->state() != KNetwork::KStreamSocket::Connecting )
	{

		QString resolvedServer = server;

		changeState ( client, Ready::instance() );
		disconnect ( client );

		// since a lookup on a non-existant host can take a lot of time we
		// try to get the IP of server before and we do the lookup ourself
		KNetwork::KResolver resolver ( server );
		resolver.start();
		if ( resolver.wait ( 500 ) )
		{
			KNetwork::KResolverResults results = resolver.results();
			if ( !results.empty() )
			{
				QString ip = results[0].address().asInet().ipAddress().toString();
				kDebug ( 14312 ) << "Found IP-Address for " << server << ": " << ip;
				resolvedServer = ip;
			}
			else
			{
				kWarning ( 14312 ) << "No IP-Address found for " << server;
				return false;
			}
		}
		else
		{
			kWarning ( 14312 ) << "Looking up hostname timed out, consider to use IP or correct host";
			return false;
		}

		setSocket ( client, new KNetwork::KStreamSocket ( resolvedServer, QString::number ( port ) ) );
		socket ( client )->setBlocking ( true );

		if ( !socket ( client )->connect() )
		{
			kDebug ( 14312 ) << "Socket Error: " << socket ( client )->errorString();
		}
		else
		{
			kDebug ( 14312 ) << "Successfully connected to smpppd \"" << server << ":" << port << "\"";

			static QString verRex = "^SuSE Meta pppd \\(smpppd\\), Version (.*)$";
			static QString clgRex = "^challenge = (.*)$";

			QRegExp ver ( verRex );
			QRegExp clg ( clgRex );

			QString response = read ( client ) [0];

			if ( !response.isNull() &&
			        ver.exactMatch ( response ) )
			{
				setServerID ( client, response );
				setServerVersion ( client, ver.cap ( 1 ) );
				changeState ( client, Ready::instance() );
				return true;
			}
			else if ( !response.isNull() &&
			          clg.exactMatch ( response ) )
			{
				if ( !password ( client ).isNull() )
				{
					// we are challenged, ok, respond
					write ( client, QString ( "response = %1\n" ).arg ( make_response ( clg.cap ( 1 ).trimmed(), password ( client ) ) ).toLatin1() );
					response = read ( client ) [0];
					if ( ver.exactMatch ( response ) )
					{
						setServerID ( client, response );
						setServerVersion ( client, ver.cap ( 1 ) );
						return true;
					}
					else
					{
						kWarning ( 14312 ) << "SMPPPD responded: " << response;
						changeState ( client, Ready::instance() );
						disconnect ( client );
					}
				}
				else
				{
					kWarning ( 14312 ) << "SMPPPD requested a challenge, but no password was supplied!";
					changeState ( client, Ready::instance() );
					disconnect ( client );
				}
			}
		}
	}

	return false;
}

QString Unsettled::make_response ( const QString& chex, const QString& password ) const
{

	int size = chex.length ();
	if ( size & 1 )
		return "error";
	size >>= 1;

	// convert challenge from hex to bin
	QString cbin;
	for ( int i = 0; i < size; i++ )
	{
		QString tmp = chex.mid ( 2 * i, 2 );
		cbin.append ( ( char ) strtol ( tmp.ascii (), 0, 16 ) );
	}

	// calculate response
	unsigned char rbin[MD5_DIGEST_LENGTH];
	MD5state_st md5;
	MD5_Init ( &md5 );
	MD5_Update ( &md5, cbin.ascii (), size );
	MD5_Update ( &md5, password.toAscii(), password.length () );
	MD5_Final ( rbin, &md5 );

	// convert response from bin to hex
	QString rhex;
	for ( int i = 0; i < MD5_DIGEST_LENGTH; i++ )
	{
		char buffer[3];
		snprintf ( buffer, 3, "%02x", rbin[i] );
		rhex.append ( buffer );
	}

	return rhex;
}
