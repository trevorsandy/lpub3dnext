/****************************************************************************
**
** Copyright (C) 2020 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * Piece preview is used to display the selected piece.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#pragma once

#ifndef PIECEPREVIEW_H
#define PIECEPREVIEW_H

#include <QString>
#include "lc_global.h"
#include "lc_glwidget.h"

//class PieceInfo;

class PiecePreview : public lcGLWidget
{
public:
    PiecePreview();
    virtual ~PiecePreview();

    void OnDraw() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnLeftButtonDoubleClick() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseMove() override;

    PieceInfo* GetCurrentPiece() const;
    void SetPiece(PieceInfo* pInfo, quint32 pColorCode = 0);
    void SetCurrentPiece(const QString& PieceID, int ColorCode);

protected:
    PieceInfo *m_PieceInfo;
    quint32    m_ColorIndex;

    // Mouse tracking.
    int m_Tracking;
    int m_DownX;
    int m_DownY;

    // Current camera settings.
    float m_Distance;
    float m_RotateX;
    float m_RotateZ;
    bool m_AutoZoom;
};

#endif
