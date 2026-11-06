/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "effect_status_monitor.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QtTest>

class FakeEffects : public QObject, protected QDBusContext
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.kwin.Effects")
	Q_PROPERTY(QStringList loadedEffects READ loadedEffects)
	Q_PROPERTY(QStringList listOfEffects READ listOfEffects)

public:
	QStringList loadedEffects() const
	{
		return loaded ? QStringList{QStringLiteral("saturationeffect")} : QStringList{};
	}
	QStringList listOfEffects() const
	{
		return present ? QStringList{QStringLiteral("saturationeffect")} : QStringList{};
	}
	bool present = true;
	bool loaded = false;
	bool supported = true;
	bool loadSucceeds = true;
	bool actuallyLoads = true;
	bool delaySupport = false;
	int loadCalls = 0;
	QDBusMessage delayedRequest;

public slots:
	bool isEffectSupported(const QString &name)
	{
		if (delaySupport)
		{
			setDelayedReply(true);
			delayedRequest = message();
		}
		return name == QStringLiteral("saturationeffect") && supported;
	}
	bool loadEffect(const QString &name)
	{
		++loadCalls;
		loaded = loadSucceeds && actuallyLoads && name == QStringLiteral("saturationeffect");
		return loadSucceeds;
	}
	bool isEffectLoaded(const QString &name)
	{
		return loaded && name == QStringLiteral("saturationeffect");
	}
};

class EffectStatusMonitorTest : public QObject
{
	Q_OBJECT

private slots:
	void init()
	{
		m_effects.present = true;
		m_effects.loaded = false;
		m_effects.supported = true;
		m_effects.loadSucceeds = true;
		m_effects.actuallyLoads = true;
		m_effects.delaySupport = false;
		m_effects.delayedRequest = QDBusMessage();
		m_effects.loadCalls = 0;
		QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.KWin")));
		QVERIFY(QDBusConnection::sessionBus().registerObject(QStringLiteral("/Effects"), &m_effects,
			QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties));
	}

	void cleanup()
	{
		QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/Effects"));
		QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.kde.KWin"));
	}

	void loadedEffectIsReadyEvenWhenNeutral()
	{
		// activeEffects is deliberately absent: neutral saturation is still valid.
		m_effects.loaded = true;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		QVERIFY(monitor.controlsAvailable());
		QCOMPARE(m_effects.loadCalls, 0);
	}

	void missingPlugin()
	{
		m_effects.present = false;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Missing);
		QVERIFY(!monitor.controlsAvailable());
		QCOMPARE(m_effects.loadCalls, 0);
	}

	void unsupportedPlugin()
	{
		m_effects.supported = false;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Unsupported);
		QVERIFY(!monitor.controlsAvailable());
		QCOMPARE(m_effects.loadCalls, 0);
	}

	void loadFailureAndRetry()
	{
		m_effects.loadSucceeds = false;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::LoadFailed);
		QVERIFY(!monitor.controlsAvailable());
		QCOMPARE(m_effects.loadCalls, 1);
		monitor.refresh();
		QTest::qWait(100);
		QCOMPARE(m_effects.loadCalls, 1);
		m_effects.loadSucceeds = true;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		QCOMPARE(m_effects.loadCalls, 2);
	}

	void successfulReplyMustBeVerified()
	{
		m_effects.actuallyLoads = false;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::LoadFailed);
		QVERIFY(!monitor.controlsAvailable());
	}

	void disabledEffectCanBeEnabled()
	{
		EffectStatusMonitor monitor;
		monitor.setEffectEnabled(false);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Disabled);
		QVERIFY(monitor.controlsAvailable());
		QCOMPARE(m_effects.loadCalls, 0);
		monitor.setEffectEnabled(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		QCOMPARE(m_effects.loadCalls, 1);
	}

	void compositorDisappearsAndReturns()
	{
		m_effects.loaded = true;
		EffectStatusMonitor monitor;
		monitor.refresh();
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.kde.KWin"));
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Unavailable);
		QVERIFY(!monitor.controlsAvailable());
		QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.KWin")));
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
	}

	void detectsExternalUnloadAutomatically()
	{
		m_effects.loaded = true;
		EffectStatusMonitor monitor;
		monitor.refresh();
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		m_effects.loaded = false;
		QTRY_COMPARE_WITH_TIMEOUT(monitor.state(), EffectStatusMonitor::State::LoadFailed, 6500);
		QCOMPARE(m_effects.loadCalls, 0);
		m_effects.loaded = true;
		monitor.refresh();
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
	}

	void delayedReplyTimesOut()
	{
		m_effects.delaySupport = true;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE_WITH_TIMEOUT(monitor.state(), EffectStatusMonitor::State::Unavailable, 2500);
		QVERIFY(!monitor.controlsAvailable());
		QDBusConnection::sessionBus().send(m_effects.delayedRequest.createReply(QVariantList{true}));
		m_effects.delaySupport = false;
		monitor.refresh(true);
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
	}

	void staleReplyFromPreviousCompositorIsIgnored()
	{
		m_effects.delaySupport = true;
		EffectStatusMonitor monitor;
		monitor.refresh(true);
		QTRY_COMPARE(m_effects.delayedRequest.member(), QStringLiteral("isEffectSupported"));
		const QDBusMessage oldRequest = m_effects.delayedRequest;
		QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.kde.KWin"));
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Unavailable);
		m_effects.delaySupport = false;
		QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.KWin")));
		QTRY_COMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
		QDBusConnection::sessionBus().send(oldRequest.createReply(QVariantList{false}));
		QTest::qWait(100);
		QCOMPARE(monitor.state(), EffectStatusMonitor::State::Ready);
	}

private:
	FakeEffects m_effects;
};

QTEST_GUILESS_MAIN(EffectStatusMonitorTest)
#include "effect_status_monitor_test.moc"
