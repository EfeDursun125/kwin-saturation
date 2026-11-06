/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "effect_status_monitor.h"
#include "application_theme.h"

#include <KSharedConfig>
#include <QWidget>
#include <array>
#include <memory>

class QAction;
class QCloseEvent;
class QMenu;
class QPushButton;
class QSystemTrayIcon;
namespace Ui
{
class SaturationWindow;
}

class DigitalVibranceWindow : public QWidget
{
public:
	explicit DigitalVibranceWindow(QWidget *parent = nullptr);
	~DigitalVibranceWindow() override;

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	void applyTheme();
	void setupTray();
	void showWindow();
	void setSaturation(int value);
	void setEffectEnabled(bool enabled);
	void applySettings(bool enabledChanged);
	void updateUIState();

	std::unique_ptr<Ui::SaturationWindow> m_ui;
	const KSharedConfig::Ptr m_config;
	EffectStatusMonitor m_effectMonitor;
	ApplicationTheme m_theme;
	QSystemTrayIcon *m_trayIcon = nullptr;
	QAction *m_trayEnableAction = nullptr;
	QMenu *m_trayMenu = nullptr;
	QMenu *m_trayPresetsMenu = nullptr;
	std::array<QPushButton *, 6> m_presets{};
	double m_currentSaturation = 1.25;
	bool m_isEffectEnabled = true;
};
