/*
	Kopete Oscar Protocol
	userdetails.cpp - user details from the extended status packet

	Copyright (c) 2004 by Matt Rogers <mattr@kde.org>

	Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

	*************************************************************************
	*                                                                       *
	* This library is free software; you can redistribute it and/or         *
	* modify it under the terms of the GNU Lesser General Public            *
	* License as published by the Free Software Foundation; either          *
	* version 2 of the License, or (at your option) any later version.      *
	*                                                                       *
	*************************************************************************
*/
#include "userdetails.h"

#include "buffer.h"
#include <kdebug.h>
#include <klocale.h>
#include <qptrlist.h>
#include "oscarutils.h"

using namespace Oscar;

UserDetails::UserDetails()
{
	m_warningLevel = 0;
	m_userClass = 0;
	m_idleTime = 0;
	m_extendedStatus = 0;
	m_capabilities = 0;
	m_dcOutsideIp = 0;
	m_dcInsideIp = 0;
	m_dcPort = 0;
	m_dcType = 0;
	m_dcProtoVersion = 0;
	m_dcAuthCookie = 0;
	m_dcWebFrontPort = 0;
	m_dcClientFeatures = 0;
	m_dcLastInfoUpdateTime = 0;
	m_dcLastExtInfoUpdateTime = 0;
	m_dcLastExtStatusUpdateTime = 0;
}


UserDetails::~UserDetails()
{
}

int UserDetails::warningLevel() const
{
	return m_warningLevel;
}

QString UserDetails::userId() const
{
	return m_userId;
}

WORD UserDetails::idleTime() const
{
	return m_idleTime;
}

DWORD UserDetails::dcInternalIp() const
{
	return m_dcInsideIp;
}

DWORD UserDetails::dcExternalIp() const
{
	return m_dcOutsideIp;
}

DWORD UserDetails::dcPort() const
{
	return m_dcPort;
}

QDateTime UserDetails::onlineSinceTime() const
{
	return m_onlineSince;
}

QDateTime UserDetails::memberSinceTime() const
{
	return m_memberSince;
}

int UserDetails::userClass() const
{
	return m_userClass;
}

DWORD UserDetails::extendedStatus() const
{
	return m_extendedStatus;
}

void UserDetails::fill( Buffer * buffer )
{
	BYTE snLen = buffer->getByte();
	QString user = QString( buffer->getBlock( snLen ) );
	if ( !user.isEmpty() )
		m_userId = user;
	m_warningLevel = buffer->getWord();
	WORD numTLVs = buffer->getWord();
	
	kdDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Got user info for " << user << endl;
	kdDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Warning level is " << m_warningLevel << endl;
	kdDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Parsing " << numTLVs << " TLVs" << endl;
	//start parsing TLVs
	for( int i = 0; i < numTLVs; ++i  )
	{
		TLV t = buffer->getTLV();
		if ( t )
		{
			Buffer b( t.data, t.length );
			switch( t.type )
			{
			case 0x0001: //user class
				m_userClass = b.getWord();
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "User class is " << m_userClass << endl;
				break;
			case 0x0002: //member since
			case 0x0005: //member since
				m_memberSince.setTime_t( b.getDWord() );
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Member since " << m_memberSince << endl;
				break;
			case 0x0003: //sigon time
				m_onlineSince.setTime_t( b.getDWord() );
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Signed on at " << m_onlineSince << endl;
				break;
			case 0x0004: //idle time
				m_idleTime = b.getWord();
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Idle time is " << m_idleTime << endl;
				break;
			case 0x0006: //extended user status
				m_extendedStatus = b.getDWord();
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Extended status is " << QString::number( m_extendedStatus, 16 ) << endl;
				break;
			case 0x000A: //external IP address
				m_dcOutsideIp = b.getDWord();
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "External IP address is " << m_dcOutsideIp << endl;
				break;
			case 0x000C: //DC info
				m_dcInsideIp = b.getDWord();
				m_dcPort = b.getDWord();
				m_dcType = b.getByte();
				m_dcProtoVersion = b.getWord();
				m_dcAuthCookie = b.getDWord();
				m_dcWebFrontPort = b.getDWord();
				m_dcClientFeatures = b.getDWord();
				m_dcLastInfoUpdateTime = b.getDWord();
				m_dcLastExtInfoUpdateTime = b.getDWord();
				m_dcLastExtStatusUpdateTime = b.getDWord();
				b.getWord(); //unknown.
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Got DC info" << endl;
				break;
			case 0x000D: //capability info
				m_capabilities = Oscar::parseCapabilities( b, m_clientVersion );
				break;
			case 0x0010:
			case 0x000F: //online time
				m_numSecondsOnline = b.getDWord();
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Online for " << m_numSecondsOnline << endl;
				break;
			default:
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Unknown TLV, type=" << t.type << ", length=" << t.length
					<< " in userinfo" << endl;
				break;
			};
			//detach buffer and free TLV data
			b.clear();
		}
	}
	
	//do client detection on fill
	detectClient();
}

void UserDetails::detectClient()
{
	/* My thanks to mETz for stealing^Wusing this code from SIM.
	 * Client type detection ---
	 * Most of this code is based on sim-icq code
	 * Thanks a lot for all the tests you guys must have made
	 * without sim-icq I would have only checked for the capabilities
	 */
	bool clientMatched = false;
	if (m_capabilities != 0)
	{
		bool clientMatched = false;
		if (hasCap(CAP_KOPETE))
		{
			m_clientName=i18n("Kopete");
			clientMatched=true;
		}
		else if (hasCap(CAP_MICQ))
		{
			m_clientName=i18n("MICQ");
			clientMatched=true;
		}
		else if (hasCap(CAP_SIMNEW) || hasCap(CAP_SIMOLD))
		{
			m_clientName=i18n("SIM");
			clientMatched=true;
		}
		else if (hasCap(CAP_TRILLIANCRYPT) || hasCap(CAP_TRILLIAN))
		{
			m_clientName=i18n("Trillian");
			clientMatched=true;
		}
		else if (hasCap(CAP_MACICQ))
		{
			m_clientName=i18n("MacICQ");
			clientMatched=true;
		}
		else if ((m_dcLastInfoUpdateTime & 0xFF7F0000L) == 0x7D000000L)
		{
			unsigned ver = m_dcLastInfoUpdateTime & 0xFFFF;
			if (m_dcLastInfoUpdateTime & 0x00800000L)
				m_clientName=i18n("Licq SSL");
			else
				m_clientName=i18n("Licq");
			
			if (ver % 10)
				m_clientVersion.sprintf("%d%d%u", ver/1000, (ver/10)%100, ver%10);
			else
				m_clientVersion.sprintf("%d%u", ver/1000, (ver/10)%100);
			clientMatched=true;
		}
		else // some client we could not detect using capabilities
		{
		
			clientMatched=true; // default case will set it to false again if we did not find anything
			switch (m_dcLastInfoUpdateTime)
			{
			case 0xFFFFFFFFL: //gaim behaves like official AIM so we can't detect them, only look for miranda
				{
					if (m_dcLastExtStatusUpdateTime & 0x80000000)
						m_clientName=QString::fromLatin1("Miranda alpha");
					else
						m_clientName=QString::fromLatin1("Miranda");
					
					DWORD version = (m_dcLastExtInfoUpdateTime & 0xFFFFFF);
					BYTE major1 = ((version >> 24) & 0xFF);
					BYTE major2 = ((version >> 16) & 0xFF);
					BYTE minor1 = ((version >> 8) & 0xFF);
					BYTE minor2 = (version & 0xFF);
					if (minor2 > 0) // w.x.y.z
					{
						m_clientVersion.sprintf("%u.%u.%u.%u", major1, major2,
												minor1, minor2);
					}
					else if (minor1 > 0)  // w.x.y
					{
						m_clientVersion.sprintf("%u.%u.%u", major1, major2, minor1);
					}
					else // w.x
					{
						m_clientVersion.sprintf("%u.%u", major1, major2);
					}
				}
				break;
			case 0xFFFFFF8FL:
				m_clientName = QString::fromLatin1("StrICQ");
				break;
			case 0xFFFFFF42L:
				m_clientName = QString::fromLatin1("mICQ");
				break;
			case 0xFFFFFFBEL:
				m_clientName = QString::fromLatin1("alicq");
				break;
			case 0xFFFFFF7FL:
				m_clientName = QString::fromLatin1("&RQ");
				break;
			case 0xFFFFFFABL:
				m_clientName = QString::fromLatin1("YSM");
				break;
			case 0x3AA773EEL:
				if ((m_dcLastExtStatusUpdateTime == 0x3AA66380L) &&
				    (m_dcLastExtInfoUpdateTime == 0x3A877A42L))
				{
					m_clientName=QString::fromLatin1("libicq2000");
				}
				break;
			default:
				clientMatched=false;
				break;
			}
		}
	}
	
	if (!clientMatched) // now the fuzzy clientsearch starts =)
	{
		if (hasCap(CAP_TYPING))
		{
			kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Client protocol version = " << m_dcProtoVersion << endl;
			switch (m_dcProtoVersion)
			{
			case 10:
				m_clientName=QString::fromLatin1("ICQ 2003b");
				break;
			case 9:
				m_clientName=QString::fromLatin1("ICQ Lite");
				break;
			default:
				m_clientName=QString::fromLatin1("ICQ2go");
			}
		}
		else if (hasCap(CAP_BUDDYICON)) // only gaim seems to advertize this on ICQ
		{
			m_clientName = QString::fromLatin1("Gaim");
		}
		else if (hasCap(CAP_XTRAZ))
		{
			m_clientName = QString::fromLatin1("ICQ 4.0 Lite");
		}
		else if ((hasCap(CAP_STR_2001) || hasCap(CAP_ICQSERVERRELAY)) &&
		         hasCap(CAP_IS_2001))
		{
			m_clientName = QString::fromLatin1( "ICQ 2001");
		}
		else if ((hasCap(CAP_STR_2001) || hasCap(CAP_ICQSERVERRELAY)) &&
		         hasCap(CAP_STR_2002))
		{
			m_clientName = QString::fromLatin1("ICQ 2002");
		}
		else if (hasCap(CAP_RTFMSGS) && hasCap(CAP_UTF8) &&
		         hasCap(CAP_ICQSERVERRELAY) && hasCap(CAP_ISICQ))
		{
			m_clientName = QString::fromLatin1("ICQ 2003a");
		}
		else if (hasCap(CAP_ICQSERVERRELAY) && hasCap(CAP_ISICQ))
		{
			m_clientName =QString::fromLatin1("ICQ 2001b");
		}
		else if ((m_dcProtoVersion == 7) && hasCap(CAP_RTFMSGS))
		{
			m_clientName = QString::fromLatin1("GnomeICU");
		}
	}
}

bool UserDetails::hasCap( int capNumber )
{
	return ( m_capabilities & ( 1 << capNumber ) ) != 0;
}

//kate: tab-width 4; indent-mode csands;