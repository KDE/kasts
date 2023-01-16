/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "gstmediabackend.h"
#include "gstmediabackendlogging.h"
#include "gstsignalslogging.h"

#include <QAudio>
#include <QDebug>
#include <QImage>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>

class GstMediaBackendPrivate
{
private:
    friend class GstMediaBackend;

    const qint64 m_notifyInterval = 500; // interval for position updates (in ms)

    KMediaSession *m_kMediaSession = nullptr;
    GstElement *m_pipeline = nullptr;
    GstElement *m_playbin = nullptr;
    GstElement *m_audioBin = nullptr;
    GstElement *m_audioSink = nullptr;
    GstElement *m_videoBin = nullptr;
    GstElement *m_videoSink = nullptr;
    GstElement *m_scaleTempo = nullptr;
    GstElement *m_audioConvert = nullptr;
    GstElement *m_audioResample = nullptr;
    GstBus *m_bus = nullptr;
    GstMessage *m_msg = nullptr;
    GstStateChangeReturn m_ret;

    QTimer *m_timer = nullptr;
    QUrl m_source;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    qreal m_rate = 1.0;
    qreal m_volume = 100;
    bool m_muted = false;
    bool m_seekable = false;
    KMediaSession::MediaStatus m_mediaStatus = KMediaSession::MediaStatus::NoMedia;
    KMediaSession::Error m_error = KMediaSession::Error::NoError;
    KMediaSession::PlaybackState m_playbackState = KMediaSession::PlaybackState::StoppedState;
    std::unique_ptr<QTemporaryDir> m_imageCacheDir = nullptr;

    bool m_seekPending = false;
    qint64 m_positionBeforeSeek = 0;
    qint64 m_positionAfterSeek = 0;

    void parseMetaData(GstTagList *tags);

    void playerSeekableChanged(GstMediaBackend *backend);
    static void playerSignalVolumeChanged(GObject *o, GParamSpec *p, gpointer d);
    static void playerSignalMutedChanged(GObject *o, GParamSpec *p, gpointer d);
    static void playerSignalPlaybackRateChanged(GObject *o, GParamSpec *p, gpointer d);

    static gboolean busCallback(GstBus *, GstMessage *message, gpointer pointer)
    {
        gst_message_ref(message);
        reinterpret_cast<GstMediaBackend *>(pointer)->handleMessage(message);
        return TRUE;
    };
};

GstMediaBackend::GstMediaBackend(QObject *parent)
    : AbstractMediaBackend(parent)
    , d(std::make_unique<GstMediaBackendPrivate>())
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::GstMediaBackend()";
    d->m_kMediaSession = static_cast<KMediaSession *>(parent);

    // create timer to get position updates
    d->m_timer = new QTimer(this);
    connect(d->m_timer, &QTimer::timeout, this, &GstMediaBackend::timerUpdate);

    // gstreamer initialization
    gst_init(nullptr, nullptr);

    // building pipeline
    d->m_pipeline = gst_element_factory_make("playbin", "myplaybin");
    d->m_scaleTempo = gst_element_factory_make("scaletempo", "scale_tempo");
    d->m_audioConvert = gst_element_factory_make("audioconvert", "convert");
    d->m_audioSink = gst_element_factory_make("autoaudiosink", "audio_sink");
    d->m_videoSink = gst_element_factory_make("fakevideosink", "video_sink");
    if (!d->m_scaleTempo || !d->m_audioConvert || !d->m_audioSink || !d->m_videoSink) {
        qCDebug(GstMediaBackendLog) << "Not all elements could be created.";
    }

    /* Create the audio sink bin, add the elements and link them */
    d->m_audioBin = gst_bin_new("audio_sink_bin");
    gst_bin_add_many(GST_BIN(d->m_audioBin), d->m_scaleTempo, d->m_audioConvert, d->m_audioSink, NULL);
    gst_element_link_many(d->m_scaleTempo, d->m_audioConvert, d->m_audioSink, nullptr);
    GstPad *pad_audio = gst_element_get_static_pad(d->m_scaleTempo, "sink");
    GstPad *ghost_pad_audio = gst_ghost_pad_new("sink", pad_audio);
    gst_pad_set_active(ghost_pad_audio, TRUE);
    gst_element_add_pad(d->m_audioBin, ghost_pad_audio);
    gst_object_unref(pad_audio);

    /* Set playbin's audio sink to be our sink bin */
    g_object_set(GST_OBJECT(d->m_pipeline), "audio-sink", d->m_audioBin, nullptr);

    /* Create the video sink bin, add the elements and link them */
    // TODO: handle video (using fakevideosink currently)
    d->m_videoBin = gst_bin_new("video_sink_bin");
    gst_bin_add_many(GST_BIN(d->m_audioBin), d->m_videoSink, NULL);
    // gst_element_link_many(d->m_videoSink, nullptr); // only needed when > 1 element
    GstPad *pad_video = gst_element_get_static_pad(d->m_videoSink, "sink");
    GstPad *ghost_pad_video = gst_ghost_pad_new("sink", pad_video);
    gst_pad_set_active(ghost_pad_video, TRUE);
    gst_element_add_pad(d->m_videoBin, ghost_pad_video);
    gst_object_unref(pad_video);

    /* Set playbin's audio sink to be our sink bin */
    g_object_set(GST_OBJECT(d->m_pipeline), "video-sink", d->m_videoBin, nullptr);

    // get bus and connect to get messages sent through callbacks
    // TODO: implement fallback in case gmainloop support is not available in qt
    d->m_bus = gst_element_get_bus(d->m_pipeline);
    gst_bus_add_watch_full(d->m_bus, G_PRIORITY_DEFAULT, d->busCallback, this, nullptr);

    // connect to other signals
    g_signal_connect(G_OBJECT(d->m_pipeline), "notify::volume", G_CALLBACK(GstMediaBackendPrivate::playerSignalVolumeChanged), this);
    g_signal_connect(G_OBJECT(d->m_pipeline), "notify::mute", G_CALLBACK(GstMediaBackendPrivate::playerSignalMutedChanged), this);
    g_signal_connect(G_OBJECT(d->m_scaleTempo), "notify::rate", G_CALLBACK(GstMediaBackendPrivate::playerSignalPlaybackRateChanged), this);
}

GstMediaBackend::~GstMediaBackend()
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::~GstMediaBackend()";

    // free memory
    if (d->m_msg != nullptr) {
        gst_message_unref(d->m_msg);
    }
    if (d->m_bus != nullptr) {
        gst_bus_remove_watch(d->m_bus);
        gst_object_unref(d->m_bus);
    }
    gst_element_set_state(d->m_pipeline, GST_STATE_NULL);
    if (d->m_pipeline != nullptr) {
        gst_object_unref(d->m_pipeline);
    }
}

KMediaSession::MediaBackends GstMediaBackend::backend() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::backend()";
    return KMediaSession::MediaBackends::Gst;
}

bool GstMediaBackend::muted() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::muted()";
    bool isMuted = false;
    g_object_get(G_OBJECT(d->m_pipeline), "mute", &isMuted, nullptr);
    if (isMuted != d->m_muted) {
        d->m_muted = isMuted;
    }
    return d->m_muted;
}

qreal GstMediaBackend::volume() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::volume()";
    gdouble rawVolume = 1.0;
    g_object_get(G_OBJECT(d->m_pipeline), "volume", &rawVolume, nullptr);
    qreal volume = static_cast<qreal>(QAudio::convertVolume(rawVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale)) * 100.0;

    if (volume < 0.01) {
        volume = 100.0;
    }

    if (abs(volume - d->m_volume) > 0.01) {
        d->m_volume = volume;
    }
    return d->m_volume;
}

QUrl GstMediaBackend::source() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::source()";
    return d->m_source;
}

KMediaSession::MediaStatus GstMediaBackend::mediaStatus() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::mediaStatus()";
    return d->m_mediaStatus;
}

KMediaSession::PlaybackState GstMediaBackend::playbackState() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::playbackState()";
    GstState state, pending_state;
    gst_element_get_state(d->m_pipeline, &state, &pending_state, static_cast<GstClockTime>(1e9));
    qCDebug(GstMediaBackendLog) << "Current state is" << gst_element_state_get_name(state);
    switch (state) {
    case GST_STATE_PLAYING:
        if (!d->m_timer->isActive()) {
            d->m_timer->start(d->m_notifyInterval);
        }
        return KMediaSession::PlaybackState::PlayingState;
        break;
    case GST_STATE_PAUSED:
        if (d->m_timer->isActive()) {
            d->m_timer->stop();
        }
        return KMediaSession::PlaybackState::PausedState;
        break;
    case GST_STATE_VOID_PENDING:
    case GST_STATE_NULL:
    case GST_STATE_READY:
    default:
        if (d->m_timer->isActive()) {
            d->m_timer->stop();
        }
        d->m_rate = 1.0;
        return KMediaSession::PlaybackState::StoppedState;
        break;
    }
    return KMediaSession::PlaybackState::StoppedState;
}

qreal GstMediaBackend::playbackRate() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::playbackRate()";

    return d->m_rate;
}

KMediaSession::Error GstMediaBackend::error() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::error()";
    return d->m_error;
}

qint64 GstMediaBackend::duration() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::duration()";
    gint64 rawDuration = 0;

    if (d->m_pipeline) {
        gst_element_query_duration(d->m_pipeline, GST_FORMAT_TIME, &rawDuration);
    }
    d->m_duration = static_cast<qint64>(rawDuration) / 1000000;
    qCDebug(GstMediaBackendLog) << "duration: " << d->m_duration;

    return d->m_duration;
}

qint64 GstMediaBackend::position() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::position()";

    gint64 position = 0;

    if (d->m_pipeline && d->m_playbackState != KMediaSession::PlaybackState::StoppedState) {
        gst_element_query_position(d->m_pipeline, GST_FORMAT_TIME, &position);
    }
    qCDebug(GstMediaBackendLog) << "position:" << position / 1000000;

    if (d->m_seekPending) {
        if (abs(position / 1000000 - d->m_positionAfterSeek) < 1000) {
            d->m_seekPending = false;
            d->m_positionBeforeSeek = 0;
            d->m_positionAfterSeek = 0;
        } else {
            qCDebug(GstMediaBackendLog) << "but reporting position:" << d->m_positionBeforeSeek << "due to pending seek";
            return d->m_positionBeforeSeek;
        }
    }

    return position / 1000000;
}

bool GstMediaBackend::seekable() const
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::seekable()";
    switch (d->m_playbackState) {
    case KMediaSession::PlaybackState::PlayingState:
    case KMediaSession::PlaybackState::PausedState:
        d->m_seekable = true;
        break;
    case KMediaSession::PlaybackState::StoppedState:
    default:
        d->m_seekable = false;
        break;
    }
    return d->m_seekable;
}

void GstMediaBackend::setMuted(bool muted)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::setMuted(" << muted << ")";
    g_object_set(G_OBJECT(d->m_pipeline), "mute", muted, nullptr);
    if (d->m_muted != muted) {
        d->m_muted = muted;
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT mutedChanged(d->m_muted);
        });
    }
}

void GstMediaBackend::setVolume(qreal volume)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::setVolume(" << volume << ")";
    if (abs(d->m_volume - volume) > 0.01) {
        g_object_set(G_OBJECT(d->m_pipeline),
                     "volume",
                     static_cast<gdouble>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale)),
                     nullptr);
        d->m_volume = volume;
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT volumeChanged(d->m_volume);
        });
    }
}

void GstMediaBackend::setSource(const QUrl &source)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::setSource(" << source << ")";

    if (playbackState() != KMediaSession::PlaybackState::StoppedState) {
        stop();
    }

    gst_element_set_state(d->m_pipeline, GST_STATE_NULL);
    d->m_seekPending = false;
    d->m_positionBeforeSeek = 0;
    d->m_positionAfterSeek = 0;
    d->m_duration = 0;
    // TODO: restore playbackRate of previous source?

    d->m_mediaStatus = KMediaSession::MediaStatus::LoadingMedia;
    Q_EMIT mediaStatusChanged(d->m_mediaStatus);

    g_object_set(G_OBJECT(d->m_pipeline), "uri", source.toEncoded().constData(), nullptr);

    if (d->m_error != KMediaSession::Error::NoError) {
        d->m_error = KMediaSession::Error::NoError;
        Q_EMIT errorChanged(d->m_error);
    }

    if (source.isEmpty()) {
        d->m_mediaStatus = KMediaSession::MediaStatus::NoMedia;
        Q_EMIT mediaStatusChanged(d->m_mediaStatus);
    } else {
        d->m_mediaStatus = KMediaSession::MediaStatus::LoadedMedia;
        Q_EMIT mediaStatusChanged(d->m_mediaStatus);

        gst_element_set_state(d->m_pipeline, GST_STATE_PAUSED);

        d->m_mediaStatus = KMediaSession::MediaStatus::BufferedMedia;
        Q_EMIT mediaStatusChanged(d->m_mediaStatus);
    }
    d->m_source = source;
    Q_EMIT sourceChanged(source);

    QTimer::singleShot(0, this, [this]() {
        Q_EMIT volumeChanged(volume());
        Q_EMIT mutedChanged(muted());
    });
}

void GstMediaBackend::setPosition(qint64 position)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::setPosition(" << position << ")";

    d->m_positionBeforeSeek = this->position();
    d->m_positionAfterSeek = position;
    d->m_seekPending = true;

    qint64 from = d->m_rate > 0 ? position : 0;
    qint64 to = d->m_rate > 0 ? duration() : position;

    GstSeekFlags seekFlags = GstSeekFlags(GST_SEEK_FLAG_FLUSH);

    gst_element_seek(d->m_pipeline, d->m_rate, GST_FORMAT_TIME, seekFlags, GST_SEEK_TYPE_SET, from * 1000000, GST_SEEK_TYPE_SET, to * 1000000);

    qCDebug(GstMediaBackendLog) << "Seeking: " << from << to;
}

void GstMediaBackend::setPlaybackRate(qreal rate)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::setPlaybackRate(" << rate << ")";
    qint64 currentPosition = position();
    qint64 from = rate > 0 ? currentPosition : 0;
    qint64 to = rate > 0 ? duration() : currentPosition;
    gst_element_seek(d->m_pipeline,
                     rate,
                     GST_FORMAT_TIME,
                     GstSeekFlags(GST_SEEK_FLAG_FLUSH),
                     GST_SEEK_TYPE_SET,
                     from * 1000000,
                     GST_SEEK_TYPE_SET,
                     to * 1000000);
    if (!qFuzzyCompare(rate, d->m_rate)) {
        d->m_rate = rate;
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT playbackRateChanged(d->m_rate);
        });
    }
}

void GstMediaBackend::pause()
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::pause()";
    gst_element_set_state(d->m_pipeline, GST_STATE_PAUSED);
    d->m_timer->stop();
}

void GstMediaBackend::play()
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::play()";
    gst_element_set_state(d->m_pipeline, GST_STATE_PLAYING);
    d->m_timer->start(d->m_notifyInterval);
    QTimer::singleShot(0, this, [this]() {
        d->m_mediaStatus = KMediaSession::MediaStatus::BufferedMedia;
        Q_EMIT mediaStatusChanged(d->m_mediaStatus);
    });
}

void GstMediaBackend::stop()
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackend::stop()";
    d->m_seekPending = false;
    d->m_positionBeforeSeek = 0;
    d->m_positionAfterSeek = 0;

    gst_element_set_state(d->m_pipeline, GST_STATE_READY);
    d->m_timer->stop();
}

void GstMediaBackend::handleMessage(GstMessage *message)
{
    qCDebug(GstSignalsLog) << "GstMediaBackend::handleMessage(" << message << ")";
    qCDebug(GstSignalsLog) << "message type" << gst_message_type_get_name(message->type);
    if (message->type == GST_MESSAGE_ASYNC_DONE) {
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT durationChanged(duration());
        });
    } else if (message->type == GST_MESSAGE_STATE_CHANGED) {
        GstState rawOldState, rawState, rawPendingState;
        gst_message_parse_state_changed(message, &rawOldState, &rawState, &rawPendingState);
        KMediaSession::PlaybackState newState;
        switch (rawState) {
        case GST_STATE_PLAYING:
            newState = KMediaSession::PlaybackState::PlayingState;
            break;
        case GST_STATE_PAUSED:
            newState = KMediaSession::PlaybackState::PausedState;
            break;
        case GST_STATE_VOID_PENDING:
        case GST_STATE_NULL:
        case GST_STATE_READY:
        default:
            newState = KMediaSession::PlaybackState::StoppedState;
            d->m_seekPending = false;
            d->m_positionBeforeSeek = 0;
            d->m_positionAfterSeek = 0;
            break;
        }
        if (newState != d->m_playbackState) {
            d->m_playbackState = newState;
            QTimer::singleShot(0, this, [this, newState]() {
                Q_EMIT playbackStateChanged(newState);
                Q_EMIT positionChanged(position());
                d->playerSeekableChanged(this);
            });
        }
    } else if (message->type == GST_MESSAGE_BUFFERING) {
        gint bufferPercent;
        gst_message_parse_buffering(message, &bufferPercent);
        KMediaSession::MediaStatus newStatus;
        if (bufferPercent < 100) {
            newStatus = KMediaSession::MediaStatus::BufferingMedia;
        } else {
            newStatus = KMediaSession::MediaStatus::BufferedMedia;
        }
        if (d->m_mediaStatus != newStatus) {
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT mediaStatusChanged(d->m_mediaStatus);
            });
        }
    } else if (message->type == GST_MESSAGE_EOS) {
        KMediaSession::MediaStatus newStatus = KMediaSession::MediaStatus::EndOfMedia;
        if (newStatus != d->m_mediaStatus) {
            stop();
            d->m_mediaStatus = newStatus;
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT mediaStatusChanged(d->m_mediaStatus);
            });
        }
    } else if (message->type == GST_MESSAGE_DURATION_CHANGED) {
        if (d->m_duration != duration()) {
            // calling duration will already update d->m_duration as side-effect
            QTimer::singleShot(0, this, [this]() {
                Q_EMIT durationChanged(d->m_duration);
            });
        }
    } else if (message->type == GST_MESSAGE_TAG) {
        GstTagList *tags = nullptr;
        gst_message_parse_tag(message, &tags);

        d->parseMetaData(tags);
        gst_tag_list_unref(tags);
    } else if (message->type == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
        gchar *dbg_info = nullptr;

        gst_message_parse_error(message, &err, &dbg_info);
        // TODO: implement more error parsing
        if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND) {
            d->m_error = KMediaSession::Error::FormatError;
        } else {
            d->m_error = KMediaSession::Error::ResourceError;
        }
        d->m_mediaStatus = KMediaSession::MediaStatus::InvalidMedia;
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT mediaStatusChanged(d->m_mediaStatus);
            Q_EMIT errorChanged(d->m_error);
        });
        g_error_free(err);
        g_free(dbg_info);
    }
    gst_message_unref(message);
}

void GstMediaBackend::timerUpdate()
{
    QTimer::singleShot(0, this, [this]() {
        Q_EMIT positionChanged(position());
    });
}

void GstMediaBackendPrivate::playerSignalVolumeChanged(GObject *o, GParamSpec *p, gpointer d)
{
    qCDebug(GstSignalsLog) << "GstMediaBackendPrivate::playerSignalVolumeChanged()";
    Q_UNUSED(o);
    Q_UNUSED(p);
    GstMediaBackend *gstMediaBackend = reinterpret_cast<GstMediaBackend *>(d);
    QTimer::singleShot(0, gstMediaBackend, [gstMediaBackend]() {
        Q_EMIT gstMediaBackend->volumeChanged(gstMediaBackend->volume());
    });
}

void GstMediaBackendPrivate::playerSignalMutedChanged(GObject *o, GParamSpec *p, gpointer d)
{
    qCDebug(GstSignalsLog) << "GstMediaBackendPrivate::playerSignalMutedChanged()";
    Q_UNUSED(o);
    Q_UNUSED(p);
    GstMediaBackend *gstMediaBackend = reinterpret_cast<GstMediaBackend *>(d);
    QTimer::singleShot(0, gstMediaBackend, [gstMediaBackend]() {
        Q_EMIT gstMediaBackend->mutedChanged(gstMediaBackend->muted());
    });
}

void GstMediaBackendPrivate::playerSignalPlaybackRateChanged(GObject *o, GParamSpec *p, gpointer d)
{
    qCDebug(GstSignalsLog) << "GstMediaBackendPrivate::playerSignalPlaybackRateChanged()";
    Q_UNUSED(o);
    Q_UNUSED(p);
    GstMediaBackend *gstMediaBackend = reinterpret_cast<GstMediaBackend *>(d);
    QTimer::singleShot(0, gstMediaBackend, [gstMediaBackend]() {
        gdouble rawRate;
        g_object_get(gstMediaBackend->d->m_scaleTempo, "rate", &rawRate, nullptr);

        qreal rate = static_cast<qreal>(rawRate);
        if (qFuzzyCompare(rate, 0.0)) {
            rate = 1.0;
        }

        if (!qFuzzyCompare(rate, gstMediaBackend->d->m_rate)) {
            gstMediaBackend->d->m_rate = rate;
            Q_EMIT gstMediaBackend->playbackRateChanged(gstMediaBackend->playbackRate());
        }
    });
}

void GstMediaBackendPrivate::playerSeekableChanged(GstMediaBackend *backend)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackendPrivate::playerSeekableChanged()";
    bool oldSeekable = m_seekable;
    bool newSeekable = backend->seekable(); // this will change m_seekable
    if (newSeekable != oldSeekable) {
        Q_EMIT backend->seekableChanged(m_seekable);
        if (newSeekable && !qFuzzyCompare(m_rate, 1.0)) {
            backend->setPlaybackRate(m_rate);
        }
    }
}

void GstMediaBackendPrivate::parseMetaData(GstTagList *tags)
{
    qCDebug(GstMediaBackendLog) << "GstMediaBackendPrivate::parseMetaData()";
    // qCDebug(GstSignalsLog) << "dump of all tag info:" << gst_tag_list_to_string(tags);

    char *rawString;
    GstSample *sample;

    if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &rawString)) {
        QString title = QString::fromUtf8(rawString);
        if (m_kMediaSession->metaData()->title().isEmpty()) {
            m_kMediaSession->metaData()->setTitle(title);
        }
        g_free(rawString);
    }
    if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &rawString)) {
        QString artist = QString::fromUtf8(rawString);
        if (m_kMediaSession->metaData()->artist().isEmpty()) {
            m_kMediaSession->metaData()->setArtist(artist);
        }
        g_free(rawString);
    }
    if (gst_tag_list_get_string(tags, GST_TAG_ALBUM, &rawString)) {
        QString album = QString::fromUtf8(rawString);
        if (m_kMediaSession->metaData()->album().isEmpty()) {
            m_kMediaSession->metaData()->setAlbum(album);
        }
        g_free(rawString);
    }
    if (gst_tag_list_get_sample(tags, GST_TAG_IMAGE, &sample)) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        gsize offset = 0;
        gsize dest_size;
        gpointer dest;
        QByteArray rawImage;
        gst_buffer_extract_dup(buffer, offset, gst_buffer_get_size(buffer), &dest, &dest_size);
        for (int i = 0; i < static_cast<int>(dest_size); i++) {
            rawImage.append(reinterpret_cast<const char *>(dest)[i]);
        }
        m_imageCacheDir = std::make_unique<QTemporaryDir>();
        if (m_imageCacheDir->isValid()) {
            QString filePath = m_imageCacheDir->path() + QStringLiteral("/coverimage");

            bool success = QImage::fromData(rawImage).save(filePath, "PNG");

            if (success) {
                QString localFilePath = QStringLiteral("file://") + filePath;
                m_kMediaSession->metaData()->setArtworkUrl(QUrl(localFilePath));
            }
        }
        gst_sample_unref(sample);
    }
}
