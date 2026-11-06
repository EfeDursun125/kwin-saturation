/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <KSharedConfig>
#include <QFileSystemWatcher>
#include <QObject>
#include <QPalette>
#include <QTimer>

class ApplicationTheme : public QObject
{
	Q_OBJECT

public:
	enum class Source
	{
		Fallback, Kde, Qt
	};

	explicit ApplicationTheme(QObject *parent = nullptr);
	const QPalette &colors() const
	{
		return m_colors;
	}
	Source source() const
	{
		return m_source;
	}

signals:
	void colorsChanged();

protected:
	bool eventFilter(QObject *object, QEvent *event) override;

private:
	void reload();
	void updateWatches();

	const KSharedConfig::Ptr m_config;
	const QStringList m_configRoots;
	QFileSystemWatcher m_watcher;
	QTimer m_reloadTimer;
	QPalette m_colors;
	Source m_source = Source::Fallback;
};
