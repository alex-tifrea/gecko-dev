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
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/HttpChannelParent.h"

using namespace mozilla::dom;
using namespace mozilla::net;

namespace {

void
AssertIsOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

} // anonymous namespace


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
  SendMyselfToMainThread(already_AddRefed<ContentParent> aContentParent,
                         PHttpRetargetChannelParent* aHttpRetargetChannelParent)
    :  mContentParent(aContentParent),
       mHttpRetargetChannelParent(aHttpRetargetChannelParent)

  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mContentParent);
  }

  void Dispatch()
  {
    AssertIsOnBackgroundThread();

    nsresult rv = NS_DispatchToMainThread(this);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  NS_IMETHOD Run()
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(mContentParent);

    HttpRetargetChannelParent* tmpHttpRetarget =
        static_cast<HttpRetargetChannelParent*>(mHttpRetargetChannelParent);

    mContentParent->AddHttpRetargetChannel(/* key */ tmpHttpRetarget->GetChannelId(),
                                           /* value */ mHttpRetargetChannelParent);

    // TODO: need to debug this to see if it ever gets to this part of the code
    if (mContentParent->GetMustCallAsyncOpen()) {
      // | HttpChannelParent::DoAsyncOpen1 | could not call
      // | HttpChannelParent::DoAsyncOpen2 |; that's why it will be called now.

      // TODO: I assumed that a | ContentParent | actor manages only one
      // instance of | NeckoParent |
      const InfallibleTArray<PHttpChannelParent*>& channels =
        mContentParent->ManagedPNeckoParent()[0]->ManagedPHttpChannelParent();

      // Searching through the HttpChannelParent actors for the one
      // corresponding to the current HttpRetargetChannelParent instance
      for (uint32_t i = 0; i < channels.Length(); i++) {
        HttpChannelParent* curr_channel = static_cast<HttpChannelParent*>(channels[i]);
        if (curr_channel->GetChannelID() ==
            static_cast<HttpRetargetChannelParent*>(mHttpRetargetChannelParent)->GetChannelId()) {
          curr_channel->DoAsyncOpen2(mHttpRetargetChannelParent);
        }
      }
    }

    mContentParent = NULL;
    return NS_OK;
  }

private:
  nsRefPtr<ContentParent> mContentParent;
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
  AssertIsOnBackgroundThread();

  PBackgroundParent* backgroundParent = this->Manager();

  mChannelId = aChannelId;
  nsRefPtr<SendMyselfToMainThread> runnable =
    new SendMyselfToMainThread(BackgroundParent::GetContentParent(backgroundParent),
                               this);
  runnable->Dispatch();
  return true;
}

} // namespace net
} // namespace mozilla

