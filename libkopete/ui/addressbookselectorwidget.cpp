/*
    AddressBookSelectorWidget

    Copyright (c) 2005 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Based on LinkAddressBookUI whose code was shamelessly stolen from 
    kopete's add new contact wizard, used in Konversation, and then 
    reappropriated by Kopete.

    LinkAddressBookUI:
    Copyright (c) 2004 by John Tapsell           <john@geola.co.uk>
    Copyright (c) 2003-2005 by Will Stephenson   <will@stevello.free-online.co.uk>
    Copyright (c) 2002 by Nick Betcher           <nbetcher@kde.org>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qcheckbox.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>

#include <kdeversion.h>
#include <kinputdialog.h>

#include <kpushbutton.h>
#include <kactivelabel.h>
#include <kdebug.h>
#include <klistview.h>
#include <klistviewsearchline.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "addressbookselectorwidget.h"
#include <addresseeitem.h>
#include "kabcpersistence.h"

AddressBookSelectorWidget::AddressBookSelectorWidget( QWidget *parent, const char *name )
		: AddressBookSelectorWidget_Base( parent, name )
{
	m_addressBook = Kopete::KABCPersistence::self()->addressBook();

	// Addressee validation connections
	connect( addAddresseeButton, SIGNAL( clicked() ), SLOT( slotAddAddresseeClicked() ) );
	connect( addAddresseeButton, SIGNAL( clicked() ), SIGNAL( addAddresseeClicked() ) );	

	connect( addresseeListView, SIGNAL( clicked(QListViewItem * ) ),
			SIGNAL( addresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( selectionChanged( QListViewItem * ) ),
			SIGNAL( addresseeListClicked( QListViewItem * ) ) );
	connect( addresseeListView, SIGNAL( spacePressed( QListViewItem * ) ),
			SIGNAL( addresseeListClicked( QListViewItem * ) ) );
	
	connect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
	connect( Kopete::KABCPersistence::self()->addressBook(), SIGNAL( addresseesChanged()), this, SLOT(slotLoadAddressees()));
	
	//We should add a clear KAction here.  But we can't really do that with a designer file :\  this sucks

	addresseeListView->setColumnText(2, SmallIconSet(QString::fromLatin1("email")), i18n("Email"));

	kListViewSearchLine->setListView(addresseeListView);
	slotLoadAddressees();

	addresseeListView->setColumnWidthMode(0, QListView::Manual);
	addresseeListView->setColumnWidth(0, 63); //Photo is 60, and it's nice to have a small gap, imho
}


AddressBookSelectorWidget::~AddressBookSelectorWidget()
{
	disconnect( m_addressBook, SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
}


KABC::Addressee AddressBookSelectorWidget::addressee()
{
	AddresseeItem *item = 0L;
	item = static_cast<AddresseeItem *>( addresseeListView->selectedItem() );

	if ( item )
		m_addressee = item->addressee();

	return m_addressee;
}

void AddressBookSelectorWidget::selectAddressee( const QString &uid )
{
	// iterate trough list view
	QListViewItemIterator it( addresseeListView );
	while( it.current() )
	{
		AddresseeItem *addrItem = (AddresseeItem *) it.current();
		if ( addrItem->addressee().uid() == uid )
		{
			// select the contact item
			addresseeListView->setSelected( addrItem, true );
			addresseeListView->ensureItemVisible( addrItem );
		}
		++it;
	}
}

bool AddressBookSelectorWidget::addresseeSelected()
{
	return addresseeListView->selectedItem() ? true : false;
}

/**  Read in contacts from addressbook, and select the contact that is for our nick. */
void AddressBookSelectorWidget::slotLoadAddressees()
{
	addresseeListView->clear();
	KABC::AddressBook::Iterator it;
	AddresseeItem *addr;
	for( it = m_addressBook->begin(); it != m_addressBook->end(); ++it )
	{
		 addr = new AddresseeItem( addresseeListView, (*it));
	}
	
}

void AddressBookSelectorWidget::setLabelMessage( const QString &msg )
{
	lblHeader->setText(msg);
}

void AddressBookSelectorWidget::slotAddAddresseeClicked()
{
	// Pop up add addressee dialog
	QString addresseeName = KInputDialog::getText( i18n( "New Address Book Entry" ), i18n( "Name the new entry:" ), QString::null, 0, this );

	if ( !addresseeName.isEmpty() )
	{
		KABC::Addressee addr;
		addr.setNameFromString( addresseeName );
		m_addressBook->insertAddressee(addr);
		Kopete::KABCPersistence::self()->writeAddressBook( 0 );
		slotLoadAddressees();
		// select the addressee we just added
		QListViewItem * added = addresseeListView->findItem( addresseeName, 1 );
		addresseeListView->setSelected( added, true );
		addresseeListView->ensureItemVisible( added );
	}
}

#include "addressbookselectorwidget.moc"

// vim: set noet ts=4 sts=4 sw=4:
