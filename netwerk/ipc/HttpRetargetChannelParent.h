/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HttpRetargetChannelParent_h
#define _HttpRetargetChannelParent_h

#include "mozilla/net/PHttpRetargetChannelParent.h"

namespace mozilla {
namespace net {

class HttpRetargetChannelParent MOZ_FINAL :
  public PHttpRetargetChannelParent
{
public:
  virtual void ActorDestroy(ActorDestroyReason aWhy);

  HttpRetargetChannelParent();
  ~HttpRetargetChannelParent();
  virtual bool Init(uint32_t aChannelId);

  uint32_t GetChannelId() { return mChannelId; }

private:
  uint32_t mChannelId;
};

} // namespace net
} // namespace mozilla

#endif // _HttpRetargetChannelParent_h
