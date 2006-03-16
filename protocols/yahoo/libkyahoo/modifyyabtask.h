/*
    Kopete Yahoo Protocol
    modifyyabtask.h - Saves a YAB entry

    Copyright (c) 2006 André Duffeck <andre.duffeck@kdemail.net>
    Kopete (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef MODIFYYABTASK_H
#define MODIFYYABTASK_H

#include "task.h"
#include "yabentry.h"

struct KUrl;
namespace KIO	{ 
	class Job;
	class TransferJob; 
}
class QDomElement;

/**
@author André Duffeck
*/
class ModifyYABTask : public Task
{
	Q_OBJECT
public:
	enum Action { AddEntry, EditEntry, DeleteEntry };
	ModifyYABTask(Task *parent);
	~ModifyYABTask();

	virtual void onGo();
	void setAction( Action action );
	void setEntry( const YABEntry & );
signals:
	void gotEntry( YABEntry * );
	void gotRevision( long rev, bool merged );
	void error( YABEntry *, const QString &);
private slots:
	void connectSucceeded();
	void connectFailed( int );
	void slotRead();
private:
	KIO::TransferJob *m_transferJob;
	QString m_postData;
	Action m_action;
};

#endif