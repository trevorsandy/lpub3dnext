/****************************************************************************
**
** Copyright (C) 2019 - 2020 Trevor SANDY. All rights reserved.
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
 * This class provides the options object passed between LPub3D, the Native
 * renderer and the 3DViewer.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QString>
#include <QStringList>

namespace Options
{
    enum Mt { PLI, CSI ,SMP};
}

class xyzVector
{
private:
bool _populated;

public:
  xyzVector()
  {
    _populated  = false;
  }
  xyzVector(const float _x, const float _y, const float _z)
      : x(_x), y(_y), z(_z)
  {
    _populated  = x != 0.0f || y != 0.0f || z != 0.0f;
  }
  bool isPopulated()
  {
    return _populated;
  }

  friend bool operator==(const xyzVector& a, const xyzVector& b);
  friend bool operator!=(const xyzVector& a, const xyzVector& b);

  float x, y, z;
};

inline bool operator==(const xyzVector &a, const xyzVector &b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool operator!=(const xyzVector &a, const xyzVector &b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

class ViewerOptions
{
public:
  ViewerOptions()
  {
    ImageType      = Options::CSI;
    CameraName     = QString();
    IsOrtho        = false;
    StudLogo       = 0;
    ImageWidth     = 800 ;
    ImageHeight    = 600;
    PageWidth      = 800;
    PageHeight     = 600;
    Resolution     = 150.0f;
    ModelScale     = 1.0f;
    CameraDistance = 0.0f;
    FoV            = 0.0f;
    Latitude       = 0.0f;
    Longitude      = 0.0f;
    RotStep        = xyzVector(0, 0, 0);
    Target         = xyzVector(0, 0, 0);
  }
//  virtual ~ViewerOptions(){}
  QString ViewerStepKey;
  Options::Mt ImageType;
  QString ImageFileName;
  QString RotStepType;
  QString CameraName;
  int ImageWidth;
  int ImageHeight;
  int PageWidth;
  int PageHeight;
  int StudLogo;
  float Resolution;
  float ModelScale;
  float CameraDistance;
  float FoV;
  float Latitude;
  float Longitude;
  bool IsOrtho;
  xyzVector RotStep;
  xyzVector Target;
};

class NativeOptions : public ViewerOptions
{
public:
  NativeOptions(const ViewerOptions &rhs)
      : ViewerOptions(rhs),
        IniFlag(-1),
        ExportMode(-1 /*EXPORT_NONE*/),
        LineWidth(1.0),
        TransBackground(true),
        HighlightNewParts(false)
  { }
  NativeOptions()
      : ViewerOptions()
  {
    TransBackground   = true;
    HighlightNewParts = false;
    LineWidth         = 1.0;
    ExportMode        = -1; //NONE
    IniFlag           = -1; //NONE
  }
//  virtual ~NativeOptions(){}
  QStringList ExportArgs;
  QString InputFileName;
  QString OutputFileName;
  QString ExportFileName;
  int IniFlag;
  int ExportMode;
  float LineWidth;
  bool TransBackground;
  bool HighlightNewParts;
};

// Page Options Routines

class Meta;
class Where;
class PgSizeData;
class PliPartGroupMeta;

class FindPageOptions
{

public:
    FindPageOptions(
            int             &_pageNum,
            Where           &_current,
            PgSizeData      &_pageSize,
            bool             _updateViewer,
            bool             _isMirrored,
            bool             _printing,
            int              _buildModLevel,
            int              _contStepNumber,
            int              _groupStepNumber = 0,
            QString          _renderParentModel = "")
        :
          pageNum           (_pageNum),
          current           (_current),
          pageSize          (_pageSize),

          updateViewer      (_updateViewer),
          isMirrored        (_isMirrored),
          printing          (_printing),
          buildModLevel     (_buildModLevel),
          contStepNumber    (_contStepNumber),
          groupStepNumber   (_groupStepNumber),
          renderParentModel (_renderParentModel)
    {  }
    int           &pageNum;
    Where         &current;
    PgSizeData    &pageSize;

    bool           updateViewer;
    bool           isMirrored;
    bool           printing;
    int            buildModLevel;
    int            contStepNumber;
    int            groupStepNumber;
    QString        renderParentModel;
};

class DrawPageOptions
{

public:
    DrawPageOptions(
            Where                       &_current,
            QStringList                 &_csiParts,
            QStringList                 &_pliParts,
            QStringList                 &_bfxParts,
            QStringList                 &_ldrStepFiles,
            QStringList                 &_csiKeys,
            QHash<QString, QStringList> &_bfx,
            QList<PliPartGroupMeta>     &_pliPartGroups,
            QVector<int>                &_lineTypeIndexes,
            QHash<QString, QVector<int>>&_bfxLineTypeIndexes,

            int                          _stepNum,
            int                          _groupStepNumber,
            bool                         _updateViewer,
            bool                         _isMirrored,
            bool                         _printing,
            int                          _buildModLevel,
            bool                         _bfxStore2,
            bool                         _assembledCallout = false,
            bool                         _calledOut        = false)
        :
          current                       (_current),
          csiParts                      (_csiParts),
          pliParts                      (_pliParts),
          bfxParts                      (_bfxParts),
          ldrStepFiles                  (_ldrStepFiles),
          csiKeys                       (_csiKeys),
          bfx                           (_bfx),
          pliPartGroups                 (_pliPartGroups),
          lineTypeIndexes               (_lineTypeIndexes),
          bfxLineTypeIndexes            (_bfxLineTypeIndexes),

          stepNum                       (_stepNum),
          groupStepNumber               (_groupStepNumber),
          updateViewer                  (_updateViewer),
          isMirrored                    (_isMirrored),
          printing                      (_printing),
          buildModLevel                 (_buildModLevel),
          bfxStore2                     (_bfxStore2),
          assembledCallout              (_assembledCallout),
          calledOut                     (_calledOut)
    {  }
    Where                       &current;
    QStringList                 &csiParts;
    QStringList                 &pliParts;
    QStringList                 &bfxParts;
    QStringList                 &ldrStepFiles;
    QStringList                 &csiKeys;
    QHash<QString, QStringList> &bfx;
    QList<PliPartGroupMeta>     &pliPartGroups;
    QVector<int>                &lineTypeIndexes;
    QHash<QString, QVector<int>>&bfxLineTypeIndexes;

    int                          stepNum;
    int                          groupStepNumber;
    bool                         updateViewer;
    bool                         isMirrored;
    bool                         printing;
    int                          buildModLevel;
    bool                         bfxStore2;
    bool                         assembledCallout;
    bool                         calledOut;
};

class PartLineAttributes
{
public:
    PartLineAttributes(
            QStringList           &_csiParts,
            QVector<int>          &_lineTypeIndexes,
            QStringList           &_buildModCsiParts,
            QVector<int>          &_buildModLineTypeIndexes,
            int                    _buildModLevel,
            bool                   _buildModIgnore,
            bool                   _buildModItems)
        :
          csiParts                (_csiParts),
          lineTypeIndexes         (_lineTypeIndexes),
          buildModCsiParts        (_buildModCsiParts),
          buildModLineTypeIndexes (_buildModLineTypeIndexes),
          buildModLevel           (_buildModLevel),
          buildModIgnore          (_buildModIgnore),
          buildModItems           (_buildModItems)
    {  }
    QStringList           &csiParts;
    QVector<int>          &lineTypeIndexes;
    QStringList           &buildModCsiParts;
    QVector<int>          &buildModLineTypeIndexes;
    int                    buildModLevel;
    bool                   buildModIgnore;
    bool                   buildModItems;
};

#endif // OPTIONS_H
