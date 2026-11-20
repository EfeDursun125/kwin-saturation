/*
 * SPDX-FileCopyrightText: 2026 EfeDursun125
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "gui_window.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <new>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName(QStringLiteral("Digital Vibrance"));
	app.setDesktopFileName(QStringLiteral("kwin-saturation-gui"));
	app.setQuitOnLastWindowClosed(false);

	const QString serverName = QStringLiteral("kwin-saturation-gui-instance-") + QString::number(getuid());

	const auto showExistingInstance = [&serverName]()
	{
		QLocalSocket socket;
		socket.connectToServer(serverName);
		if (!socket.waitForConnected(300))
			return false;

		socket.write("SHOW\n");
		socket.waitForBytesWritten(300);
		socket.disconnectFromServer();
		return true;
	};
	if (showExistingInstance())
		return 0;

	auto *localServer = new(std::nothrow) QLocalServer(&app);
	if (!localServer)
		std::cerr << "Warning: Failed to allocate QLocalServer. Running without single-instance socket." << std::endl;
	else if (!localServer->listen(serverName))
	{
		if (showExistingInstance())
			return 0;

		// A live instance may own this socket, so do not remove it.
		std::cerr << "Warning: Single-instance socket unavailable. Running without it." << std::endl;
		localServer->deleteLater();
		localServer = nullptr;
	}

	DigitalVibranceWindow window;
	window.showNormal();

	if (localServer)
	{
		QObject::connect(localServer, &QLocalServer::newConnection, [&window, localServer]()
		{
			QLocalSocket *clientSocket = localServer->nextPendingConnection();
			if (clientSocket)
			{
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
