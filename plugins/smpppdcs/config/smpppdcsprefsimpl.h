/*
    smpppdcsprefsimpl.h
 
    Copyright (c) 2004      by Heiko Schaefer        <heiko@rangun.de>
 
    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; version 2 of the License.               *
    *                                                                       *
    *************************************************************************
*/

#ifndef SMPPPDCSPREFSIMPL_H
#define SMPPPDCSPREFSIMPL_H

#include <klineedit.h>

#include "smpppdcsprefs.h"

/**
@author Heiko Schaefer <heiko@rangun.de>
*/
class SMPPPDCSPrefs : public SMPPPDCSPrefsBase 
{
	Q_OBJECT

	SMPPPDCSPrefs(const SMPPPDCSPrefs&);
	SMPPPDCSPrefs& operator=(const SMPPPDCSPrefs&);
	
public:

    SMPPPDCSPrefs(QWidget* parent, const char* name = 0, WFlags fl = 0);
    ~SMPPPDCSPrefs();

protected slots:
    void enableSMPPPDSettings();
    void disableSMPPPDSettings();
    void determineCSType();
    void smpppdFound(const QString & host);
    void smpppdNotFound();

};

#endif