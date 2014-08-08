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
using namespace mozilla::ipc;

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

class AddToHashtableRunnable : public nsRunnable
{
public:
  AddToHashtableRunnable(already_AddRefed<ContentParent> aContentParent,
                         PHttpRetargetChannelParent* aHttpRetargetChannelParent)
    :  mContentParent(aContentParent)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mContentParent);
    mHttpRetargetChannelParent =
      static_cast<HttpRetargetChannelParent*>(aHttpRetargetChannelParent);
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

    uint32_t channelId = mHttpRetargetChannelParent->GetChannelId();
    mContentParent->AddHttpRetargetChannel(channelId,
                                           mHttpRetargetChannelParent);

    if (mContentParent->GetMustCallAsyncOpen(channelId)) {
      // | HttpChannelParent::DoAsyncOpen1 | could not call
      // | HttpChannelParent::DoAsyncOpen2 |; that's why it will be called now.

      HttpChannelParent* httpChannel =
        static_cast<HttpChannelParent*>(mContentParent->GetHttpChannel(channelId));

      if (httpChannel)
        httpChannel->DoAsyncOpen2(mHttpRetargetChannelParent);
      else
        return NS_ERROR_FAILURE;

      mContentParent->ResetMustCallAsyncOpen(channelId);
    }

    mContentParent = nullptr;
    return NS_OK;
  }

private:
  nsRefPtr<ContentParent> mContentParent;
  HttpRetargetChannelParent* mHttpRetargetChannelParent;
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
  nsRefPtr<AddToHashtableRunnable> runnable =
    new AddToHashtableRunnable(BackgroundParent::GetContentParent(backgroundParent),
                               this);
  runnable->Dispatch();
  return true;
}

} // namespace net
} // namespace mozilla

