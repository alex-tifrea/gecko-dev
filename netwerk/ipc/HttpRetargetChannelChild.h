/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HttpRetargetChannelChild_h
#define _HttpRetargetChannelChild_h

#include "mozilla/net/HttpChannelChild.h"
#include "mozilla/net/PHttpRetargetChannelChild.h"
//#include "nsIRequest.h"
// #include "mozilla/net/HttpBaseChannel.h"

namespace mozilla {
namespace net {
class HttpRetargetChannelChild MOZ_FINAL :
  public PHttpRetargetChannelChild
{
public:
  HttpRetargetChannelChild(uint32_t aChannelId);
  ~HttpRetargetChannelChild();

  virtual bool RecvOnTransportAndData(const nsresult& channelStatus,
                                      const nsresult& transportStatus,
                                      const uint64_t& progress,
                                      const uint64_t& progressMax,
                                      const nsCString& data,
                                      const uint64_t& offset,
                                      const uint32_t& count);

  uint32_t GetChannelId() { return mChannelId; }

  virtual nsresult Init(HttpChannelChild* aHttpChannel);

private:
  uint32_t mChannelId;
//  HttpChannelChild* mHttpChannel;
};

} // namespace net
} // namespace mozilla

#endif // _HttpRetargetChannelChild_h
