/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/Assertions.h"
#include "nsThreadUtils.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"
#include "mozilla/net/HttpRetargetChannelParent.h"
#include "mozilla/net/PHttpRetargetChannelParent.h"
#include "BackgroundParent.h"
#include "mozilla/dom/PContentParent.h"
#include "mozilla/dom/ContentParent.h"

using namespace mozilla::dom;
using namespace mozilla::net;

namespace mozilla {
namespace dom {

class ContentParent;

//-----------------------------------------------------------------------------
// Helper class for sending a reference to the HttpRetargetChannelParent to the
// Content actor on the main thread
//-----------------------------------------------------------------------------

class SendMyselfToMainThread : public nsRunnable
{
public:
    SendMyselfToMainThread(PContentParent* aContentParent,
                           PHttpRetargetChannelParent* aHttpRetargetChannelParent)
      : mContentParent(aContentParent),
        mHttpRetargetChannelParent(aHttpRetargetChannelParent)
    {
        AssertIsOnBackgroundThread();
        MOZ_ASSERT(aContentParent);
    }

    void Dispatch()
    {
        AssertIsOnBackgroundThread();

        nsresult rv = NS_DispatchToMainThread(this);
        NS_ENSURE_SUCCESS_VOID(rv);
    }

    NS_IMETHOD Run()
    {
        AssertIsOnBackgroundThread();
        MOZ_ASSERT(mContentParent);

        ContentParent* tmpContent = static_cast<ContentParent*>(mContentParent);
        HttpRetargetChannelParent* tmpHttpRetarget =
            static_cast<HttpRetargetChannelParent*>(mHttpRetargetChannelParent);
        // TODO: add a method in ContentParent that adds an entry in the
        // hashtable; it should look like this:
        // tmp->AddEntryInHashtable(/* key */ tmpHttpRetarget->GetChannelId(),
        //                          /* value */ aHttpRetargetChannelParent);
        return NS_OK;
    }

private:
    PContentParent* mContentParent;
    PHttpRetargetChannelParent* mHttpRetargetChannelParent;
};

} // namespace mozilla
} // namespace dom

namespace mozilla {
namespace net {

void
AssertIsInMainProcess()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
}

void
HttpRetargetChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

HttpRetargetChannelParent::HttpRetargetChannelParent()
{
  AssertIsInMainProcess();
  MOZ_COUNT_CTOR(HttpRetargetChannelParent);
}

HttpRetargetChannelParent::~HttpRetargetChannelParent()
{
  MOZ_COUNT_DTOR(HttpRetargetChannelParent);
}

bool
HttpRetargetChannelParent::Init(uint32_t aChannelId)
{
  mChannelId = aChannelId;
  return true;
}

} // namespace net
} // namespace mozilla

