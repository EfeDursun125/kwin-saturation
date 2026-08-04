/*
    SPDX-FileCopyrightText: 2026 EfeDursun125

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "saturationeffect.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(SaturationEffect, "metadata.json", return SaturationEffect::supported();)

} // namespace KWin

#include "main.moc"