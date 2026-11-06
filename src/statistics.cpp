/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "statistics.h"

#include <QCoreApplication>
#include <QObject>
#include <QSqlQuery>

#include "audiomanager.h"
#include "database.h"
#include "kmediasession/kmediasession.h"
#include "objectslogging.h"
#include "statisticslogging.h"

Statistics::Statistics(AudioManager *audioManager)
    : QObject(static_cast<QObject *>(audioManager))
    , m_audioManager(audioManager)
{
    connect(m_audioManager, &AudioManager::playbackStateChanged, this, &Statistics::logEvent);

    // track could still be playing when closing the app
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        if (m_state == KMediaSession::PlaybackState::PlayingState) {
            logEvent(KMediaSession::PlaybackState::StoppedState);
        }
    });

    qCDebug(kastsObjects) << "StatisticsLogger constructed" << this;
}

Statistics::~Statistics()
{
    qCDebug(kastsObjects) << "StatisticsLogger destructed" << this;
}

void Statistics::logEvent(const KMediaSession::PlaybackState state)
{
    if (state != m_state) {
        if (state == KMediaSession::PlaybackState::PlayingState) { // starts playing
            m_entryuid = m_audioManager->entryuid();
            m_start = std::chrono::steady_clock::now();
            qCDebug(kastsStatistics) << "Play statistics clock starting point for entryuid" << m_audioManager->entryuid() << "; start time"
                                     << m_start.time_since_epoch();
        } else if (m_state == KMediaSession::PlaybackState::PlayingState) { // paused or stopped and previous state was playing
            if (m_entryuid > 0) {
                std::chrono::duration interval = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start);
                qint64 now = QDateTime::currentSecsSinceEpoch();

                qCDebug(kastsStatistics) << "Logging statistics for entryuid" << m_entryuid << "; delta played time (ms)" << interval << "lastplayeddate"
                                         << now;

                QSqlQuery query;
                Database::instance().transaction();
                query.prepare(QStringLiteral("UPDATE Entries SET playedtime=playedtime+:playedtime, lastplayeddate=:lastplayeddate WHERE entryuid=:entryuid"));
                query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
                query.bindValue(QStringLiteral(":playedtime"), static_cast<qint64>(interval.count()));
                query.bindValue(QStringLiteral(":lastplayeddate"), now);
                Database::instance().execute(query);
                if (Database::instance().commit()) {
                    // reset m_start anyway since the duration has been logged to the db
                    m_start = std::chrono::steady_clock::now();
                }
            } else {
                // this shouldn't happen, but let's reset m_start anyway
                m_start = std::chrono::steady_clock::now();
            }
        }
        m_state = state;
    }
}
