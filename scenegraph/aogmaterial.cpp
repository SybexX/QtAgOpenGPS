// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Custom QSGMaterial implementations for scene graph rendering

#include "aogmaterial.h"
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(aogmaterial_log, "aogmaterial.qtagopengps")

//void AOGMaterial::AOGMaterial() {

//}

void AOGMaterial::setMvpMatrix(const QMatrix4x4 &mvpMatrix)
{
    m_mvpMatrix = mvpMatrix;
}

void AOGMaterial::setViewportSize(const QSize &size)
{
    m_viewportSize = size;
}
