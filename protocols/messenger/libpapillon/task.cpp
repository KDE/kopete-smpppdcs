/*
   task.cpp - Papillon Task base class.

   Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>

   Based on code Copyright (c) 2004 Matt Rogers <mattr@kde.org>
   Based on code copyright (c) 2004 SuSE Linux AG <http://www.suse.com>
   Based on Iris, Copyright (C) 2003  Justin Karneges

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/

#include <QTimer>

// Papillon includes
#include "transfer.h"
#include "safedelete.h"
#include "task.h"

namespace Papillon
{

class Task::Private
{
public:
	Private() {}

	quint32 id;
	bool success;
	int statusCode;
	QString statusString;
	//Connection *client;
	bool insignificant, deleteme, autoDelete;
	bool done;
	Transfer *transfer;
};

Task::Task(Task *parent)
:QObject(parent)
{
	init();
	//	d->client = parent->client();
	//	connect(d->client, SIGNAL(disconnected()), SLOT(clientDisconnected()));
}

// Task::Task(Connection* parent, bool)
// :QObject(0)
// {
// 	init();
// 	d->client = parent;
// 	connect(d->client, SIGNAL(disconnected()), SLOT(clientDisconnected()));
// }

Task::~Task()
{
	delete d;
}

void Task::init()
{
	d = new Private;
	d->success = false;
	d->insignificant = false;
	d->deleteme = false;
	d->autoDelete = false;
	d->done = false;
	d->id = 0;
	d->transfer = 0;
}

Task *Task::parent() const
{
	return (Task *)QObject::parent();
}

// Connection *Task::client() const
// {
// 	return d->client;
// }

Transfer *Task::transfer() const
{
	return d->transfer;
}

void Task::setTransfer(Transfer *transfer)
{
	d->transfer = transfer;
}

quint32 Task::id() const
{
	return d->id;
}

bool Task::success() const
{
	return d->success;
}

int Task::statusCode() const
{
	return d->statusCode;
}

const QString &Task::statusString() const
{
	return d->statusString;
}

void Task::go(bool autoDelete)
{
	d->autoDelete = autoDelete;

	onGo();
}

bool Task::take(Transfer *transfer)
{
	const QObjectList &childTaskList = children();

	// pass along the transfer to our children
	Task *t;
	foreach(QObject *obj, childTaskList)
	{
		t = qobject_cast<Task*>(obj);
		if(!t)
			continue;
		
		if(t->take( transfer ))
		{
			return true;
		}
	}

	return false;
}

void Task::safeDelete()
{
	if(d->deleteme)
		return;

	d->deleteme = true;
	if(!d->insignificant)
		SafeDelete::deleteSingle(this);
}

void Task::onGo()
{
	qDebug( "ERROR: calling default NULL onGo() for this task, you should reimplement this!");
}

void Task::onDisconnect()
{
	if(!d->done) {
		d->success = false;
		d->statusCode = ErrDisc;
		d->statusString = tr("Disconnected");

		// delay this so that tasks that react don't block the shutdown
		QTimer::singleShot(0, this, SLOT(done()));
	}
}

void Task::send(Transfer *request)
{
	//client()->send( request );
}

void Task::setSuccess(int code, const QString &str)
{
	if(!d->done) {
		d->success = true;
		d->statusCode = code;
		d->statusString = str;
		done();
	}
}

void Task::setError(int code, const QString &str)
{
	if(!d->done) {
		d->success = false;
		d->statusCode = code;
		d->statusString = str;
		done();
	}
}

void Task::done()
{
	debug("Task::done()");
	if(d->done || d->insignificant)
		return;
	d->done = true;

	if(d->deleteme || d->autoDelete)
		d->deleteme = true;

	d->insignificant = true;
	debug("emitting finished");
	emit finished();
	d->insignificant = false;

	if(d->deleteme)
		SafeDelete::deleteSingle(this);
}

void Task::clientDisconnected()
{
	onDisconnect();
}

void Task::debug(const QString &str)
{
	//black hole
	Q_UNUSED( str );
	//client()->debug(QString("%1: ").arg(className()) + str);
}

bool Task::forMe(Transfer *transfer) const
{
	Q_UNUSED( transfer );
	return false;
}

void Task::setId( quint32 id )
{
	d->id = id;
}

}
#include "task.moc"

//kate: tab-width 4; indent-mode csands;
