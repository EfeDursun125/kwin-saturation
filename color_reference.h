/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QPainter>
#include <QWidget>
#include <array>

class ColorReference : public QWidget
{
public:
	explicit ColorReference(QWidget *parent = nullptr) : QWidget(parent)
	{
		setFixedHeight(8);
		setAccessibleName(tr("Color reference"));
		setToolTip(tr("Reference colors are affected by the desktop saturation effect."));
	}

protected:
	void paintEvent(QPaintEvent *) override
	{
		static constexpr std::array<QRgb, 7> colors{
			0xffcb6e65, 0xffd5ae62, 0xffa9ba79, 0xff77b5a6,
			0xff779dbb, 0xffa38db6, 0xffdedbd3
		};
		QPainter painter(this);
		int index = 0;
		for (; index < static_cast<int>(colors.size()); ++index)
		{
			const int left = width() * index / static_cast<int>(colors.size());
			const int right = width() * (index + 1) / static_cast<int>(colors.size());
			painter.fillRect(QRect(left, 0, right - left - 3, height()), QColor::fromRgb(colors[index]));
		}
	}
};
