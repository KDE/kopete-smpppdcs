/*
   qtconnector.h - Connector using QtNetwork.

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
#ifndef PAPILLONQTCONNECTOR_H
#define PAPILLONQTCONNECTOR_H

#include <connector.h>

namespace Papillon 
{

/**
 * Connector using QtNetwork sockets classes.
 *
 * @author Michaël Larouche <michael.larouche@kdemail.net>
*/
class QtConnector : public Connector
{
public:
	QtConnector(QObject *parent);
	~QtConnector();

	virtual ByteStream* stream() const;
	virtual void connectToServer(const QString& server, quint16);
	virtual void done();

private slots:
	void slotConnected();

private:
	void init();

private:
	class Private;
	Private *d;
};

}

#endif