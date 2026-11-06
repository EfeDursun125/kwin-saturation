/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "effect_status_monitor.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QVariantMap>
#include <new>

namespace
{
const QString service = QStringLiteral("org.kde.KWin");
const QString path = QStringLiteral("/Effects");
const QString interface = QStringLiteral("org.kde.kwin.Effects");
const QString effect = QStringLiteral("saturationeffect");
constexpr int requestTimeoutMs = 1500;
constexpr int refreshIntervalMs = 5000;
}

EffectStatusMonitor::EffectStatusMonitor(QObject *parent)
	: QObject(parent)
	, m_serviceWatcher(service, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange)
{
	m_timer.setInterval(refreshIntervalMs);
	m_timer.setTimerType(Qt::CoarseTimer);
	connect(&m_timer, &QTimer::timeout, this, [this]()
	{
		refresh();
	});
	connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
		[this](const QString &, const QString &, const QString &)
	{
		// Discard replies from the previous compositor instance.
		delete m_pending;
		m_pending = nullptr;
		m_refreshPending = false;
		m_retryPending = false;
		m_state = State::Checking;
		emit statusChanged();
		refresh(true);
	});
	m_timer.start();
}

void EffectStatusMonitor::setEffectEnabled(bool enabled)
{
	if (m_enabled == enabled)
		return;

	m_enabled = enabled;
	m_state = State::Checking;
	emit statusChanged();
	refresh(true);
}

void EffectStatusMonitor::refresh(bool tryLoading)
{
	if (m_pending)
	{
		m_refreshPending = true;
		m_retryPending |= tryLoading;
		return;
	}

	m_tryLoading = tryLoading;
	QDBusMessage message = QDBusMessage::createMethodCall(service, path,
		QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));
	message << interface;
	request(message, Query::Properties);
}

void EffectStatusMonitor::requestEffect(const QString &method, Query query)
{
	QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
	message << effect;
	request(message, query);
}

void EffectStatusMonitor::request(QDBusMessage message, Query query)
{
	message.setAutoStartService(false);
	m_pending = new(std::nothrow) QDBusPendingCallWatcher(
		QDBusConnection::sessionBus().asyncCall(message, requestTimeoutMs), this);
	if (!m_pending)
	{
		finish(State::Unavailable);
		return;
	}

	connect(m_pending, &QDBusPendingCallWatcher::finished, this,
		[this, query](QDBusPendingCallWatcher *watcher)
	{
		m_pending = nullptr;
		watcher->deleteLater();
		if (query == Query::Properties)
		{
			const QDBusPendingReply<QVariantMap> reply = *watcher;
			const QVariantMap properties = reply.value();
			if (reply.isError() || !properties.contains(QStringLiteral("loadedEffects"))
				|| !properties.contains(QStringLiteral("listOfEffects")))
			{
				finish(State::Unavailable);
				return;
			}

			if (qdbus_cast<QStringList>(properties.value(QStringLiteral("loadedEffects"))).contains(effect))
				finish(m_enabled ? State::Ready : State::Disabled);
			else if (!qdbus_cast<QStringList>(properties.value(QStringLiteral("listOfEffects"))).contains(effect))
				finish(State::Missing);
			else
				requestEffect(QStringLiteral("isEffectSupported"), Query::Support);
			return;
		}

		const QDBusPendingReply<bool> reply = *watcher;
		if (reply.isError())
		{
			finish(State::Unavailable);
			return;
		}

		if (query == Query::Support)
		{
			if (!reply.value())
				finish(State::Unsupported);
			else if (!m_enabled)
				finish(State::Disabled);
			else if (m_tryLoading)
				requestEffect(QStringLiteral("loadEffect"), Query::Load);
			else
				finish(State::LoadFailed);
		}
		else if (query == Query::Load)
		{
			if (reply.value())
				requestEffect(QStringLiteral("isEffectLoaded"), Query::VerifyLoaded);
			else
				finish(State::LoadFailed);
		}
		else
			finish(reply.value() ? (m_enabled ? State::Ready : State::Disabled) : State::LoadFailed);
	});
}

void EffectStatusMonitor::finish(State state)
{
	if (m_state != state)
	{
		m_state = state;
		emit statusChanged();
	}

	if (m_refreshPending)
	{
		const bool retry = m_retryPending;
		m_refreshPending = false;
		m_retryPending = false;
		refresh(retry);
	}
}
