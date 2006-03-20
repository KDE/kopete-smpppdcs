/*
   logintask.cpp - Windows Live Messenger Login Task

   Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#include "logintask.h"

// Qt includes
#include <QLatin1String>
#include <QStringList>
#include <QtDebug>

// Papillon includes
#include "transfer.h"
#include "connection.h"

namespace Papillon 
{

class LoginTask::Private
{
public:
	Private()
	 : currentState(LoginTask::StateVersion)
	{}

	QString passportId;
	// TODO: Use QSecureArray
	QString password;

	LoginTask::LoginState currentState;

	// Keep track of the expected transaction ID.
	QString currentTransactionId;

	QString tweenerTicket;
};

LoginTask::LoginTask(Task *parent)
 : Task(parent), d(new Private)
{
}

LoginTask::~LoginTask()
{
	delete d;
}

void LoginTask::setUserInfo(const QString &passportId, const QString &password)
{
	d->passportId = passportId;
	d->password = password;
}

LoginTask::LoginState LoginTask::loginState() const
{
	return d->currentState;
}

bool LoginTask::take(Transfer *transfer)
{
	if( forMe(transfer) )
	{
		bool proceeded = false;
		switch(d->currentState)
		{
			case StateVersion:
			{
				if( transfer->command() == QLatin1String("VER") )
				{
					d->currentState = StateCVR;
					proceeded = true;
					sendCvrCommand();
				}
				break;
			}
			case StateCVR:
			{
				if( transfer->command() == QLatin1String("CVR") )
				{
					d->currentState = StateTweenerInvite;
					proceeded = true;
					sendTweenerInviteCommand();
				}
				break;
			}
			case StateTweenerInvite:
			{
				if( transfer->command() == QLatin1String("USR") )
				{
					if( transfer->arguments()[0] == QLatin1String("TWN") && transfer->arguments()[1] == QLatin1String("S") )
					{
						d->currentState = StateTweenerConfirmed;
						proceeded = true;
						// TODO: Get the ticket then send the ticket.
						d->currentState = StateError;
						setError();
					}
				}
				else if( transfer->command() == QLatin1String("XFR") )
				{
					QString newServer = transfer->arguments()[1].section(":", 0, 0);
					QString tempPort = transfer->arguments()[1].section(":", 1, 1);
					bool dummy;
					int newPort = tempPort.toUInt(&dummy);

					proceeded = true;
					emit redirection(newServer, newPort);
				}
				break;
			}
			case StateTweenerConfirmed:
			{
				if( transfer->command() == QLatin1String("USR") )
				{
					if( transfer->arguments()[0] == QLatin1String("OK") )
					{
						proceeded = true;
						d->currentState = StateFinish;
						// End the login task.
						setSuccess();
						}
				}
				break;
			}
			default:
				return false;
		}

		return proceeded;
	}

	return false;
}

bool LoginTask::forMe(Transfer *transfer)
{
	if( transfer->type() == Transfer::TransactionTransfer )
	{
		if( transfer->transactionId() == d->currentTransactionId )
			return true;
	}

	return false;
}

void LoginTask::onGo()
{
	Q_ASSERT(!d->passportId.isEmpty());
	Q_ASSERT(!d->password.isEmpty());

	qDebug() << PAPILLON_FUNCINFO << "Begin login process...";

	sendVersionCommand();
}

void LoginTask::sendVersionCommand()
{
	qDebug() << PAPILLON_FUNCINFO << "Sending version command.";
	Transfer *versionTransfer = new Transfer(Transfer::TransactionTransfer);
	versionTransfer->setCommand( QLatin1String("VER") );

	d->currentTransactionId = QString::number( connection()->transactionId() );
	versionTransfer->setTransactionId( d->currentTransactionId );

	QStringList arguments = QString("MSNP11 MSNP10 CVR0").split(" ");
	versionTransfer->setArguments(arguments);

	send(versionTransfer);
}

void LoginTask::sendCvrCommand()
{
	qDebug() << PAPILLON_FUNCINFO << "Sending CVR command.";
	Transfer *cvrTransfer = new Transfer(Transfer::TransactionTransfer);
	cvrTransfer->setCommand( QLatin1String("CVR") );
	
	d->currentTransactionId = QString::number( connection()->transactionId() );
	cvrTransfer->setTransactionId( d->currentTransactionId );

	QString arguments = QString("0x040c winnt 5.1 i386 MSNMSGR 7.0.0777 msmsgs %1").arg(d->passportId);
	QStringList argsList = arguments.split(" ");
	cvrTransfer->setArguments(argsList);

	send(cvrTransfer);
}

void LoginTask::sendTweenerInviteCommand()
{
	qDebug() << PAPILLON_FUNCINFO << "Sending Tweener Invite Command";
	Transfer *twnTransfer = new Transfer(Transfer::TransactionTransfer);
	twnTransfer->setCommand("USR");

	d->currentTransactionId = QString::number( connection()->transactionId() );
	twnTransfer->setTransactionId( d->currentTransactionId );

	QString arguments = QString("TWN I %1").arg(d->passportId);
	QStringList args = arguments.split(" ");
	twnTransfer->setArguments(args);

	send(twnTransfer);
}

void LoginTask::sendTweenerConfirmation()
{
	qDebug() << PAPILLON_FUNCINFO << "Sending Tweener confirmation command.";
	Transfer *twnTransfer = new Transfer(Transfer::TransactionTransfer);
	twnTransfer->setCommand("USR");

	d->currentTransactionId = QString::number( connection()->transactionId() );
	twnTransfer->setTransactionId( d->currentTransactionId );

	QString arguments = QString("TWN S %1").arg(d->tweenerTicket);
	QStringList args = arguments.split(" ");
	twnTransfer->setArguments(args);

	send(twnTransfer);
}

}

#include "logintask.moc"
