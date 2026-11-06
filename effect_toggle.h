/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QCheckBox>
#include <QPainter>
#include <algorithm>

// Keep QCheckBox's keyboard and accessibility behavior while drawing a switch.
class EffectToggle : public QCheckBox
{
public:
	explicit EffectToggle(QWidget *parent = nullptr) : QCheckBox(parent)
	{
		setCursor(Qt::PointingHandCursor);
		setFocusPolicy(Qt::StrongFocus);
		setAccessibleName(tr("Enable saturation effect"));
	}

	QSize sizeHint() const override
	{
		return QSize(fontMetrics().horizontalAdvance(text()) + 64,
			std::max(32, fontMetrics().height() + 12));
	}

	QSize minimumSizeHint() const override
	{
		return sizeHint();
	}

protected:
	bool hitButton(const QPoint &position) const override
	{
		return rect().contains(position);
	}

	void paintEvent(QPaintEvent *) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		if (!isEnabled())
			painter.setOpacity(0.45);

		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawText(QRect(4, 0, width() - 60, height()), Qt::AlignLeft | Qt::AlignVCenter, text());

		const QRectF track(width() - 48, (height() - 24) / 2.0, 44, 24);
		painter.setPen(isChecked() ? QPen(Qt::NoPen) : QPen(palette().color(QPalette::Mid), 1.5));
		painter.setBrush(palette().color(isChecked() ? QPalette::Highlight : QPalette::Button));
		painter.drawRoundedRect(track, 12, 12);
		painter.setPen(Qt::NoPen);
		painter.setBrush(palette().color(isChecked() ? QPalette::HighlightedText : QPalette::WindowText));
		painter.drawEllipse(QPointF(isChecked() ? track.right() - 12 : track.left() + 12,
			track.center().y()), 8, 8);

		if (hasFocus())
		{
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(palette().color(QPalette::Highlight), 1));
			painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 5, 5);
		}
	}
};
