/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QTimer>

class EffectStatusMonitor : public QObject
{
	Q_OBJECT

public:
	enum class State
	{
		Checking, Ready, Disabled, Missing, Unsupported, LoadFailed, Unavailable
	};
	Q_ENUM(State)

	explicit EffectStatusMonitor(QObject *parent = nullptr);
	State state() const
	{
		return m_state;
	}
	bool controlsAvailable() const
	{
		return m_state == State::Ready || m_state == State::Disabled;
	}
	void setEffectEnabled(bool enabled);
	void refresh(bool tryLoading = false);

signals:
	void statusChanged();

private:
	enum class Query
	{
		Properties, Support, Load, VerifyLoaded
	};
	void request(QDBusMessage message, Query query);
	void requestEffect(const QString &method, Query query);
	void finish(State state);

	QTimer m_timer;
	QDBusServiceWatcher m_serviceWatcher;
	QDBusPendingCallWatcher *m_pending = nullptr;
	State m_state = State::Checking;
	bool m_enabled = true;
	bool m_tryLoading = false;
	bool m_refreshPending = false;
	bool m_retryPending = false;
};
