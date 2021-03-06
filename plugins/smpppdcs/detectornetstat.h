/*
    detectornetstat.h
 
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

#ifndef DETECTORNETSTAT_H
#define DETECTORNETSTAT_H

#include <qobject.h>

#include "detector.h"

class K3Process;
class IConnector;

/**
	@author Heiko Sch&auml;fer <heiko@rangun.de>
*/
class DetectorNetstat : protected QObject, public Detector {
    Q_OBJECT

public:
    explicit DetectorNetstat(IConnector* connector);
    virtual ~DetectorNetstat();

    virtual void checkStatus() const;

private slots:
    // Original cs-plugin code
    void slotProcessStdout(K3Process * process, char * buffer, int len);

    /**
     * Notify when the netstat process has exited
     */
    void slotProcessExited(K3Process *process);

private:
    Q_DISABLE_COPY(DetectorNetstat)

    mutable QString   m_buffer;
    mutable K3Process *m_process;
};

#endif
