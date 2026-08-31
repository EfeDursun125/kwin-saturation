/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFrame>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QIcon>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDBusMessage>
#include <QDBusConnection>

#include <KSharedConfig>
#include <KConfigGroup>
#include <cmath>
#include <new>
#include <iostream>

class DigitalVibranceWindow : public QWidget
{
public:
	DigitalVibranceWindow(QWidget *parent = nullptr) : QWidget(parent)
	{
		setWindowTitle(QStringLiteral("Digital Vibrance - KWin Saturation"));
		QIcon appIcon(QStringLiteral(":/kwin-saturation.svg"));
		if (appIcon.isNull())
			appIcon = QIcon::fromTheme(QStringLiteral("kwin-saturation"));

		setWindowIcon(appIcon);
		setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
		setAttribute(Qt::WA_TranslucentBackground, true);
		resize(470, 430);

		createWinId();

		KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Effect-saturationeffect"));
		m_currentSaturation = config.readEntry(QStringLiteral("Saturation"), 1.25);
		m_isEffectEnabled = config.readEntry(QStringLiteral("Enabled"), true);

		if (!setupUI())
		{
			std::cerr << "Error: Failed to allocate UI components due to insufficient memory." << std::endl;
			return;
		}

		setupTray();
		updateUIState();
		centerOnScreen();
	}

protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		if (event->button() == Qt::LeftButton)
		{
			if (windowHandle())
				windowHandle()->startSystemMove();
			else
			{
				m_isDragging = true;
				m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
			}

			event->accept();
			return;
		}

		QWidget::mousePressEvent(event);
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		if (m_isDragging && (event->buttons() & Qt::LeftButton))
		{
			move(event->globalPosition().toPoint() - m_dragPos);
			event->accept();
			return;
		}

		QWidget::mouseMoveEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent *event) override
	{
		m_isDragging = false;
		QWidget::mouseReleaseEvent(event);
	}

	void closeEvent(QCloseEvent *event) override
	{
		if (m_trayIcon && m_trayIcon->isVisible() && !m_isQuitting)
		{
			hide();
			event->ignore();
		}
		else
			event->accept();
	}

	void keyPressEvent(QKeyEvent *event) override
	{
		if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Q)
			hide();
		else if (event->key() == Qt::Key_Left)
		{
			if (m_slider)
				m_slider->setValue(m_slider->value() - 1);
		}
		else if (event->key() == Qt::Key_Right) {
			if (m_slider)
				m_slider->setValue(m_slider->value() + 1);
		}
		else
			QWidget::keyPressEvent(event);
	}

	void wheelEvent(QWheelEvent *event) override
	{
		if (m_slider)
		{
			if (event->angleDelta().y() > 0)
				m_slider->setValue(m_slider->value() + 1);
			else if (event->angleDelta().y() < 0)
				m_slider->setValue(m_slider->value() - 1);
		}

		event->accept();
	}

private:
	bool setupUI()
	{
		auto *rootLayout = new(std::nothrow) QVBoxLayout(this);
		if (!rootLayout)
			return false;

		rootLayout->setContentsMargins(14, 14, 14, 14);

		m_card = new(std::nothrow) QFrame(this);
		if (!m_card)
			return false;

		m_card->setObjectName(QStringLiteral("mainCard"));

		auto *shadow = new(std::nothrow) QGraphicsDropShadowEffect(this);
		if (shadow)
		{
			shadow->setBlurRadius(30);
			shadow->setColor(QColor(0, 0, 0, 180));
			shadow->setOffset(0, 8);
			m_card->setGraphicsEffect(shadow);
		}

		auto *cardLayout = new(std::nothrow) QVBoxLayout(m_card);
		if (!cardLayout)
			return false;

		cardLayout->setContentsMargins(24, 20, 24, 22);
		cardLayout->setSpacing(16);

		auto *headerLayout = new(std::nothrow) QHBoxLayout();
		if (!headerLayout)
			return false;

		headerLayout->setSpacing(12);

		auto *iconBadge = new(std::nothrow) QLabel(m_card);
		if (iconBadge)
		{
			iconBadge->setObjectName(QStringLiteral("iconBadge"));
			iconBadge->setAlignment(Qt::AlignCenter);
			iconBadge->setFixedSize(36, 36);
			QPixmap iconPx(QStringLiteral(":/kwin-saturation.svg"));

			if (!iconPx.isNull())
				iconBadge->setPixmap(iconPx.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
			else
				iconBadge->setText(QStringLiteral("✨"));

			iconBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
			headerLayout->addWidget(iconBadge);
		}

		auto *titleLayout = new(std::nothrow) QVBoxLayout();
		if (titleLayout)
		{
			titleLayout->setSpacing(2);
			auto *titleLabel = new(std::nothrow) QLabel(QStringLiteral("DIGITAL VIBRANCE"), m_card);
			if (titleLabel)
			{
				titleLabel->setObjectName(QStringLiteral("titleLabel"));
				titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
				titleLayout->addWidget(titleLabel);
			}

			auto *subtitleLabel = new(std::nothrow) QLabel(QStringLiteral("Made by EfeDursun125"), m_card);
			if (subtitleLabel)
			{
				subtitleLabel->setObjectName(QStringLiteral("subtitleLabel"));
				subtitleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
				titleLayout->addWidget(subtitleLabel);
			}

			headerLayout->addLayout(titleLayout);
		}

		headerLayout->addStretch();

		m_statusBadge = new(std::nothrow) QLabel(m_card);
		if (m_statusBadge)
		{
			m_statusBadge->setObjectName(QStringLiteral("statusBadge"));
			m_statusBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
			headerLayout->addWidget(m_statusBadge);
		}

		auto *minimizeBtn = new(std::nothrow) QPushButton(QStringLiteral("✕"), m_card);
		if (minimizeBtn)
		{
			minimizeBtn->setObjectName(QStringLiteral("closeButton"));
			minimizeBtn->setFixedSize(28, 28);
			minimizeBtn->setToolTip(QStringLiteral("Minimize to System Tray (Right click tray to Quit)"));
			minimizeBtn->setCursor(Qt::PointingHandCursor);
			connect(minimizeBtn, &QPushButton::clicked, this, &QWidget::hide);
			headerLayout->addWidget(minimizeBtn);
		}

		cardLayout->addLayout(headerLayout);

		auto *sep = new(std::nothrow) QFrame(m_card);
		if (sep)
		{
			sep->setObjectName(QStringLiteral("separator"));
			sep->setFrameShape(QFrame::HLine);
			sep->setAttribute(Qt::WA_TransparentForMouseEvents);
			cardLayout->addWidget(sep);
		}

		auto *displayBox = new(std::nothrow) QFrame(m_card);
		if (displayBox)
		{
			displayBox->setObjectName(QStringLiteral("displayBox"));
			displayBox->setAttribute(Qt::WA_TransparentForMouseEvents);
			auto *displayLayout = new(std::nothrow) QVBoxLayout(displayBox);
			if (displayLayout)
			{
				displayLayout->setContentsMargins(16, 12, 16, 12);
				displayLayout->setSpacing(4);

				m_percentageLabel = new(std::nothrow) QLabel(QStringLiteral("125%"), displayBox);
				if (m_percentageLabel)
				{
					m_percentageLabel->setObjectName(QStringLiteral("percentageLabel"));
					m_percentageLabel->setAlignment(Qt::AlignCenter);
					m_percentageLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
					displayLayout->addWidget(m_percentageLabel);
				}

				m_descriptionLabel = new(std::nothrow) QLabel(QStringLiteral("Vibrant & Enhanced"), displayBox);
				if (m_descriptionLabel)
				{
					m_descriptionLabel->setObjectName(QStringLiteral("descriptionLabel"));
					m_descriptionLabel->setAlignment(Qt::AlignCenter);
					m_descriptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
					displayLayout->addWidget(m_descriptionLabel);
				}
			}

			cardLayout->addWidget(displayBox);
		}

		auto *sliderContainer = new(std::nothrow) QVBoxLayout();
		if (!sliderContainer)
			return false;

		sliderContainer->setSpacing(8);

		auto *sliderRow = new(std::nothrow) QHBoxLayout();
		if (!sliderRow)
			return false;

		sliderRow->setSpacing(12);

		auto *btnMinus = new(std::nothrow) QPushButton(QStringLiteral("−"), m_card);
		if (btnMinus)
		{
			btnMinus->setObjectName(QStringLiteral("stepBtn"));
			btnMinus->setFixedSize(32, 32);
			btnMinus->setCursor(Qt::PointingHandCursor);
			connect(btnMinus, &QPushButton::clicked, this, [this]() { if (m_slider) m_slider->setValue(m_slider->value() - 1); });
			sliderRow->addWidget(btnMinus);
		}

		m_slider = new(std::nothrow) QSlider(Qt::Horizontal, m_card);
		if (!m_slider)
			return false;

		m_slider->setObjectName(QStringLiteral("vibranceSlider"));
		m_slider->setRange(0, 50);
		m_slider->setValue(static_cast<int>(std::round(m_currentSaturation * 20.0)));
		m_slider->setCursor(Qt::PointingHandCursor);
		connect(m_slider, &QSlider::valueChanged, this, &DigitalVibranceWindow::onSliderChanged);
		sliderRow->addWidget(m_slider);

		auto *btnPlus = new(std::nothrow) QPushButton(QStringLiteral("+"), m_card);
		if (btnPlus)
		{
			btnPlus->setObjectName(QStringLiteral("stepBtn"));
			btnPlus->setFixedSize(32, 32);
			btnPlus->setCursor(Qt::PointingHandCursor);
			connect(btnPlus, &QPushButton::clicked, this, [this]() { if (m_slider) m_slider->setValue(m_slider->value() + 1); });
			sliderRow->addWidget(btnPlus);
		}

		sliderContainer->addLayout(sliderRow);

		auto *markersRow = new(std::nothrow) QHBoxLayout();
		if (markersRow)
		{
			markersRow->setContentsMargins(44, 0, 44, 0);

			auto *lbl0 = new(std::nothrow) QLabel(QStringLiteral("0%\nB&W"), m_card);
			if (lbl0)
			{
				lbl0->setAlignment(Qt::AlignLeft);
				lbl0->setStyleSheet(QStringLiteral("color: #777; font-size: 10px; font-weight: 500;"));
				lbl0->setAttribute(Qt::WA_TransparentForMouseEvents);
				markersRow->addWidget(lbl0);
			}

			markersRow->addStretch();

			auto *lbl100 = new(std::nothrow) QLabel(QStringLiteral("100%\nNeutral"), m_card);
			if (lbl100)
			{
				lbl100->setAlignment(Qt::AlignCenter);
				lbl100->setStyleSheet(QStringLiteral("color: #9da5b4; font-size: 10px; font-weight: bold;"));
				lbl100->setAttribute(Qt::WA_TransparentForMouseEvents);
				markersRow->addWidget(lbl100);
			}

			markersRow->addStretch();

			auto *lbl150 = new(std::nothrow) QLabel(QStringLiteral("150%\nGaming"), m_card);
			if (lbl150)
			{
				lbl150->setAlignment(Qt::AlignCenter);
				lbl150->setStyleSheet(QStringLiteral("color: #00e676; font-size: 10px; font-weight: bold;"));
				lbl150->setAttribute(Qt::WA_TransparentForMouseEvents);
				markersRow->addWidget(lbl150);
			}

			markersRow->addStretch();

			auto *lbl250 = new(std::nothrow) QLabel(QStringLiteral("250%\nMax"), m_card);
			if (lbl250)
			{
				lbl250->setAlignment(Qt::AlignRight);
				lbl250->setStyleSheet(QStringLiteral("color: #ff3366; font-size: 10px; font-weight: 500;"));
				lbl250->setAttribute(Qt::WA_TransparentForMouseEvents);
				markersRow->addWidget(lbl250);
			}

			sliderContainer->addLayout(markersRow);
		}

		cardLayout->addLayout(sliderContainer);

		auto *presetsLabel = new(std::nothrow) QLabel(QStringLiteral("QUICK PRESETS"), m_card);
		if (presetsLabel)
		{
			presetsLabel->setObjectName(QStringLiteral("sectionLabel"));
			presetsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
			cardLayout->addWidget(presetsLabel);
		}

		auto *presetsGrid = new(std::nothrow) QGridLayout();
		if (presetsGrid)
		{
			presetsGrid->setSpacing(8);

			auto createPresetBtn = [this](const QString &title, const QString &valText, int sliderVal) -> QPushButton*
			{
				auto *btn = new(std::nothrow) QPushButton(m_card);
				if (!btn)
					return nullptr;

				btn->setObjectName(QStringLiteral("presetBtn"));
				btn->setCursor(Qt::PointingHandCursor);
				btn->setText(QStringLiteral("%1 (%2)").arg(title, valText));
				connect(btn, &QPushButton::clicked, this, [this, sliderVal]() { if (m_slider) m_slider->setValue(sliderVal); });
				return btn;
			};

			auto addPreset = [&](const QString &name, const QString &val, int sliderVal, int row, int col)
			{
				if (auto *btn = createPresetBtn(name, val, sliderVal))
					presetsGrid->addWidget(btn, row, col);
			};

			addPreset(QStringLiteral("Default"), QStringLiteral("100%"), 20, 0, 0);
			addPreset(QStringLiteral("Cinema"), QStringLiteral("115%"), 23, 0, 1);
			addPreset(QStringLiteral("Esports"), QStringLiteral("140%"), 28, 0, 2);
			addPreset(QStringLiteral("Vivid"), QStringLiteral("175%"), 35, 1, 0);
			addPreset(QStringLiteral("Ultra"), QStringLiteral("200%"), 40, 1, 1);
			addPreset(QStringLiteral("Mono"), QStringLiteral("0%"), 0, 1, 2);

			cardLayout->addLayout(presetsGrid);
		}

		auto *footerLayout = new(std::nothrow) QHBoxLayout();
		if (footerLayout)
		{
			footerLayout->setContentsMargins(0, 8, 0, 0);

			m_enableCheckBox = new(std::nothrow) QCheckBox(QStringLiteral("Enable Effect"), m_card);
			if (m_enableCheckBox)
			{
				m_enableCheckBox->setObjectName(QStringLiteral("enableCheckBox"));
				m_enableCheckBox->setChecked(m_isEffectEnabled);
				m_enableCheckBox->setCursor(Qt::PointingHandCursor);
				connect(m_enableCheckBox, &QCheckBox::toggled, this, &DigitalVibranceWindow::onEnableToggled);
				footerLayout->addWidget(m_enableCheckBox);
			}

			footerLayout->addStretch();

			auto *hintLabel = new(std::nothrow) QLabel(QStringLiteral("Tray Icon Active • Esc / ✕ to minimize"), m_card);
			if (hintLabel)
			{
				hintLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 11px;"));
				hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
				footerLayout->addWidget(hintLabel);
			}

			cardLayout->addLayout(footerLayout);
		}

		rootLayout->addWidget(m_card);

		setStyleSheet(QStringLiteral(
			"#mainCard {"
			"	background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1b20, stop:1 #121316);"
			"	border: 1px solid rgba(255, 255, 255, 0.12);"
			"	border-radius: 18px;"
			"}"
			"#iconBadge {"
			"	background-color: transparent;"
			"	border-radius: 8px;"
			"}"
			"#titleLabel {"
			"	color: #ffffff;"
			"	font-size: 15px;"
			"	font-weight: 800;"
			"	letter-spacing: 1px;"
			"}"
			"#subtitleLabel {"
			"	color: #8c93a0;"
			"	font-size: 11px;"
			"	font-weight: 500;"
			"}"
			"#statusBadge {"
			"	background-color: rgba(0, 230, 118, 0.15);"
			"	color: #00e676;"
			"	border: 1px solid rgba(0, 230, 118, 0.35);"
			"	border-radius: 12px;"
			"	padding: 3px 10px;"
			"	font-size: 10px;"
			"	font-weight: bold;"
			"}"
			"#closeButton {"
			"	background-color: rgba(255, 255, 255, 0.06);"
			"	color: #a0a0a0;"
			"	border: none;"
			"	border-radius: 14px;"
			"	font-size: 13px;"
			"}"
			"#closeButton:hover {"
			"	background-color: #ff334b;"
			"	color: #ffffff;"
			"}"
			"#separator {"
			"	background-color: rgba(255, 255, 255, 0.08);"
			"	max-height: 1px;"
			"}"
			"#displayBox {"
			"	background-color: rgba(0, 0, 0, 0.35);"
			"	border: 1px solid rgba(255, 255, 255, 0.07);"
			"	border-radius: 14px;"
			"}"
			"#percentageLabel {"
			"	color: #00e676;"
			"	font-size: 32px;"
			"	font-weight: 900;"
			"	font-family: 'Segoe UI', 'Ubuntu', 'Cantarell', sans-serif;"
			"}"
			"#descriptionLabel {"
			"	color: #9da5b4;"
			"	font-size: 12px;"
			"	font-weight: 500;"
			"}"
			"#stepBtn {"
			"	background-color: #262830;"
			"	color: #ffffff;"
			"	border: 1px solid rgba(255, 255, 255, 0.12);"
			"	border-radius: 8px;"
			"	font-size: 16px;"
			"	font-weight: bold;"
			"}"
			"#stepBtn:hover {"
			"	background-color: #353842;"
			"	border-color: #00e676;"
			"}"
			"#vibranceSlider::groove:horizontal {"
			"	height: 8px;"
			"	background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #444, stop:0.4 #3daee9, stop:0.6 #00e676, stop:1 #ff0055);"
			"	border-radius: 4px;"
			"}"
			"#vibranceSlider::sub-page:horizontal {"
			"	background: transparent;"
			"}"
			"#vibranceSlider::handle:horizontal {"
			"	background: #ffffff;"
			"	border: 2px solid #00e676;"
			"	width: 22px;"
			"	height: 22px;"
			"	margin: -7px 0;"
			"	border-radius: 11px;"
			"}"
			"#vibranceSlider::handle:horizontal:hover {"
			"	background: #00e676;"
			"	border-color: #ffffff;"
			"}"
			"#sectionLabel {"
			"	color: #717885;"
			"	font-size: 10px;"
			"	font-weight: 700;"
			"	letter-spacing: 1px;"
			"}"
			"#presetBtn {"
			"	background-color: #21232a;"
			"	color: #c5cbd6;"
			"	border: 1px solid rgba(255, 255, 255, 0.08);"
			"	border-radius: 8px;"
			"	padding: 8px 4px;"
			"	font-size: 11px;"
			"	font-weight: 600;"
			"}"
			"#presetBtn:hover {"
			"	background-color: #2d303a;"
			"	color: #ffffff;"
			"	border-color: #00e676;"
			"}"
			"#enableCheckBox {"
			"	color: #e0e0e0;"
			"	font-size: 12px;"
			"	font-weight: 600;"
			"	spacing: 8px;"
			"}"
			"#enableCheckBox::indicator {"
			"	width: 18px;"
			"	height: 18px;"
			"	border-radius: 5px;"
			"	border: 1px solid #555;"
			"	background: #222;"
			"}"
			"#enableCheckBox::indicator:checked {"
			"	background: #00e676;"
			"	border-color: #00e676;"
			"}"
		));

		return true;
	}

	void setupTray()
	{
		m_trayIcon = new(std::nothrow) QSystemTrayIcon(this);
		if (!m_trayIcon)
			return;

		QIcon icon(QStringLiteral(":/kwin-saturation.svg"));
		if (icon.isNull())
			icon = QIcon::fromTheme(QStringLiteral("kwin-saturation"));

		if (icon.isNull())
			icon = QIcon(QStringLiteral("/usr/share/icons/hicolor/scalable/apps/kwin-saturation.svg"));

		if (icon.isNull())
			icon = QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);

		if (!icon.isNull())
			m_trayIcon->setIcon(icon);

		m_trayIcon->setToolTip(QStringLiteral("Digital Vibrance (KWin)"));

		auto *trayMenu = new(std::nothrow) QMenu();
		if (!trayMenu)
			return;

		trayMenu->setStyleSheet(QStringLiteral(
			"QMenu { background-color: #1e1e24; color: #fff; border: 1px solid #444; border-radius: 8px; padding: 4px; }"
			"QMenu::item { padding: 6px 18px; border-radius: 4px; }"
			"QMenu::item:selected { background-color: #00e676; color: #000; }"
			"QMenu::separator { height: 1px; background: #333; margin: 4px 8px; }"
		));

		auto *showAction = trayMenu->addAction(QStringLiteral("Show Digital Vibrance"), this, [this]()
		{
			showNormal();
			activateWindow();
			raise();
		});

		if (showAction && !icon.isNull())
			showAction->setIcon(icon);

		trayMenu->addSeparator();

		m_trayEnableAction = trayMenu->addAction(QStringLiteral("Enable Vibrance Effect"));
		if (m_trayEnableAction)
		{
			m_trayEnableAction->setCheckable(true);
			m_trayEnableAction->setChecked(m_isEffectEnabled);
			connect(m_trayEnableAction, &QAction::toggled, this, [this](bool checked) { if (m_enableCheckBox) m_enableCheckBox->setChecked(checked); });
		}

		auto *presetsMenu = trayMenu->addMenu(QStringLiteral("Quick Presets"));
		if (presetsMenu)
		{
			presetsMenu->addAction(QStringLiteral("Default (100%)"), this, [this](){ if (m_slider) m_slider->setValue(20); });
			presetsMenu->addAction(QStringLiteral("Cinema (115%)"), this, [this]() { if (m_slider) m_slider->setValue(23); });
			presetsMenu->addAction(QStringLiteral("Esports (140%)"), this, [this]() { if (m_slider) m_slider->setValue(28); });
			presetsMenu->addAction(QStringLiteral("Vivid (175%)"), this, [this]() { if (m_slider) m_slider->setValue(35); });
			presetsMenu->addAction(QStringLiteral("Ultra (200%)"), this, [this]() { if (m_slider) m_slider->setValue(40); });
			presetsMenu->addAction(QStringLiteral("Mono (0%)"), this, [this]() { if (m_slider) m_slider->setValue(0); });
		}

		trayMenu->addSeparator();

		auto *quitAction = trayMenu->addAction(QStringLiteral("Quit Digital Vibrance"), this, [this]()
		{
			m_isQuitting = true;
			qApp->quit();
		});

		if (quitAction)
			quitAction->setShortcut(QKeySequence::Quit);

		m_trayIcon->setContextMenu(trayMenu);

		connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
		{
			if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
			{
				if (isVisible())
					hide();
				else
				{
					showNormal();
					activateWindow();
					raise();
				}
			}
		});

		m_trayIcon->show();
	}

	void onSliderChanged(int val)
	{
		m_currentSaturation = val / 20.0;
		updateUIState();

		KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Effect-saturationeffect"));
		config.writeEntry(QStringLiteral("Saturation"), m_isEffectEnabled ? m_currentSaturation : 1.0);
		config.writeEntry(QStringLiteral("Enabled"), m_isEffectEnabled);
		config.sync();

		if (m_isEffectEnabled)
			reconfigureKWinEffect();
	}

	void onEnableToggled(bool enabled)
	{
		m_isEffectEnabled = enabled;
		if (m_trayEnableAction && m_trayEnableAction->isChecked() != enabled)
			m_trayEnableAction->setChecked(enabled);

		KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Effect-saturationeffect"));
		config.writeEntry(QStringLiteral("Enabled"), enabled);
		config.writeEntry(QStringLiteral("Saturation"), enabled ? m_currentSaturation : 1.0);
		config.sync();

		KConfigGroup pluginsConfig(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Plugins"));
		pluginsConfig.writeEntry(QStringLiteral("saturationeffectEnabled"), enabled);
		pluginsConfig.sync();

		reconfigureKWinEffect();
		updateUIState();
	}

	void reconfigureKWinEffect()
	{
		QDBusMessage msg = QDBusMessage::createMethodCall(
			QStringLiteral("org.kde.KWin"),
			QStringLiteral("/Effects"),
			QStringLiteral("org.kde.kwin.Effects"),
			QStringLiteral("reconfigureEffect")
		);
	
		msg << QStringLiteral("saturationeffect");
		QDBusConnection::sessionBus().send(msg);

		QDBusMessage msg2 = QDBusMessage::createMethodCall(
			QStringLiteral("org.kde.KWin"),
			QStringLiteral("/KWin"),
			QStringLiteral("org.kde.KWin"),
			QStringLiteral("reconfigure")
		);

		QDBusConnection::sessionBus().send(msg2);
	}

	void updateUIState()
	{
		const int percent = static_cast<int>(std::round(m_currentSaturation * 100.0));
		if (m_percentageLabel)
			m_percentageLabel->setText(QStringLiteral("%1%").arg(percent));

		if (!m_isEffectEnabled)
		{
			if (m_statusBadge)
			{
				m_statusBadge->setText(QStringLiteral("◉ Disabled"));
				m_statusBadge->setStyleSheet(QStringLiteral("background: rgba(255, 50, 50, 0.15); color: #ff5252; border: 1px solid rgba(255, 50, 50, 0.3);"));
			}

			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Effect is paused (Screen at 100% natural)"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #777;"));

			return;
		}

		if (m_statusBadge)
		{
			m_statusBadge->setText(QStringLiteral("● Active on KWin"));
			m_statusBadge->setStyleSheet(QStringLiteral("background: rgba(0, 230, 118, 0.15); color: #00e676; border: 1px solid rgba(0, 230, 118, 0.35);"));
		}

		if (percent == 0)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Monochrome / Grayscale"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #888888;"));
		}
		else if (percent < 100)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Muted & Desaturated Colors"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #5bc0be;"));
		}
		else if (percent == 100)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Standard Neutral Display (1.0x)"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #ffffff;"));
		}
		else if (percent <= 125)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Rich & Cinematic Colors"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #38ef7d;"));
		}
		else if (percent <= 150)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Esports Vibrance Boost"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #00e676;"));
		}
		else if (percent <= 180)
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("High Dynamic Saturation Pop"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #ff9100;"));
		}
		else
		{
			if (m_descriptionLabel)
				m_descriptionLabel->setText(QStringLiteral("Extreme Vivid Saturation"));

			if (m_percentageLabel)
				m_percentageLabel->setStyleSheet(QStringLiteral("color: #ff1744;"));
		}
	}

	void centerOnScreen()
	{
		if (auto *screen = QGuiApplication::primaryScreen())
		{
			const QRect geom = screen->availableGeometry();
			move(geom.center() - rect().center());
		}
	}

	QPoint m_dragPos;
	bool m_isDragging = false;
	bool m_isQuitting = false;

	QFrame *m_card = nullptr;
	QLabel *m_statusBadge = nullptr;
	QLabel *m_percentageLabel = nullptr;
	QLabel *m_descriptionLabel = nullptr;
	QSlider *m_slider = nullptr;
	QCheckBox *m_enableCheckBox = nullptr;
	QSystemTrayIcon *m_trayIcon = nullptr;
	QAction *m_trayEnableAction = nullptr;

	double m_currentSaturation = 1.25;
	bool m_isEffectEnabled = true;
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName(QStringLiteral("Digital Vibrance"));
	app.setDesktopFileName(QStringLiteral("kwin-saturation-gui"));
	app.setQuitOnLastWindowClosed(false);

	const QString serverName = QStringLiteral("kwin-saturation-gui-instance-") + QString::fromLocal8Bit(qgetenv("USER"));

	QLocalSocket socket;
	socket.connectToServer(serverName);
	if (socket.waitForConnected(300))
	{
		socket.write("SHOW\n");
		socket.waitForBytesWritten(300);
		socket.disconnectFromServer();
		return 0;
	}

	QLocalServer::removeServer(serverName);
	auto *localServer = new(std::nothrow) QLocalServer(&app);
	if (!localServer)
		std::cerr << "Warning: Failed to allocate QLocalServer. Running without single-instance socket." << std::endl;
	else
		localServer->listen(serverName);

	DigitalVibranceWindow window;
	window.showNormal();

	if (localServer)
	{
		QObject::connect(localServer, &QLocalServer::newConnection, [&window, localServer]()
		{
			QLocalSocket *clientSocket = localServer->nextPendingConnection();
			if (clientSocket)
			{
				clientSocket->waitForReadyRead(200);
				window.showNormal();
				window.activateWindow();
				window.raise();
				clientSocket->disconnectFromServer();
				clientSocket->deleteLater();
			}
		});
	}

	return app.exec();
}
