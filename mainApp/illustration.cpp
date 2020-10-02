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

#include <QtWidgets>
#include "illustration.h"
#include "pointer.h"
#include "step.h"
#include "commonmenus.h"

#include "lpub.h"
#include "paths.h"

//---------------------------------------------------------------------------

Illustration::Illustration(
  Meta                 &_meta,
  QGraphicsView        *_view)
  : view(_view)
{
  relativeType  = IllustrationType;
  meta          = _meta;
  shared        = false;
}

Illustration::~Illustration()
{
  Steps::list.clear();
  pointerList.clear();
}

AllocEnc Illustration::allocType()
{
  if (relativeType == IllustrationType) {
    return meta.LPub.illustration.alloc.value();
  } else {
    return meta.LPub.multiStep.alloc.value();
  }
}

AllocMeta &Illustration::allocMeta()
{
  if (relativeType == IllustrationType) {
    return meta.LPub.illustration.alloc;
  } else {
    return meta.LPub.multiStep.alloc;
  }
}

void Illustration::appendPointer(const Where &here, PointerMeta &pointerMeta)
{
  Pointer *pointer = new Pointer(here,pointerMeta);
  pointerList.append(pointer);
}

void Illustration::sizeIt()
{
  AllocEnc allocEnc = meta.LPub.illustration.alloc.value();

  if (allocEnc == Vertical) {
    Steps::sizeit(allocEnc,XX,YY);
  } else {
    Steps::sizeit(allocEnc,YY,XX);
  }

  BorderData borderData = meta.LPub.illustration.border.valuePixels();

  size[XX] += int(borderData.margin[XX]);
  size[YY] += int(borderData.margin[YY]);

  size[XX] += int(borderData.thickness);
  size[YY] += int(borderData.thickness);

  size[XX] += int(borderData.margin[XX]);
  size[YY] += int(borderData.margin[YY]);

  size[XX] += int(borderData.thickness);
  size[YY] += int(borderData.thickness);
}

// Illustrations that have round corners are tricky, trying to get the pointer to start/end on the
// rounded corner.  To avoid trying to know the shape of the curve, we make sure the pointer
// is below (think zDepth) the illustration.  If we make the pointer start at the center of the curved
// corner rather than the edge, then the illustration hides the starting point of the arrow, and the
// arrow always appears to start right at the edge of the illustration (no matter the shape of the
// corner's curve.
void Illustration::addGraphicsItems(
  int            offsetX,
  int            offsetY,
  QRect         &csiRect,
  QGraphicsItem *parent,
  bool           movable)
{
  PlacementData placementData = placement.value();

  if (placementData.relativeTo == PageType ||
      placementData.relativeTo == StepGroupType ||
      placementData.relativeTo == CalloutType) {
    offsetX = 0;
    offsetY = 0;
  }

  int newLoc[2] = { offsetX + loc[XX], offsetY + loc[YY] };

  // Add an invisible rectangle underneath the illustration background item called underpinning.
  // Pointers will be added with underpinning as the parent.  This makes sure the start of the
  // arrow is covered by the illustration background.

  // If we have the pointers use illustration background as parent, the pointer is on top of the
  // background.  So by using underpinnings, the illustration end of the pointer is under the
  // background.  This allows us to have the pointers look correct for round cornered illustrations.

  underpinnings = new QGraphicsRectItem(
      qreal(newLoc[XX]),qreal(newLoc[YY]),qreal(size[XX]),qreal(size[YY]),parent);
  underpinnings->setZValue(97);
  QPen pen;
  QColor none(0,0,0,0);
  pen.setColor(none);
  underpinnings->setPen(pen);
  underpinnings->setPos(newLoc[XX],newLoc[YY]);

  QRect illustrationRect(newLoc[XX],newLoc[YY],size[XX],size[YY]);

  // This is the background for the entire illustration.

  background = new IllustrationItem(
                     this,
                     illustrationRect,
                     csiRect,
                     parentRelativeType,
                    &meta,
                     meta.submodelStack.size(),
                     path(),
                     parent,
                     view);

  background->setPos(newLoc[XX],newLoc[YY]);
  background->setFlag(QGraphicsItem::ItemIsMovable, movable);

  int saveX = loc[XX];
  int saveY = loc[YY];

  BorderData borderData = meta.LPub.illustration.border.valuePixels();

  loc[XX] = int(borderData.margin[0]);
  loc[YY] = int(borderData.margin[1]);

  if (meta.LPub.illustration.alloc.value() == Vertical) {
    Steps::addGraphicsItems(Vertical,
                            0,
                            int(borderData.thickness),
                            background);
  } else {
    Steps::addGraphicsItems(Horizontal,
                            int(borderData.thickness),
                            0,
                            background);
  }

  background->setFlag(QGraphicsItem::ItemIsMovable,movable);

  loc[XX] = saveX;
  loc[YY] = saveY;

  qDebug() << "\nILLUSTRATION PLACEMENT (addGraphicsItems) - "
           << "\nPLACEMENT DATA -         "
           << " \nPlacement:              "   << PlacNames[placement.value().placement]     << " (" << placement.value().placement << ")"
           << " \nJustification:          "   << PlacNames[placement.value().justification] << " (" << placement.value().justification << ")"
           << " \nRelativeTo:             "   << RelNames[placement.value().relativeTo]     << " (" << placement.value().relativeTo << ")"
           << " \nPreposition:            "   << PrepNames[placement.value().preposition]   << " (" << placement.value().preposition << ")"
           << " \nRectPlacement:          "   << RectNames[placement.value().rectPlacement] << " (" << placement.value().rectPlacement << ")"
           << " \nOffset[0]:              "   << placement.value().offsets[0]
           << " \nOffset[1]:              "   << placement.value().offsets[1]
           << "\nOTHER DATA -             "
           << " \nRelative Type:          "   << RelNames[relativeType]       << " (" << relativeType << ")"
           << " \nParent Relative Type:   "   << RelNames[parentRelativeType] << " (" << parentRelativeType << ")"
           << " \nSize[XX]:               "   << size[XX]
           << " \nSize[YY]:               "   << size[YY]
           << " \nnewLoc[XX]:             "   << newLoc[XX]
           << " \nnewLoc[YY]:             "   << newLoc[YY]
           << " \nRelative To Size[0]:    "   << relativeToSize[0]
           << " \nRelative To Size[1]:    "   << relativeToSize[1]
              ;

}

void Illustration::sizeitFreeform(
  int xx,
  int yy)
{
  Steps::sizeitFreeform(xx,yy);

  size[XX] += 2*int(meta.LPub.callout.border.valuePixels().thickness);
  size[YY] += 2*int(meta.LPub.callout.border.valuePixels().thickness);
}

void Illustration::addGraphicsPointerItem(
  Pointer *pointer,
  QGraphicsItem *parent)
{
  IllustrationPointerItem *t =
    new IllustrationPointerItem(
          this,
         &meta,
          pointer,
           parent,
          view);
  graphicsPointerList.append(t);
}

void Illustration::updatePointers(QPoint &delta)
{
  for (int i = 0; i < graphicsPointerList.size(); i++) {
    IllustrationPointerItem *pointer = graphicsPointerList[i];
    pointer->updatePointer(delta);
  }
}

void Illustration::drawTips(QPoint &delta)
{
  for (int i = 0; i < graphicsPointerList.size(); i++) {
    IllustrationPointerItem *pointer = graphicsPointerList[i];
    pointer->drawTip(delta);
  }
}

/*
 * Background Item
 *
 */

IllustrationItem::IllustrationItem(
  Illustration  *_illustration,
  QRect         &_illustrationRect,
  QRect         &_csiRect,
  PlacementType  _parentRelativeType,
  Meta          *_meta,
  int            _submodelLevel,
  QString        _path,
  QGraphicsItem *_parent,
  QGraphicsView *_view)
{
  illustration     = _illustration;
  view             = _view;
  illustrationRect = _illustrationRect;
  csiRect          = _csiRect;

  QPixmap *pixmap = new QPixmap(_illustrationRect.width(),_illustrationRect.height());
  QString toolTip("Illustration " + _path + "popup menu");

  //gradient settings
  if (_meta->LPub.illustration.background.value().gsize[0] == 0 &&
      _meta->LPub.illustration.background.value().gsize[1] == 0) {
      _meta->LPub.illustration.background.value().gsize[0] = pixmap->width();
      _meta->LPub.illustration.background.value().gsize[1] = pixmap->width();
      QSize gSize(_meta->LPub.illustration.background.value().gsize[0],
          _meta->LPub.illustration.background.value().gsize[1]);
      int h_off = gSize.width() / 10;
      int v_off = gSize.height() / 8;
      _meta->LPub.illustration.background.value().gpoints << QPointF(gSize.width() / 2, gSize.height() / 2)
                                                    << QPointF(gSize.width() / 2 - h_off, gSize.height() / 2 - v_off);
    }

  setBackground(pixmap,
                IllustrationType,
                _parentRelativeType,
                _meta->LPub.illustration.placement,
                _meta->LPub.illustration.background,
                _meta->LPub.illustration.border,
                _meta->LPub.illustration.margin,
                _meta->LPub.illustration.subModelColor,
                _submodelLevel,
                toolTip);
  setPixmap(*pixmap);
  delete pixmap;
  setParentItem(_parent);

  illustrationMeta = _meta->LPub.illustration;
  alloc   = &illustrationMeta.alloc;
  page    = &_meta->LPub.page;

  setZValue(98);
  setFlag(QGraphicsItem::ItemIsSelectable,true);
  setFlag(QGraphicsItem::ItemIsMovable,true);
}

void IllustrationItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString name = "Illustration ";

  PlacementData placementData = illustration->meta.LPub.illustration.placement.value();
  QAction *placementAction  = commonMenus.placementMenu(menu,name,
                              commonMenus.naturalLanguagePlacementWhatsThis(IllustrationType,placementData,name));

  QAction *allocAction = NULL;
        if (illustration->allocType() == Vertical) {
          allocAction = commonMenus.displayRowsMenu(menu,name);
        } else {
          allocAction = commonMenus.displayColumnsMenu(menu, name);
        }

  QAction *editBackgroundAction = commonMenus.backgroundMenu(menu,name);
  QAction *editBorderAction     = commonMenus.borderMenu(menu,name);
  QAction *marginAction         = commonMenus.marginMenu(menu,name);

  QAction *removeIllustrationAction = NULL;
  removeIllustrationAction = menu.addAction("Remove Illustration");
  removeIllustrationAction->setIcon(QIcon(":/resources/remove.png"));

  QAction *addPointerAction = menu.addAction("Add Arrow");
  addPointerAction->setWhatsThis("Add arrow from this illustration to the step where it is used");
  addPointerAction->setIcon(QIcon(":/resources/addarrow.png"));

  QAction *selectedAction = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
      return;
  } else if (selectedAction == addPointerAction) {
    Pointer *pointer = new Pointer(illustration->topOfIllustration(),illustrationMeta.pointer);
    float _loc = 0, _x1 = 0, _y1 = 0, _base = -1, _segments = 1;
    float           _x2 = 0, _y2 = 0;
    float           _x3 = 0, _y3 = 0;
    float           _x4 = 0, _y4 = 0;
    pointer->pointerMeta.setValue(
    PlacementEnc(TopLeft),
    _loc,
    _base,
    _segments,
    _x1,_y1,_x2,_y2,_x3,_y3,_x4,_y4);
    IllustrationPointerItem *illustrationPointer =
      new IllustrationPointerItem(illustration,&illustration->meta,pointer,this,view);
    illustrationPointer->defaultPointer();

  } else if (selectedAction == placementAction) {
    changePlacement(parentRelativeType,
                    relativeType,
                    "Placement",
                    illustration->topOfIllustration(),
                    illustration->bottomOfIllustration(),
                    &placement,false,0,false,false);

  } else if (selectedAction == editBackgroundAction) {
    changeBackground("Background",
                     illustration->topOfIllustration(),
                     illustration->bottomOfIllustration(),
                     &background,false,0,false);

  } else if (selectedAction == editBorderAction) {
    changeBorder("Border",
                 illustration->topOfIllustration(),
                 illustration->bottomOfIllustration(),
                 &border,false,0,false);

  } else if (selectedAction == marginAction) {
    changeMargins("Illustration Margins",
                  illustration->topOfIllustration(),
                  illustration->bottomOfIllustration(),
                  &margin,false,0,false);

  } else if (selectedAction == removeIllustrationAction) {
      // TODO - Create remove Illustration Function
//      removeIllustration(illustration->modelName(),
//                         illustration->topOfIllustration(),
//                         illustration->bottomOfIllustration());
  } else if (selectedAction == allocAction) {
    changeAlloc(illustration->topOfIllustration(),
                illustration->bottomOfIllustration(),
                illustration->allocMeta(),
                0);
  }
}

/*
 * As the illustration moves, the CSI stays in place, yet since the illustration is
 * grouped with the pointer, the pointer is moved.
 */

void IllustrationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  positionChanged = false;
  position        = pos();
  QGraphicsItem::mousePressEvent(event);
}

/*
 * When moving a illustration, we want the tip of any pointers to stay still.  We
 * need to figure out how much the illustration moved, and then compensate
 */

void IllustrationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsPixmapItem::mouseMoveEvent(event);
  // when sliding the illustration up, we get negative Y
  // when sliding left of illustration base we get negativeX?
  if ((flags() & QGraphicsItem::ItemIsMovable) && isSelected()) {
    QPoint delta(int(position.x() - pos().x() + 0.5),
                 int(position.y() - pos().y() + 0.5));

    if (delta.x() || delta.y()) {
      illustration->drawTips(delta);
      positionChanged = true;
    }
  }
}

void IllustrationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable) && positionChanged) {
    beginMacro(QString("DraggingIllustration"));

    QPointF delta(position.x() - pos().x(),
                  position.y() - pos().y());

    if (delta.x() || delta.y()) {

      QPoint deltaI(int(delta.x()+0.5),int(delta.y()+0.5));
      for (int i = 0; i < illustration->graphicsPointerList.size(); i++) {
        IllustrationPointerItem *pointer = illustration->graphicsPointerList[i];
        pointer->updatePointer(deltaI);
      }
      PlacementData placementData = placement.value();

      float w = delta.x()/gui->pageSize(illustration->meta.LPub.page, 0);
      float h = delta.y()/gui->pageSize(illustration->meta.LPub.page, 1);

      if (placementData.relativeTo == CsiType) {
        w = delta.x()/csiRect.width();
        h = delta.y()/csiRect.height();
      }

      placementData.offsets[0] -= w;
      placementData.offsets[1] -= h;
      placement.setValue(placementData);

      changePlacementOffset(illustration->topOfIllustration(),&placement,IllustrationType,false,0);
    }
    QGraphicsItem::mouseReleaseEvent(event);
    endMacro();
  } else {
    QGraphicsItem::mouseReleaseEvent(event);
  }
}

/*
 * Pointer Item
 *
 */

//---------------------------------------------------------------------------

/*
 * This is the constructor of a graphical pointer
 */

IllustrationPointerItem::IllustrationPointerItem(
  Illustration       *il,
  Meta          *meta,
  Pointer       *_pointer,
  QGraphicsItem *parent,
  QGraphicsView *_view)

  : PointerItem(parent)
{
  view          = _view;
  illustration       = il;
  pointer       = *_pointer;

  PointerData pointerData = pointer.pointerMeta.value();
  BorderData  border = meta->LPub.illustration.border.valuePixels();
  PlacementData illustrationPlacement = meta->LPub.illustration.placement.value();

  borderColor     = border.color;
  borderThickness = border.thickness;

  placement       = pointerData.placement;

  baseX           = il->size[XX];
  baseY           = il->size[YY];

  if (pointerData.segments == OneSegment) {
      int cX = illustration->parentStep->csiItem->loc[XX];
      int cY = illustration->parentStep->csiItem->loc[YY];
      int dX = pointerData.x1*illustration->parentStep->csiItem->size[XX];
      int dY = pointerData.y1*illustration->parentStep->csiItem->size[YY];

      if (illustration->placement.value().relativeTo == IllustrationType) {
          cX += illustration->parentStep->loc[XX];
          cY += illustration->parentStep->loc[YY];
          points[Tip] = QPoint(cX + dX - illustration->loc[XX], cY + dY - illustration->loc[YY]);
      } else {
          points[Tip] = QPoint(cX + dX - illustration->loc[XX], cY + dY - illustration->loc[YY]);
      }
       /*
       * What does it take to convert csiItem->loc[] and size[] to the position of
       * the tip in these cases:
       *   single step
       *     illustration relative to csi
       *     illustration relative to stepNumber
       *     illustration relative to pli
       *     illustration relative to page
       *
       *   step group
       *     illustration relative to csi
       *     illustration relative to stepNumber
       *     illustration relative to pli
       *     illustration relative to page
       *     illustration relative to stepGroup
       */
      if ( ! illustration->parentStep->onlyChild()) {
          switch (illustrationPlacement.relativeTo) {
          case PageType:
          case StepGroupType:
              points[Tip] += QPoint(illustration->parentStep->grandparent()->loc[XX],
                                    illustration->parentStep->grandparent()->loc[YY]);
              points[Tip] += QPoint(illustration->parentStep->range()->loc[XX],
                                    illustration->parentStep->range()->loc[YY]);
              points[Tip] += QPoint(illustration->parentStep->loc[XX],
                                    illustration->parentStep->loc[YY]);
              break;
          default:
              break;
          }
      }
  } else {
      points[Tip] = QPointF(pointerData.x1, pointerData.y1);
  }

  points[MidBase] = QPointF(pointerData.x3,pointerData.y3);
  points[MidTip]  = QPointF(pointerData.x4,pointerData.y4);

  QColor qColor(border.color);
  QPen pen(qColor);
  pen.setWidth(borderThickness);
  pen.setCapStyle(Qt::RoundCap);

  // shaft segments
  for (int i = 0; i < pointerData.segments; i++) {
      QLineF linef;
      shaft = new QGraphicsLineItem(linef,this);
      shaft->setPen(pen);
      shaft->setZValue(-5);
      shaft->setFlag(QGraphicsItem::ItemIsSelectable,false);
      shaft->setToolTip(QString("Arrow segment %1 - drag to move; right click to modify").arg(i+1));
      shaftSegments.append(shaft);
      addToGroup(shaft);
  }

  autoLocFromTip();

  QPolygonF poly;

  head = new QGraphicsPolygonItem(poly, this);
  head->setPen(qColor);
  head->setBrush(qColor);
  head->setFlag(QGraphicsItem::ItemIsSelectable,false);
  head->setToolTip("Arrow head - drag to move");
  addToGroup(head);

  for (int i = 0; i < NumGrabbers; i++) {
    grabbers[i] = NULL;
  }

  drawPointerPoly();
  setFlag(QGraphicsItem::ItemIsFocusable,true);
}

void IllustrationPointerItem::defaultPointer()
{
  points[Tip] = QPointF(illustration->parentStep->csiItem->loc[XX]+
                        illustration->parentStep->csiItem->size[XX]/2,
                        illustration->parentStep->csiItem->loc[YY]+
                        illustration->parentStep->csiItem->size[YY]/2);

  if ( ! illustration->parentStep->onlyChild()) {
    PlacementData illustrationPlacement = illustration->placement.value();
    switch (illustrationPlacement.relativeTo) {
      case PageType:
      case StepGroupType:
        points[Tip] += QPoint(illustration->parentStep->grandparent()->loc[XX],
                              illustration->parentStep->grandparent()->loc[YY]);
        points[Tip] += QPoint(illustration->parentStep->range()->loc[XX],
                              illustration->parentStep->range()->loc[YY]);
        points[Tip] += QPoint(illustration->parentStep->loc[XX],
                              illustration->parentStep->loc[YY]);
        points[Tip] -= QPoint(illustration->loc[XX],illustration->loc[YY]);
      break;
      default:
      break;
    }
  }
  autoLocFromTip();
  drawPointerPoly();
  calculatePointerMeta();
  addPointerMeta();
}

/*
 * Given the location of the Tip (as dragged around by the user)
 * calculate a reasonable placement and Loc for Base or MidTip points.
 */

bool IllustrationPointerItem::autoLocFromTip()
{
    int width = illustration->size[XX];
    int height = illustration->size[YY];
    int left = 0;
    int right = width;
    int top = 0;
    int bottom = height;

    QPoint intersect;
    int tx,ty;

    tx = points[Tip].x();
    ty = points[Tip].y();

    if (segments() != OneSegment) {
        PointerData pointerData = pointer.pointerMeta.value();
        int mx = pointerData.x3;
        points[Base] = QPointF(pointerData.x2,pointerData.y2);
        placement = pointerData.placement;

        if (segments() == ThreeSegments){
            points[MidTip].setY(ty);
            points[MidTip].setX(mx);
        }
    } else {
        /* Figure out which corner */
        BorderData borderData = illustration->background->border.valuePixels();
        int radius = (int) borderData.radius;

        if (ty >= top+radius && ty <= bottom-radius) {
            if (tx < left) {
                intersect = QPoint(left,ty);
                points[Tip].setY(ty);
                placement = Left;
            } else if (tx > right) {
                intersect = QPoint(right,ty);
                points[Tip].setY(ty);
                placement = Right;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx >= left+radius && tx <= right-radius) {
            if (ty < top) {
                intersect = QPoint(tx,top);
                points[Tip].setX(tx);
                placement = Top;
            } else if (ty > bottom) {
                intersect = QPoint(tx,bottom);
                points[Tip].setX(tx);
                placement = Bottom;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx < radius) {  // left?
            if (ty < radius) {
                intersect = QPoint(left+radius,top+radius);
                placement = TopLeft;
            } else {
                intersect = QPoint(radius,height-radius);
                placement = BottomLeft;
            }
        } else { // right!
            if (ty < radius) {
                intersect = QPoint(width-radius,radius);
                placement = TopRight;
            } else {
                intersect = QPoint(width-radius,height-radius);
                placement = BottomRight;
            }
        }

        points[Base] = intersect;
    }

    return true;
}

/*
 * Given the location of the MidBase point (as dragged around by the user)
 * calculate a reasonable placement and Loc for Base point.
 */

bool IllustrationPointerItem::autoLocFromMidBase()
{
        int width = illustration->size[XX];
        int height = illustration->size[YY];
        int left = 0;
        int right = width;
        int top = 0;
        int bottom = height;

        QPoint intersect;
        int tx,ty;

        tx = points[MidBase].x();
        ty = points[MidBase].y();

        /* Figure out which corner */
        BorderData borderData = illustration->background->border.valuePixels();
        int radius = (int) borderData.radius;

        if (ty >= top+radius && ty <= bottom-radius) {
            if (tx < left) {
                intersect = QPoint(left,ty);
                points[MidBase].setY(ty);
                placement = Left;
            } else if (tx > right) {
                intersect = QPoint(right,ty);
                points[MidBase].setY(ty);
                placement = Right;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx >= left+radius && tx <= right-radius) {
            if (ty < top) {
                intersect = QPoint(tx,top);
                points[MidBase].setX(tx);
                placement = Top;
            } else if (ty > bottom) {
                intersect = QPoint(tx,bottom);
                points[MidBase].setX(tx);
                placement = Bottom;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx < radius) {  // left?
            if (ty < radius) {
                intersect = QPoint(left+radius,top+radius);
                placement = TopLeft;
            } else {
                intersect = QPoint(radius,height-radius);
                placement = BottomLeft;
            }
        } else { // right!
            if (ty < radius) {
                intersect = QPoint(width-radius,radius);
                placement = TopRight;
            } else {
                intersect = QPoint(width-radius,height-radius);
                placement = BottomRight;
            }
        }

        points[MidTip].setX(tx);
        points[Base] = intersect;

    return true;
}


void IllustrationPointerItem::calculatePointerMetaLoc()
{
  float loc = 0;

  switch (placement) {
    case TopLeft:
    case TopRight:
    case BottomLeft:
    case BottomRight:
      loc = 0;
    break;
    case Top:
    case Bottom:
    {
      if (segments() == OneSegment)
         loc = points[Base].x()/illustration->size[XX];
      else
         loc = points[Base].x();
    }
    break;
    case Left:
    case Right:
    {
      if (segments() == OneSegment)
         loc = points[Base].y()/illustration->size[YY];
      else
         loc = points[Base].y();
    }
    break;
    default:
    break;
  }
  PointerData pointerData = pointer.pointerMeta.value();
  pointer.pointerMeta.setValue(
    placement,
    loc,
    0,
    segments(),
    pointerData.x1,
    pointerData.y1,
    pointerData.x2,
    pointerData.y2,
    pointerData.x3,
    pointerData.y3,
    pointerData.x4,
    pointerData.y4);
}

void IllustrationPointerItem::calculatePointerMeta()
{
  calculatePointerMetaLoc();

  PointerData pointerData = pointer.pointerMeta.value();
  if (segments() == OneSegment) {
      if (illustration->parentStep->onlyChild()) {
          points[Tip] += QPoint(illustration->loc[XX],illustration->loc[YY]);
      } else {
          PlacementData illustrationPlacement = illustration->meta.LPub.illustration.placement.value();

          switch (illustrationPlacement.relativeTo) {
          case CsiType:
          case PartsListType:
          case StepNumberType:
              points[Tip] += QPoint(illustration->loc[XX],illustration->loc[YY]);
              break;
          case PageType:
          case StepGroupType:
              points[Tip] -= QPoint(illustration->parentStep->grandparent()->loc[XX],
                                    illustration->parentStep->grandparent()->loc[YY]);
              points[Tip] -= QPoint(illustration->parentStep->loc[XX],
                                    illustration->parentStep->loc[YY]);
              points[Tip] += QPoint(illustration->loc[XX],illustration->loc[YY]);
              break;
          default:
              break;
          }
      }

      if (illustration->placement.value().relativeTo == IllustrationType) {
          points[Tip] -= QPoint(illustration->parentStep->loc[XX],
                                illustration->parentStep->loc[YY]);
      }

      pointerData.x1 = (points[Tip].x() - illustration->parentStep->csiItem->loc[XX])/illustration->parentStep->csiItem->size[XX];
      pointerData.y1 = (points[Tip].y() - illustration->parentStep->csiItem->loc[YY])/illustration->parentStep->csiItem->size[YY];
  } else {
      pointerData.x1 = points[Tip].x();
      pointerData.y1 = points[Tip].y();
      pointerData.x2 = points[Base].x();
      pointerData.y2 = points[Base].y();
      pointerData.x3 = points[MidBase].x();
      pointerData.y3 = points[MidBase].y();
      pointerData.x4 = points[MidTip].x();
      pointerData.y4 = points[MidTip].y();
  }
  pointer.pointerMeta.setValue(
    placement,
    pointerData.loc,
    0,
    segments(),
    pointerData.x1,
    pointerData.y1,
    pointerData.x2,
    pointerData.y2,
    pointerData.x3,
    pointerData.y3,
    pointerData.x4,
    pointerData.y4);
}
