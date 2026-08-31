/*
	SPDX-FileCopyrightText: 2026 EfeDursun125

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <effect/effect.h>
#include <effect/effecthandler.h>

#include <memory>

namespace KWin
{

class SaturationEffect : public Effect
{
	Q_OBJECT

public:
	SaturationEffect();
	~SaturationEffect() override;

	static bool supported();

	void reconfigure(ReconfigureFlags flags) override;
	void drawWindow(const RenderTarget &renderTarget,
					const RenderViewport &viewport,
					EffectWindow *w,
					int mask,
					const Region &region,
					WindowPaintData &data) override;
	bool isActive() const override;

	int requestedEffectChainPosition() const override
	{
		// runs after the other effects so it acts as a final color-grade pass
		return 99;
	}

private:
	void reloadConfig();

	float m_saturation = 1.25f; // 1.0 = neutral, >1 = more vibrant, <1 = desaturated
};

} // namespace KWin
