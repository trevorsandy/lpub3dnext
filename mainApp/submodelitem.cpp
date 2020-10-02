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
#include <QMenu>
#include <QAction>
#include <QGraphicsRectItem>
#include <QGraphicsSceneContextMenuEvent>
#include "submodelitem.h"
#include "commonmenus.h"
#include "step.h"
#include "ranges.h"
#include "color.h"

void SubModel::setSize(
    UnitsMeta _size,
    float     _borderThickness){
  subModelImageSize = _size;
  borderThickness   = _borderThickness;
}

void SubModel::sizeit(){
  size[0] = int(subModelImageSize.valuePixels(0));
  size[1] = int(subModelImageSize.valuePixels(1));

  size[0] += 2*int(borderThickness);
  size[1] += 2*int(borderThickness);
}

void SubModelItem::setAttributes(
  Step           *_step,
  PlacementType   _parentRelativeType,
  SubModelMeta   &_subModelMeta,
  QGraphicsItem  *_parent)
{
  step               = _step;
  parentRelativeType = _parentRelativeType;

  size           = _subModelMeta.size;
  modelScale     = _subModelMeta.modelScale;
  border         = _subModelMeta.border;
  display        = _subModelMeta.display;
  background     = _subModelMeta.background;
  subModelColor  = _subModelMeta.subModelColor;

  placement      = _subModelMeta.placement;
  margin         = _subModelMeta.margin;
  relativeType   = SubModelType;

  // initialize pixmap using icon demensions
  pixmap         = new QPixmap(size.valuePixels(XX),
                               size.valuePixels(YY));

  // set image size
  subModel.sizeit();

  setSubModelImage(pixmap);

  QString toolTip("Sub Model - right-click to modify");
  setToolTip(toolTip);

  setZValue(10000);
  setParentItem(_parent);
  setPixmap(*pixmap);
  setTransformationMode(Qt::SmoothTransformation);
  setFlag(QGraphicsItem::ItemIsSelectable,true);

  delete pixmap;
}

SubModelItem::SubModelItem(){
  relativeType       = SubModelType;
  step               = NULL;
  pixmap             = NULL;
}

SubModelItem::SubModelItem(
    Step           *_step,
    PlacementType   _parentRelativeType,
    SubModelMeta   &_subModelMeta,
    QGraphicsItem  *_parent){

  setAttributes(
        _step,
        _parentRelativeType,
        _subModelMeta,
        _parent);
}

void SubModelItem::setSubModelImage(QPixmap *pixmap){
  // set pixmap to transparent
  pixmap->fill(Qt::transparent);

  // set border and background parameters
  BorderData     borderData     = border.valuePixels();
  BackgroundData backgroundData = background.value();

  // set rectangle size and demensions parameters
  int ibt = int(borderData.thickness);
  QRectF irect(ibt/2,ibt/2,pixmap->width()-ibt,pixmap->height()-ibt);

  // set painter and render hints (initialized with pixmap)
  QPainter painter;
  painter.begin(pixmap);
  painter.setRenderHints(QPainter::Antialiasing,true);

  /* BACKGROUND */

  // set icon background colour
  bool useGradient = false;
  QColor brushColor;
  switch(backgroundData.type) {
    case BackgroundData::BgTransparent:
      brushColor = Qt::transparent;
      break;
    case BackgroundData::BgGradient:
      useGradient = true;
      brushColor = Qt::transparent;
      break;
    case BackgroundData::BgColor:
    case BackgroundData::BgSubmodelColor:
      if (backgroundData.type == BackgroundData::BgColor) {
          brushColor = LDrawColor::color(backgroundData.string);
        } else {
          brushColor = LDrawColor::color(subModelColor.value(0));
        }
      break;
    case BackgroundData::BgImage:
      break;
    }

  useGradient ? painter.setBrush(QBrush(setGradient())) :
                painter.setBrush(brushColor);

  /* BORDER */

  // set icon border pen colour
  QPen borderPen;
  QColor borderPenColor;
  if (borderData.type == BorderData::BdrNone) {
      borderPenColor = Qt::transparent;
    } else {
      borderPenColor =  LDrawColor::color(borderData.color);
    }
  borderPen.setColor(borderPenColor);
  borderPen.setCapStyle(Qt::RoundCap);
  borderPen.setJoinStyle(Qt::RoundJoin);
  if (borderData.line == BorderData::BdrLnSolid){
      borderPen.setStyle(Qt::SolidLine);
    }
  else if (borderData.line == BorderData::BdrLnDash){
      borderPen.setStyle(Qt::DashLine);
    }
  else if (borderData.line == BorderData::BdrLnDot){
      borderPen.setStyle(Qt::DotLine);
    }
  else if (borderData.line == BorderData::BdrLnDashDot){
      borderPen.setStyle(Qt::DashDotLine);
    }
  else if (borderData.line == BorderData::BdrLnDashDotDot){
      borderPen.setStyle(Qt::DashDotDotLine);
    }
  borderPen.setWidth(ibt);

  painter.setPen(borderPen);

  // set icon border demensions
  qreal rx = borderData.radius;
  qreal ry = borderData.radius;
  qreal dx = pixmap->width();
  qreal dy = pixmap->height();

  if (dx && dy) {
      if (dx > dy) {
          rx *= dy;
          rx /= dx;
        } else {
          ry *= dx;
          ry /= dy;
        }
    }

  // draw icon rectangle
  if (borderData.type == BorderData::BdrRound) {
      painter.drawRoundRect(irect,int(rx),int(ry));
    } else {
      painter.drawRect(irect);
    }
  painter.end();
}

QGradient SubModelItem::setGradient(){

  BackgroundData backgroundData = background.value();
  QPolygonF pts;
  QGradientStops stops;

  QSize gSize(backgroundData.gsize[0],backgroundData.gsize[1]);

    for (int i=0; i<backgroundData.gpoints.size(); i++)
      pts.append(backgroundData.gpoints.at(i));

  QGradient::CoordinateMode mode = QGradient::LogicalMode;
  switch (backgroundData.gmode){
    case BackgroundData::LogicalMode:
      mode = QGradient::LogicalMode;
    break;
    case BackgroundData::StretchToDeviceMode:
      mode = QGradient::StretchToDeviceMode;
    break;
    case BackgroundData::ObjectBoundingMode:
      mode = QGradient::ObjectBoundingMode;
    break;
    }

  QGradient::Spread spread = QGradient::RepeatSpread;
  switch (backgroundData.gspread){
    case BackgroundData::PadSpread:
      spread = QGradient::PadSpread;
    break;
    case BackgroundData::RepeatSpread:
      spread = QGradient::RepeatSpread;
    break;
    case BackgroundData::ReflectSpread:
      spread = QGradient::ReflectSpread;
    break;
    }

  QGradient g = QLinearGradient(pts.at(0), pts.at(1));
  switch (backgroundData.gtype){
    case BackgroundData::LinearGradient:
      g = QLinearGradient(pts.at(0), pts.at(1));
    break;
    case BackgroundData::RadialGradient:
      {
        QLineF line(pts[0], pts[1]);
        if (line.length() > 132){
            line.setLength(132);
          }
        g = QRadialGradient(line.p1(),  qMin(gSize.width(), gSize.height()) / 3.0, line.p2());
      }
    break;
    case BackgroundData::ConicalGradient:
      {
        qreal angle = backgroundData.gangle;
        g = QConicalGradient(pts.at(0), angle);
      }
    break;
    case BackgroundData::NoGradient:
    break;
    }

  for (int i=0; i<backgroundData.gstops.size(); ++i) {
      stops.append(backgroundData.gstops.at(i));
    }

  g.setStops(stops);
  g.setSpread(spread);
  g.setCoordinateMode(mode);

  return g;
}

void SubModelItem::setFlag(GraphicsItemFlag flag, bool value)
{
  QGraphicsItem::setFlag(flag,value);
}

void SubModelItem::contextMenuEvent(
    QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  PlacementData placementData = placement.value();

  QString pl = "Submodel";
  QAction *placementAction = NULL;
  bool singleStep = parentRelativeType == SingleStepType;
  if (! singleStep) {
      placementAction  = commonMenus.placementMenu(menu,pl,
                                                   commonMenus.naturalLanguagePlacementWhatsThis(relativeType,placementData,pl));
    }

  QAction *backgroundAction = commonMenus.backgroundMenu(menu,pl);
  QAction *borderAction     = commonMenus.borderMenu(menu,pl);
  QAction *marginAction     = commonMenus.marginMenu(menu,pl);
  QAction *displayAction    = commonMenus.displayMenu(menu,pl);

  QAction *deleteSubModelAction = menu.addAction("Delete " +pl);
  deleteSubModelAction->setWhatsThis("Delete this Submodel");
  deleteSubModelAction->setIcon(QIcon(":/resources/delete.png"));

  QAction *selectedAction  = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
      return;
    }

  Where top;
  Where bottom;

  switch (parentRelativeType) {
    case CalloutType:
      top    = step->topOfCallout();
      bottom = step->bottomOfCallout();
    break;
    default:
      top    = step->topOfStep();
      bottom = step->bottomOfStep();
    break;
  }

  if (selectedAction == placementAction) {

      // logging only
      if (! singleStep) {
      bool multiStep = parentRelativeType == StepGroupType;
      logInfo() << "\nMOVE SUB_MODEL - "
                << "\nPAGE- "
                << (multiStep ? " \nMulti-Step Page" : " \nSingle-Step Page")
                << "\nPAGE WHERE -                  "
                << " \nPage TopOf (Model Name):     " << top.modelName
                << " \nPage TopOf (Line Number):    " << top.lineNumber
                << " \nPage BottomOf (Model Name):  " << bottom.modelName
                << " \nPage BottomOf (Line Number): " << bottom.lineNumber
                << "\nUSING PLACEMENT DATA -        "
                << " \nPlacement:                   " << PlacNames[placement.value().placement]     << " (" << placement.value().placement << ")"
                << " \nJustification:               " << PlacNames[placement.value().justification] << " (" << placement.value().justification << ")"
                << " \nPreposition:                 " << PrepNames[placement.value().preposition]   << " (" << placement.value().justification << ")"
                << " \nRelativeTo:                  " << RelNames[placement.value().relativeTo]     << " (" << placement.value().relativeTo << ")"
                << " \nRectPlacement:               " << RectNames[placement.value().rectPlacement] << " (" << placement.value().rectPlacement << ")"
                << " \nOffset[0]:                   " << placement.value().offsets[0]
                << " \nOffset[1]:                   " << placement.value().offsets[1]
                << "\nOTHER DATA -                  "
                << " \nRelativeType:                " << RelNames[relativeType]       << " (" << relativeType << ")"
                << " \nParentRelativeType:          " << RelNames[parentRelativeType] << " (" << parentRelativeType << ")"
                                                ;
        } // end logging only

      changePlacement(parentRelativeType,
                      SingleStepType,         //not using SubModelType intentionally
                      pl+" Placement",
                      top,
                      bottom,
                      &placement,true,1,0,false);
    } else if (selectedAction == backgroundAction) {
      changeBackground(pl+" Background",
                       top,
                       bottom,
                       &background);
    } else if (selectedAction == borderAction) {
      changeBorder(pl+" Border",
                   top,
                   bottom,
                   &border);
    } else if (selectedAction == marginAction) {
      changeMargins(pl+" Margins",
                    top,
                    bottom,
                    &margin);
    } else if (selectedAction == displayAction){
      changeBool(top,
                 bottom,
                 &display);
    } else if (selectedAction == deleteSubModelAction) {
      beginMacro("DeleteSubModel");
      deleteSubModel(top);
      endMacro();
    }
}

void SubModelItem::mousePressEvent(
    QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mousePressEvent(event);
  positionChanged = false;
  position = pos();
}

void SubModelItem::mouseMoveEvent(
    QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseMoveEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
      positionChanged = true;
    }
}

void SubModelItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

      Where top;

      switch (parentRelativeType) {
        case CalloutType:
          top    = step->topOfCallout();
          break;
        default:
          top    = step->topOfStep();
          break;
        }

      if (positionChanged) {

          beginMacro(QString("DragSubModel"));

          QPointF newPosition;
          newPosition = pos() - position;

          if (newPosition.x() || newPosition.y()) {
              positionChanged = true;

              PlacementData placementData = placement.value();
              placementData.offsets[0] += newPosition.x()/relativeToSize[0];
              placementData.offsets[1] += newPosition.y()/relativeToSize[1];
              placement.setValue(placementData);

              logInfo() << "\nCHANGE SUB_MODEL - "
                        << "\nPAGE WHERE - "
                        << " \nStep TopOf (Model Name): "    << top.modelName
                        << " \nStep TopOf (Line Number): "   << top.lineNumber
                        << "\nUSING PLACEMENT DATA - "
                        << " \nPlacement: "                 << PlacNames[placement.value().placement]     << " (" << placement.value().placement << ")"
                        << " \nJustification: "             << PlacNames[placement.value().justification] << " (" << placement.value().justification << ")"
                        << " \nPreposition: "               << PrepNames[placement.value().preposition]   << " (" << placement.value().justification << ")"
                        << " \nRelativeTo: "                << RelNames[placement.value().relativeTo]     << " (" << placement.value().relativeTo << ")"
                        << " \nRectPlacement: "             << RectNames[placement.value().rectPlacement] << " (" << placement.value().rectPlacement << ")"
                        << " \nOffset[0]: "                 << placement.value().offsets[0]
                        << " \nOffset[1]: "                 << placement.value().offsets[1]
                        << "\nOTHER DATA - "
                        << " \nRelativeType: "               << RelNames[relativeType]       << " (" << relativeType << ")"
                        << " \nParentRelativeType: "         << RelNames[parentRelativeType] << " (" << parentRelativeType << ")"
                           ;

              changePlacementOffset(top,
                                    &placement,
                                    relativeType);
              endMacro();
            }
        }
    }
}

void SubModelItem::change()
{
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

      Where top;
      Where bottom;

      switch (parentRelativeType) {
        case CalloutType:
          top    = step->topOfCallout();
          bottom = step->bottomOfCallout();
        break;
        default:
          top    = step->topOfStep();
          bottom = step->bottomOfStep();
        break;
      }

      if (sizeChanged) {

          beginMacro(QString("ResizeSubModel"));

          PlacementData placementData = placement.value();

          qreal topLeft[2] = { sceneBoundingRect().left(),  sceneBoundingRect().top() };
          qreal size[2]    = { sceneBoundingRect().width(), sceneBoundingRect().height() };
          calcOffsets(placementData,placementData.offsets,topLeft,size);
          placement.setValue(placementData);

          changePlacementOffset(top, &placement, relativeType);

          modelScale.setValue(modelScale.value()*oldScale);
          changeFloat(top,bottom,&modelScale, 1, false);

          logInfo() << "\nRESIZE SUB MODEL - "
                    << "\nPICTURE DATA - "
                    << " \nmodelScale: "                 << modelScale.value()
                    << " \nMargin X: "                   << margin.value(0)
                    << " \nMargin Y: "                   << margin.value(1)
                    << " \nDisplay: "                    << display.value()
                    << "\nPAGE WHERE - "
                    << " \nStep TopOf (Model Name): "    << top.modelName
                    << " \nStep TopOf (Line Number): "   << top.lineNumber
                    << " \nStep BottomOf (Model Name): " << bottom.modelName
                    << " \nStep BottomOf (Line Number): "<< bottom.lineNumber
                    << "\nUSING PLACEMENT DATA - "
                    << " \nPlacement: "                  << PlacNames[placement.value().placement]     << " (" << placement.value().placement << ")"
                    << " \nJustification: "              << PlacNames[placement.value().justification] << " (" << placement.value().justification << ")"
                    << " \nPreposition: "                << PrepNames[placement.value().preposition]   << " (" << placement.value().justification << ")"
                    << " \nRelativeTo: "                 << RelNames[placement.value().relativeTo]     << " (" << placement.value().relativeTo << ")"
                    << " \nRectPlacement: "              << RectNames[placement.value().rectPlacement] << " (" << placement.value().rectPlacement << ")"
                    << " \nOffset[0]: "                  << placement.value().offsets[0]
                    << " \nOffset[1]: "                  << placement.value().offsets[1]
                    << "\nMETA WHERE - "
                    << " \nMeta Here (Model Name): "     << placement.here().modelName
                    << " \nMeta Here (Line Number): "    << placement.here().lineNumber
                    << "\nOTHER DATA - "
                    << " \nRelativeType: "               << RelNames[relativeType]       << " (" << relativeType << ")"
                    << " \nParentRelativeType: "         << RelNames[parentRelativeType] << " (" << parentRelativeType << ")"
                       ;
          endMacro();
        }
    }
}
