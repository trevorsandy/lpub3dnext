
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
#ifndef RIGHTWRONG_H
#define RIGHTWRONG_H

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsItemGroup>

#include "ranges.h"
#include "range.h"
#include "placement.h"
#include "where.h"
#include "pli.h"

class QGraphicsView;
class Step;
class AbstractRangeElement;
class Pointer;
class RightWrongPointerItem;
class RightWrongItem;

class RightWrong : public Steps
{
public:
  Step              *parentStep;
  PlacementType      parentRelativeType;
  QGraphicsView     *view;
  bool               shared;

  QList<Pointer *>                 pointerList;         /* Pointers and arrows */
  QList<RightWrongPointerItem *> graphicsPointerList;

  RightWrongItem    *background;
  QGraphicsRectItem *underpinnings;
  Where              topRightWrong,bottomRightWrong;

  Pli                pli;

  Where &topOfRightWrong()
  {
    return topRightWrong;
  }
  Where &bottomOfRightWrong()
  {
    return bottomRightWrong;
  }
  void setTopOfRightWrong(const Where &topOfRightWrong)
  {
    topRightWrong = topOfRightWrong;
  }
  void setBottomOfRightWrong(const Where &bottomOfRightWrong)
  {
    bottomRightWrong = bottomOfRightWrong;
  }

  RightWrong(
    Meta          &_meta,
    QGraphicsView *_view);

  virtual AllocEnc allocType();

  virtual AllocMeta &allocMeta();

  virtual ~RightWrong();

  virtual void appendPointer(const Where &here, PointerMeta &attrib);

  virtual void sizeIt();

          void sizeitFreeform(int xx, int yy);

  void addGraphicsItems(int offsetX, int offsetY, QRect &csiRect,QGraphicsItem *parent, bool movable);

  virtual void addGraphicsPointerItem(Pointer *pointer,QGraphicsItem *parent);

  virtual void drawTips(QPoint &delta);

  virtual void updatePointers(QPoint &delta);

};

/*
 * Background Item
 *
 */

#include "backgrounditem.h"

class RightWrong;

class RightWrongItem : public PlacementBackgroundItem
{
public:
  RightWrong       *rightWrong;
  RightWrongMeta    rightWrongMeta;
  QRect               rightWrongRect;
  QRect               csiRect;
  AllocMeta          *alloc;
  PageMeta           *page;

  QGraphicsView      *view;
  QGraphicsTextItem  *cursor;

  RightWrongItem(
    RightWrong  *_rightWrong,
    QRect         &_rightWrongRect,
    QRect         &_csiRect,
    PlacementType  _parentRelativeType,
    Meta          *_meta,
    int            _submodelLevel,
    QString        _path,
    QGraphicsItem *_parent,
    QGraphicsView *_view);

  void setPos(float x, float y)
  {
    QGraphicsPixmapItem::setPos(x,y);
  }

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

/*
 * Pointer Item
 *
 */

#include "pointeritem.h"
#include "metaitem.h"

class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QGraphicsItemGroup;
class RightWrong;

class RightWrongPointerItem : public PointerItem
{
public:
  RightWrongPointerItem(
    RightWrong  *il,
    Meta          *meta,
    Pointer       *pointer,
    QGraphicsItem *parent,
    QGraphicsView *view);

private:
  RightWrong    *rightWrong;

  /*
   *   +--------------------------------------------++
   *   |                                             |
   *   | . +-------------------------------------+   |
   *   |   |                                     | . |
   *   |   |                                     |   |
   *
   *
   *  callout size defines the outside edge of the callout.
   *  When there is a border, the inside rectangle starts
   *  at +thickness,+thickness, and ends at size-thickness,
   *  size-tickness.
   *
   *  Using round end cap caps the ends of the lines that
   *  intersect the callout are at +- tickness/2.  I'm not
   *  sure the affect of thickness is even vs. odd.
   *
   *  Loc should be calculated on the inside rectangle?
   *  The triangles have to go to the edge of the inner
   *  rectangle to obscure the border.
   *
   */

public:

  /* When the user "Add Pointer", we need to give a default/
     reasonable pointer */

  virtual void defaultPointer();

private:
  /* Drag the tip of the pointer, and calculate a good
   * location for the pointer to connect to the callout. */

  virtual bool autoLocFromTip();

  /* Drag the MidBase point of the pointer, and calculate a good
   * location for the pointer to connect to the callout. */

  virtual bool autoLocFromMidBase();

  /* When we drag the CSI or the pointer's callout, we
   * need recalculate the Location portion of the pointer
   * meta, but the offset remains unchanged.
   * When we have more than one segment we calculate
   * from the Tip to the segment point and from the
   * Tip to the base when we have one segment (default) */

  virtual void calculatePointerMetaLoc();

  virtual void calculatePointerMeta();

};
#endif // RIGHTWRONG_H
