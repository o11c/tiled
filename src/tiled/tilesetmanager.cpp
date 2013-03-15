/*
 * tilesetmanager.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tilesetmanager.h"

#include "filesystemwatcher.h"
#include "tileset.h"

#include <QImage>

using namespace Tiled;
using namespace Tiled::Internal;

TilesetManager *TilesetManager::mInstance = 0;

TilesetManager::TilesetManager():
    mWatcher(new FileSystemWatcher(this)),
    mReloadTilesetsOnChange(false)
{
    connect(mWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(fileChanged(QString)));

    mChangedFilesTimer.setInterval(500);
    mChangedFilesTimer.setSingleShot(true);

    connect(&mChangedFilesTimer, SIGNAL(timeout()),
            this, SLOT(fileChangedTimeout()));
}

TilesetManager::~TilesetManager()
{
    // Since all MapDocuments should be deleted first, we assert that there are
    // no remaining tileset references.
    Q_ASSERT(mTilesets.size() == 0);
}

TilesetManager *TilesetManager::instance()
{
    if (!mInstance)
        mInstance = new TilesetManager;

    return mInstance;
}

void TilesetManager::deleteInstance()
{
    delete mInstance;
    mInstance = 0;
}

QSharedPointer<Tileset> TilesetManager::findTileset(const QString &fileName) const
{
    foreach (QSharedPointer<Tileset> tileset, tilesets())
        if (tileset->fileName() == fileName)
            return tileset;

    return QSharedPointer<Tileset>();
}

QSharedPointer<Tileset> TilesetManager::findTileset(const TilesetSpec &spec) const
{
    foreach (QSharedPointer<Tileset> tileset, tilesets()) {
        if (tileset->imageSource() == spec.imageSource
            && tileset->tileWidth() == spec.tileWidth
            && tileset->tileHeight() == spec.tileHeight
            && tileset->tileSpacing() == spec.tileSpacing
            && tileset->margin() == spec.margin)
        {
            return tileset;
        }
    }

    return QSharedPointer<Tileset>();
}

QList<QSharedPointer<Tileset> > TilesetManager::tilesets() const
{
    return mTilesets.values();
}

void TilesetManager::forceTilesetReload(QSharedPointer<Tileset> tileset)
{
    if (!mTilesets.contains(tileset))
        return;

    QString fileName = tileset->imageSource();
    if (tileset->loadFromImage(QImage(fileName), fileName))
        emit tilesetChanged(tileset);
}

void TilesetManager::setReloadTilesetsOnChange(bool enabled)
{
    mReloadTilesetsOnChange = enabled;
    // TODO: Clear the file system watcher when disabled
}

void TilesetManager::fileChanged(const QString &path)
{
    if (!mReloadTilesetsOnChange)
        return;

    /*
     * Use a one-shot timer since GIMP (for example) seems to generate many
     * file changes during a save, and some of the intermediate attempts to
     * reload the tileset images actually fail (at least for .png files).
     */
    mChangedFiles.insert(path);
    mChangedFilesTimer.start();
}

void TilesetManager::fileChangedTimeout()
{
    foreach (QSharedPointer<Tileset> tileset, tilesets()) {
        QString fileName = tileset->imageSource();
        if (mChangedFiles.contains(fileName))
            if (tileset->loadFromImage(QImage(fileName), fileName))
                emit tilesetChanged(tileset);
    }

    mChangedFiles.clear();
}
