/*
    msneditaccountwidget.h - MSN Identity Widget

    Copyright (c) 2003 by Olivier Goffart  <ogoffart@tiscalinet.be>

    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/



#ifndef MSNEDITIDENTITYWIDEGET_H
#define MSNEDITIDENTITYWIDEGET_H

#include <qwidget.h>
#include "editaccountwidget.h"

/**
  *@author Olivier Goffart <ogoffart@tiscalinet.be>
  */

class MSNProtocol;
class QCheckBox;
class QLineEdit;

class MSNEditIdentityWidget : public QWidget, public EditIdentityWidget
{
	Q_OBJECT

	public:
		MSNEditIdentityWidget(MSNProtocol *proto, KopeteIdentity *, QWidget *parent=0, const char *name=0);
		~MSNEditIdentityWidget();
		virtual bool validateData();
		virtual KopeteIdentity *apply();
		

	private:
		MSNProtocol *m_protocol;
		QLineEdit *m_password;
		QLineEdit *m_login;
		QCheckBox *m_autologin;
		QCheckBox *m_rememberpasswd;
};


#endif
