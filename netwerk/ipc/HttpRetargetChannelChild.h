/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HttpRetargetChannelChild_h
#define _HttpRetargetChannelChild_h

#include "mozilla/net/PHttpChannelChild.h"
#include "mozilla/net/PHttpRetargetChannelChild.h"

namespace mozilla {
namespace net {
class HttpRetargetChannelChild MOZ_FINAL :
  public PHttpRetargetChannelChild
{
public:
  HttpRetargetChannelChild(uint32_t aChannelId);
  ~HttpRetargetChannelChild();

  virtual bool RecvOnTransportAndDataBackground(const nsresult& aChannelStatus,
                                                const nsresult& aTransportStatus,
                                                const uint64_t& aProgress,
                                                const uint64_t& aProgressMax,
                                                const nsCString& aData,
                                                const uint64_t& aOffset,
                                                const uint32_t& aCount);

  virtual bool RecvOnProgressBackground(const uint64_t& aProgress,
                                        const uint64_t& aProgressMax);

  virtual bool RecvOnStatusBackground(const nsresult& aStatus);

  virtual bool RecvOnStopRequestBackground(const nsresult& aStatusCode);

  uint32_t GetChannelId() { return mChannelId; }

  virtual nsresult Init(PHttpChannelChild* aHttpChannel);

  virtual void NotifyRedirect(uint32_t newChannelId);

private:
  uint32_t mChannelId;
  PHttpChannelChild* mHttpChannel;
};

} // namespace net
} // namespace mozilla

#endif // _HttpRetargetChannelChild_h
