/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "gui_window.h"
#include "effect_toggle.h"

#include <KConfigGroup>
#include <QApplication>
#include <QCheckBox>
#include <QDBusConnection>
#include <QDir>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QSaveFile>
#include <QSlider>
#include <QStandardPaths>
#include <QStyle>
#include <QTemporaryDir>
#include <QtTest>

class GuiTestEffects : public QObject
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.kwin.Effects")
	Q_PROPERTY(QStringList loadedEffects READ loadedEffects)
	Q_PROPERTY(QStringList listOfEffects READ listOfEffects)

public:
	bool loaded = true;
	bool present = true;
	QStringList loadedEffects() const
	{
		return loaded ? QStringList{QStringLiteral("saturationeffect")} : QStringList{};
	}
	QStringList listOfEffects() const
	{
		return present ? QStringList{QStringLiteral("saturationeffect")} : QStringList{};
	}

public slots:
	bool isEffectSupported(const QString &)
	{
		return true;
	}
	bool loadEffect(const QString &)
	{
		loaded = true;
		return true;
	}
	bool isEffectLoaded(const QString &)
	{
		return loaded;
	}
	void reconfigureEffect(const QString &)
	{
	}
};

class GuiWindowTest : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.KWin")));
		QVERIFY(QDBusConnection::sessionBus().registerObject(QStringLiteral("/Effects"), &m_effects,
			QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties));
	}

	void init()
	{
		QFile::remove(themeFile());
		QFile::remove(qEnvironmentVariable("XDG_CONFIG_DIRS") + QStringLiteral("/kdeglobals"));
		QApplication::setPalette(QApplication::style()->standardPalette());
		m_effects.present = true;
		m_effects.loaded = true;
		const KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
		config->deleteGroup(QStringLiteral("Effect-saturationeffect"));
		config->deleteGroup(QStringLiteral("Plugins"));
		config->sync();
	}

	void missingPluginAndRecovery()
	{
		m_effects.loaded = false;
		m_effects.present = false;
		DigitalVibranceWindow window;
		window.show();
		auto *const slider = window.findChild<QSlider *>(QStringLiteral("vibranceSlider"));
		auto *const notice = window.findChild<QLabel *>(QStringLiteral("effectNotice"));
		auto *const retry = window.findChild<QPushButton *>(QStringLiteral("retryEffectButton"));
		QVERIFY(slider && notice && retry);
		QTRY_VERIFY(notice->text().contains(QStringLiteral("cannot find")));
		QVERIFY(!slider->isEnabled());
		QVERIFY(!slider->isVisible());
		QVERIFY(retry->isVisible());
		savePreview(window, QStringLiteral("unavailable.png"));
		m_effects.present = true;
		retry->click();
		QTRY_VERIFY(slider->isEnabled());
		QVERIFY(slider->isVisible());
		QVERIFY(!notice->isVisible());
		QVERIFY(!retry->isVisible());
		QTest::qWait(20);
		savePreview(window, QStringLiteral("ready.png"));
	}

	void presetsAndKeyboard()
	{
		DigitalVibranceWindow window;
		window.show();
		auto *const slider = window.findChild<QSlider *>(QStringLiteral("vibranceSlider"));
		auto *const preset = window.findChild<QPushButton *>(QStringLiteral("vividPreset"));
		auto *const original = window.findChild<QPushButton *>(QStringLiteral("originalPreset"));
		QVERIFY(slider && preset && original);
		QTRY_VERIFY(slider->isEnabled());
		preset->click();
		QCOMPARE(slider->value(), 35);
		QVERIFY(preset->isChecked());
		preset->click();
		QVERIFY(preset->isChecked());
		original->click();
		QCOMPARE(slider->value(), 20);
		QVERIFY(original->isChecked());
		QVERIFY(!preset->isChecked());
		slider->setFocus();
		QTest::keyClick(slider, Qt::Key_Right);
		QCOMPARE(slider->value(), 21);
		QVERIFY(!original->isChecked());
		QCOMPARE(KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")),
			QStringLiteral("Effect-saturationeffect")).readEntry(QStringLiteral("Saturation"), 0.0), 1.05);
	}

	void grayscaleIsFirstPreset()
	{
		DigitalVibranceWindow window;
		window.show();
		auto *const slider = window.findChild<QSlider *>(QStringLiteral("vibranceSlider"));
		auto *const grid = window.findChild<QGridLayout *>(QStringLiteral("presetsGrid"));
		QVERIFY(slider && grid);
		QTRY_VERIFY(slider->isEnabled());
		QCOMPARE(grid->itemAtPosition(0, 0)->widget()->objectName(), QStringLiteral("grayscalePreset"));
		auto *const grayscale = window.findChild<QPushButton *>(QStringLiteral("grayscalePreset"));
		QVERIFY(grayscale);
		grayscale->click();
		QCOMPARE(slider->value(), 0);
		QVERIFY(grayscale->isChecked());
	}

	void themeFallbackAndLiveChanges()
	{
		DigitalVibranceWindow window;
		window.show();
		QCOMPARE(window.palette().color(QPalette::Window), QColor(QStringLiteral("#1a1110")));
		QCOMPARE(window.palette().color(QPalette::Highlight), QColor(QStringLiteral("#ffb4a8")));
		writeTheme(QByteArray(
			"[Colors:View]\nBackgroundNormal=16,32,48\n"
			"[Colors:Window]\nBackgroundNormal=#203040\nForegroundNormal=#ffffff\n"
			"[Colors:Selection]\nBackgroundNormal=#80b4ff\nForegroundActive=#102030\n"));
		QTRY_COMPARE(window.palette().color(QPalette::Window), QColor(QStringLiteral("#102030")));
		auto *const toggle = window.findChild<QCheckBox *>(QStringLiteral("enableCheckBox"));
		QVERIFY(toggle);
		QCOMPARE(toggle->palette().color(QPalette::Highlight), QColor(QStringLiteral("#80b4ff")));
		writeTheme(QByteArray(
			"[Colors:View]\nBackgroundNormal=#fafafa\n"
			"[Colors:Window]\nBackgroundNormal=#eeeeee\nForegroundNormal=#181818\n"
			"[Colors:Button]\nBackgroundNormal=#e8e8e8\nForegroundNormal=#181818\n"
			"[Colors:Selection]\nBackgroundNormal=#0056a0\nForegroundNormal=#ffffff\n"));
		QTRY_COMPARE(window.palette().color(QPalette::Window), QColor(QStringLiteral("#fafafa")));
		QCOMPARE(window.palette().color(QPalette::WindowText), QColor(QStringLiteral("#181818")));
		QCOMPARE(toggle->palette().color(QPalette::HighlightedText), QColor(Qt::white));
		QTest::qWait(20);
		savePreview(window, QStringLiteral("light-theme.png"));
		QVERIFY(QFile::remove(themeFile()));
		QTRY_COMPARE(window.palette().color(QPalette::Window), QColor(QStringLiteral("#1a1110")));
	}

	void malformedColorsUseFallback()
	{
		writeTheme(QByteArray(
			"[Colors:View]\nBackgroundNormal=999,20,30\n"
			"[Colors:Window]\nForegroundNormal=not-a-color\n"
			"[Colors:Selection]\nBackgroundNormal=#ff0000\n"));
		ApplicationTheme theme;
		QCOMPARE(theme.source(), ApplicationTheme::Source::Fallback);
		QCOMPARE(theme.colors().color(QPalette::Window), QColor(QStringLiteral("#1a1110")));
	}

	void qtPaletteChangesAreObserved()
	{
		ApplicationTheme theme;
		QCOMPARE(theme.source(), ApplicationTheme::Source::Fallback);
		QPalette system = QApplication::palette();
		system.setColor(QPalette::Window, QColor(QStringLiteral("#eeeeff")));
		system.setColor(QPalette::WindowText, QColor(QStringLiteral("#111122")));
		system.setColor(QPalette::Highlight, QColor(QStringLiteral("#553388")));
		QApplication::setPalette(system);
		QTRY_COMPARE(theme.source(), ApplicationTheme::Source::Qt);
		QCOMPARE(theme.colors().color(QPalette::Window), QColor(QStringLiteral("#eeeeff")));
	}

	void systemColorsCascadeAndUserAccentWins()
	{
		QSaveFile system(qEnvironmentVariable("XDG_CONFIG_DIRS") + QStringLiteral("/kdeglobals"));
		QVERIFY(system.open(QIODevice::WriteOnly));
		const QByteArray colors(
			"[Colors:View]\nBackgroundNormal=#102030\n"
			"[Colors:Window]\nForegroundNormal=#ffffff\n"
			"[Colors:Selection]\nBackgroundNormal=#90c0f0\n");
		QCOMPARE(system.write(colors), colors.size());
		QVERIFY(system.commit());
		writeTheme(QByteArray("[Colors:Selection]\nBackgroundNormal=#88ddaa\n"));
		ApplicationTheme theme;
		QCOMPARE(theme.source(), ApplicationTheme::Source::Kde);
		QCOMPARE(theme.colors().color(QPalette::Window), QColor(QStringLiteral("#102030")));
		QCOMPARE(theme.colors().color(QPalette::Highlight), QColor(QStringLiteral("#88ddaa")));
	}

	void toggleSupportsMouseAndKeyboard()
	{
		EffectToggle toggle;
		toggle.setText(QStringLiteral("Enable effect"));
		toggle.resize(toggle.sizeHint());
		toggle.show();
		QSignalSpy changes(&toggle, &QCheckBox::toggled);
		QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier,
			QPoint(toggle.width() - 12, toggle.height() / 2));
		QVERIFY(toggle.isChecked());
		toggle.setFocus();
		QTest::keyClick(&toggle, Qt::Key_Space);
		QVERIFY(!toggle.isChecked());
		QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier, QPoint(8, toggle.height() / 2));
		QVERIFY(toggle.isChecked());
		QCOMPARE(changes.count(), 3);
		toggle.setEnabled(false);
		QTest::mouseClick(&toggle, Qt::LeftButton);
		QTest::keyClick(&toggle, Qt::Key_Space);
		QCOMPARE(changes.count(), 3);
	}

	void pausePreservesChosenValue()
	{
		{
			DigitalVibranceWindow window;
			window.show();
			auto *const slider = window.findChild<QSlider *>(QStringLiteral("vibranceSlider"));
			auto *const enabled = window.findChild<QCheckBox *>(QStringLiteral("enableCheckBox"));
			QVERIFY(slider && enabled);
			QTRY_VERIFY(slider->isEnabled());
			slider->setValue(35);
			enabled->setChecked(false);
			QTRY_COMPARE(window.findChild<QLabel *>(QStringLiteral("statusBadge"))->text(), QStringLiteral("Paused"));
			QTest::qWait(20);
			savePreview(window, QStringLiteral("paused.png"));
		}
		DigitalVibranceWindow restored;
		restored.show();
		auto *const slider = restored.findChild<QSlider *>(QStringLiteral("vibranceSlider"));
		auto *const enabled = restored.findChild<QCheckBox *>(QStringLiteral("enableCheckBox"));
		QVERIFY(slider && enabled);
		QTRY_VERIFY(slider->isEnabled());
		QCOMPARE(slider->value(), 35);
		QVERIFY(!enabled->isChecked());
		enabled->setChecked(true);
		QTRY_COMPARE(restored.findChild<QLabel *>(QStringLiteral("statusBadge"))->text(), QStringLiteral("Connected"));
	}

private:
	QString themeFile() const
	{
		return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kdeglobals");
	}

	void writeTheme(const QByteArray &colors)
	{
		QSaveFile file(themeFile());
		QVERIFY(file.open(QIODevice::WriteOnly));
		QCOMPARE(file.write(colors), colors.size());
		QVERIFY(file.commit());
	}

	void savePreview(QWidget &window, const QString &filename)
	{
		const QString directory = qEnvironmentVariable("SATURATION_PREVIEW_DIR");
		if (directory.isEmpty())
			return;
		QVERIFY(QDir().mkpath(directory));
		QVERIFY(window.grab().save(directory + QLatin1Char('/') + filename));
	}

	GuiTestEffects m_effects;
};

int main(int argc, char **argv)
{
	QTemporaryDir config;
	if (!config.isValid())
		return 1;
	qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
	qputenv("XDG_CONFIG_DIRS", (config.path() + QStringLiteral("/system")).toUtf8());
	if (!QDir().mkpath(config.path() + QStringLiteral("/system")))
		return 1;
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	GuiWindowTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "gui_window_test.moc"
