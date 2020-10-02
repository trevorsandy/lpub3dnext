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

#ifndef LGRAPHICSVIEW_H
#define LGRAPHICSVIEW_H

#include <QGraphicsView>
#include "qgraphicsscene.h"

enum FitMode { FitNone, FitWidth, FitVisible, FitTwoPages, FitContinuousScroll };

enum zValues {
  PageBackgroundZValue = 0,
  PageNumberZValue = 10,
  PagePLIZValue = 20,
  PageInstanceZValue = 30,
  AssemZValue = 30,
  StepGroupZValue = 30,
  CalloutPointerZValue = 45,
  CalloutBackgroundZValue = 50,
  CalloutAssemZValue = 55,
  CalloutInstanceZValue = 60,
};

//class QGraphicsScene;
class LGraphicsScene;
class PageBackgroundItem;

class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QMimeData;
class QUrl;

class LGraphicsView : public QGraphicsView
{
public:
  LGraphicsView(LGraphicsScene *scene);
  PageBackgroundItem *pageBackgroundItem;   // page background used for sizing
  FitMode             fitMode;              // how to fit the scene into the view

  void fitVisible(const QRectF rect);
  void fitWidth(const QRectF rect);
  void actualSize();
  void zoomIn();
  void zoomOut();

protected:

  void resizeEvent(QResizeEvent *event);

  // drag and drop
  void dragMoveEvent(QDragMoveEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);

private:

  QRectF  pageRect;

};

/*
 * View Ruler Class
 *
 */
#include <QGridLayout>
#define RULER_BREADTH 20

class LRuler : public QWidget
{
Q_OBJECT
Q_ENUMS(RulerType)
Q_PROPERTY(qreal origin READ origin WRITE setOrigin)
Q_PROPERTY(qreal rulerUnit READ rulerUnit WRITE setRulerUnit)
Q_PROPERTY(qreal rulerZoom READ rulerZoom WRITE setRulerZoom)

public:
  enum RulerType { Horizontal, Vertical };
LRuler(LRuler::RulerType rulerType, QWidget* parent)
: QWidget(parent), mRulerType(rulerType), mOrigin(0.), mRulerUnit(1.),
  mRulerZoom(1.), mMouseTracking(false), mDrawText(true)
{
  // disabled due to performance hit
  // setMouseTrack(true); // new changed from setMouseTracking(true)
  QFont txtFont("SansSerif", 5,20);
  txtFont.setStyleHint(QFont::SansSerif,QFont::PreferOutline);
  setFont(txtFont);
}

QSize minimumSizeHint() const
{
  return QSize(RULER_BREADTH,RULER_BREADTH);
}

LRuler::RulerType rulerType() const
{
  return mRulerType;
}

qreal origin() const
{
  return mOrigin;
}

qreal rulerUnit() const
{
  return mRulerUnit;
}

qreal rulerZoom() const
{
  return mRulerZoom;
}

public slots:
  void setOrigin(const qreal origin);
  void setRulerUnit(const qreal rulerUnit);
  void setRulerZoom(const qreal rulerZoom);
  void setCursorPos(const QPoint cursorPos);
  void setMouseTrack(const bool track);

protected:
  void mouseMoveEvent(QMouseEvent* event);
  void paintEvent(QPaintEvent* event);

private:
  void drawAScaleMeter(QPainter* painter, QRectF rulerRect, qreal scaleMeter, qreal startPositoin);
  void drawFromOriginTo(QPainter* painter, QRectF rulerRect, qreal startMark, qreal endMark, int startTickNo, qreal step, qreal startPosition);
  void drawMousePosTick(QPainter* painter);

  RulerType mRulerType;
  qreal mOrigin;
  qreal mRulerUnit;
  qreal mRulerZoom;
  QPoint mCursorPos;
  bool mMouseTracking;
  bool mDrawText;
};
/*
*/

#endif // LGRAPHICSVIEW_H
