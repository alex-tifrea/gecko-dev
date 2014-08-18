/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsPIThreadRetargetableProgressSink_h__
#define nsPIThreadRetargetableProgressSink_h__

/**
 * nsPIThreadRetargetableProgressSink
 *
 * Should be implemented by progress sinks that support retargeting delivery of
 * data off the main thread.
 */
namespace mozilla {
namespace net {

#define NS_PITHREADRETARGETABLEPROGRESSSINK_IID \
{ 0xfd5d2b45, 0xaba9, 0x4e69, \
  { 0xbf, 0x32, 0xa0, 0x9e, 0x5e, 0x52, 0x41, 0xe7} }

class nsPIThreadRetargetableProgressSink : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PITHREADRETARGETABLEPROGRESSSINK_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIThreadRetargetableProgressSink, NS_PITHREADRETARGETABLEPROGRESSSINK_IID)

} // namespace net
} // namespace mozilla

#endif // nsPIThreadRetargetableProgressSink_h__
