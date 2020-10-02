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

#include <QPen>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QGraphicsView>
#include "lgraphicsscene.h"
#include "lgraphicsview.h"

LGraphicsScene::LGraphicsScene(QObject *parent)
  : QGraphicsScene(parent)
{
  selectedItem  = NULL;
  guidePen      = QPen(Qt::black, 1);
  drawAxis      = false;
  guidesEnabled = false;
}

QPointF LGraphicsScene::setAxisPos(const QPointF &mousePos){
  if (drawAxis) {
      QPointF itemAxisPos;
      if (selectedItem){
          itemAxisPos = selectedItem->boundingRect().topLeft(); // temporary position....
          // we need to be able to set and drag the axis based on where we select the
          // object we are dragging

//          QPointF itemCenter = selectedItem->boundingRect().center();
//          if (mousePos.x() >= itemCenter.x()) {
//              if (mousePos.y() >= itemCenter.y()){
//                  itemAxisPos = selectedItem->boundingRect().topRight();
//                } else {
//                  itemAxisPos = selectedItem->boundingRect().bottomRight();
//                }
//            } else {
//              if (mousePos.y() >= itemCenter.y()){
//                  itemAxisPos = selectedItem->boundingRect().topLeft();
//                } else {
//                  itemAxisPos = selectedItem->boundingRect().bottomLeft();
//                }
//            }
        } else {
          itemAxisPos = mousePos;
        }
      return itemAxisPos;
    }
  return QPointF(0,0);
}

void LGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  if (guidesEnabled) {
      axisPos = setAxisPos(event->scenePos());
      invalidate(sceneRect(),QGraphicsScene::ForegroundLayer);
    }
  QGraphicsScene::mouseMoveEvent(event);
}

void LGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event){
  if (guidesEnabled) {
      drawAxis = true;
      QGraphicsView *view;
      foreach(view, this->views()){
          if (view->underMouse() || view->hasFocus()) break;
        }
      if (view && this->selectedItems().size() > 0) {
          selectedItem = itemAt(event->scenePos(), view->transform());
        }
    }
  QGraphicsScene::mousePressEvent(event);
}

void LGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
  if (guidesEnabled) {
      drawAxis = false;
      invalidate(sceneRect(),QGraphicsScene::ForegroundLayer);
      selectedItem = NULL;
    }
  QGraphicsScene::mouseReleaseEvent(event);
}

void LGraphicsScene::drawForeground(QPainter *painter, const QRectF &rect){
  if (drawAxis) {
      QRectF sceneRect = this->sceneRect();
      painter->setClipRect(sceneRect);
      painter->setPen(guidePen);
      painter->drawLine(axisPos.x(), sceneRect.top(), axisPos.x(), sceneRect.bottom());
      painter->drawLine(sceneRect.left(), axisPos.y(), sceneRect.right(), axisPos.y());
    }
  QGraphicsScene::drawForeground(painter, rect);
}
