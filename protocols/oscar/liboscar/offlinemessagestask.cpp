/*
   Kopete Oscar Protocol
   offlinemessagestask.cpp - Offline messages handling

   Copyright (c) 2004 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>

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
#include "config.h"
#include "offlinemessagestask.h"

#include <time.h>

#include "transfer.h"
#include "buffer.h"
#include "connection.h"

#include <kdebug.h>

OfflineMessagesTask::OfflineMessagesTask( Task* parent )
 : ICQTask( parent )
{
	tzset();
	m_msgCount = 0;
	m_sequence = 0;
}

OfflineMessagesTask::~OfflineMessagesTask()
{
}

void OfflineMessagesTask::onGo()
{
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Requesting offline messages" << endl;
	
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0015, 0x0002, 0x0000, client()->snacSequence() };
	
	setRequestType( 0x003c ); //offline message request
	setSequence( f.sequence );
	Buffer* buf = addInitialData();
	Transfer* t = createTransfer( f, s, buf );
	send( t );
}

bool OfflineMessagesTask::forMe( const Transfer* t ) const
{
	const SnacTransfer* st = dynamic_cast<const SnacTransfer*>( t );
	
	if ( !st )
		return false;
	
	if ( st->snacService() != 0x0015 || st->snacSubtype() != 0x0003 )
		return false;
		
	Buffer buf( st->buffer()->buffer(), st->buffer()->length() );
	const_cast<OfflineMessagesTask*>(this)->parseInitialData( buf );
	
	if ( requestType() == 0x0041 || requestType() == 0x0042 )
		return true;
		
	return false;
}

bool OfflineMessagesTask::take( Transfer* t )
{
	if ( forMe( t ) )
	{
		setTransfer( t );
		
		if ( requestType() == 0x0041 ) // Offline message
			handleOfflineMessage();
		else if ( requestType() == 0x0042 ) // end-of-offline messages
			endOfMessages();
			
		setTransfer( 0 );
		return true;
	}
	return false;
}

Oscar::Message OfflineMessagesTask::parseOfflineMessage(Buffer *b)
{

	TLV tlv1 = b->getTLV();
	Buffer* buffer = new Buffer( tlv1.data, tlv1.length );
	
	buffer->getLEWord(); // data chunk size
	DWORD receiverUin = buffer->getLEDWord(); // target uin
	buffer->getLEWord(); // request type
	buffer->getLEWord(); // request sequence number: 0x0002
	
	DWORD senderUin = buffer->getLEDWord();
	WORD year = buffer->getLEWord();
	BYTE month = buffer->getByte();
	BYTE day = buffer->getByte();
	BYTE hour = buffer->getByte();
	BYTE minute = buffer->getByte();
	
	BYTE type = buffer->getByte(); // msg type
	BYTE flags = buffer->getByte(); // msg flags
	
	WORD msgLength = buffer->getLEWord();
	QByteArray msg = buffer->getBlock( msgLength );

	QDateTime utcTime( QDate(year, month, day), QTime(hour, minute), Qt::UTC );

	Oscar::Message message( Oscar::Message::UserDefined, msg, type, flags, utcTime.toLocalTime() );
	message.setSender( QString::number( senderUin ) );
	message.setReceiver( QString::number( receiverUin ) );
	
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Received offline message '" << msg.data() << "' from " << senderUin << endl;
	
	return message;
}

void OfflineMessagesTask::handleOfflineMessage()
{
	Oscar::Message message = parseOfflineMessage(transfer()->buffer());
	m_msgCount++;
	emit receivedOfflineMessage( message );
}

void OfflineMessagesTask::endOfMessages()
{
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "End of Offline Messages" << endl;
	
	TLV tlv1 = transfer()->buffer()->getTLV();
	Buffer* buffer = new Buffer( tlv1.data, tlv1.length );
	
	buffer->skipBytes( 8 );
	m_sequence = buffer->getLEWord();
	
	if ( m_msgCount > 0 )
		deleteOfflineMessages();
	
	setSuccess( true );
}

void OfflineMessagesTask::deleteOfflineMessages()
{
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0015, 0x0002, 0x0000, client()->snacSequence() };
	
	
	setRequestType( 0x003E ); //delete offline messages
	setSequence( m_sequence );
	Buffer* buf = addInitialData();
	Transfer* t = createTransfer( f, s, buf );
	send( t );
}

#include "offlinemessagestask.moc"
