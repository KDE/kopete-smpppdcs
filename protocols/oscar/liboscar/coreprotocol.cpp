/*
    Kopete Oscar Protocol
    coreprotocol.h- the core Oscar protocol

    Copyright (c) 2004 by Matt Rogers <mattr@kde.org>

    Based on code Copyright (c) 2004 SuSE Linux AG http://www.suse.com

    Based on Iris, Copyright (C) 2003  Justin Karneges
    url_escape_string from Gaim src/protocols/novell/nmconn.c
    Copyright (c) 2004 Novell, Inc. All Rights Reserved

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

#include "coreprotocol.h"

#include <qdatastream.h>
#include <qdatetime.h>
#include <qtextstream.h>

#include <kdebug.h>
#include <ctype.h>

#include "oscartypes.h"
#include "transfer.h"
#include "flapprotocol.h"
#include "snacprotocol.h"


using namespace Oscar;

CoreProtocol::CoreProtocol() : QObject()
{
	m_snacProtocol = new SnacProtocol( this, "snacprotocol" );
	m_flapProtocol = new FlapProtocol( this, "flapprotocol" );
}

CoreProtocol::~CoreProtocol() 
{
}

int CoreProtocol::state()
{
	return m_state;
}

void CoreProtocol::addIncomingData( const QByteArray & incomingBytes )
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Received " << incomingBytes.count() << " bytes. " << endl;
	// store locally
	int oldsize = m_in.size();
	m_in.resize( oldsize + incomingBytes.size() );
	memcpy( m_in.data() + oldsize, incomingBytes.data(), incomingBytes.size() );
	m_state = Available;
	
	// convert every event in the chunk to a Transfer, signalling it back to the clientstream
	int parsedBytes = 0;
	int transferCount = 0;
	// while there is data left in the input buffer, and we are able to parse something out of it
	while ( m_in.size() && ( parsedBytes = wireToTransfer( m_in ) ) )
	{
		transferCount++;
		//kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "parsed transfer #" << transferCount << " in chunk" << endl;
		int size =  m_in.size();
		if ( parsedBytes < size )
		{
			//kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "more data in chunk!" << endl;
			// copy the unparsed bytes into a new qbytearray and replace m_in with that
			QByteArray remainder( size - parsedBytes );
			memcpy( remainder.data(), m_in.data() + parsedBytes, remainder.size() );
			m_in = remainder;
		}
		else
			m_in.truncate( 0 );
	}
	
	if ( m_state == NeedMore )
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "message was incomplete, waiting for more..." << endl;
	
	if ( m_snacProtocol->state() == OutOfSync || m_flapProtocol->state() == OutOfSync )
	{	
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "protocol thinks it's out of sync. "
			<< "discarding the rest of the buffer and hoping the server regains sync soon..." << endl;
		m_in.truncate( 0 );
	}
// 	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "done processing chunk" << endl;
}

Transfer* CoreProtocol::incomingTransfer()
{	
	if ( m_state == Available )
	{
		m_state = NoData;
		return m_inTransfer;
		m_inTransfer = 0;
	}
	else
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "we shouldn't be here!" << kdBacktrace() << endl;
		return 0;
	}
}

void cp_dump( const QByteArray &bytes )
{
#ifdef OSCAR_COREPROTOCOL_DEBUG
	kdDebug(OSCAR_RAW_DEBUG) << "contains: " << bytes.count() << " bytes" << endl;
	for ( uint i = 0; i < bytes.count(); ++i )
	{
		printf( "%02x ", bytes[ i ] );
	}
	printf( "\n" );
#else
	Q_UNUSED( bytes );
#endif
}

void CoreProtocol::outgoingTransfer( Transfer* outgoing )
{
	//kdDebug(OSCAR_RAW_DEBUG) << "CoreProtocol::outgoingTransfer()" << endl;
	// Convert the outgoing data into wire format
	// pretty leet, eh?
	emit outgoingData( outgoing->toWire() );
	delete outgoing;
	
	return;
}

int CoreProtocol::wireToTransfer( const QByteArray& wire )
{
	// processing incoming data and reassembling it into transfers
	// may be an event or a response
	
	BYTE flapStart, flapChannel = 0;
	WORD flapLength = 0;
	WORD s1, s2 = 0;
	uint bytesParsed = 0;
			
	if ( wire.size() < 6 ) //check for valid flap length
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo 
				<< "packet not long enough! couldn't parse FLAP!" << endl;
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "packet size is " << wire.size() << endl;
		m_state = NeedMore;
		return bytesParsed;
	}	
	
	m_din = new QDataStream( wire, IO_ReadOnly );
	
	// look at first four bytes and decide what to do with the chunk
	if ( okToProceed() )
	{
		*m_din >> flapStart;
		QByteArray packet;
		packet.duplicate( wire );
		if ( flapStart == 0x2A )
		{
			*m_din >> flapChannel;
			*m_din >> flapLength; //discard the first one it's not really the flap length
			*m_din >> flapLength;
			if ( wire.size() < flapLength )
			{
				kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo 
					<< "Not enough bytes to make a correct transfer. Have " << wire.size()
					<< " bytes. need " << flapLength << " bytes" << endl;
				m_state = NeedMore;
				return bytesParsed;
			}
			
			if ( flapChannel != 2 )
			{
				Transfer *t = m_flapProtocol->parse( packet, bytesParsed );
				if ( t )
				{
					m_inTransfer = t;
					m_state = Available;
					emit incomingData();
				}
				else
					bytesParsed = 0;
			}
			
			if ( flapChannel == 2 )
			{
				*m_din >> s1;
				*m_din >> s2;
				
				Transfer * t = m_snacProtocol->parse( packet, bytesParsed );
				if ( t )
				{
					m_inTransfer = t;
					m_state = Available;
					emit incomingData();
				}
				else
					bytesParsed = 0;
			}
		}
		else 
		{ //unknown wire format
			kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "unknown wire format detected!" << endl;
			kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "start byte is " << flapStart << endl;
		}
		
	}
	delete m_din;
	return bytesParsed;
}

void CoreProtocol::reset()
{
	m_in.resize( 0 );
}

void CoreProtocol::slotOutgoingData( const QCString &out )
{
	kdDebug(OSCAR_RAW_DEBUG) << out.data() << endl;
}

bool CoreProtocol::okToProceed()
{
	if ( m_din )
	{
		if ( m_din->atEnd() )
		{
			m_state = NeedMore;
			kdDebug(OSCAR_RAW_DEBUG) << "EventProtocol::okToProceed() - Server message ended prematurely!" << endl;
		}
		else
			return true;
	}
	return false;
}

#include "coreprotocol.moc"