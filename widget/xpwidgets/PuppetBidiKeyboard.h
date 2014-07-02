/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PuppetBidiKeyboard_h_
#define PuppetBidiKeyboard_h_

#include "nsIBidiKeyboard.h"

namespace mozilla {
namespace widget {
class PuppetBidiKeyboard : public nsIBidiKeyboard
{
private:
    bool mIsLangRTL;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIBIDIKEYBOARD

    PuppetBidiKeyboard();
    virtual ~PuppetBidiKeyboard();

    void SetIsLangRTL(bool aIsLangRTL);
};

} // namespace widget
} // namespace mozilla

#endif // PuppetBidiKeyboard_h_
