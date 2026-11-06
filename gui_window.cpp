/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "gui_window.h"
#include "ui_saturation_window.h"

#include <KConfigGroup>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QMenu>
#include <QPalette>
#include <QScreen>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSystemTrayIcon>
#include <algorithm>
#include <cmath>
#include <new>
#include <iostream>

namespace
{
constexpr std::array<int, 6> presetValues{0, 20, 23, 28, 35, 40};
const QString effectGroup = QStringLiteral("Effect-saturationeffect");
const QString pluginKey = QStringLiteral("saturationeffectEnabled");

void reconfigureEffect(bool enabledChanged)
{
	QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
		QStringLiteral("/Effects"), QStringLiteral("org.kde.kwin.Effects"), QStringLiteral("reconfigureEffect"));
	message << QStringLiteral("saturationeffect");
	message.setAutoStartService(false);
	QDBusConnection::sessionBus().send(message);
	if (!enabledChanged)
		return;

	message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
		QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reconfigure"));
	message.setAutoStartService(false);
	QDBusConnection::sessionBus().send(message);
}
}

DigitalVibranceWindow::DigitalVibranceWindow(QWidget *parent)
	: QWidget(parent)
	, m_ui(new(std::nothrow) Ui::SaturationWindow)
	, m_config(KSharedConfig::openConfig(QStringLiteral("kwinrc")))
{
	if (!m_ui)
	{
		std::cerr << "Cannot create the saturation window: insufficient memory.\n";
		QTimer::singleShot(0, qApp, &QApplication::quit);
		return;
	}

	m_ui->setupUi(this);
	setWindowIcon(QIcon(QStringLiteral(":/kwin-saturation.svg")));
	connect(&m_theme, &ApplicationTheme::colorsChanged, this, &DigitalVibranceWindow::applyTheme);
	applyTheme();

	const KConfigGroup config(m_config, effectGroup);
	const double savedSaturation = config.readEntry(QStringLiteral("Saturation"), 1.25);
	m_currentSaturation = std::isfinite(savedSaturation) ? std::clamp(savedSaturation, 0.0, 2.5) : 1.25;
	m_isEffectEnabled = config.readEntry(QStringLiteral("Enabled"), true)
		&& KConfigGroup(m_config, QStringLiteral("Plugins")).readEntry(pluginKey, true);
	m_ui->vibranceSlider->setValue(static_cast<int>(std::round(m_currentSaturation * 20.0)));
	m_presets = {m_ui->grayscalePreset, m_ui->originalPreset, m_ui->cinemaPreset,
		m_ui->boostPreset, m_ui->vividPreset, m_ui->strongPreset};

	std::size_t index = 0;
	for (; index < m_presets.size(); ++index)
	{
		connect(m_presets[index], &QPushButton::clicked, this, [this, index]()
		{
			m_ui->vibranceSlider->setValue(presetValues[index]);
			updateUIState();
		});
	}
	connect(m_ui->vibranceSlider, &QSlider::valueChanged, this, &DigitalVibranceWindow::setSaturation);
	connect(m_ui->decreaseButton, &QPushButton::clicked, this, [this]()
	{
		m_ui->vibranceSlider->setValue(m_ui->vibranceSlider->value() - 1);
	});
	connect(m_ui->increaseButton, &QPushButton::clicked, this, [this]()
	{
		m_ui->vibranceSlider->setValue(m_ui->vibranceSlider->value() + 1);
	});
	connect(m_ui->enableCheckBox, &QCheckBox::toggled, this, &DigitalVibranceWindow::setEffectEnabled);
	connect(m_ui->retryEffectButton, &QPushButton::clicked, this, [this]()
	{
		m_effectMonitor.refresh(true);
	});
	connect(&m_effectMonitor, &EffectStatusMonitor::statusChanged, this, &DigitalVibranceWindow::updateUIState);

	auto *const quitShortcut = new(std::nothrow) QShortcut(QKeySequence::Quit, this);
	if (quitShortcut)
		connect(quitShortcut, &QShortcut::activated, qApp, &QApplication::quit);
	auto *const closeShortcut = new(std::nothrow) QShortcut(QKeySequence(Qt::Key_Escape), this);
	if (closeShortcut)
		connect(closeShortcut, &QShortcut::activated, this, &QWidget::close);

	setupTray();
	m_effectMonitor.setEffectEnabled(m_isEffectEnabled);
	updateUIState();
	m_effectMonitor.refresh(true);
	adjustSize();
	if (const QScreen *const screen = QGuiApplication::primaryScreen())
		move(screen->availableGeometry().center() - rect().center());
}

DigitalVibranceWindow::~DigitalVibranceWindow() = default;

void DigitalVibranceWindow::applyTheme()
{
	QString stylesheet = QStringLiteral(
		"QWidget#SaturationWindow { background: palette(window); }"
		"QLabel, QCheckBox { color: palette(window-text); }"
		"QLabel#titleLabel { font-size: 24px; font-weight: 600; }"
		"QLabel#subtitleLabel, QLabel#descriptionLabel, QLabel#footerLabel { color: %1; }"
		"QLabel#statusBadge { color: palette(highlight); }"
		"QLabel#percentageLabel { font-size: 56px; font-weight: 500; }"
		"QLabel#presetsLabel { font-weight: 600; }"
		"QLabel#minimumLabel, QLabel#maximumLabel { color: %1; }"
		"QLabel#errorTitle { color: palette(window-text); font-size: 18px; font-weight: 600; }"
		"QLabel#effectNotice { color: %1; }"
		"QFrame#separator { color: palette(midlight); max-height: 1px; }"
		"QPushButton { color: palette(button-text); background: palette(button); border: 1px solid palette(midlight); border-radius: 8px; padding: 9px 12px; }"
		"QPushButton:hover { border-color: palette(mid); }"
		"QPushButton:checked { color: palette(bright-text); background: palette(alternate-base); border-color: palette(highlight); }"
		"QPushButton:pressed { color: palette(highlighted-text); background: palette(highlight); }"
		"QPushButton:focus { border-color: palette(highlight); }"
		"QPushButton#decreaseButton, QPushButton#increaseButton { padding: 0; min-width: 30px; max-width: 30px; min-height: 30px; font-size: 18px; }"
		"QSlider::groove:horizontal { height: 4px; background: palette(midlight); border-radius: 2px; }"
		"QSlider::sub-page:horizontal { background: palette(highlight); border-radius: 2px; }"
		"QSlider::handle:horizontal { width: 14px; margin: -6px 0; background: palette(highlight); border: 1px solid palette(window); border-radius: 7px; }"
		"QSlider::handle:horizontal:hover, QSlider::handle:horizontal:focus { background: palette(window-text); }"
	).arg(m_theme.colors().color(QPalette::PlaceholderText).name());
	struct ColorToken
	{
		const char *name;
		QPalette::ColorRole role;
	};
	static constexpr std::array<ColorToken, 11> tokens{{
		{"window", QPalette::Window}, {"window-text", QPalette::WindowText},
		{"highlight", QPalette::Highlight}, {"midlight", QPalette::Midlight},
		{"button-text", QPalette::ButtonText}, {"button", QPalette::Button},
		{"mid", QPalette::Mid}, {"bright-text", QPalette::BrightText},
		{"alternate-base", QPalette::AlternateBase}, {"highlighted-text", QPalette::HighlightedText},
		{"text", QPalette::Text}
	}};
	std::size_t index = 0;
	for (; index < tokens.size(); ++index)
	{
		stylesheet.replace(QStringLiteral("palette(%1)").arg(QLatin1String(tokens[index].name)),
			m_theme.colors().color(tokens[index].role).name());
	}
	setStyleSheet(stylesheet);
	setPalette(m_theme.colors());
	m_ui->enableCheckBox->setPalette(m_theme.colors());
	if (m_trayMenu)
		m_trayMenu->setPalette(m_theme.colors());
	if (m_trayPresetsMenu)
		m_trayPresetsMenu->setPalette(m_theme.colors());
}

void DigitalVibranceWindow::closeEvent(QCloseEvent *event)
{
	if (m_trayIcon && m_trayIcon->isVisible() && QSystemTrayIcon::isSystemTrayAvailable())
	{
		hide();
		event->ignore();
		return;
	}

	event->accept();
	qApp->quit();
}

void DigitalVibranceWindow::showWindow()
{
	showNormal();
	activateWindow();
	raise();
}

void DigitalVibranceWindow::setupTray()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	m_trayIcon = new(std::nothrow) QSystemTrayIcon(windowIcon(), this);
	if (!m_trayIcon)
		return;

	m_trayMenu = new(std::nothrow) QMenu(this);
	if (!m_trayMenu)
		return;

	m_trayMenu->setPalette(m_theme.colors());
	auto *const menu = m_trayMenu;
	menu->addAction(tr("Show Digital Vibrance"), this, &DigitalVibranceWindow::showWindow);
	menu->addSeparator();
	m_trayEnableAction = menu->addAction(tr("Enable effect"));
	if (m_trayEnableAction)
	{
		m_trayEnableAction->setCheckable(true);
		connect(m_trayEnableAction, &QAction::toggled, this, &DigitalVibranceWindow::setEffectEnabled);
	}
	m_trayPresetsMenu = menu->addMenu(tr("Presets"));
	if (m_trayPresetsMenu)
	{
		m_trayPresetsMenu->setPalette(m_theme.colors());
		std::size_t index = 0;
		for (; index < m_presets.size(); ++index)
		{
			m_trayPresetsMenu->addAction(m_presets[index]->text(), this, [this, index]()
			{
				m_ui->vibranceSlider->setValue(presetValues[index]);
			});
		}
	}
	menu->addSeparator();
	menu->addAction(tr("Quit"), qApp, &QApplication::quit);
	m_trayIcon->setContextMenu(menu);
	connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
	{
		if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
			return;
		if (isVisible())
			hide();
		else
			showWindow();
	});
	m_trayIcon->show();
}

void DigitalVibranceWindow::setSaturation(int value)
{
	if (!m_effectMonitor.controlsAvailable())
		return;

	m_currentSaturation = value / 20.0;
	updateUIState();
	applySettings(false);
}

void DigitalVibranceWindow::setEffectEnabled(bool enabled)
{
	if (!m_effectMonitor.controlsAvailable() || m_isEffectEnabled == enabled)
		return;

	m_isEffectEnabled = enabled;
	applySettings(true);
	m_effectMonitor.setEffectEnabled(enabled);
	updateUIState();
}

void DigitalVibranceWindow::applySettings(bool enabledChanged)
{
	KConfigGroup config(m_config, effectGroup);
	config.writeEntry(QStringLiteral("Saturation"), m_currentSaturation);
	config.writeEntry(QStringLiteral("Enabled"), m_isEffectEnabled);
	if (enabledChanged)
		KConfigGroup(m_config, QStringLiteral("Plugins")).writeEntry(pluginKey, m_isEffectEnabled);
	m_config->sync();

	if (m_isEffectEnabled || enabledChanged)
		reconfigureEffect(enabledChanged);
}

void DigitalVibranceWindow::updateUIState()
{
	if (!m_ui)
		return;

	const bool available = m_effectMonitor.controlsAvailable();
	if (m_ui->controls->isHidden() == available)
	{
		m_ui->controls->setVisible(available);
		QTimer::singleShot(0, this, &QWidget::adjustSize);
	}
	m_ui->controls->setEnabled(available);
	m_ui->statusPanel->setVisible(!available);
	m_ui->retryEffectButton->setVisible(!available && m_effectMonitor.state() != EffectStatusMonitor::State::Checking);
	if (m_trayEnableAction)
	{
		const QSignalBlocker blocker(m_trayEnableAction);
		m_trayEnableAction->setEnabled(available);
		m_trayEnableAction->setChecked(m_isEffectEnabled);
	}
	if (m_trayPresetsMenu)
		m_trayPresetsMenu->setEnabled(available);
	{
		const QSignalBlocker blocker(m_ui->enableCheckBox);
		m_ui->enableCheckBox->setChecked(m_isEffectEnabled);
	}

	if (!available)
	{
		switch (m_effectMonitor.state())
		{
		case EffectStatusMonitor::State::Checking:
			m_ui->errorTitle->setText(tr("Checking the effect"));
			m_ui->effectNotice->setText(tr("Waiting for KWin to report the plugin status."));
			break;
		case EffectStatusMonitor::State::Missing:
			m_ui->errorTitle->setText(tr("Plugin not found"));
			m_ui->effectNotice->setText(tr("KWin cannot find the saturation plugin. Install it or rebuild and reinstall it for your current KWin version, then log out and back in."));
			break;
		case EffectStatusMonitor::State::Unsupported:
			m_ui->errorTitle->setText(tr("Plugin is not supported"));
			m_ui->effectNotice->setText(tr("KWin cannot use the saturation plugin. Check that OpenGL compositing is enabled. If this started after a KWin update, rebuild and reinstall the plugin, then log out and back in."));
			break;
		case EffectStatusMonitor::State::LoadFailed:
			m_ui->errorTitle->setText(tr("Plugin could not be loaded"));
			m_ui->effectNotice->setText(tr("The saturation plugin is not loaded in KWin. If KWin was updated, rebuild and reinstall the plugin, then log out and back in. Check again to retry loading."));
			break;
		default:
			m_ui->errorTitle->setText(tr("KWin is unavailable"));
			m_ui->effectNotice->setText(tr("Cannot contact KWin or read its effect status. A running KDE Plasma 6 session is required. Check again when KWin is available."));
			break;
		}
		m_ui->statusBadge->setText(m_effectMonitor.state() == EffectStatusMonitor::State::Checking ? tr("Checking…") : tr("Unavailable"));
	}
	else
	{
		m_ui->statusBadge->setText(m_isEffectEnabled ? tr("Connected") : tr("Paused"));
		m_ui->percentageLabel->setText(QStringLiteral("%1%").arg(static_cast<int>(std::round(m_currentSaturation * 100.0))));
		m_ui->descriptionLabel->setText(m_isEffectEnabled
			? tr("%1 × original saturation").arg(m_currentSaturation, 0, 'f', 2)
			: tr("Effect paused · original colors"));
		std::size_t index = 0;
		for (; index < m_presets.size(); ++index)
			m_presets[index]->setChecked(qFuzzyCompare(m_currentSaturation + 1.0, presetValues[index] / 20.0 + 1.0));
	}
	if (m_trayIcon)
		m_trayIcon->setToolTip(tr("Digital Vibrance — %1").arg(m_ui->statusBadge->text()));
}
