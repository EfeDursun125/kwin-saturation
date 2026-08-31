/*
	SPDX-FileCopyrightText: 2026 EfeDursun125

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "saturationeffect.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace KWin
{

SaturationEffect::SaturationEffect()
{
	reloadConfig();
}

SaturationEffect::~SaturationEffect() = default;

bool SaturationEffect::supported()
{
	return effects->isOpenGLCompositing();
}

void SaturationEffect::reconfigure(ReconfigureFlags flags)
{
	Q_UNUSED(flags)
	reloadConfig();
}

void SaturationEffect::reloadConfig()
{
	auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
	config->reparseConfiguration();
	KConfigGroup cfg(config, "Effect-saturationeffect");
	const bool enabled = cfg.readEntry("Enabled", true);
	m_saturation = enabled ? static_cast<float>(cfg.readEntry("Saturation", 1.25)) : 1.0f;
	effects->addRepaintFull();
}

bool SaturationEffect::isActive() const
{
	return !qFuzzyCompare(m_saturation, 1.0f);
}

void SaturationEffect::drawWindow(const RenderTarget &renderTarget,
								  const RenderViewport &viewport,
								  EffectWindow *w,
								  int mask,
								  const Region &region,
								  WindowPaintData &data)
{
	data.multiplySaturation(m_saturation);
	effects->drawWindow(renderTarget, viewport, w, mask, region, data);
}

} // namespace KWin