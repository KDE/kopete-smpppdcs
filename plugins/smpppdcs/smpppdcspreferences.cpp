/*
    smpppdcspreferences.cpp

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

#include <qlayout.h>
#include <qregexp.h>
#include <qradiobutton.h>
#include <QListWidgetItem>

#include <k3listview.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kgenericfactory.h>

#include "kopeteaccount.h"
#include "kopeteprotocol.h"
#include "kopeteaccountmanager.h"

#include "smpppdlocationwidget.h"
#include "smpppdcspreferences.h"
#include "smpppdcsprefsimpl.h"
#include "smpppdcsconfig.h"

typedef KGenericFactory<SMPPPDCSPreferences> SMPPPDCSPreferencesFactory;
K_EXPORT_COMPONENT_FACTORY(kcm_kopete_smpppdcs, SMPPPDCSPreferencesFactory("kcm_kopete_smpppdcs"))

SMPPPDCSPreferences::SMPPPDCSPreferences(QWidget * parent, const QStringList& args)
 : KCModule(SMPPPDCSPreferencesFactory::componentData(), parent, args), m_ui(NULL) {

 	Kopete::AccountManager * manager = Kopete::AccountManager::self(); 

	QVBoxLayout* l = new QVBoxLayout(this);
	QWidget* w = new QWidget;
	m_ui = new Ui::SMPPPDCSPrefsUI;
	m_ui->setupUi(w);
	l->addWidget(w);

	foreach (Kopete::Account* it, manager->accounts())
	{
		QString protoName;
		QRegExp rex("(.*)Protocol");
		
		if(rex.search(it->protocol()->pluginId()) > -1) {
			protoName = rex.cap(1);
		} else {
			protoName = it->protocol()->pluginId();
		}
		
		if(it->inherits("Kopete::ManagedConnectionAccount")) {
			protoName += QString(", %1").arg(i18n("connection status is managed by Kopete"));
		}
		
		QListWidgetItem * item = new QListWidgetItem( it->accountId() + " (" + protoName + ')', m_ui->accountList);
		item->setIcon ( QIcon(it->accountIcon()));
		item->setCheckState (Qt::Unchecked);
		connect ( m_ui->accountList, SIGNAL(itemChanged()), this, SLOT(slotModified()));
		
		m_accountMapOld[item->text()] = AccountPrivMap(false, it->protocol()->pluginId() + '_' + it->accountId());
		m_accountMapCur[item->text()] = AccountPrivMap(false, it->protocol()->pluginId() + '_' + it->accountId());;
		m_ui->accountList->addItem(item);
	}

//	connect(m_ui->accountList, SIGNAL(clicked(QListWidgetItem *)), this, SLOT(listClicked(QListWidgetItem *)));
	// connect for modified
	connect(m_ui->useNetstat, SIGNAL(clicked()), this, SLOT(slotModified()));
	connect(m_ui->useSmpppd,  SIGNAL(clicked()), this, SLOT(slotModified()));
	
	connect(m_ui->SMPPPDLocation->server,   SIGNAL(textChanged(const QString&)), this, SLOT(slotModified()));
	connect(m_ui->SMPPPDLocation->port,     SIGNAL(valueChanged(int)), this, SLOT(slotModified()));
	connect(m_ui->SMPPPDLocation->password, SIGNAL(textChanged(const QString&)), this, SLOT(slotModified()));
	
	load();
}

SMPPPDCSPreferences::~SMPPPDCSPreferences() {
	delete m_ui;
}
/*
void SMPPPDCSPreferences::listClicked(QListWidgetItem * item)
{
	if(item->checkState() != m_accountMapCur[item->text(0)].m_on) {
		AccountMap::iterator itOld = m_accountMapOld.begin();
		AccountMap::iterator itCur;
		bool change = false;
		
		for(itCur = m_accountMapCur.begin(); itCur != m_accountMapCur.end(); ++itCur, ++itOld) {
			if((*itCur).m_on != (*itOld).m_on){
				change = true;
				break;
			}
		}
		emit KCModule::changed(change);
	}
	m_accountMapCur[cli->text(0)].m_on = cli->isOn();
}
*/
void SMPPPDCSPreferences::defaults()
{
	for ( int i = 0; i < m_ui->accountList->count(); i++ )
		m_ui->accountList->item(i)->setCheckState (Qt::Unchecked);
	SMPPPDCSConfig::self()->setDefaults();
	
	m_ui->useNetstat->setChecked(SMPPPDCSConfig::self()->useNetstat());
	m_ui->useSmpppd->setChecked(SMPPPDCSConfig::self()->useSmpppd());
	
	m_ui->SMPPPDLocation->server->setText(SMPPPDCSConfig::self()->server());
	m_ui->SMPPPDLocation->port->setValue(SMPPPDCSConfig::self()->port());
	m_ui->SMPPPDLocation->password->setText(SMPPPDCSConfig::self()->password());
}

void SMPPPDCSPreferences::load()
{
	
	SMPPPDCSConfig::self()->readConfig();
	
	static QString rexStr = "^(.*) \\((.*)\\)";
	QRegExp rex(rexStr);
	QStringList list = SMPPPDCSConfig::self()->ignoredAccounts();
	for ( int i = 0; i < m_ui->accountList->count(); i++ )
	{
		QListWidgetItem * item = m_ui->accountList->item(i);
		if(rex.search(item->text()) > -1) {
			bool isOn = list.contains(rex.cap(2) + "Protocol_" + rex.cap(1));
			if (isOn)
				item->setCheckState (Qt::Checked);
		}
	}
	
	m_ui->useNetstat->setChecked(SMPPPDCSConfig::self()->useNetstat());
	m_ui->useSmpppd->setChecked(SMPPPDCSConfig::self()->useSmpppd());
	
	m_ui->SMPPPDLocation->server->setText(SMPPPDCSConfig::self()->server());
	m_ui->SMPPPDLocation->port->setValue(SMPPPDCSConfig::self()->port());
	m_ui->SMPPPDLocation->password->setText(SMPPPDCSConfig::self()->password());
	
	emit KCModule::changed(false);
}

void SMPPPDCSPreferences::save()
{
	QStringList list;
	QString str, accountStr;
	QRegExp rex(".*\\(.*\\).*");
	for ( int i = 0; i < m_ui->accountList->count(); i++ )
	{
		if(m_ui->accountList->item(i)->checkState()) {
			accountStr = m_ui->accountList->item(i)->text();
			str = rex.lastIndexIn (accountStr);
			str += "Protocol_";
			str += accountStr.left ( accountStr.lastIndexOf ("(") - 1 );
			list.append(str);
		}
	}
	
	SMPPPDCSConfig::self()->setIgnoredAccounts(list);
	
	SMPPPDCSConfig::self()->setUseNetstat(m_ui->useNetstat->isChecked());
	SMPPPDCSConfig::self()->setUseSmpppd(m_ui->useSmpppd->isChecked());
	
	SMPPPDCSConfig::self()->setServer(m_ui->SMPPPDLocation->server->text());
	SMPPPDCSConfig::self()->setPort(m_ui->SMPPPDLocation->port->value());
	SMPPPDCSConfig::self()->setPassword(m_ui->SMPPPDLocation->password->text());
	
	SMPPPDCSConfig::self()->writeConfig();
	
	emit KCModule::changed(false);
}

void SMPPPDCSPreferences::slotModified() {
	emit KCModule::changed(true);
}

#include "smpppdcspreferences.moc"
