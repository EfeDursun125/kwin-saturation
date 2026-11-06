/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "application_theme.h"

#include <KConfigGroup>
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStyle>
#include <array>
#include <cmath>

namespace
{
QColor readColor(const KConfigGroup &group, const char *key)
{
	const QString value = group.readEntry(key, QString()).trimmed();
	if (value.isEmpty())
		return {};
	if (!value.contains(QLatin1Char(',')))
		return QColor(value);

	const QStringList components = value.split(QLatin1Char(','));
	if (components.size() != 3 && components.size() != 4)
		return {};

	std::array<int, 4> rgba{0, 0, 0, 255};
	qsizetype index = 0;
	for (; index < components.size(); ++index)
	{
		bool valid = false;
		rgba[index] = components[index].trimmed().toInt(&valid);
		if (!valid || rgba[index] < 0 || rgba[index] > 255)
			return {};
	}
	return QColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

QColor colorOr(const KConfigGroup &group, const char *key, const QColor &fallback)
{
	const QColor value = readColor(group, key);
	return value.isValid() ? value : fallback;
}

QColor blend(const QColor &first, const QColor &second, double amount)
{
	return QColor::fromRgbF(first.redF() * (1.0 - amount) + second.redF() * amount,
		first.greenF() * (1.0 - amount) + second.greenF() * amount,
		first.blueF() * (1.0 - amount) + second.blueF() * amount);
}

double luminance(const QColor &color)
{
	const auto linear = [](double component)
	{
		return component <= 0.04045 ? component / 12.92 : std::pow((component + 0.055) / 1.055, 2.4);
	};
	return linear(color.redF()) * 0.2126 + linear(color.greenF()) * 0.7152 + linear(color.blueF()) * 0.0722;
}

double contrast(const QColor &first, const QColor &second)
{
	const double firstLuminance = luminance(first) + 0.05;
	const double secondLuminance = luminance(second) + 0.05;
	return firstLuminance > secondLuminance ? firstLuminance / secondLuminance : secondLuminance / firstLuminance;
}

QColor readableText(const QColor &background, const QColor &preferred)
{
	if (preferred.isValid() && contrast(background, preferred) >= 4.5)
		return preferred;
	return luminance(background) > 0.179 ? QColor(Qt::black) : QColor(Qt::white);
}

QPalette fallbackPalette()
{
	// Material 3 roles matching the warm dark reference, independent of KDE.
	QPalette palette;
	palette.setColor(QPalette::Window, QColor::fromRgb(0xff1a1110));
	palette.setColor(QPalette::WindowText, QColor::fromRgb(0xfff1dfdc));
	palette.setColor(QPalette::Base, QColor::fromRgb(0xff271d1c));
	palette.setColor(QPalette::Text, QColor::fromRgb(0xfff1dfdc));
	palette.setColor(QPalette::Button, QColor::fromRgb(0xff322826));
	palette.setColor(QPalette::ButtonText, QColor::fromRgb(0xfff1dfdc));
	palette.setColor(QPalette::Highlight, QColor::fromRgb(0xffffb4a8));
	palette.setColor(QPalette::HighlightedText, QColor::fromRgb(0xff561e16));
	palette.setColor(QPalette::AlternateBase, QColor::fromRgb(0xff73342b));
	palette.setColor(QPalette::BrightText, QColor::fromRgb(0xffffdad4));
	palette.setColor(QPalette::PlaceholderText, QColor::fromRgb(0xffd8c2be));
	palette.setColor(QPalette::Mid, QColor::fromRgb(0xffa08c89));
	palette.setColor(QPalette::Midlight, QColor::fromRgb(0xff534340));
	return palette;
}

bool applyKdeColors(const KSharedConfig::Ptr &config, QPalette &palette)
{
	const KConfigGroup window(config, QStringLiteral("Colors:Window"));
	const KConfigGroup view(config, QStringLiteral("Colors:View"));
	const KConfigGroup button(config, QStringLiteral("Colors:Button"));
	const KConfigGroup selection(config, QStringLiteral("Colors:Selection"));
	const QColor surface = colorOr(view, "BackgroundNormal", readColor(window, "BackgroundNormal"));
	const QColor text = readColor(window, "ForegroundNormal");
	const QColor primary = readColor(selection, "BackgroundNormal");
	if (!surface.isValid() || !text.isValid() || !primary.isValid())
		return false;

	palette.setColor(QPalette::Window, surface);
	palette.setColor(QPalette::WindowText, text);
	palette.setColor(QPalette::Base, colorOr(window, "BackgroundNormal", surface));
	palette.setColor(QPalette::Text, text);
	palette.setColor(QPalette::Button, colorOr(button, "BackgroundNormal", blend(surface, text, 0.08)));
	palette.setColor(QPalette::ButtonText, colorOr(button, "ForegroundNormal", text));
	palette.setColor(QPalette::Highlight, primary);
	const QColor selectionText = readColor(selection, "ForegroundActive");
	palette.setColor(QPalette::HighlightedText, readableText(primary,
		selectionText.isValid() && contrast(primary, selectionText) >= 4.5
			? selectionText : readColor(selection, "ForegroundNormal")));
	palette.setColor(QPalette::AlternateBase, colorOr(window, "BackgroundAlternate", blend(surface, primary, 0.18)));
	palette.setColor(QPalette::BrightText, readableText(palette.color(QPalette::AlternateBase), text));
	palette.setColor(QPalette::PlaceholderText, colorOr(window, "ForegroundInactive", blend(surface, text, 0.7)));
	palette.setColor(QPalette::Mid, blend(surface, text, 0.55));
	palette.setColor(QPalette::Midlight, blend(surface, text, 0.2));
	return true;
}

void finishPalette(QPalette &palette)
{
	palette.setColor(QPalette::ToolTipBase, palette.color(QPalette::Base));
	palette.setColor(QPalette::ToolTipText, palette.color(QPalette::WindowText));
	palette.setColor(QPalette::Link, palette.color(QPalette::Highlight));
	constexpr std::array<QPalette::ColorRole, 4> textRoles{
		QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::PlaceholderText
	};
	std::size_t index = 0;
	for (; index < textRoles.size(); ++index)
	{
		palette.setColor(QPalette::Disabled, textRoles[index], blend(palette.color(QPalette::Window), palette.color(QPalette::Active, textRoles[index]), 0.45));
	}
}
}

ApplicationTheme::ApplicationTheme(QObject *parent)
	: QObject(parent)
	, m_config(KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals))
	, m_configRoots(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation))
{
	m_reloadTimer.setSingleShot(true);
	m_reloadTimer.setInterval(80);
	connect(&m_reloadTimer, &QTimer::timeout, this, &ApplicationTheme::reload);
	connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this]()
	{
		m_reloadTimer.start();
	});
	connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ApplicationTheme::updateWatches);
	qApp->installEventFilter(this);
	reload();
}

bool ApplicationTheme::eventFilter(QObject *object, QEvent *event)
{
	if (object == qApp && (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange))
		m_reloadTimer.start();
	return false;
}

void ApplicationTheme::updateWatches()
{
	const QStringList files = m_watcher.files();
	const QStringList directories = m_watcher.directories();
	QStringList::const_iterator root = m_configRoots.cbegin();
	for (; root != m_configRoots.cend(); ++root)
	{
		const QString file = QDir(*root).filePath(QStringLiteral("kdeglobals"));
		if (QFileInfo::exists(file) && !files.contains(file))
		{
			m_watcher.addPath(file);
			m_reloadTimer.start();
		}
		QDir directory(*root);
		while (!directory.exists() && directory.cdUp())
		{
		}
		if (directory.exists() && !directories.contains(directory.absolutePath()))
			m_watcher.addPath(directory.absolutePath());
	}
}

void ApplicationTheme::reload()
{
	updateWatches();
	m_reloadTimer.stop();
	m_config->reparseConfiguration();
	QPalette next = fallbackPalette();
	Source source = Source::Fallback;
	if (applyKdeColors(m_config, next))
		source = Source::Kde;
	else if (QApplication::palette() != QApplication::style()->standardPalette())
	{
		next = QApplication::palette();
		next.setColor(QPalette::PlaceholderText, blend(next.color(QPalette::Window), next.color(QPalette::WindowText), 0.7));
		next.setColor(QPalette::AlternateBase, blend(next.color(QPalette::Window), next.color(QPalette::Highlight), 0.18));
		next.setColor(QPalette::BrightText, readableText(next.color(QPalette::AlternateBase), next.color(QPalette::WindowText)));
		source = Source::Qt;
	}
	finishPalette(next);
	if (m_colors == next && m_source == source)
		return;
	m_colors = next;
	m_source = source;
	emit colorsChanged();
}
