/*
    Kopete Oscar Protocol
    icqlogintask.cpp - Handles logging into to the ICQ service

    Copyright (c) 2004 Matt Rogers <mattr@kde.org>

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

#include "icqlogintask.h"

#include <qstring.h>
#include <kdebug.h>
#include "transfer.h"
#include "connection.h"
#include "oscarutils.h"

using namespace Oscar;

IcqLoginTask::IcqLoginTask( Task* parent )
	: Task( parent )
{
}

IcqLoginTask::~IcqLoginTask()
{
}

bool IcqLoginTask::take( Transfer* transfer )
{
	Q_UNUSED( transfer );
	return false;
}

bool IcqLoginTask::forMe( Transfer* transfer ) const
{
	//there shouldn't be a incoming transfer for this task
	Q_UNUSED( transfer );
	return false;
}

void IcqLoginTask::onGo()
{
	FLAP f = { 0x01, 0, 0 };
	Oscar::DWORD flapVersion = 0x00000001;
	Buffer *outbuf = new Buffer();

	QString encodedPassword = encodePassword( client()->password() );

	const Oscar::ClientVersion* version = client()->version();
	outbuf->addDWord( flapVersion );
	outbuf->addTLV( 0x0001, client()->userId().toLatin1() );
	outbuf->addTLV( 0x0002, encodedPassword.toLatin1() );
	outbuf->addTLV( 0x0003, version->clientString.toLatin1() );
	outbuf->addTLV16( 0x0016, version->clientId );
	outbuf->addTLV16( 0x0017, version->major );
	outbuf->addTLV16( 0x0018, version->minor );
	outbuf->addTLV16(  0x0019, version->point );
	outbuf->addTLV16(0x001a, version->build );
	outbuf->addTLV32( 0x0014, version->other );
	outbuf->addTLV( 0x000f, version->lang.toLatin1() );
	outbuf->addTLV( 0x000e, version->country.toLatin1() );

	Transfer* ft = createTransfer( f, outbuf );
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Sending ICQ channel 0x01 login packet" << endl;
	send( ft );
	emit finished();
}


QString IcqLoginTask::encodePassword( const QString& loginPassword )
{
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Called." << endl;

	// TODO: check if latin1 is the right conversion
	QByteArray password = loginPassword.toLatin1();

	const uint MAX_PASSWORD_SIZE = 8;
	unsigned int i = 0;
	QString encodedPassword;

	//encoding table used in ICQ password XOR transformation
	unsigned char encoding_table[] =
	{
		0xf3, 0x26, 0x81, 0xc4,
		0x39, 0x86, 0xdb, 0x92,
		0x71, 0xa3, 0xb9, 0xe6,
		0x53, 0x7a, 0x95, 0x7c
	};
	
	const uint size = qMin( (uint)password.size(), MAX_PASSWORD_SIZE );
	for (i = 0; i < size; i++)
		encodedPassword.append( password.at(i) ^ encoding_table[i] );

#ifdef OSCAR_PWDEBUG
	kDebug(OSCAR_RAW_DEBUG) << " plaintext pw='" << loginPassword << "', length=" <<
		loginPassword.length() << endl;

	kDebug(OSCAR_RAW_DEBUG) << " encoded   pw='" << encodedPassword  << "', length=" <<
		encodedPassword.length() << endl;
#endif

	return encodedPassword;
}

#include "icqlogintask.moc"