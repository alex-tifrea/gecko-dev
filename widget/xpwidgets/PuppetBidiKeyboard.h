/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PuppetBidiKeyboard_h_
#define PuppetBidiKeyboard_h_

#include "nsIBidiKeyboard.h"

class PuppetBidiKeyboard : public nsIBidiKeyboard
{
  bool mIsLangRTL;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  PuppetBidiKeyboard();
  virtual ~PuppetBidiKeyboard();

  NS_IMETHOD SetIsLangRTL(const bool& aIsLangRTL);
};

#endif // PuppetBidiKeyboard_h_
