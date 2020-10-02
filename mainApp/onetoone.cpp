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

#include "onetoone.h"
#include "lpub.h"

int OneToOne::addOneToOne(
    int       submodelLevel,
    QGraphicsItem *parent)
{
  if (parts.size()) {
      background =
          new PliBackgroundItem(
            this,
            size[0],
          size[1],
          parentRelativeType,
          submodelLevel,
          parent);

      if ( ! background) {
          return -1;
        }

      background->size[0] = size[0];
      background->size[1] = size[1];

      positionChildren(size[1],1.0,1.0);
    } else {
      background = NULL;
    }
  return 0;
}


int OneToOne::sizeOneToOne(ConstrainData::Constrain constrain, unsigned height)
{
  if (parts.size() == 0) {
      return 1;
    }

  if (meta) {
      ConstrainData constrainData;
      constrainData.type = constrain;
      constrainData.constraint = height;

      return resizePli(meta,constrainData);
    }
  return 1;
}
