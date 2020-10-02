 
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
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
 * This class implements a fundamental class for placing things relative to
 * other things.  This concept is the cornerstone of LPub's meta commands
 * for describing what building instructions should look like without having
 * to specify inches, centimeters or pixels.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef placementH
#define placementH


#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QSize>
#include <QRect>

#include "meta.h"
#include "metaitem.h"

class QPixmap;
class Step;
class Steps;


//---------------------------------------------------------------------------
/*
 * Think of the possible placement as a two dimensional table, of
 * places where something can be placed within a rectangle.
 * -- see step.cpp for detail walkthrough --
 *
*  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
*  CMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMC
*  CMCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCMC
*  CMCOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOCMC
*  CMCOCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCOCMC
*  CMCOCIIIIIIIIIIIIIIIIIIIIIIIIIIIIICOCMC
*  CMCOCICCCCCCCCCCCCCCCCCCCCCCCCCCCICOCMC
*  CMCOCICWWWWWWWWWWWWWWWWWWWWWWWWWCICOCMC
*  CMCOCICWCCCCCCCCCCCCCCCCCCCCCCCWCICOCMC
*  CMCOCICWCTTTTTTTTTTTTTTTTTTTTTCWCICOCMC
*  CMCOCICWCTCCCCCCCCCCCCCCCCCCCTCWCICOCMC
*  CMCOCICWCTCDDDDDDDDDDDDDDDDDCTCWCICOCMC
*  CMCOCICWCTCDCCCCCCCCCCCCCCCDCTCWCICOCMC
*  CMCOCICWCTCDCRRRRRRRRRRRRRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCCCCCCCCCCCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSSSSSSSSSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCCCCCCCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCPPPPPCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCPCCCPCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCPCACPCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCPCCCPCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCPPPPPCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSCCCCCCCSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCSSSSSSSSSCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRCCCCCCCCCCCRCDCTCWCICOCMC
*  CMCOCICWCTCDCRRRRRRRRRRRRRCDCTCWCICOCMC
*  CMCOCICWCTCDCCCCCCCCCCCCCCCDCTCWCICOCMC
*  CMCOCICWCTCDDDDDDDDDDDDDDDDDCTCWCICOCMC
*  CMCOCICWCTCCCCCCCCCCCCCCCCCCCTCWCICOCMC
*  CMCOCICWCTTTTTTTTTTTTTTTTTTTTTCWCICOCMC
*  CMCOCICWCCCCCCCCCCCCCCCCCCCCCCCWCICOCMC
*  CMCOCICWWWWWWWWWWWWWWWWWWWWWWWWWCICOCMC
*  CMCOCICCCCCCCCCCCCCCCCCCCCCCCCCCCICOCMC
*  CMCOCIIIIIIIIIIIIIIIIIIIIIIIIIIIIICOCMC
*  CMCOCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCOCMC
*  CMCOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOCMC
*  CMCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCMC
*  CMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMC
*  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
 *
 *  The table above represents either the Horizontal slice
 *  going through the CSI (represented by A for assembly),
 *  or the Vertical slice going through the CSI.
 *
 *  C0  - callout relative to submodel
 *  M0  - SubModel relative to PLI
 *  C1  - callout relative to oneone
 *  O0  - OneToOne relative to PLI
 *  C2  - callout relative to illustration
 *  I0  - Illustration relative to Csi
 *  C3  - callout relative to rightwrong
 *  W0  - RightWrong relative to Csi
 *  C4  - callout relative to sticker
 *  T0  - Sticker relative to csi
 *  C5  - callout relative to partID
 *  D1  - PartID relative to csi
 *  C6  - callout relative to rotateIcon     C0
 *  R0  - rotateIcon relateive to csi	     R0
 *  C7  - callout relative to step number    C1
 *  S0  - step number relative to csi        S0
 *  C8  - callout relative to PLI            C2
 *  P0  - pli relative to csi                P0
 *  C9  - callout relative to csi            C3
 *  A   - csi                                A
 *  C10 - callout relative to csi            C4
 *  P1  - pli relative to csi                P1
 *  C11 - callout relative to PLI            C5
 *  S1  - step number relative to csi        S1
 *  C12 - callout relative to step number    C6
 *  R1  - rotateIcon relateive to csi        R1
 *  C13 - callout relative to rotateIcon     C7
 *  D1  - PartID relative to csi
 *  C14 - callout relative to partID
 *  T1  - Sticker relative to csi
 *  C15 - callout relative to sticker
 *  W1  - RightWrong relative to Csi
 *  C16 - callout relative to rightwrong
 *  I1  - Illustration relative to Csi
 *  C17 - callout relative to illustration
 *  O1  - OneToOne relative to PLI
 *  C18 - callout relative to oneone
 *  M1  - SubModel relative to PLI
 *  C19 - callout relative to submodel
 *
 */
//---------------------------------------------------------------------------

enum Boundary {
  StartOfSteps       = 1,
  StartOfRange       = 2,
  EndOfRange         = 4,
  EndOfSteps         = 8,
  StartAndEndOfSteps = 9,
  StartAndEndOfRange = 6,
  Middle             = 16
};

enum {    	 // -1
TblCo0 = 0,      //  0
TblSm0,   	 //  1  - SubModel
TblCo1,   	 //  2
TblOto0,   	 //  3  - OneToOne
TblCo2,   	 //  4
TblIl0,   	 //  5	- Illustration
TblCo3,   	 //  6
TblRw0,   	 //  7	- RightWrong
TblCo4,   	 //  8
TblSt0,   	 //  9	- Sticker
TblCo5,   	 //  10
TblId0,   	 //  11	- PartID
TblCo6,   	 //  12                 TblCo0 = 0,
TblRi0,   	 //  13 - RotateIcon    TblRi0
TblCo7,   	 //  14          	TblCo1
TblSn0,   	 //  15 - StepNumber    TblSn0
TblCo8,   	 //  16          	TblCo2
TblPli0,   	 //  17 - PartslList    TblPli0
TblCo9,   	 //  18          	TblCo3
TblCsi,   	 //  19 - CSI
TblCo10,   	 //  20          	TblCo4
TblPli1,   	 //  21 - PartsList     TblPli1
TblCo11,   	 //  22          	TblCo5
TblSn1,   	 //  23 - StepNumber    TblSn1
TblCo12,   	 //  24          	TblCo6
TblRi1,   	 //  25 - RotateIcon    TblRi1
TblCo13,   	 //  26          	TblCo7
TblId1,   	 //  27	- PartID
TblCo14,  	 //  28
TblSt1,   	 //  29	- Sticker
TblCo15,   	 //  30
TblRw1,   	 //  31	- RightWrong
TblCo16,   	 //  32
TblIl1,   	 //  33	- Illustration
TblCo17,   	 //  34
TblOto1,   	 //  35	- OneToOne
TblCo18,   	 //  36
TblSm1,   	 //  37 - SubModel
TblCo19,   	 //  38
NumPlaces        //  39
};

enum dim {
  XX = 0,
  YY = 1
};

class Placement {
  public:
    int           size[2];         // How big am I?
    int           loc[2];          // Where do I live within my group
    int           tbl[2];          // Where am I in my grid?
    int           boundingSize[2]; // Me and my neighbors
    int           boundingLoc[2];  // Where do I live within my group
    
    void setSize(int _size[2])
    {
      size[XX] = boundingSize[XX] = _size[XX];
      size[YY] = boundingSize[YY] = _size[YY];
    }
    
    void setSize(int x, int y)
    {
      size[XX] = boundingSize[XX] = x;
      size[YY] = boundingSize[YY] = y;
    }
    
    void setBoundingSize()
    {
      boundingSize[XX] = size[XX];
      boundingSize[YY] = size[YY];
    }
      
    PlacementType relativeType;  // What am I?
    PlacementMeta placement;     // Where am I placed?
    MarginsMeta   margin;        // How much room do I need?
    Placement    *relativeToParent;
    qreal         relativeToLoc[2];
    qreal         relativeToSize[2];

    QList<Placement *> relativeToList; // things placed relative to me

    Placement()
    {
      size[0] = 0;
      size[1] = 0;
      loc[0]  = 0;
      loc[1]  = 0;
      tbl[0]  = 0;
      tbl[1]  = 0;
      relativeToLoc[0] = 0;
      relativeToLoc[1] = 0;
      relativeToSize[0] = 1;
      relativeToSize[1] = 1;
      relativeToParent = NULL;
      boundingSize[0] = 0;
      boundingSize[1] = 0;
      boundingLoc[0] = 0;
      boundingLoc[1] = 0;
    }
    
    void assign(Placement *from)
    {
      size[0] = from->size[0];
      size[1] = from->size[1];
      loc[0] = from->loc[0];
      loc[1] = from->loc[1];
      tbl[0] = from->tbl[0];
      tbl[1] = from->tbl[1];
      relativeType = from->relativeType;
      placement = from->placement;
      margin = from->margin;
      relativeToParent = from->relativeToParent;
      relativeToLoc[0] = from->relativeToLoc[0];
      relativeToLoc[1] = from->relativeToLoc[1];
      relativeToSize[0] = from->relativeToSize[0];
      relativeToSize[1] = from->relativeToSize[1];
      boundingSize[0] = from->boundingSize[0];
      boundingSize[1] = from->boundingSize[1];
      boundingLoc[0] = from->boundingLoc[0];
      boundingLoc[1] = from->boundingLoc[1];
    }

    virtual ~Placement()
    {
      relativeToList.empty();
    }

    void appendRelativeTo(Placement *element);

    int  relativeTo(
      Step      *step);

    int relativeToSg(
      Steps    *steps);

    void placeRelative(
      Placement *placement);

    void placeRelativeBounding(
      Placement *placement);

    void placeRelative(
      Placement *placement,
      int        margin[]);
      
    void placeRelative(
      Placement *them,
      int   them_size[2],
      int   lmargin[2]);

    void justifyX(
      int origin,
      int height);

    void justifyY(
      int origin,
      int height);
      
    void calcOffsets(
      PlacementData &placementData,
      float offsets[2],
      qreal topLeft[2],
      qreal size[2]);

};

class PlacementPixmap : public Placement {
  public:
    QPixmap   *pixmap;

    PlacementPixmap()
    {
    }
};

class PlacementNum : public Placement {
  public:
    QString str;
    QString font;
    QString color;
    int  number;

    PlacementNum()
    {
      number = 0;
    }
    void format(char *format)
    {
      str.sprintf(format,number);
    }
    void sizeit();
    void sizeit(QString fmt);
};

class PlacementHeader : public Placement,
                        public QGraphicsPixmapItem {
public:
    PlacementHeader() {}
    PlacementHeader(
        PageHeaderMeta &_pageHeaderMeta,
        QGraphicsItem  *_parent)
    {
      relativeType = PageHeaderType;
      placement    = _pageHeaderMeta.placement;
      size[XX]     = _pageHeaderMeta.size.valuePixels(0);
      size[YY]     = _pageHeaderMeta.size.valuePixels(1);
      setParentItem(_parent);
    }
};

class PlacementFooter : public Placement ,
                        public QGraphicsPixmapItem {
public:
    PlacementFooter() {}
    PlacementFooter(
        PageFooterMeta &_pageFooterMeta,
        QGraphicsItem  *_parent)
    {
      relativeType = PageFooterType;
      placement    = _pageFooterMeta.placement;
      size[XX]     = _pageFooterMeta.size.valuePixels(0);
      size[YY]     = _pageFooterMeta.size.valuePixels(1);
      setParentItem(_parent);
    }
};



#endif
