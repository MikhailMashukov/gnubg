/* $Id: tr.c,v 1.16 2021/10/30 13:46:21 plm Exp $ */

/*
 * $originalLog: tr.c,v $
 * Revision 1.9  1998/01/29  16:56:54  brianp
 * allow trOrtho() and trFrustum() to be called at any time, minor clean-up
 *
 * Revision 1.8  1998/01/28  19:47:39  brianp
 * minor clean-up for C++
 *
 * Revision 1.7  1997/07/21  17:34:38  brianp
 * added tile borders
 *
 * Revision 1.6  1997/07/21  15:47:35  brianp
 * renamed all "near" and "far" variables
 *
 * Revision 1.5  1997/04/26  21:23:25  brianp
 * added trRasterPos3f function
 *
 * Revision 1.4  1997/04/26  19:59:36  brianp
 * set CurrentTile to -1 before first tile and after last tile
 *
 * Revision 1.3  1997/04/22  23:51:15  brianp
 * added WIN32 header stuff, removed tabs
 *
 * Revision 1.2  1997/04/19  23:26:10  brianp
 * many API changes
 *
 * Revision 1.1  1997/04/18  21:53:05  brianp
 * Initial revision
 *
 */


/*
 * Tiled Rendering library
 * Version 1.1
 * Copyright (C) 1997 Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Notice added by Russ Allbery on 2013-07-21 based on the license information
 * in tr-1.3.tar.gz retrieved from <http://www.mesa3d.org/brianp/TR.html>.
 */

#include "config.h"
#include "legacyGLinc.h"
#include "fun3d.h"
#include "tr.h"

#define DEFAULT_TILE_WIDTH  256
#define DEFAULT_TILE_HEIGHT 256
#define DEFAULT_TILE_BORDER 0

/*
 * Misc setup including computing number of tiles (rows and columns).
 */
static void
Setup(TRcontext * tr)
{
    if (!tr)
        return;

    tr->Columns = (tr->ImageWidth + tr->TileWidthNB - 1) / tr->TileWidthNB;
    tr->Rows = (tr->ImageHeight + tr->TileHeightNB - 1) / tr->TileHeightNB;
    tr->CurrentTile = 0;

    g_assert(tr->Columns >= 0);
    g_assert(tr->Rows >= 0);
}



TRcontext *
trNew(void)
{
    TRcontext *tr = (TRcontext *) calloc(1, sizeof(TRcontext));
    if (tr) {
        tr->TileWidth = DEFAULT_TILE_WIDTH;
        tr->TileHeight = DEFAULT_TILE_HEIGHT;
        tr->TileBorder = DEFAULT_TILE_BORDER;
        tr->RowOrder = TR_BOTTOM_TO_TOP;
        tr->CurrentTile = -1;

        /* Save user's viewport, will be restored after last tile rendered */
        glGetIntegerv(GL_VIEWPORT, tr->ViewportSave);
    }

    return (TRcontext *) tr;
}


void
trDelete(TRcontext * tr)
{
    if (tr)
        free(tr);
}



void
trTileSize(TRcontext * tr, int width, int height, int border)
{
    if (!tr)
        return;

    g_assert(border >= 0);
    g_assert(width >= 1);
    g_assert(height >= 1);
    g_assert(width >= 2 * border);
    g_assert(height >= 2 * border);

    tr->TileBorder = border;
    tr->TileWidth = width;
    tr->TileHeight = height;
    tr->TileWidthNB = width - 2 * border;
    tr->TileHeightNB = height - 2 * border;
    Setup(tr);
}


#if 0
void
trTileBuffer(TRcontext * tr, GLenum format, GLenum type, GLvoid * image)
{
    if (!tr)
        return;

    tr->TileFormat = format;
    tr->TileType = type;
    tr->TileBuffer = image;
}
#endif


void
trImageSize(TRcontext * tr, unsigned int width, unsigned int height)
{
    if (!tr)
        return;

    tr->ImageWidth = (int) width;
    tr->ImageHeight = (int) height;
    Setup(tr);
}


void
trImageBuffer(TRcontext * tr, GLenum format, GLenum type, GLvoid * image)
{
    if (!tr)
        return;

    tr->ImageFormat = format;
    tr->ImageType = type;
    tr->ImageBuffer = image;
}

#if 0
int
trGet(const TRcontext * tr, TRenum param)
{
    if (!tr)
        return 0;

    switch (param) {
    case TR_TILE_WIDTH:
        return tr->TileWidth;
    case TR_TILE_HEIGHT:
        return tr->TileHeight;
    case TR_TILE_BORDER:
        return tr->TileBorder;
    case TR_IMAGE_WIDTH:
        return tr->ImageWidth;
    case TR_IMAGE_HEIGHT:
        return tr->ImageHeight;
    case TR_ROWS:
        return tr->Rows;
    case TR_COLUMNS:
        return tr->Columns;
    case TR_CURRENT_ROW:
        if (tr->CurrentTile < 0)
            return -1;
        else
            return tr->CurrentRow;
    case TR_CURRENT_COLUMN:
        if (tr->CurrentTile < 0)
            return -1;
        else
            return tr->CurrentColumn;
    case TR_CURRENT_TILE_WIDTH:
        return tr->CurrentTileWidth;
    case TR_CURRENT_TILE_HEIGHT:
        return tr->CurrentTileHeight;
    case TR_ROW_ORDER:
        return (int) tr->RowOrder;
    case TR_TOP_TO_BOTTOM:
        return tr->RowOrder == TR_TOP_TO_BOTTOM;
    case TR_BOTTOM_TO_TOP:
        return tr->RowOrder == TR_BOTTOM_TO_TOP;
    default:
        return 0;
    }
}


void
trRowOrder(TRcontext * tr, TRenum order)
{
    if (!tr)
        return;

    if (order == TR_TOP_TO_BOTTOM || order == TR_BOTTOM_TO_TOP)
        tr->RowOrder = order;
}
#endif

void
trOrtho(TRcontext * tr, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble znear, GLdouble zfar)
{
    if (!tr)
        return;

    tr->Perspective = GL_FALSE;
    tr->Left = left;
    tr->Right = right;
    tr->Bottom = bottom;
    tr->Top = top;
    tr->Near = znear;
    tr->Far = zfar;
}


void
trFrustum(TRcontext * tr, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble znear, GLdouble zfar)
{
    if (!tr)
        return;

    tr->Perspective = GL_TRUE;
    tr->Left = left;
    tr->Right = right;
    tr->Bottom = bottom;
    tr->Top = top;
    tr->Near = znear;
    tr->Far = zfar;
}

#if 0
void
trPerspective(TRcontext * tr, GLdouble fovy, GLdouble aspect, GLdouble znear, GLdouble zfar)
{
    GLdouble xmin, xmax, ymin, ymax;
    ymax = znear * tan(fovy * 3.14159265 / 360.0);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;
    trFrustum(tr, xmin, xmax, ymin, ymax, znear, zfar);
}
#endif

void
trBeginTile(TRcontext * tr)
{
#if !GTK_CHECK_VERSION(3,0,0)
    int matrixMode;
    int tileWidth, tileHeight, border;
    GLdouble left, right, bottom, top;

    if (!tr)
        return;

    if (tr->CurrentTile <= 0) {
        Setup(tr);
    }

    /* which tile (by row and column) we're about to render */
    if (tr->RowOrder == TR_BOTTOM_TO_TOP) {
        tr->CurrentRow = tr->CurrentTile / tr->Columns;
        tr->CurrentColumn = tr->CurrentTile % tr->Columns;
    } else if (tr->RowOrder == TR_TOP_TO_BOTTOM) {
        tr->CurrentRow = (tr->Rows - (tr->CurrentTile / tr->Columns)) - 1;
        tr->CurrentColumn = tr->CurrentTile % tr->Columns;
    } else {
        /* This should never happen */
        abort();
    }
    g_assert(tr->CurrentRow < tr->Rows);
    g_assert(tr->CurrentColumn < tr->Columns);

    border = tr->TileBorder;

    /* Compute actual size of this tile with border */
    if (tr->CurrentRow < tr->Rows - 1)
        tileHeight = tr->TileHeight;
    else
        tileHeight = (tr->ImageHeight - (tr->Rows - 1) * (tr->TileHeightNB)) + (2 * border);

    if (tr->CurrentColumn < tr->Columns - 1)
        tileWidth = tr->TileWidth;
    else
        tileWidth = (tr->ImageWidth - (tr->Columns - 1) * (tr->TileWidthNB)) + 2 * border;

    /* Save tile size, with border */
    tr->CurrentTileWidth = tileWidth;
    tr->CurrentTileHeight = tileHeight;

    glViewport(0, 0, tileWidth, tileHeight);    /* tile size including border */

    /* save current matrix mode */
    glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* compute projection parameters */
    left = tr->Left + (tr->Right - tr->Left)
        * (tr->CurrentColumn * tr->TileWidthNB - border) / tr->ImageWidth;
    right = left + (tr->Right - tr->Left) * tileWidth / tr->ImageWidth;
    bottom = tr->Bottom + (tr->Top - tr->Bottom)
        * (tr->CurrentRow * tr->TileHeightNB - border) / tr->ImageHeight;
    top = bottom + (tr->Top - tr->Bottom) * tileHeight / tr->ImageHeight;

    if (tr->Perspective)
        glFrustum(left, right, bottom, top, tr->Near, tr->Far);
    else
        glOrtho(left, right, bottom, top, tr->Near, tr->Far);

    /* restore user's matrix mode */
    glMatrixMode((GLenum) matrixMode);
#else
    (void)tr;	/* suppress unused parameter compiler warning */
#endif
}



int
trEndTile(TRcontext * tr)
{
    int prevRowLength, prevSkipRows, prevSkipPixels;  /*, prevAlignment; */

    if (!tr)
        return 0;

    g_assert(tr->CurrentTile >= 0);

    /* be sure OpenGL rendering is finished */
    glFlush();

    /* save current glPixelStore values */
    glGetIntegerv(GL_PACK_ROW_LENGTH, &prevRowLength);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &prevSkipRows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &prevSkipPixels);
    /*glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlignment); */

    if (tr->TileBuffer) {
        int srcX = tr->TileBorder;
        int srcY = tr->TileBorder;
        int srcWidth = tr->TileWidthNB;
        int srcHeight = tr->TileHeightNB;
        glReadPixels(srcX, srcY, srcWidth, srcHeight, tr->TileFormat, tr->TileType, tr->TileBuffer);
    }

    if (tr->ImageBuffer) {
        int srcX = tr->TileBorder;
        int srcY = tr->TileBorder;
        int srcWidth = tr->CurrentTileWidth - 2 * tr->TileBorder;
        int srcHeight = tr->CurrentTileHeight - 2 * tr->TileBorder;
        int destX = tr->TileWidthNB * tr->CurrentColumn;
        int destY = tr->TileHeightNB * tr->CurrentRow;

        /* setup pixel store for glReadPixels */
        glPixelStorei(GL_PACK_ROW_LENGTH, tr->ImageWidth);
        glPixelStorei(GL_PACK_SKIP_ROWS, destY);
        glPixelStorei(GL_PACK_SKIP_PIXELS, destX);
        /*glPixelStorei(GL_PACK_ALIGNMENT, 1); */

        /* read the tile into the final image */
        glReadPixels(srcX, srcY, srcWidth, srcHeight, tr->ImageFormat, tr->ImageType, tr->ImageBuffer);
    }

    /* restore previous glPixelStore values */
    glPixelStorei(GL_PACK_ROW_LENGTH, prevRowLength);
    glPixelStorei(GL_PACK_SKIP_ROWS, prevSkipRows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, prevSkipPixels);
    /*glPixelStorei(GL_PACK_ALIGNMENT, prevAlignment); */

    /* increment tile counter, return 1 if more tiles left to render */
    tr->CurrentTile++;
    if (tr->CurrentTile >= tr->Rows * tr->Columns) {
        /* restore user's viewport */
        glViewport(tr->ViewportSave[0], tr->ViewportSave[1], tr->ViewportSave[2], tr->ViewportSave[3]);
        tr->CurrentTile = -1;   /* all done */
        return 0;
    } else
        return 1;
}

#if 0
/*
 * Replacement for glRastePos3f() which avoids the problem with invalid
 * raster pos.
 */
void
trRasterPos3d(const TRcontext * tr, GLdouble x, GLdouble y, GLdouble z)
{
    if (tr->CurrentTile < 0) {
        /* not doing tile rendering right now.  Let OpenGL do this. */
        glRasterPos3d(x, y, z);
    } else {
        GLdouble modelview[16], proj[16];
        int viewport[4];
        GLdouble winX, winY, winZ;

        /* Get modelview, projection and viewport */
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        viewport[0] = 0;
        viewport[1] = 0;
        viewport[2] = tr->CurrentTileWidth;
        viewport[3] = tr->CurrentTileHeight;

        /* Project object coord to window coordinate */
        if (gluProject(x, y, z, modelview, proj, viewport, &winX, &winY, &winZ)) {

            /* set raster pos to window coord (0,0) */
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0.0, (double) tr->CurrentTileWidth, 0.0, (double) tr->CurrentTileHeight, 0.0, 1.0);
            glRasterPos3d(0.0, 0.0, -winZ);

            /* Now use empty bitmap to adjust raster position to (winX,winY) */
            {
                GLubyte bitmap[1] = { 0 };
                glBitmap(1, 1, 0.0f, 0.0f, (GLfloat) winX, (GLfloat) winY, bitmap);
            }

            /* restore original matrices */
            glPopMatrix();      /*proj */
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }
#ifdef DEBUG
        if (glGetError())
            printf("GL error!\n");
#endif
    }
}
#endif
