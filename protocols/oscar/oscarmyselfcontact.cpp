/*
  oscarmyselfcontact.cpp  -  Oscar Protocol Plugin Myself Contact

  Copyright (c) 2004      by Richard Smith          <kde@metafoo.co.uk>
  Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
*/

#include <klocale.h>
#include "oscartypes.h"
#include "oscaraccount.h"

#include "oscarmyselfcontact.h"


OscarMyselfContact::OscarMyselfContact( OscarAccount* account )
	: Kopete::Contact( account, account->accountId(), 0, QString::null )
{
	QObject::connect( account->engine(), SIGNAL( haveOwnInfo() ), this, SLOT( userInfoUpdated() ) );
}

OscarMyselfContact::~OscarMyselfContact()
{
}

bool OscarMyselfContact::isReachable()
{
	return false;
}

Kopete::ChatSession* OscarMyselfContact::manager(CanCreateFlags canCreate )
{
	return 0;
}

UserDetails OscarMyselfContact::details()
{
	OscarAccount *acct = static_cast<OscarAccount*>(account());
	return acct->engine()->ourInfo();
}

void OscarMyselfContact::deleteContact()
{
	kdWarning( OSCAR_GEN_DEBUG ) << k_funcinfo << "called on myself contact! Ignoring." << endl << kdBacktrace() << endl;
}

#include "oscarmyselfcontact.moc"

//kate: indent-mode csands;