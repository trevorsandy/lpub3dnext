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

#ifndef ONETOONE_H
#define ONETOONE_H

#include <QHash>
#include "pli.h"

class OneToOnePart;
class OneToOne : public Pli
{

public:
  OneToOne(PliType _type = TypeOneToOne)
  {
    bom = false;
    oto = false;
    rw  = false;
    switch(_type){
      case TypePli:
        break;
      case TypeBom:
        bom = true;
        break;
      case TypeOneToOne:
        oto = true;
        break;
      case TypeRightWrong:
        rw  = true;
        break;
      }
    relativeType = PartsListType;
    initAnnotationString();
    steps = NULL;
    step = NULL;
    meta = NULL;
    widestPart = 1;
    tallestPart = 1;
    background = NULL;
    splitBom = false;
  }
  ~OneToOne()
  {
    clear();
  }
  int  addOneToOne (int, QGraphicsItem *);
  int  sizeOneToOne(ConstrainData::Constrain, unsigned height);

};

#endif // ONETOONE_H
