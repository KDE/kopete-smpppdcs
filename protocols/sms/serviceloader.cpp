/*  *************************************************************************
    *   copyright: (C) 2003 Richard L�rk�ng <nouseforaname@home.se>         *
    *   copyright: (C) 2003 Gav Wood <gav@kde.org>                          *
    *************************************************************************
*/

/*  *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include "serviceloader.h"
#include "smssend.h"
#include "smsclient.h"

SMSService* ServiceLoader::loadService(const QString& name, KopeteAccount* account)
{
	kdWarning( 14160 ) << k_funcinfo << endl;

	SMSService* s;
	if (name == "SMSSend")
		s = new SMSSend(account);
	else if (name == "SMSClient")
		s = new SMSClient(account);
	else
	{
		KMessageBox::sorry(0L, i18n("Could not load service %1.").arg(name),
			i18n("Error Loading Service"));
		s = 0L;
	}

	return s;
}

QStringList ServiceLoader::services()
{
	QStringList toReturn;
	toReturn.append("SMSSend");
	toReturn.append("SMSClient");
	return toReturn;
}


/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

