/*
    smpppdcsprefsimpl.h
 
    Copyright (c) 2004-2006 by Heiko Schaefer        <heiko@rangun.de>
 
    Kopete    (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>
 
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

#include <qgroupbox.h>

#include <kprogressdialog.h>

#include "ui_smpppdcsprefs.h"

class SMPPPDCSPlugin;
class SMPPPDSearcher;

/**
@author Heiko Sch&auml;fer <heiko@rangun.de>
*/
class SMPPPDCSPrefs : public Ui::SMPPPDCSPrefsUI 
{
    Q_OBJECT

	
public:

    explicit SMPPPDCSPrefs(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~SMPPPDCSPrefs();

signals:
	void foundSMPPPD(bool found);
	
protected slots:
    void enableSMPPPDSettings();
    void disableSMPPPDSettings();
    void determineCSType();
    void smpppdFound(const QString & host);
    void smpppdNotFound();
    void scanStarted(uint total);
    void scanProgress(uint cur);
    void scanFinished();
    void cancelScanning();

private:
    Q_DISABLE_COPY(SMPPPDCSPrefs)

    SMPPPDCSPlugin  * m_plugin;
    KProgressDialog * m_scanProgressDlg;
    SMPPPDSearcher  * m_curSearcher;
};

inline void SMPPPDCSPrefs::enableSMPPPDSettings() {
	smpppdPrefs->setEnabled(true);
}

inline void SMPPPDCSPrefs::disableSMPPPDSettings() {
	smpppdPrefs->setEnabled(false);
}

inline void SMPPPDCSPrefs::scanFinished() {
	m_scanProgressDlg->hide();
}

#endif
