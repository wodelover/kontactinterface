/*
  This file is part of the KDE Kontact Plugin Interface Library.

  Copyright (c) 2001 Matthias Hoelzer-Kluepfel <mhk@kde.org>
  Copyright (c) 2002-2003 Daniel Molkentin <molkentin@kde.org>
  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "core.h"

#include "kontactinterface_debug.h"

#include <KParts/Part>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QDateTime>
#include <QTimer>

using namespace KontactInterface;

//@cond PRIVATE
class Q_DECL_HIDDEN KontactInterface::Core::Private
{
    Core *const q;

public:
    explicit Private(Core *qq);

    void slotPartDestroyed(QObject *);
    void checkNewDay();

    QString lastErrorMessage;
    QDate mLastDate;
    QMap<QByteArray, KParts::Part *> mParts;
};

Core::Private::Private(Core *qq)
    : q(qq), mLastDate(QDate::currentDate())
{
}
//@endcond

Core::Core(QWidget *parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f), d(new Private(this))
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { d->checkNewDay(); });
    timer->start(1000 * 60);
}

Core::~Core()
{
    delete d;
}

KParts::Part *Core::createPart(const char *libname)
{
    qCDebug(KONTACTINTERFACE_LOG) << libname;

    QMap<QByteArray, KParts::Part *>::ConstIterator it;
    it = d->mParts.constFind(libname);
    if (it != d->mParts.constEnd()) {
        return it.value();
    }

    qCDebug(KONTACTINTERFACE_LOG) << "Creating new KPart";

    KPluginLoader loader(QString::fromLatin1(libname));
    qCDebug(KONTACTINTERFACE_LOG) << loader.fileName();
    KPluginFactory *factory = loader.factory();
    KParts::Part *part = nullptr;
    if (factory) {
        part = factory->create<KParts::Part>(this);
    }

    if (part) {
        d->mParts.insert(libname, part);
        QObject::connect(part, &KParts::Part::destroyed,
                         this, [this](QObject* obj) { d->slotPartDestroyed(obj);});
    } else {
        d->lastErrorMessage = loader.errorString();
        qCWarning(KONTACTINTERFACE_LOG) << d->lastErrorMessage;
    }

    return part;
}

//@cond PRIVATE
void Core::Private::slotPartDestroyed(QObject *obj)
{
    // the part was deleted, we need to remove it from the part map to not return
    // a dangling pointer in createPart
    const QMap<QByteArray, KParts::Part *>::Iterator end = mParts.end();
    QMap<QByteArray, KParts::Part *>::Iterator it = mParts.begin();
    for (; it != end; ++it) {
        if (it.value() == obj) {
            mParts.erase(it);
            return;
        }
    }
}

void Core::Private::checkNewDay()
{
    if (mLastDate != QDate::currentDate()) {
        Q_EMIT q->dayChanged(QDate::currentDate());
    }

    mLastDate = QDate::currentDate();
}
//@endcond

QString Core::lastErrorMessage() const
{
    return d->lastErrorMessage;
}

#include "moc_core.cpp"
