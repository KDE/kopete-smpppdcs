#include "cryptographyplugin.h"  //(for the cached passphrase)
//Code from KGPG

/***************************************************************************
                          kgpginterface.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <klocale.h>
#include <kpassdlg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <qfile.h>

#include <kprocio.h>

//#include "kdetailedconsole.h"

#include "kgpginterface.h"

KgpgInterface::KgpgInterface()
{}

KgpgInterface::~KgpgInterface()
{}

QString KgpgInterface::KgpgEncryptText(QString text,QString userIDs, QString Options)
{
	FILE *fp;
	QString dests,encResult;
	char buffer[200];
	
	userIDs=userIDs.stripWhiteSpace();
	userIDs=userIDs.simplifyWhiteSpace();
	Options=Options.stripWhiteSpace();
	
	int ct=userIDs.find(" ");
	while (ct!=-1)  // if multiple keys...
	{
		dests+=" --recipient "+userIDs.section(' ',0,0);
		userIDs.remove(0,ct+1);
		ct=userIDs.find(" ");
	}
	dests+=" --recipient "+userIDs;
	
	QCString gpgcmd = "echo ";
	gpgcmd += KShellProcess::quote(text).utf8();
	gpgcmd += " | gpg --no-secmem-warning --no-tty ";
	gpgcmd += Options.local8Bit();
	gpgcmd += " -e ";
	gpgcmd += dests.local8Bit();
	
	//////////   encode with untrusted keys or armor if checked by user
	fp = popen( gpgcmd, "r");
	while ( fgets( buffer, sizeof(buffer), fp))
		encResult+=buffer;
	pclose(fp);
	
	if( !encResult.isEmpty() )
		return encResult;
	else
		return QString::null;
}

QString KgpgInterface::KgpgDecryptText(QString text,QString userID)
{
	FILE *fp,*pass;
	QString encResult;
	
	char buffer[200];
	int counter=0,ppass[2];
	QCString password = CryptographyPlugin::cachedPass();
	bool passphraseHandling=CryptographyPlugin::passphraseHandling();
	
	while ((counter<3) && (encResult.isEmpty()))
	{
		counter++;
		if(passphraseHandling && password.isNull())
		{
			/// pipe for passphrase
			//userID=QString::fromUtf8(userID);
			userID.replace('<',"&lt;");
			QString passdlg=i18n("Enter passphrase for <b>%1</b>:").arg(userID);
			if (counter>1)
				passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 tries left.<br>").arg(QString::number(4-counter)));
	
			/// pipe for passphrase
			int code=KPasswordDialog::getPassword(password,passdlg);
			if (code!=QDialog::Accepted)
				return QString::null;
			CryptographyPlugin::setCachedPass(password);
		}
	
		if(passphraseHandling)
		{
			pipe(ppass);
			pass = fdopen(ppass[1], "w");
			fwrite(password, sizeof(char), strlen(password), pass);
			//        fwrite("\n", sizeof(char), 1, pass);
			fclose(pass);
		}
	
		QCString gpgcmd="echo ";
		gpgcmd += KShellProcess::quote(text).utf8();
		gpgcmd += " | gpg --no-secmem-warning --no-tty ";
		if(passphraseHandling)
			gpgcmd += "--passphrase-fd " + ppass[0];
		gpgcmd += " -d ";
		
		//////////   encode with untrusted keys or armor if checked by user
		fp = popen(gpgcmd, "r");
		while ( fgets( buffer, sizeof(buffer), fp))
			encResult += QString::fromUtf8(buffer);
		
		pclose(fp);
		password = QCString();
	}
	
	if( !encResult.isEmpty() )
		return encResult;
	else
		return QString::null;
}


#include "kgpginterface.moc"
