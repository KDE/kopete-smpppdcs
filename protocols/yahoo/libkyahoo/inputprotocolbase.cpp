/*
    Kopete Groupwise Protocol
    inputprotocolbase.cpp - Ancestor of all protocols used for reading GroupWise input

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
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

#include "inputprotocolbase.h"

#include <Q3CString>
#include <QDataStream>

InputProtocolBase::InputProtocolBase(QObject *parent)
 : QObject(parent)
{
}


InputProtocolBase::~InputProtocolBase()
{
}

uint InputProtocolBase::state() const
{
	return m_state;
}

bool InputProtocolBase::readString( QString &message )
{
	uint len;
	Q3CString rawData;
	if ( !safeReadBytes( rawData, len ) )
		return false;
	message = QString::fromUtf8( rawData.data(), len - 1 );
	return true;
}


bool InputProtocolBase::okToProceed()
{
	if ( m_din )
	{
		if ( m_din->atEnd() )
		{
			m_state = NeedMore;
			qDebug( "InputProtocol::okToProceed() - Server message ended prematurely!" );
		}
		else
			return true;
	}
	return false;
}

bool InputProtocolBase::safeReadBytes( Q3CString & data, uint & len )
{
	// read the length of the bytes
	Q_UINT32 val;
	if ( !okToProceed() )
		return false;
	*m_din >> val;
	m_bytes += sizeof( Q_UINT32 );
	if ( val > 1024 )
		return false;
	//qDebug( "EventProtocol::safeReadBytes() - expecting %i bytes", val );
	Q3CString temp( val );
	if ( val != 0 )
	{
		if ( !okToProceed() )
			return false;
		// if the server splits packets here we are in trouble,
		// as there is no way to see how much data was actually read
		m_din->readRawData( temp.data(), val );
		// the rest of the string will be filled with FF,
		// so look for that in the last position instead of \0
		// this caused a crash - guessing that temp.length() is set to the number of bytes actually read...
		// if ( (Q_UINT8)( * ( temp.data() + ( temp.length() - 1 ) ) ) == 0xFF )
		if ( temp.length() < ( val - 1 ) )
		{
			qDebug( "InputProtocol::safeReadBytes() - string broke, giving up, only got: %i bytes out of %i",  temp.length(), val );
			m_state = NeedMore;
			return false;
		}
	}
	data = temp;
	len = val;
	m_bytes += val;
	return true;
}

#include "inputprotocolbase.moc"
