/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsIAppShell.h"
#include "nsIObserver.h"
#include <Pt.h>

/**
 * Native Photon Application shell wrapper
 */

class nsIEventQueueService;
class EventQueueTokenQueue;

class nsAppShell : public nsIAppShell,
                   public nsIObserver
{
  public:
    nsAppShell(); 
    virtual           ~nsAppShell();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIOBSERVER

    PRBool            OnPaint();

    // nsIAppShellInterface
  
    NS_IMETHOD        Create(int* argc, char ** argv);
    virtual nsresult  Run(); 
    NS_IMETHOD        Spinup();
    NS_IMETHOD        Spindown();
    NS_IMETHOD        GetNativeEvent(PRBool &aRealEvent, void *&aEvent);
    NS_IMETHOD        DispatchNativeEvent(PRBool aRealEvent, void * aEvent);
    NS_IMETHOD        EventIsForModalWindow(PRBool aRealEvent, void *aEvent, nsIWidget *aWidget,
                                  PRBool *aForWindow);

    NS_IMETHOD        Exit();
    NS_IMETHOD        SetDispatchListener(nsDispatchListener* aDispatchListener);

private:
  void          RegisterObserver(PRBool aRegister);

    nsDispatchListener   *mDispatchListener;
	EventQueueTokenQueue *mEventQueueTokens;
    static PRBool        mPtInited;

//  unsigned long        mEventBufferSz;
//  PhEvent_t            *mEvent;
//  nsIEventQueueService * mEventQService;
//	PRLock               *mLock;

};

#endif // nsAppShell_h__
