/*
    kopeteonlinestatusmanager.h

    Copyright (c) 2004 by Olivier Goffart  <ogoffart @ tiscalinet.be>

    Kopete    (c) 2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef __kopeteonlinestatusmanager_h__
#define __kopeteonlinestatusmanager_h__

#include <qobject.h>
#include "kopeteonlinestatus.h"

class QString;
class QPixmap;
class QColor;


namespace Kopete
{
	class OnlineStatus;

/**
 * OnlineStatusManager is a singleton which manage OnlineStatus
 *
 * @author Olivier Goffart 
 */
class OnlineStatusManager : public QObject
{
	Q_OBJECT
public:
	static OnlineStatusManager* self();
	~OnlineStatusManager();
	
	enum Categories
	{
		Offline=0x01,
		Online=0x02,
		Away=0x04,
		Idle=0x10, /** Status used for auto away  */
		Busy=0x20,
		Invisible=0x40
	};
	
	enum Options
	{
		HasAwayMessage = 0x01  /** The user may set away messages for this online status   */
	};
	
	/**
	 * You need to register each status an account can be.
	 * Registered statuses will appear in the account menu.
	 * The Protocol constructor is a good place to call this function
	 *
	 * You can set the status to be in the predefined categories.
	 * Ideally, each category should own one status.
	 * A status may be in several categories, or in none.
	 *
	 * @param status The status to register
	 * @param caption The caption that will appear in menus (e.g. "Set &Away")
	 * @param categories A bitflag of @ref Categories
	 * @param options is a bitflag of @ref Options
	 */
	void registerOnlineStatus(const OnlineStatus& status, const QString &caption, unsigned int categories=0x00 , unsigned int options=0x0);
	
	

private:
	friend class OnlineStatus;
	QPixmap cacheLookupByObject( const OnlineStatus &statusFor, const QString& icon, int size, QColor color, bool idle = false);
	QPixmap cacheLookupByMimeSource( const QString &mimeSource );
	QString fingerprint( const OnlineStatus &statusFor, const QString& icon, int size, QColor color, bool idle = false);
	QPixmap* renderIcon( const OnlineStatus &statusFor, const QString& baseicon, int size, QColor color, bool idle = false) const;

signals:
	void iconsChanged();

private slots:
	void slotIconsChanged();

private:
	
	static OnlineStatusManager *s_self;
	OnlineStatusManager();
	class Private;
	Private *d;
};

}  //END namespace Kopete 

#endif

// vim: set noet ts=4 sts=4 sw=4:
