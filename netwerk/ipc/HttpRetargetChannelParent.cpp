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
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/PBackgroundParent.h"
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

class AddToHashtableRunnable : public nsRunnable
{
public:
  AddToHashtableRunnable(already_AddRefed<ContentParent> aContentParent,
                         PHttpRetargetChannelParent* aHttpRetargetChannelParent)
    :  mContentParent(aContentParent)
  {
    mozilla::ipc::AssertIsOnBackgroundThread();
    MOZ_ASSERT(mContentParent);
    mHttpRetargetChannelParent =
      static_cast<HttpRetargetChannelParent*>(aHttpRetargetChannelParent);
  }

  void Dispatch()
  {
    mozilla::ipc::AssertIsOnBackgroundThread();

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
      // `HttpChannelParent::AsyncOpen` needs to be delayed until after the
      // `HttpRetargetChannelParent` object has been created and added to the
      // `mHttpRetargetChannels` hashtable that resides in `ContentParent`. In
      // order to do this, the old `HttpChannelParent::DoAsyncOpen` has been
      // split into two: the first part does the required initializations while
      // the second half calls `HttpChannelParent::AsyncOpen`. If the
      // corresponding `HttpRetargetChannelParent` object is not in the
      // hashtable by the time `HttpChannelParent::DoAsyncOpen2` should be called,
      // then the responsability for calling `HttpChannelParent::DoAsyncOpen2` is
      // passed to `AddHttpRetargetChannel::Run()`.

      HttpChannelParent* httpChannel =
        static_cast<HttpChannelParent*>(mContentParent->GetHttpChannel(channelId));

      if (httpChannel)
        httpChannel->DoAsyncOpen2();
      else
        return NS_ERROR_FAILURE;

      httpChannel->SetHttpRetargetChannel(mHttpRetargetChannelParent);
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
  //TODO: instead of using BackgroundParent::GetBackgroundThread(),
  // save the current thread here as a member
  mozilla::ipc::AssertIsOnBackgroundThread();

  mozilla::ipc::PBackgroundParent* backgroundParent = this->Manager();

  mChannelId = aChannelId;
  nsRefPtr<AddToHashtableRunnable> runnable =
    new AddToHashtableRunnable(mozilla::ipc::BackgroundParent::GetContentParent(backgroundParent),
                               this);
  runnable->Dispatch();
  return true;
}

} // namespace net
} // namespace mozilla

