/*
    MetaContactSelectorWidget

    Copyright (c) 2005 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef MetaContactSelectorWidget_H
#define MetaContactSelectorWidget_H

#include <kdemacros.h>
#include <qwidget.h>
#include "kopetelistviewitem.h"
#include "kopete_export.h"

class Kopete::MetaContact;

/**
 * @author Duncan Mac-Vicar Prett <duncan@kde.org>
 */

class KOPETE_EXPORT MetaContactSelectorWidget : public QWidget
{
	Q_OBJECT
public:
	MetaContactSelectorWidget( QWidget *parent = 0, const char *name  = 0 );
	~MetaContactSelectorWidget();
	Kopete::MetaContact* metaContact();
	/**
	 * sets the widget label message
	 * example: Please select a contact
	 * or, Choose a contact to delete
	 */
	void setLabelMessage( const QString &msg );
	/**
	 * pre-selects a contact
	 */
	void selectMetaContact( Kopete::MetaContact *mc );
	/**
	 * @return true if there is a contact selected
	 */
	bool metaContactSelected();
protected slots:
	/**
	 * Utility function, populates the metacontact list
	 */
	void slotLoadMetaContacts();
signals:
	void metaContactListClicked( QListViewItem *mc );
private:
	class Private;
	Private *d;
};

/**
 * @author Duncan Mac-Vicar Prett <duncan@kde.org>
 */

class MetaContactSelectorWidgetLVI : public Kopete::UI::ListView::Item
{
	Q_OBJECT
public:
	MetaContactSelectorWidgetLVI(Kopete::MetaContact *mc, QListView *parent, QObject *owner = 0, const char *name = 0 );
protected slots:
	void slotPhotoChanged();
	void slotDisplayNameChanged();
	void buildVisualComponents();
private:
	class Private;
	Private *d;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:
