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
