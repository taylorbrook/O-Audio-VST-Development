/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-simpleWavetable - BuiltInBanks implementation (Stage 2.1)

  ==============================================================================
*/

#include "BuiltInBanks.h"

#include "BankFactory.h"
#include "MipmapBuilder.h"

BuiltInBanks::BuiltInBanks()
{
    const double t0 = juce::Time::getMillisecondCounterHiRes();

    MipmapBuilder builder;   // one FFT for all five banks (this thread only)
    for (int i = 0; i < kCount; ++i)
        BankFactory::build (i, banks[(size_t) i], builder);

    buildMillis = juce::Time::getMillisecondCounterHiRes() - t0;
}
