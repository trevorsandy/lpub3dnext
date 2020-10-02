/****************************************************************************
**
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the
** GNU General Public Liceense (GPL) version 3.0
** which accompanies this distribution, and is
** available at http://www.gnu.org/licenses/gpl.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef SUBMODELITEM_H
#define SUBMODELITEM_H

#include <QGraphicsPixmapItem>
#include <QSize>
#include <QRect>
#include "meta.h"
#include "metaitem.h"
#include "resize.h"

class SubModel : public Placement
{
public:
  SubModelMeta subModelMeta;
  UnitsMeta    subModelImageSize;
  float        borderThickness;
  SubModel(){}
  ~SubModel(){}
  void setSize(
      UnitsMeta _size,
      float     _borderThickness = 0);
  void sizeit();
};

class SubModelItem : public ResizePixmapItem
{
public:
  Step                     *step;
  QPixmap                  *pixmap;
  PlacementType             relativeType;
  PlacementType             parentRelativeType;
  SubModel                  subModel;

  UnitsMeta                 size;
  FloatMeta                 modelScale;
  BorderMeta                border;
  BackgroundMeta            background;
  BoolMeta                  display;
  StringListMeta            subModelColor;

//  qreal                     relativeToLoc[2];
//  qreal                     relativeToSize[2];
//  bool                      positionChanged;
//  QPointF                   position;

  SubModelItem();

  SubModelItem(
    Step           *_step,
    PlacementType   _parentRelativeType,
    SubModelMeta   &_subModelMeta,
    QGraphicsItem  *_parent = 0);
  ~SubModelItem()
  {
  }

  void setAttributes(
      Step           *_step,
      PlacementType   _parentRelativeType,
      SubModelMeta   &_subModelMeta,
      QGraphicsItem  *_parent = 0);

  void setSubModelImage(QPixmap *pixmap);
  QGradient setGradient();

  void setFlag(GraphicsItemFlag flag, bool value);

protected:
  virtual void change();
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

#endif // SUBMODELITEM_H
