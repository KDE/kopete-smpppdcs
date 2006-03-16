/*
   transfer.h - Represent a transfer between the Messenger server.

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
#ifndef PAPILLONCONNECTION_H
#define PAPILLONCONNECTION_H

#include <QObject>

namespace Papillon 
{

class ClientStream;
class Transfer;
class Task;
/**
 * Connection encapsulate a connection to a Windows Live Messenger service.
 * Don't use this class directly, use derived classes instead like NotificationConnection and SwitchboardConnection
 *
 * Contains the transaction ID reference count, the root Task. Dispatch also the transfers throught the task.
 *
 * @author Michaël Larouche
*/
class Connection : public QObject
{
	Q_OBJECT
public:
	Connection(ClientStream *stream);
	~Connection();

	/**
	 * Get the root task for this conenction.
	 * Root task is the parent Task for all task of this connection.
	 */
	Task *rootTask();

	/**
	 * Increment the transaction id and return its value.
	 * @return the current transaction id.
	 */
	int transactionId();

	/**
	 * Check if this connection is active.
	 * @return true if the connection is active.
	 */
	bool isConnected();

signals:
	/**
	 * Emitted when this connection gets disconnected.
	 */
	void disconnected();

public slots:
	/**
	 * Send a transfer to the current Messenger service.
	 */
	void send(Transfer *transfer);

private slots:
	/**
	 * @internal
	 * Called when the stream has received a new Papillon::Transfer.
	 */
	void transferReceived();
	/**
	 * @internal
	 * Dispatch a transfer through the task.
	 * After, the transfer gets deleted.
	 */
	void dispatchTransfer(Transfer *currentTransfer);

private:
	class Private;
	Private *d;
};

}

#endif