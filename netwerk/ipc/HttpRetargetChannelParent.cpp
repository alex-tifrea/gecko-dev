/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpRetargetChannelParent.h"

namespace mozilla {
namespace net {
void
HttpRetargetChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

HttpRetargetChannelParent::HttpRetargetChannelParent()
{
  MOZ_COUNT_CTOR(HttpRetargetChannelParent);
}

HttpRetargetChannelParent::~HttpRetargetChannelParent()
{
  MOZ_COUNT_DTOR(HttpRetargetChannelParent);
}

} // namespace net
} // namespace mozilla

