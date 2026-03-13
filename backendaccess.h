#ifndef BACKENDACCESS_H
#define BACKENDACCESS_H

#define CONCAT(a, b) a ## b

#define BACKEND_TRACK(NAME) CTrack *CONCAT(NAME, _ptr); \
    Q_ASSERT(CONCAT(NAME, _ptr) = qobject_cast<CTrack *>(Backend::instance()->m_track)); \
    CTrack &NAME = *CONCAT(NAME, _ptr);

#define BACKEND_YT(NAME) CYouTurn *CONCAT(NAME, _ptr); \
    Q_ASSERT(CONCAT(NAME, _ptr) = qobject_cast<CYouTurn *>(Backend::instance()->m_yt)); \
    CYouTurn &NAME = *CONCAT(NAME, _ptr);

#endif // BACKENDACCESS_H
