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
 * This class represents one step including a step number, and assembly
 * image, possibly a parts list image and zero or more callouts needed for
 * the step.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QFileInfo>
#include <QDir>
#include <QFile>

#include "lpub.h"
#include "step.h"
#include "range.h"
#include "ranges.h"
#include "ranges_element.h"
#include "render.h"
//#include "callout.h"
#include "calloutbackgrounditem.h"
#include "pointer.h"
#include "calloutpointeritem.h"
//#include "pli.h"
#include "numberitem.h"
//#include "csiitem.h"
#include "resolution.h"
#include "dependencies.h"
#include "paths.h"

/*********************************************************************
 *
 * Create a new step and remember the meta-command state at the time
 * that it was created.
 *
 ********************************************************************/

Step::Step(
    Where                 &_topOfStep,
    AbstractStepsElement  *_parent,
    int                    _num,            // step number as seen by the user
    Meta                  &_meta,           // the current state of the meta-commands
    bool                   _calledOut,      // if we're a callout
    bool                   _multiStep)
  : calledOut(_calledOut)
  , multiStep(_multiStep)
{
  top                       = _topOfStep;
  parent                    = _parent;
  submodelLevel             = _meta.submodelStack.size();
  stepNumber.number         = _num;             // record step number

  modelDisplayStep               = false;
  relativeType                   = StepType;
  csiPlacement.relativeType      = CsiType;
//  subModelPlacement.relativeType = SubModelType;

  stepNumber.relativeType   = StepNumberType;
  rotateIcon.relativeType   = RotateIconType;
  subModel.relativeType     = SubModelType;
  pageHeader.relativeType   = PageHeaderType;
  pageHeader.placement      = _meta.LPub.page.pageHeader.placement;
  pageHeader.size[XX]       = _meta.LPub.page.pageHeader.size.valuePixels(XX);
  pageHeader.size[YY]       = _meta.LPub.page.pageHeader.size.valuePixels(YY);
  pageFooter.relativeType   = PageFooterType;
  pageFooter.placement      = _meta.LPub.page.pageFooter.placement;
  pageFooter.size[XX]       = _meta.LPub.page.pageFooter.size.valuePixels(XX);
  pageFooter.size[YY]       = _meta.LPub.page.pageFooter.size.valuePixels(YY);
  csiItem                   = NULL;
//  subModel                = NULL;

  if (_calledOut) {
      csiPlacement.margin         = _meta.LPub.callout.csi.margin;
      csiPlacement.placement      = _meta.LPub.callout.csi.placement;
//      subModelPlacement.margin    = _meta.LPub.callout.subModel.margin;
//      subModelPlacement.placement = _meta.LPub.callout.subModel.placement;
      pli.margin                  = _meta.LPub.callout.pli.margin;
      pli.placement               = _meta.LPub.callout.pli.placement;
      oneToOne.margin             = _meta.LPub.callout.oneToOne.margin;
      oneToOne.placement          = _meta.LPub.callout.oneToOne.placement;
      rotateIcon.placement        = _meta.LPub.callout.rotateIcon.placement;
      rotateIcon.margin           = _meta.LPub.callout.rotateIcon.margin;
      rotateIcon.rotateIconMeta   = _meta.LPub.callout.rotateIcon;
      numberPlacemetMeta          = _meta.LPub.callout.stepNum;
      stepNumber.placement        = _meta.LPub.callout.stepNum.placement;
      stepNumber.font             = _meta.LPub.callout.stepNum.font.valueFoo();
      stepNumber.color            = _meta.LPub.callout.stepNum.color.value();
      stepNumber.margin           = _meta.LPub.callout.stepNum.margin;
      pliPerStep                  = _meta.LPub.callout.pli.perStep.value();
    } else if (_multiStep) {
      csiPlacement.margin         = _meta.LPub.multiStep.csi.margin;
      csiPlacement.placement      = _meta.LPub.multiStep.csi.placement;
//      subModelPlacement.margin    = _meta.LPub.multiStep.subModel.margin;
//      subModelPlacement.placement = _meta.LPub.multiStep.subModel.placement;
      pli.margin                  = _meta.LPub.multiStep.pli.margin;
      pli.placement               = _meta.LPub.multiStep.pli.placement;
      oneToOne.margin             = _meta.LPub.multiStep.oneToOne.margin;
      oneToOne.placement          = _meta.LPub.multiStep.oneToOne.placement;
      rotateIcon.placement        = _meta.LPub.multiStep.rotateIcon.placement;
      rotateIcon.margin           = _meta.LPub.multiStep.rotateIcon.margin;
      rotateIcon.rotateIconMeta   = _meta.LPub.multiStep.rotateIcon;
      numberPlacemetMeta          = _meta.LPub.multiStep.stepNum;
      stepNumber.placement        = _meta.LPub.multiStep.stepNum.placement;
      stepNumber.font             = _meta.LPub.multiStep.stepNum.font.valueFoo();
      stepNumber.color            = _meta.LPub.multiStep.stepNum.color.value();
      stepNumber.margin           = _meta.LPub.multiStep.stepNum.margin;
      pliPerStep                  = _meta.LPub.multiStep.pli.perStep.value();
    } else {
      csiPlacement.margin         = _meta.LPub.assem.margin;
      csiPlacement.placement      = _meta.LPub.assem.placement;
      placement                   = _meta.LPub.assem.placement;
//      subModelPlacement.margin    = _meta.LPub.subModel.margin;
//      subModelPlacement.placement = _meta.LPub.subModel.placement;
      pli.margin                  = _meta.LPub.pli.margin;
      pli.placement               = _meta.LPub.pli.placement;
      oneToOne.margin             = _meta.LPub.oneToOne.margin;
      oneToOne.placement          = _meta.LPub.oneToOne.placement;
      rotateIcon.placement        = _meta.LPub.rotateIcon.placement;
      rotateIcon.margin           = _meta.LPub.rotateIcon.margin;
      stepNumber.placement        = _meta.LPub.stepNumber.placement;
      stepNumber.font             = _meta.LPub.stepNumber.font.valueFoo();
      stepNumber.color            = _meta.LPub.stepNumber.color.value();
      stepNumber.margin           = _meta.LPub.stepNumber.margin;
      pliPerStep                  = false;
    }
  pli.steps                 = grandparent();
  pli.step                  = this;

  oneToOne.steps            = grandparent();
  oneToOne.step             = this;
  showOneToOne              = _meta.LPub.oneToOne.show.value();

  rotateIcon.setSize(         _meta.LPub.rotateIcon.size,
                              _meta.LPub.rotateIcon.border.valuePixels().thickness);
  placeRotateIcon           = false;

  subModel.setSize(           _meta.LPub.subModel.size,
                              _meta.LPub.subModel.border.valuePixels().thickness);
  displaySubModel           = _meta.LPub.subModel.display.value();

  showStepNumber            = _meta.LPub.assem.showStepNumber.value();

  showIllustrations         = _meta.LPub.illustration.show.value();

  showRightWrongs           = _meta.LPub.rightWrong.show.value();

  showStickers              = _meta.LPub.sticker.show.value();
}

/* step destructor destroys pointers */

Step::~Step() {
  for (int i = 0; i < calloutList.size(); i++) {
      Callout *callout = calloutList[i];
      delete callout;
    }
//  for (int i = 0; i < oneToOneList.size(); i++) {
//      OneToOne *oneToOne = oneToOneList[i];
//      oneToOne->clear();
//      delete oneToOne;
//    }
  for (int i = 0; i < illustrationList.size(); i++) {
      Illustration *illustration = illustrationList[i];
      delete illustration;
    }
  for (int i = 0; i < rightWrongList.size(); i++) {
      RightWrong *rightWrong = rightWrongList[i];
      delete rightWrong;
    }
  for (int i = 0; i < stickerList.size(); i++) {
      Sticker *sticker = stickerList[i];
      delete sticker;
    }

  pli.clear();
  oneToOne.clear();

  calloutList.clear();
  rightWrongList.clear();
  illustrationList.clear();
  stickerList.clear();

}

Step *Step::nextStep()
{
  const AbstractRangeElement *re = dynamic_cast<const AbstractRangeElement *>(this);
  return dynamic_cast<Step *>(nextElement(re));
}

Range *Step::range()
{
  Range *range = dynamic_cast<Range *>(parent);
  return range;
}

/*
 * given a set of parts, generate a CSI
 */

int Step::createCsi(QString     const &addLine,
    QStringList const &csiParts,  // the partially assembles model
    QPixmap           *pixmap,
    Meta              &meta,
    bool               isSubModel)
{
  int         sn         = stepNumber.number;
  qreal       modelScale = meta.LPub.assem.modelScale.value();
  QString     csi_Name   = modelDisplayStep ? csiName()+"_fm" : csiName();
  if (isSubModel) csi_Name+"_sm";

  bool        csiExist   = false;
  ldrName.clear();

  // 1 color x y z a b c d e f g h i foo.dat
  // 0 1     2 3 4 5 6 7 8 9 0 1 2 3 4
  QStringList tokens;
  split(addLine,tokens);
  QString orient;
  if (tokens.size() == 15) {
      for (int i = 5; i < 14; i++) {
          orient += "_" + tokens[i];
        }
    }

  QString csiFilePath = QString("%1/%2").arg(QDir::currentPath()).arg(Paths::tmpDir);

  QString key = QString("%1_%2_%3_%4_%5_%6")
      .arg(csi_Name+orient)
      .arg(sn)
      .arg(gui->pageSize(meta.LPub.page, 0))
      .arg(resolution())
      .arg(resolutionType() == DPI ? "DPI" : "DPCM")
      .arg(modelScale);

  // populate png name
  pngName = QDir::currentPath() + "/" +
      Paths::assemDir + "/" + key + ".png";

  csiOutOfDate = false;

  QFile csi(pngName);
  csiExist = csi.exists();
  if (csiExist) {
      QDateTime lastModified = QFileInfo(pngName).lastModified();
      QStringList stack = submodelStack();
      stack << parent->modelName();
      if ( ! isOlder(stack,lastModified)) {
          csiOutOfDate = true;
        }
    }

  // generate CSI file as appropriate

  if ( ! csiExist || csiOutOfDate ) {

      int rc;
      if (renderer->useLDViewSCall()) {

          // populate ldr file name
          ldrName = QString("%1/%2.ldr").arg(csiFilePath).arg(key);

          // Create the CSI ldr file and rotate its parts
          rc = renderer->rotateParts(addLine, meta.rotStep, csiParts, ldrName);
          if (rc != 0) {
              emit gui->statusMessage(false,QMessageBox::tr("Creation and rotation of CSI ldr file failed for:\n%1.")
                                      .arg(ldrName));
              return rc;
          }
      } else {

          QElapsedTimer timer;
          timer.start();

          // render the partially assembled model if single step and not called out
          rc = renderer->renderCsi(addLine,csiParts, pngName, meta);
          if (rc != 0) {
              emit gui->statusMessage(false,QMessageBox::tr("Render CSI part failed for:\n%1.")
                                      .arg(pngName));
              return rc;
          }

          logTrace() << "\n" << Render::getRenderer()
                     << "CSI render call took "
                     << timer.elapsed() << "milliseconds"
                     << "to render image for " << (calledOut ? "called out," : "simple,")
                     << (multiStep ? "step group" : "single step") << sn
                     << "on page " << gui->stepPageNum << ".";
      }
  }

  // If not using LDView SCall, populate pixmap
  if (! renderer->useLDViewSCall()) {
      pixmap->load(pngName);
      if (isSubModel) {
          subModel.size[0] = pixmap->width();
          subModel.size[1] = pixmap->height();
        } else {
          csiPlacement.size[0] = pixmap->width();
          csiPlacement.size[1] = pixmap->height();
        }
    }

  // Populate the 3D Viewer
  if (! gui->exporting()) {

      // Populate the viewerCsiName
      QString viewerName = QString("%1;%2;%3").arg(top.modelName).arg(top.lineNumber).arg(sn);
      viewerCsiName      = modelDisplayStep ? viewerName+"_fm" : viewerName;

      // Populate the csi file paths
      QString csiFullFilePath;
       if (renderer->useLDViewSCall()) {
           csiFullFilePath = QString("%1/%2.ldr").arg(csiFilePath).arg(key);
       } else {
           csiFullFilePath = QString("%1/csi.ldr").arg(csiFilePath);
       }

      // Populate the step content
      QStringList rotatedParts = csiParts;
      renderer->rotateParts(addLine,meta.rotStep,rotatedParts);
      QString rotsComment = QString("0 // ROTSTEP %1 %2 %3 %4")
                                    .arg(meta.rotStep.value().type)
                                    .arg(meta.rotStep.value().rots[0])
                                    .arg(meta.rotStep.value().rots[1])
                                    .arg(meta.rotStep.value().rots[2]);
      rotatedParts.prepend(rotsComment);
      gui->insertViewerStep(viewerCsiName,rotatedParts,csiFullFilePath,multiStep,calledOut);

      // set camera settings
      viewMatrix = renderer->cameraSettings(meta.LPub.assem);

//      if (! calledOut) {
          // set the step in viewer
          if (! gMainWindow->ViewStepContent(viewerCsiName,viewMatrix)) {
              emit gui->statusMessage(false,QMessageBox::tr("Load failed for viewer csi_Name: %1")
                                      .arg(viewerCsiName));
              return -1;
          }
//      }
  }

  return 0;
}


/*
 * LPub is able to pack steps together into multi-step pages or callouts.
 *
 * Multiple steps gathered on a page and callouts share a lot of
 * commonality.  They are organized into rows or columns of steps.
 *
 * From this springs two algorithms, the first algorithm is based on
 * similarity between steps, in that that across steps sub-components
 * within steps are placed in sub-columns or sub-rows. This format
 * is common these days in LEGO building instructions.  For lack of
 * a better name, I refer to this modern algorithm as tabular.
 *
 * The other algorithm, which is new to LPub 3, is that of a more
 * free format.
 *
 * These concepts and algorithms are described below.
 *   1. tabular format
 *      a) either Vertically broken down into sub-columns for
 *         csi, pli, stepNumber, rotateIcon and/or callouts.
 *      b) or Horizontally broken down into sub-rows for
 *         csi, pli, stepNumber, rotateIcon and/or callouts.
 *
 *   2. free form format
 *      a) either Vertically composed into columns of steps
 *      b) or rows of steps
 *
 *      This format does not force PLI's or step numbers
 *      to be organized across steps, but it does force steps themselves
 *      to be organized into columns or rows.
 *
 * The default is tabular format because that is the first algorithm
 * implemented.  This is also the most common algorithm used by LEGO
 * today (2007 AD).
 *
 * The free format is similar to the algorithms used early by LEGO
 * and provides the maximum area compression of building instructions,
 * even if they are possibly harder to follow.
 */

/*
 * the algorithms below implement tabular placement.
 *
 * size - allocate step sub-components into sub-rows or sub-columns.
 * place - determine the rectangle that is needed to totally contain
 *   the subcomponents (CSI, step number, PLI, roitateIcon, step-relative callouts.)
 *   Also place the CSI, step number, PLI, rotateIcon and step-relative callouts
 *   within the step's rectangle.
 *
 * making all this look nice takes a few passes:
 *   1.  determine the height and width of each step's sub-columns and
 *       sub-rows.
 *   2.  If we're creating a Vertically allocated multi-step or callout
 *       then make all the sub-columns line up.
 *
 *       If we're creating a Horizontally allocated multi-step or callout
 *       them make all the sub-rows line up.
 *
 * from here we've sized each of the steps.
 *
 * From here, we sum up the the height of each column or row, depending on
 * whether we're creating a Vertical or Horizontal multi-step/callout.  We
 * also keep track of the tallest (widest) column/row within the sets of rows,
 * and how wide (tall) the multi-step/callout is.
 *
 * Now we know the enclosing rectangle for the entire multi-step or callout.
 * Given this we can place the multi-step or callout conglomeration relative
 * to the thing they are to be placed next to.
 *
 * Multi-steps can only be placed relative to the page.
 *
 * Callouts can be place relative to CSI, PLI, step-number, rotateIcon, multi-step, or
 * page.
 */

/*
 * Size the set of ranges by sizing each range
 * and then placing them relative to each other
 */

/*
 * Think of the possible placement as a two dimensional table, of
 * places where something can be placed within a step's rectangle.
 *
  *  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
  *  CMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMC
  *  CMCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCMC
  *  CMCOOOOOOOOOOOOOOOOOOOOOOOOOOOOOCMC
  *  CMCOCCCCCCCCCCCCCCCCCCCCCCCCCCCOCMC
  *  CMCOCIIIIIIIIIIIIIIIIIIIIIIIIICOCMC
  *  CMCOCICCCCCCCCCCCCCCCCCCCCCCCICOCMC
  *  CMCOCICWWWWWWWWWWWWWWWWWWWWWCICOCMC
  *  CMCOCICWCCCCCCCCCCCCCCCCCCCWCICOCMC
  *  CMCOCICWCTTTTTTTTTTTTTTTTTCWCICOCMC
  *  CMCOCICWCTCCCCCCCCCCCCCCCTCWCICOCMC
  *  CMCOCICWCTCRRRRRRRRRRRRRCTCWCICOCMC
  *  CMCOCICWCTCRCCCCCCCCCCCRCTCWCICOCMC
  *  CMCOCICWCTCRCSSSSSSSSSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCCCCCCCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCPPPPPCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCPCCCPCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCPCACPCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCPCCCPCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCPPPPPCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSCCCCCCCSCRCTCWCICOCMC
  *  CMCOCICWCTCRCSSSSSSSSSCRCTCWCICOCMC
  *  CMCOCICWCTCRCCCCCCCCCCCRCTCWCICOCMC
  *  CMCOCICWCTCRRRRRRRRRRRRRCTCWCICOCMC
  *  CMCOCICWCTCCCCCCCCCCCCCCCTCWCICOCMC
  *  CMCOCICWCTTTTTTTTTTTTTTTTTCWCICOCMC
  *  CMCOCICWCCCCCCCCCCCCCCCCCCCWCICOCMC
  *  CMCOCICWWWWWWWWWWWWWWWWWWWWWCICOCMC
  *  CMCOCICCCCCCCCCCCCCCCCCCCCCCCICOCMC
  *  CMCOCIIIIIIIIIIIIIIIIIIIIIIIIICOCMC
  *  CMCOCCCCCCCCCCCCCCCCCCCCCCCCCCCOCMC
  *  CMCOOOOOOOOOOOOOOOOOOOOOOOOOOOOOCMC
  *  CMCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCMC
  *  CMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMC
  *  CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC

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
 *  C5  - callout relative to rotateIcon     C0
 *  R0  - rotateIcon relateive to csi        R0
 *  C6  - callout relative to step number    C1
 *  S0  - step number relative to csi        S0
 *  C7  - callout relative to PLI            C2
 *  P0  - pli relative to csi                P0
 *  C8  - callout relative to csi            C3
 *  A   - csi                                A
 *  C9  - callout relative to csi            C4
 *  P1  - pli relative to csi                P1
 *  C10 - callout relative to PLI            C5
 *  S1  - step number relative to csi        S1
 *  C11 - callout relative to step number    C6
 *  R1  - rotateIcon relateive to csi        R1
 *  C12 - callout relative to rotateIcon     C7
 *  T1  - Sticker relative to csi
 *  C13 - callout relative to sticker
 *  W1  - RightWrong relative to Csi
 *  C14 - callout relative to rightwrong
 *  I1  - Illustration relative to Csi
 *  C15 - callout relative to illustration
 *  O1  - OneToOne relative to PLI
 *  C16 - callout relative to oneone
 *  M1  - SubModel relative to PLI
 *  C17 - callout relative to submodel
 *
 *  pliPlace[NumPlacements][2]
 *  stepNumberPlace[NumPlacements][2]
 *  rotateIconPlace[NumPlacements][2]
 *  stickerPlace[NumPlacements][2]
 *  wrightWrongPlace[NumPlacements][2]
 *  illustrationPlace[NumPlacements][2]
 *  subModelPlace[NumPlacements][2]
 *  oneToOnePlace[NumPlacements][2]
 *
 */


/*
 * this tells us where to place the pli when placing
 * relative to csi
 */
const int pliPlace[NumPlacements][2] =
{
  { TblPli0, TblPli0 }, // Top_Left
  { TblCsi,  TblPli0 }, // Top
  { TblPli1, TblPli0 }, // Top_Right
  { TblPli1, TblCsi  }, // Right
  { TblPli1, TblPli1 }, // BOTTOM_RIGHT
  { TblCsi,  TblPli1 }, // BOTTOM
  { TblPli0, TblPli1 }, // BOTTOM_LEFT
  { TblPli0, TblCsi  }, // LEFT
  { TblCsi,  TblCsi },
};

/*
 * this tells us where to place the stepNumber when placing
 * relative to csi
 */
const int stepNumberPlace[NumPlacements][2] =
{
  { TblSn0, TblSn0 },  // Top_Left
  { TblCsi, TblSn0 },  // Top
  { TblSn1, TblSn0 },  // Top_Right
  { TblSn1, TblCsi },  // Right
  { TblSn1, TblSn1 },  // BOTTOM_RIGHT
  { TblCsi, TblSn1 },  // BOTTOM
  { TblSn0, TblSn1 },  // BOTTOM_LEFT
  { TblSn0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place a rotateIcon when placing
 * relative to csi
 */
const int rotateIconPlace[NumPlacements][2] =
{
  { TblRi0, TblRi0 },  // Top_Left
  { TblCsi, TblRi0 },  // Top
  { TblRi1, TblRi0 },  // Top_Right
  { TblRi1, TblCsi },  // Right
  { TblRi1, TblRi1 },  // BOTTOM_RIGHT
  { TblCsi, TblRi1 },  // BOTTOM
  { TblRi0, TblRi1 },  // BOTTOM_LEFT
  { TblRi0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place a sticker when placing
 * relative to csi
 */
const int stPlace[NumPlacements][2] =
{
  { TblSt0, TblSt0 },  // Top_Left
  { TblCsi, TblSt0 },  // Top
  { TblSt1, TblSt0 },  // Top_Right
  { TblSt1, TblCsi },  // Right
  { TblSt1, TblSt1 },  // BOTTOM_RIGHT
  { TblCsi, TblSt1 },  // BOTTOM
  { TblSt0, TblSt1 },  // BOTTOM_LEFT
  { TblSt0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place a wrightWrong when placing
 * relative to csi
 */
const int rwPlace[NumPlacements][2] =
{
  { TblRw0, TblRw0 },  // Top_Left
  { TblCsi, TblRw0 },  // Top
  { TblRw1, TblRw0 },  // Top_Right
  { TblRw1, TblCsi },  // Right
  { TblRw1, TblRw1 },  // BOTTOM_RIGHT
  { TblCsi, TblRw1 },  // BOTTOM
  { TblRw0, TblRw1 },  // BOTTOM_LEFT
  { TblRw0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place an illustration when placing
 * relative to csi
 */
const int ilPlace[NumPlacements][2] =
{
  { TblIl0, TblIl0 },  // Top_Left
  { TblCsi, TblIl0 },  // Top
  { TblIl1, TblIl0 },  // Top_Right
  { TblIl1, TblCsi },  // Right
  { TblIl1, TblIl1 },  // BOTTOM_RIGHT
  { TblCsi, TblIl1 },  // BOTTOM
  { TblIl0, TblIl1 },  // BOTTOM_LEFT
  { TblIl0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place a subModel when placing
 * relative to csi
 */
const int subModelPlace[NumPlacements][2] =
{
  { TblSm0, TblSm0 },  // Top_Left
  { TblCsi, TblSm0 },  // Top
  { TblSm1, TblSm0 },  // Top_Right
  { TblSm1, TblCsi },  // Right
  { TblSm1, TblSm1 },  // BOTTOM_RIGHT
  { TblCsi, TblSm1 },  // BOTTOM
  { TblSm0, TblSm1 },  // BOTTOM_LEFT
  { TblSm0, TblCsi },  // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us where to place a oneToOne when placing
 * relative to csi
 */
const int oneToOnePlace[NumPlacements][2] =
{
  { TblOto0, TblOto0 },  // Top_Left
  { TblCsi,  TblOto0 },  // Top
  { TblOto1, TblOto0 },  // Top_Right
  { TblOto1, TblCsi  },  // Right
  { TblOto1, TblOto1 },  // BOTTOM_RIGHT
  { TblCsi,  TblOto1 },  // BOTTOM
  { TblOto0, TblOto1 },  // BOTTOM_LEFT
  { TblOto0, TblCsi  },  // LEFT
  { TblCsi,  TblCsi  },
};

/*
 * this tells us where to place a callout when placing
 * relative to csi
 */

const int coPlace[NumPlacements][2] =
{
  { TblCo3, TblCo3 }, // Top_Left
  { TblCsi, TblCo3 }, // Top
  { TblCo4, TblCo3 }, // Top_Right
  { TblCo4, TblCsi }, // Right
  { TblCo4, TblCo4 }, // BOTTOM_RIGHT
  { TblCsi, TblCo4 }, // BOTTOM
  { TblCo3, TblCo4 }, // BOTTOM_LEFT
  { TblCo3, TblCsi }, // LEFT
  { TblCsi, TblCsi },
};

/*
 * this tells us the row/col offset when placing
 * relative to something other than csi
 */

const int relativePlace[NumPlacements][2] =
{
  { -1, -1 },
  {  0, -1 },
  {  1, -1 },
  {  1,  0 },
  {  1,  1 },
  {  0,  1 },
  { -1,  1 },
  { -1,  0 },
  {  0,  0 },
};

void Step::maxMargin(
    MarginsMeta &margin,
    int tbl[2],
int marginRows[][2],
int marginCols[][2])
{
  if (margin.valuePixels(XX) > marginCols[tbl[XX]][0]) {
      marginCols[tbl[XX]][0] = margin.valuePixels(XX);
    }
  if (margin.valuePixels(XX) > marginCols[tbl[XX]][1]) {
      marginCols[tbl[XX]][1] = margin.valuePixels(XX);
    }
  if (margin.valuePixels(YY) > marginRows[tbl[YY]][0]) {
      marginRows[tbl[YY]][0] = margin.valuePixels(YY);
    }
  if (margin.valuePixels(YY) > marginRows[tbl[YY]][1]) {
      marginRows[tbl[YY]][1] = margin.valuePixels(YY);
    }
}

/*
 * This is the first pass of sizing a step.
 *
 *   locate the proper row/col in the placement table (see above)
 *   for each component (csi, pli, stepNumber, roateIcon, callout) in the step
 *
 *     locate the proper row/col for those relative to CSI (absolute)
 *
 *     locate the proper row/col for those relative to (pli,stepNumber)
 *
 *   determine the largest dimensions for each row/col in the table
 *
 *   record the height of this step
 *
 *   determine the pixel offset for each row/col in the table
 *
 *   place the components Vertically in pixel units using row
 */

int Step::sizeit(
    int  rows[],         // accumulate sub-row heights here
    int  cols[],         // accumulate sub-col widths here
    int  marginRows[][2],// accumulate sub-row margin heights here
int  marginCols[][2],
int  x,
int  y)// accumulate sub-col margin widths here
{

  // size up each callout

  int numCallouts = calloutList.size();

  for (int i = 0; i < numCallouts; i++) {
      calloutList[i]->sizeIt();
    }

  // size up each illustration

  int numIllustrations = illustrationList.size();

  for (int i = 0; i < numIllustrations; i++){
      illustrationList[i]->sizeIt();
    }

  // size up each rightWrong

  int numRightWrongs = rightWrongList.size();

  for (int i = 0; i < numRightWrongs; i++){
      rightWrongList[i]->sizeIt();
    }

  // size up each sticker

  int numStickers = stickerList.size();

  for (int i = 0; i < numStickers; i++){
      stickerList[i]->sizeIt();
    }

  // size up the step number

  if (showStepNumber && ! onlyChild()) {
      stepNumber.sizeit();
    }

  // size up the sub model

  if (placeRotateIcon){
      rotateIcon.sizeit();
    }

  // size up the rotate icon

  if (displaySubModel){
      subModel.sizeit();
    }

  //   size up the oneToOne (Does not apply to oneToOne)

//  if (showOneToOne){
//      oneToOne.sizeit();
//    }

  /****************************************************/
  /* figure out who is placed in which row and column */
  /****************************************************/

  csiPlacement.tbl[XX] = TblCsi;
  csiPlacement.tbl[YY] = TblCsi;

  /* Lets start with the absolutes (those relative to the CSI) */

  // PLI relative to CSI

  PlacementData pliPlacement = pli.placement.value();

  if (pliPlacement.relativeTo == StepNumberType && (! showStepNumber && onlyChild())){
      pliPlacement.relativeTo = CsiType; // could set to pageType
    } else if (pliPlacement.relativeTo == SubModelType && ! displaySubModel){
      pliPlacement.relativeTo = CsiType;
    }

  if (pliPlacement.relativeTo == CsiType) {
      if (pliPlacement.preposition == Outside) {
          pli.tbl[XX] = pliPlace[pliPlacement.placement][XX];
          pli.tbl[YY] = pliPlace[pliPlacement.placement][YY];
        } else {
          pli.tbl[XX] = TblCsi;
          pli.tbl[YY] = TblCsi;
        }
    }

  // SubModel relative to CSI

  PlacementData subModelPlacement = subModel.placement.value();

  if (subModelPlacement.relativeTo == PartsListType && ! pliPerStep) {
      subModelPlacement.relativeTo = StepNumberType;
    } else if (subModelPlacement.relativeTo == StepNumberType && (! showStepNumber && onlyChild())){
      subModelPlacement.relativeTo = CsiType;
    } else if (subModelPlacement.relativeTo == RotateIconType && ! placeRotateIcon) {
      subModelPlacement.relativeTo = CsiType;
    }

  if (displaySubModel){
      if (subModelPlacement.relativeTo == CsiType){
          if (subModelPlacement.preposition == Outside) {
              subModel.tbl[XX] = subModelPlace[subModelPlacement.placement][XX];
              subModel.tbl[YY] = subModelPlace[subModelPlacement.placement][YY];
            } else {
              subModel.tbl[XX] = TblCsi;
              subModel.tbl[YY] = TblCsi;
            }
        }
    }

  // OneToOne relative to CSI

  PlacementData oneToOnePlacement = oneToOne.placement.value();

  if (oneToOnePlacement.relativeTo == PartsListType && ! pliPerStep) {
      oneToOnePlacement.relativeTo = StepNumberType;
    } else if (oneToOnePlacement.relativeTo == StepNumberType && (! showStepNumber && onlyChild())){
      oneToOnePlacement.relativeTo = CsiType;
    } else if (oneToOnePlacement.relativeTo == RotateIconType && ! placeRotateIcon) {
      oneToOnePlacement.relativeTo = CsiType;
    }

  if (showOneToOne){
      if (oneToOnePlacement.relativeTo == CsiType){
          if (oneToOnePlacement.preposition == Outside) {
              oneToOne.tbl[XX] = oneToOnePlace[oneToOnePlacement.placement][XX];
              oneToOne.tbl[YY] = oneToOnePlace[oneToOnePlacement.placement][YY];
            } else {
              oneToOne.tbl[XX] = TblCsi;
              oneToOne.tbl[YY] = TblCsi;
            }
        }
    }

  // Rotate Icon relative to CSI

  PlacementData rotateIconPlacement = rotateIcon.placement.value();

  // if Rotate Icon relative to step number, but no step number;
  //    Step Number is relative to CSI (Assem)

  if (placeRotateIcon){
      if (rotateIconPlacement.relativeTo == StepNumberType && (! showStepNumber && onlyChild())){
            rotateIconPlacement.relativeTo = CsiType;
          }

      if (rotateIconPlacement.relativeTo == CsiType){
          if (rotateIconPlacement.preposition == Outside) {
              rotateIcon.tbl[XX] = rotateIconPlace[rotateIconPlacement.placement][XX];
              rotateIcon.tbl[YY] = rotateIconPlace[rotateIconPlacement.placement][YY];
            } else {
              rotateIcon.tbl[XX] = TblCsi;
              rotateIcon.tbl[YY] = TblCsi;
            }
        }
    }

  // Step Number relative to CSI

  PlacementData stepNumberPlacement = stepNumber.placement.value();
  
  // if Step Number relative to parts list, but no parts list or
  // if Step Number relative to sub model, but no sub model or
  // if Step Number relative to rotate icon, but no rotate icon;
  //    Step Number is relative to CSI (Assem)

  if (stepNumberPlacement.relativeTo == PartsListType && ! pliPerStep) {
      stepNumberPlacement.relativeTo = CsiType;
    } else if (stepNumberPlacement.relativeTo == SubModelType && ! displaySubModel){
      stepNumberPlacement.relativeTo = CsiType;
    } else if (stepNumberPlacement.relativeTo == RotateIconType && ! placeRotateIcon) {
      stepNumberPlacement.relativeTo = CsiType;
    }

  if (stepNumberPlacement.relativeTo == CsiType) {
      if (stepNumberPlacement.preposition == Outside) {
          stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
          stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
        } else {
          stepNumber.tbl[XX] = TblCsi;
          stepNumber.tbl[YY] = TblCsi;
        }
    }

  /* Now lets place things relative to others row/columns */

  /* first the known entities */

  /* PLI */

  if (pliPlacement.relativeTo == StepNumberType) {
      if (pliPerStep && pli.tsize()) {
          pli.tbl[XX] = stepNumber.tbl[XX]+relativePlace[pliPlacement.placement][XX];
          pli.tbl[YY] = stepNumber.tbl[YY]+relativePlace[pliPlacement.placement][YY];
        } else {
          stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
          stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
        }
    }

  if (pliPlacement.relativeTo == RotateIconType) {
      if (pliPerStep && pli.tsize()) {
          pli.tbl[XX] = rotateIcon.tbl[XX]+relativePlace[pliPlacement.placement][XX];
          pli.tbl[YY] = rotateIcon.tbl[YY]+relativePlace[pliPlacement.placement][YY];
        } else {
          rotateIcon.tbl[XX] = rotateIconPlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = rotateIconPlace[rotateIconPlacement.placement][YY];
        }
    }

  if (pliPlacement.relativeTo == SubModelType) {
      if (pliPerStep && pli.tsize()) {
          pli.tbl[XX] = subModel.tbl[XX]+relativePlace[pliPlacement.placement][XX];
          pli.tbl[YY] = subModel.tbl[YY]+relativePlace[pliPlacement.placement][YY];
        } else {
          subModel.tbl[XX] = subModelPlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = subModelPlace[subModelPlacement.placement][YY];
        }
    }

  if (pliPlacement.relativeTo == OneToOneType) {
      if (pliPerStep && pli.tsize()) {
          pli.tbl[XX] = oneToOne.tbl[XX]+relativePlace[pliPlacement.placement][XX];
          pli.tbl[YY] = oneToOne.tbl[YY]+relativePlace[pliPlacement.placement][YY];
        } else {
          oneToOne.tbl[XX] = oneToOnePlace[oneToOnePlacement.placement][XX];
          oneToOne.tbl[YY] = oneToOnePlace[oneToOnePlacement.placement][YY];
        }
    }

  /* Step Number */

  if (stepNumberPlacement.relativeTo == PartsListType) {
      stepNumber.tbl[XX] = pli.tbl[XX]+relativePlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = pli.tbl[YY]+relativePlace[stepNumberPlacement.placement][YY];
    }

  if (stepNumberPlacement.relativeTo == SubModelType) {
      stepNumber.tbl[XX] = subModel.tbl[XX]+relativePlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = subModel.tbl[YY]+relativePlace[stepNumberPlacement.placement][YY];
    }

  if (stepNumberPlacement.relativeTo == RotateIconType) {
      stepNumber.tbl[XX] = rotateIcon.tbl[XX]+relativePlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = rotateIcon.tbl[YY]+relativePlace[stepNumberPlacement.placement][YY];
    }

  if (stepNumberPlacement.relativeTo == OneToOneType) {
      stepNumber.tbl[XX] = oneToOne.tbl[XX]+relativePlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = oneToOne.tbl[YY]+relativePlace[stepNumberPlacement.placement][YY];
    }

  /* Sub Model */

  if (subModelPlacement.relativeTo == PartsListType) {
      if (placeRotateIcon) {
          subModel.tbl[XX] = pli.tbl[XX]+relativePlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = pli.tbl[YY]+relativePlace[subModelPlacement.placement][YY];
        } else {
          pli.tbl[XX] = pliPlace[pliPlacement.placement][XX];
          pli.tbl[YY] = pliPlace[pliPlacement.placement][YY];
        }
    }

  if (subModelPlacement.relativeTo == StepNumberType) {
      if (placeRotateIcon) {
          subModel.tbl[XX] = stepNumber.tbl[XX]+relativePlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = stepNumber.tbl[YY]+relativePlace[subModelPlacement.placement][YY];
        } else {
          stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
          stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
        }
    }

  if (subModelPlacement.relativeTo == RotateIconType) {
      if (placeRotateIcon) {
          subModel.tbl[XX] = rotateIcon.tbl[XX]+relativePlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = rotateIcon.tbl[YY]+relativePlace[subModelPlacement.placement][YY];
        } else {
          rotateIcon.tbl[XX] = rotateIconPlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = rotateIconPlace[rotateIconPlacement.placement][YY];
        }
    }

  if (subModelPlacement.relativeTo == OneToOneType) {
      if (placeRotateIcon) {
          subModel.tbl[XX] = oneToOne.tbl[XX]+relativePlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = oneToOne.tbl[YY]+relativePlace[subModelPlacement.placement][YY];
        } else {
          oneToOne.tbl[XX] = oneToOnePlace[oneToOnePlacement.placement][XX];
          oneToOne.tbl[YY] = oneToOnePlace[oneToOnePlacement.placement][YY];
        }
    }

  /* Rotate Icon */

  if (rotateIconPlacement.relativeTo == PartsListType) {
      if (placeRotateIcon) {
          rotateIcon.tbl[XX] = pli.tbl[XX]+relativePlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = pli.tbl[YY]+relativePlace[rotateIconPlacement.placement][YY];
        } else {
          pli.tbl[XX] = pliPlace[pliPlacement.placement][XX];
          pli.tbl[YY] = pliPlace[pliPlacement.placement][YY];
        }
    }

  if (rotateIconPlacement.relativeTo == StepNumberType) {
      if (placeRotateIcon) {
          rotateIcon.tbl[XX] = stepNumber.tbl[XX]+relativePlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = stepNumber.tbl[YY]+relativePlace[rotateIconPlacement.placement][YY];
        } else {
          stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
          stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
        }
    }

  if (rotateIconPlacement.relativeTo == SubModelType) {
      if (placeRotateIcon) {
          rotateIcon.tbl[XX] = subModel.tbl[XX]+relativePlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = subModel.tbl[YY]+relativePlace[rotateIconPlacement.placement][YY];
        } else {
          subModel.tbl[XX] = subModelPlace[subModelPlacement.placement][XX];
          subModel.tbl[YY] = subModelPlace[subModelPlacement.placement][YY];
        }
    }

  if (rotateIconPlacement.relativeTo == OneToOneType) {
      if (placeRotateIcon) {
          rotateIcon.tbl[XX] = oneToOne.tbl[XX]+relativePlace[rotateIconPlacement.placement][XX];
          rotateIcon.tbl[YY] = oneToOne.tbl[YY]+relativePlace[rotateIconPlacement.placement][YY];
        } else {
          oneToOne.tbl[XX] = oneToOnePlace[oneToOnePlacement.placement][XX];
          oneToOne.tbl[YY] = oneToOnePlace[oneToOnePlacement.placement][YY];
        }
    }

  maxMargin(csiPlacement.margin,csiPlacement.tbl,marginRows,marginCols);
  maxMargin(         pli.margin,         pli.tbl,marginRows,marginCols);
  maxMargin(  stepNumber.margin,  stepNumber.tbl,marginRows,marginCols);
  maxMargin(    subModel.margin,    subModel.tbl,marginRows,marginCols);
  maxMargin(  rotateIcon.margin,  rotateIcon.tbl,marginRows,marginCols);
  maxMargin(    oneToOne.margin,    oneToOne.tbl,marginRows,marginCols);

  /* Now place Callouts[, Illustration, RightWrong, and Sticker]
   * relative to the known (CSI, PLI, SN, SM, RI, OTO) */
  
  int square[NumPlaces][NumPlaces];
  
  for (int i = 0; i < NumPlaces; i++) {
      for (int j = 0; j < NumPlaces; j++) {
          square[i][j] = -1;
        }
    }
  
  square[            TblCsi][            TblCsi] = CsiType;
  square[       pli.tbl[XX]][       pli.tbl[YY]] = PartsListType;
  square[stepNumber.tbl[XX]][stepNumber.tbl[YY]] = StepNumberType;
  square[  subModel.tbl[XX]][  subModel.tbl[YY]] = SubModelType;
  square[rotateIcon.tbl[XX]][rotateIcon.tbl[YY]] = RotateIconType;
  square[  oneToOne.tbl[XX]][  oneToOne.tbl[YY]] = OneToOneType;
  
  int pixmapSize[2] = { csiPixmap.width(), csiPixmap.height() };
  int max = pixmapSize[y];

  /* Callouts */

  int calloutSize[2]      = { 0, 0 };
  bool calloutShared      = false;

  for (int i = 0; i < numCallouts; i++) {
      Callout *callout = calloutList[i];

      PlacementData calloutPlacement = callout->placement.value();
      bool calloutSharable = true;
      bool onSide          = false;

      if (calloutPlacement.relativeTo == CsiType) {
          onSide = x == XX ? (calloutPlacement.placement == Left ||
                              calloutPlacement.placement == Right)
                           : (calloutPlacement.placement == Top ||
                              calloutPlacement.placement == Bottom);
        }

      if (onSide) {
          if (max < callout->size[y]) {
              max = callout->size[y];
            }
        }

      int rp = calloutPlacement.placement;
      switch (calloutPlacement.relativeTo) {
        case CsiType:
          callout->tbl[XX] = coPlace[rp][XX];
          callout->tbl[YY] = coPlace[rp][YY];
          break;
        case PartsListType:
          callout->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
          callout->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
          break;
        case StepNumberType:
          callout->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
          callout->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
          break;
        case SubModelType:
          callout->tbl[XX] = subModel.tbl[XX] + relativePlace[rp][XX];
          callout->tbl[YY] = subModel.tbl[YY] + relativePlace[rp][YY];
          break;
        case RotateIconType:
          callout->tbl[XX] = rotateIcon.tbl[XX] + relativePlace[rp][XX];
          callout->tbl[YY] = rotateIcon.tbl[YY] + relativePlace[rp][YY];
          break;
        case OneToOneType:
          callout->tbl[XX] = oneToOne.tbl[XX] + relativePlace[rp][XX];
          callout->tbl[YY] = oneToOne.tbl[YY] + relativePlace[rp][YY];
          break;
        default:
          calloutSharable = false;
          break;
        }

      if ( ! pliPerStep) {
          calloutSharable = false;
        }

      square[callout->tbl[XX]][callout->tbl[YY]] = i + 1;
      int size = callout->submodelStack().size();
      if (calloutSharable && size > 1) {
          if (callout->tbl[x] < TblCsi && callout->tbl[y] == TblCsi) {
              if (calloutSize[x] < callout->size[x]) {
                  calloutSize[XX] = callout->size[XX];
                  calloutSize[YY] = callout->size[YY];
                }
              callout->shared = true;
              calloutShared = true;
            }
        }
    }

  /* Illustrations */

  int illustrationSize[2] = { 0, 0 };
  bool illustrationShared = false;

  for (int i = 0; i < numIllustrations; i++) {
    Illustration *illustration = illustrationList[i];

    PlacementData illustrationPlacement = illustration->placement.value();
    bool illustrationSharable = true;
    bool onSide          = false;

    if (illustrationPlacement.relativeTo == CsiType) {
        onSide = x == XX ? (illustrationPlacement.placement == Left ||
                            illustrationPlacement.placement == Right)
                         : (illustrationPlacement.placement == Top ||
                            illustrationPlacement.placement == Bottom);
      }

    if (onSide) {
        if (max < illustration->size[y]) {
            max = illustration->size[y];
          }
      }

    int rp = illustrationPlacement.placement;
    switch (illustrationPlacement.relativeTo) {
      case CsiType:
        illustration->tbl[XX] = ilPlace[rp][XX];
        illustration->tbl[YY] = ilPlace[rp][YY];
        break;
      case PartsListType:
        illustration->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
        illustration->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
        break;
      case StepNumberType:
        illustration->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
        illustration->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
        break;
      case SubModelType:
        illustration->tbl[XX] = subModel.tbl[XX] + relativePlace[rp][XX];
        illustration->tbl[YY] = subModel.tbl[YY] + relativePlace[rp][YY];
        break;
      case RotateIconType:
        illustration->tbl[XX] = rotateIcon.tbl[XX] + relativePlace[rp][XX];
        illustration->tbl[YY] = rotateIcon.tbl[YY] + relativePlace[rp][YY];
        break;
      case OneToOneType:
        illustration->tbl[XX] = oneToOne.tbl[XX] + relativePlace[rp][XX];
        illustration->tbl[YY] = oneToOne.tbl[YY] + relativePlace[rp][YY];
        break;
      default:
        illustrationSharable = false;
        break;
      }

    if ( ! pliPerStep) {
        illustrationSharable = false;
      }

    square[illustration->tbl[XX]][illustration->tbl[YY]] = i + 1;
    int size = illustration->submodelStack().size();
    if (illustrationSharable && size > 1) {
        if (illustration->tbl[x] < TblCsi && illustration->tbl[y] == TblCsi) {
            if (illustrationSize[x] < illustration->size[x]) {
                illustrationSize[XX] = illustration->size[XX];
                illustrationSize[YY] = illustration->size[YY];
              }
            illustration->shared = true;
            illustrationShared = true;
          }
      }
  }

  /* RightWrongs */

  int rightWrongSize[2]   = { 0, 0 };
  bool rightWrongShared   = false;

  for (int i = 0; i < numRightWrongs; i++) {
    RightWrong *rightWrong = rightWrongList[i];

    PlacementData rightWrongPlacement = rightWrong->placement.value();
    bool rightWrongSharable = true;
    bool onSide          = false;

    if (rightWrongPlacement.relativeTo == CsiType) {
        onSide = x == XX ? (rightWrongPlacement.placement == Left ||
                            rightWrongPlacement.placement == Right)
                         : (rightWrongPlacement.placement == Top ||
                            rightWrongPlacement.placement == Bottom);
      }

    if (onSide) {
        if (max < rightWrong->size[y]) {
            max = rightWrong->size[y];
          }
      }

    int rp = rightWrongPlacement.placement;
    switch (rightWrongPlacement.relativeTo) {
      case CsiType:
        rightWrong->tbl[XX] = rwPlace[rp][XX];
        rightWrong->tbl[YY] = rwPlace[rp][YY];
        break;
      case PartsListType:
        rightWrong->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
        rightWrong->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
        break;
      case StepNumberType:
        rightWrong->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
        rightWrong->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
        break;
      case SubModelType:
        rightWrong->tbl[XX] = subModel.tbl[XX] + relativePlace[rp][XX];
        rightWrong->tbl[YY] = subModel.tbl[YY] + relativePlace[rp][YY];
        break;
      case RotateIconType:
        rightWrong->tbl[XX] = rotateIcon.tbl[XX] + relativePlace[rp][XX];
        rightWrong->tbl[YY] = rotateIcon.tbl[YY] + relativePlace[rp][YY];
        break;
      case OneToOneType:
        rightWrong->tbl[XX] = oneToOne.tbl[XX] + relativePlace[rp][XX];
        rightWrong->tbl[YY] = oneToOne.tbl[YY] + relativePlace[rp][YY];
        break;
      default:
        rightWrongSharable = false;
        break;
      }

    if ( ! pliPerStep) {
        rightWrongSharable = false;
      }

    square[rightWrong->tbl[XX]][rightWrong->tbl[YY]] = i + 1;
    int size = rightWrong->submodelStack().size();
    if (rightWrongSharable && size > 1) {
        if (rightWrong->tbl[x] < TblCsi && rightWrong->tbl[y] == TblCsi) {
            if (rightWrongSize[x] < rightWrong->size[x]) {
                rightWrongSize[XX] = rightWrong->size[XX];
                rightWrongSize[YY] = rightWrong->size[YY];
              }
            rightWrong->shared = true;
            rightWrongShared = true;
          }
      }
  }

  /* Stickers */

  int stickerSize[2]      = { 0, 0 };
  bool stickerShared      = false;

  for (int i = 0; i < numStickers; i++) {
    Sticker *sticker = stickerList[i];

    PlacementData stickerPlacement = sticker->placement.value();
    bool stickerSharable = true;
    bool onSide          = false;

    if (stickerPlacement.relativeTo == CsiType) {
        onSide = x == XX ? (stickerPlacement.placement == Left ||
                            stickerPlacement.placement == Right)
                         : (stickerPlacement.placement == Top ||
                            stickerPlacement.placement == Bottom);
      }

    if (onSide) {
        if (max < sticker->size[y]) {
            max = sticker->size[y];
          }
      }

    int rp = stickerPlacement.placement;
    switch (stickerPlacement.relativeTo) {
      case CsiType:
        sticker->tbl[XX] = stPlace[rp][XX];
        sticker->tbl[YY] = stPlace[rp][YY];
        break;
      case PartsListType:
        sticker->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
        sticker->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
        break;
      case StepNumberType:
        sticker->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
        sticker->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
        break;
      case SubModelType:
        sticker->tbl[XX] = subModel.tbl[XX] + relativePlace[rp][XX];
        sticker->tbl[YY] = subModel.tbl[YY] + relativePlace[rp][YY];
        break;
      case RotateIconType:
        sticker->tbl[XX] = rotateIcon.tbl[XX] + relativePlace[rp][XX];
        sticker->tbl[YY] = rotateIcon.tbl[YY] + relativePlace[rp][YY];
        break;
      case OneToOneType:
        sticker->tbl[XX] = oneToOne.tbl[XX] + relativePlace[rp][XX];
        sticker->tbl[YY] = oneToOne.tbl[YY] + relativePlace[rp][YY];
        break;
      default:
        stickerSharable = false;
        break;
      }

//    if ( ! pliPerStep) {
//        stickerSharable = false;
//      }

    square[sticker->tbl[XX]][sticker->tbl[YY]] = i + 1;
    int size = sticker->submodelStack().size();
    if (stickerSharable && size > 1) {
        if (sticker->tbl[x] < TblCsi && sticker->tbl[y] == TblCsi) {
            if (stickerSize[x] < sticker->size[x]) {
                stickerSize[XX] = sticker->size[XX];
                stickerSize[YY] = sticker->size[YY];
              }
            sticker->shared = true;
            stickerShared = true;
          }
      }
  }

  /************************************************/
  /*                                              */
  /* Determine the biggest in each column and row */
  /*                                              */
  /************************************************/
  
  /* Constrain PLI */

  if (pli.pliMeta.constrain.isDefault()) {

      int tsize = 0;

      switch (pliPlacement.placement) {
        case Top:
        case Bottom:
          tsize = csiPlacement.size[XX];
          pli.sizePli(ConstrainData::ConstrainWidth,tsize);
          if (pli.size[YY] > gui->pageSize(gui->page.meta.LPub.page, YY)/3) {
              pli.sizePli(ConstrainData::ConstrainArea,tsize);
            }
          break;
        case Left:
        case Right:
          tsize = csiPlacement.size[YY];
          pli.sizePli(ConstrainData::ConstrainHeight,tsize);
          if (pli.size[XX] > gui->pageSize(gui->page.meta.LPub.page, XX)/3) {
              pli.sizePli(ConstrainData::ConstrainArea,tsize);
            }
          break;
        default:
          pli.sizePli(ConstrainData::ConstrainArea,tsize);
          break;
        }
    }

  /* Constrain OneToOne - HOLD FOR THE MOMENT...

  // Allow ONETOONE and CALLOUT to share one column
  // Allow ONETOONE and ILLUSTRATION to share one column
  // Allow ONETOONE and RIGHTWRONG to share one column
  // Allow ONETOONE and STICKER to share one column

  if (oneToOne.oneToOneMeta.constrain.isDefault()) {

      int tsize = 0;

      switch (oneToOnePlacement.placement) {
        case Top:
        case Bottom:
          tsize = csiPlacement.size[XX];
          oneToOne.sizeOneToOne(ConstrainData::ConstrainWidth,tsize);
          if (oneToOne.size[YY] > gui->pageSize(gui->page.meta.LPub.page, YY)/3) {
              oneToOne.sizeOneToOne(ConstrainData::ConstrainArea,tsize);
            }
          break;
        case Left:
        case Right:
          tsize = csiPlacement.size[YY];
          oneToOne.sizeOneToOne(ConstrainData::ConstrainHeight,tsize);
          if (oneToOne.size[XX] > gui->pageSize(gui->page.meta.LPub.page, XX)/3) {
              oneToOne.sizeOneToOne(ConstrainData::ConstrainArea,tsize);
            }
          break;
        default:
          oneToOne.sizeOneToOne(ConstrainData::ConstrainArea,tsize);
          break;
        }
    }
  
 */

  /* Allow PLI and CALLOUT to share one column - expand cols and/or rows */
  
  // Allow PLI and CALLOUT to share one column
  // Allow PLI and ILLUSTRATION to share one column
  // Allow PLI and RIGHTWRONG to share one column
  // Allow PLI and STICKER to share one column

  if (calloutShared && pli.tbl[y] == TblCsi) {
      int wX = 0, wY = 0;
      if (x == XX) {
          wX = pli.size[XX] + calloutSize[XX];
          wY = pli.size[YY];
        } else {
          wX = pli.size[XX];
          wY = pli.size[YY] + calloutSize[YY];
        }
      if (cols[pli.tbl[XX]] < wX) {
          cols[pli.tbl[XX]] = wX;
        }
      if (rows[pli.tbl[YY]] < wY) {
          rows[pli.tbl[YY]] = wY;
        }

    } else {

      /* Drop the PLI down on top of the CSI, and reduce the pli's size */

      bool addOn = true;

      if (onlyChild()) {
          switch (pliPlacement.placement) {
            case Top:
            case Bottom:
              if (pliPlacement.relativeTo == CsiType) {
                  if ( ! collide(square,pli.tbl,y, x)) {
                      int height = (max - pixmapSize[y])/2;
                      if (height > 0) {
                          if (height >= pli.size[y]) {  // entire thing fits
                              rows[pli.tbl[y]] = 0;
                              addOn = false;
                            } else {                      // fit what we can
                              rows[pli.tbl[y]] = pli.size[y] - height;
                              addOn = false;
                            }
                        }
                    }
                }
              break;
            default:
              break;
            }
        }

      if (cols[pli.tbl[XX]] < pli.size[XX]) {
          cols[pli.tbl[XX]] = pli.size[XX];  // HINT 1
        }
      if (addOn) {
          if (rows[pli.tbl[YY]] < pli.size[YY]) {
              rows[pli.tbl[YY]] = pli.size[YY];
            }
        }
    }

  /* Add PLI and ILLUSTRATION to share same column - expand cols and/or rows */

 if (illustrationShared && pli.tbl[y] == TblCsi) {
     int wX = 0, wY = 0;
     if (x == XX) {
         wX = pli.size[XX] + illustrationSize[XX];
         wY = pli.size[YY];
       } else {
         wX = pli.size[XX];
         wY = pli.size[YY] + illustrationSize[YY];
       }
     if (cols[pli.tbl[XX]] < wX) {
         cols[pli.tbl[XX]] = wX;
       }
     if (rows[pli.tbl[YY]] < wY) {
         rows[pli.tbl[YY]] = wY;
       }

   } else {

     /* Drop the PLI down on top of the CSI, and reduce the pli's size */

     bool addOn = true;

     if (onlyChild()) {
         switch (pliPlacement.placement) {
           case Top:
           case Bottom:
             if (pliPlacement.relativeTo == CsiType) {
                 if ( ! collide(square,pli.tbl,y, x)) {
                     int height = (max - pixmapSize[y])/2;
                     if (height > 0) {
                         if (height >= pli.size[y]) {  // entire thing fits
                             rows[pli.tbl[y]] = 0;
                             addOn = false;
                           } else {                      // fit what we can
                             rows[pli.tbl[y]] = pli.size[y] - height;
                             addOn = false;
                           }
                       }
                   }
               }
             break;
           default:
             break;
           }
       }

     if (cols[pli.tbl[XX]] < pli.size[XX]) {
         cols[pli.tbl[XX]] = pli.size[XX];  // HINT 1
       }
     if (addOn) {
         if (rows[pli.tbl[YY]] < pli.size[YY]) {
             rows[pli.tbl[YY]] = pli.size[YY];
           }
       }
   }

 /* Add PLI and RIGHTWRONG to share same column - expand cols and/or rows */

if (rightWrongShared && pli.tbl[y] == TblCsi) {
    int wX = 0, wY = 0;
    if (x == XX) {
        wX = pli.size[XX] + rightWrongSize[XX];
        wY = pli.size[YY];
      } else {
        wX = pli.size[XX];
        wY = pli.size[YY] + rightWrongSize[YY];
      }
    if (cols[pli.tbl[XX]] < wX) {
        cols[pli.tbl[XX]] = wX;
      }
    if (rows[pli.tbl[YY]] < wY) {
        rows[pli.tbl[YY]] = wY;
      }

  } else {

    /* Drop the PLI down on top of the CSI, and reduce the pli's size */

    bool addOn = true;

    if (onlyChild()) {
        switch (pliPlacement.placement) {
          case Top:
          case Bottom:
            if (pliPlacement.relativeTo == CsiType) {
                if ( ! collide(square,pli.tbl,y, x)) {
                    int height = (max - pixmapSize[y])/2;
                    if (height > 0) {
                        if (height >= pli.size[y]) {  // entire thing fits
                            rows[pli.tbl[y]] = 0;
                            addOn = false;
                          } else {                      // fit what we can
                            rows[pli.tbl[y]] = pli.size[y] - height;
                            addOn = false;
                          }
                      }
                  }
              }
            break;
          default:
            break;
          }
      }

    if (cols[pli.tbl[XX]] < pli.size[XX]) {
        cols[pli.tbl[XX]] = pli.size[XX];  // HINT 1
      }
    if (addOn) {
        if (rows[pli.tbl[YY]] < pli.size[YY]) {
            rows[pli.tbl[YY]] = pli.size[YY];
          }
      }
  }

/* Add PLI and STICKER to share same column - expand cols and/or rows */

if (stickerShared && pli.tbl[y] == TblCsi) {
   int wX = 0, wY = 0;
   if (x == XX) {
       wX = pli.size[XX] + stickerSize[XX];
       wY = pli.size[YY];
     } else {
       wX = pli.size[XX];
       wY = pli.size[YY] + stickerSize[YY];
     }
   if (cols[pli.tbl[XX]] < wX) {
       cols[pli.tbl[XX]] = wX;
     }
   if (rows[pli.tbl[YY]] < wY) {
       rows[pli.tbl[YY]] = wY;
     }

 } else {

   /* Drop the PLI down on top of the CSI, and reduce the pli's size */

   bool addOn = true;

   if (onlyChild()) {
       switch (pliPlacement.placement) {
         case Top:
         case Bottom:
           if (pliPlacement.relativeTo == CsiType) {
               if ( ! collide(square,pli.tbl,y, x)) {
                   int height = (max - pixmapSize[y])/2;
                   if (height > 0) {
                       if (height >= pli.size[y]) {  // entire thing fits
                           rows[pli.tbl[y]] = 0;
                           addOn = false;
                         } else {                      // fit what we can
                           rows[pli.tbl[y]] = pli.size[y] - height;
                           addOn = false;
                         }
                     }
                 }
             }
           break;
         default:
           break;
         }
     }

   if (cols[pli.tbl[XX]] < pli.size[XX]) {
       cols[pli.tbl[XX]] = pli.size[XX];  // HINT 1
     }
   if (addOn) {
       if (rows[pli.tbl[YY]] < pli.size[YY]) {
           rows[pli.tbl[YY]] = pli.size[YY];
         }
     }
 }

  if (cols[stepNumber.tbl[XX]] < stepNumber.size[XX]) {
      cols[stepNumber.tbl[XX]] = stepNumber.size[XX];
    }

  if (rows[stepNumber.tbl[YY]] < stepNumber.size[YY]) {
      rows[stepNumber.tbl[YY]] = stepNumber.size[YY];
    }
  
  if (cols[TblCsi] < csiPlacement.size[XX]) {
      cols[TblCsi] = csiPlacement.size[XX];
    }

  if (rows[TblCsi] < csiPlacement.size[YY]) {
      rows[TblCsi] = csiPlacement.size[YY];
    }

  if (cols[rotateIcon.tbl[XX]] < rotateIcon.size[XX]) {
      cols[rotateIcon.tbl[XX]] = rotateIcon.size[XX];
    }

  if (rows[rotateIcon.tbl[YY]] < rotateIcon.size[YY]) {
      rows[rotateIcon.tbl[YY]] = rotateIcon.size[YY];
    }

  // perhaps need to check size like PLI above - HOLD FOR THE MOMENT...
  if (rows[oneToOne.tbl[YY]] < oneToOne.size[YY]) {
      rows[oneToOne.tbl[YY]] = oneToOne.size[YY];
    }

  /******************************************************************/
  /* Determine col/row and margin for each Callout, Illustration,  */
  /* RightWrong, and Sticker that is relative                      */
  /* to step components (e.g. not page or multiStep)                */
  /******************************************************************/

  /* Callout */

  for (int i = 0; i < numCallouts; i++) {
      Callout *callout = calloutList[i];

      switch (callout->placement.value().relativeTo) {
        case CsiType:
        case PartsListType:
        case StepNumberType:
        case RotateIconType:
        case OneToOneType:
          if (callout->shared && rows[TblCsi] < callout->size[y]) {
              rows[TblCsi] = callout->size[y];
            } else {

              if (cols[callout->tbl[XX]] < callout->size[XX]) {
                  cols[callout->tbl[XX]] = callout->size[XX];
                }
              if (rows[callout->tbl[YY]] < callout->size[YY]) {
                  rows[callout->tbl[YY]] = callout->size[YY];
                }

              maxMargin(callout->margin,
                        callout->tbl,
                        marginRows,
                        marginCols);
            }
          break;
        default:
          break;
        }
    }

  /* Illustration */

  for (int i = 0; i < numIllustrations; i++) {
      Illustration *illustration = illustrationList[i];

      switch (illustration->placement.value().relativeTo) {
        case CsiType:
        case PartsListType:
        case StepNumberType:
        case RotateIconType:
        case OneToOneType:
          if (illustration->shared && rows[TblCsi] < illustration->size[y]) {
              rows[TblCsi] = illustration->size[y];
            } else {

              if (cols[illustration->tbl[XX]] < illustration->size[XX]) {
                  cols[illustration->tbl[XX]] = illustration->size[XX];
                }
              if (rows[illustration->tbl[YY]] < illustration->size[YY]) {
                  rows[illustration->tbl[YY]] = illustration->size[YY];
                }

              maxMargin(illustration->margin,
                        illustration->tbl,
                        marginRows,
                        marginCols);
            }
          break;
        default:
          break;
        }
    }

  /* RightWrong */

  for (int i = 0; i < numRightWrongs; i++) {
      RightWrong *rightWrong = rightWrongList[i];

      switch (rightWrong->placement.value().relativeTo) {
        case CsiType:
        case PartsListType:
        case StepNumberType:
        case RotateIconType:
        case OneToOneType:
          if (rightWrong->shared && rows[TblCsi] < rightWrong->size[y]) {
              rows[TblCsi] = rightWrong->size[y];
            } else {

              if (cols[rightWrong->tbl[XX]] < rightWrong->size[XX]) {
                  cols[rightWrong->tbl[XX]] = rightWrong->size[XX];
                }
              if (rows[rightWrong->tbl[YY]] < rightWrong->size[YY]) {
                  rows[rightWrong->tbl[YY]] = rightWrong->size[YY];
                }

              maxMargin(rightWrong->margin,
                        rightWrong->tbl,
                        marginRows,
                        marginCols);
            }
          break;
        default:
          break;
        }
    }

  /* Sticker */

  for (int i = 0; i < numStickers; i++) {
      Sticker *sticker = stickerList[i];

      switch (sticker->placement.value().relativeTo) {
        case CsiType:
        case PartsListType:
        case StepNumberType:
        case RotateIconType:
        case OneToOneType:
          if (sticker->shared && rows[TblCsi] < sticker->size[y]) {
              rows[TblCsi] = sticker->size[y];
            } else {

              if (cols[sticker->tbl[XX]] < sticker->size[XX]) {
                  cols[sticker->tbl[XX]] = sticker->size[XX];
                }
              if (rows[sticker->tbl[YY]] < sticker->size[YY]) {
                  rows[sticker->tbl[YY]] = sticker->size[YY];
                }

              maxMargin(sticker->margin,
                        sticker->tbl,
                        marginRows,
                        marginCols);
            }
          break;
        default:
          break;
        }
    }

  return 0;
}

bool Step::collide(
    int square[NumPlaces][NumPlaces],
    int tbl[],
    int x,
    int y)
{
  int place;
  for (place = tbl[x]; place < TblCsi; place++) {
      if (square[place][y] != -1) {
          return true;
        }
    }
  return false;
}

void Step::maxMargin(int &top, int &bot, int y)
{
  // TODO Investigate this more closely to ensure fully understand
  // Perhaps stepNumber is used because it's the leftMost item on the page

  top = csiPlacement.margin.valuePixels(y);
  bot = top;

  if (showStepNumber && ! onlyChild()) {
      if (stepNumber.tbl[YY] < TblCsi) {
          top = stepNumber.margin.valuePixels(y);
        } else if (stepNumber.tbl[y] == TblCsi) {
          int margin = stepNumber.margin.valuePixels(y);
          top = qMax(top,margin);
          bot = qMax(bot,margin);
        } else {
          bot = stepNumber.margin.valuePixels(y);
        }
    }

  if (displaySubModel){
      if (subModel.tbl[YY] < TblCsi) {
          top = subModel.margin.valuePixels(y);
        } else if (stepNumber.tbl[y] == TblCsi) {
          int margin = subModel.margin.valuePixels(y);
          top = qMax(top,margin);
          bot = qMax(bot,margin);
        } else {
          bot = subModel.margin.valuePixels(y);
        }
    }

  if (pli.size[y]) {
    if (pli.tbl[y] < TblCsi) {
      top = pli.margin.valuePixels(y);
    } else if (stepNumber.tbl[y] == TblCsi) {
      int margin = pli.margin.valuePixels(y);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = pli.margin.valuePixels(y);
    }
  }

  if (placeRotateIcon){
      if (rotateIcon.tbl[YY] < TblCsi) {
          top = rotateIcon.margin.valuePixels(y);
        } else if (stepNumber.tbl[y] == TblCsi) {
          int margin = rotateIcon.margin.valuePixels(y);
          top = qMax(top,margin);
          bot = qMax(bot,margin);
        } else {
          bot = rotateIcon.margin.valuePixels(y);
        }
    }

  if (showOneToOne){
      if (oneToOne.tbl[YY] < TblCsi) {
          top = oneToOne.margin.valuePixels(y);
        } else if (stepNumber.tbl[y] == TblCsi) {
          int margin = oneToOne.margin.valuePixels(y);
          top = qMax(top,margin);
          bot = qMax(bot,margin);
        } else {
          bot = oneToOne.margin.valuePixels(y);
        }
    }

  for (int i = 0; i < calloutList.size(); i++) {
    Callout *callout = calloutList[i];
    if (callout->tbl[y] < TblCsi) {
      top = callout->margin.valuePixels(y);
    } else if (stepNumber.tbl[y] == TblCsi) {
      int margin = callout->margin.valuePixels(y);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = callout->margin.valuePixels(y);
    }
  }

  for (int i = 0; i < illustrationList.size(); i++) {
    Illustration *illustration = illustrationList[i];
    if (illustration->tbl[y] < TblCsi) {
      top = illustration->margin.valuePixels(y);
    } else if (stepNumber.tbl[y] == TblCsi) {
      int margin = illustration->margin.valuePixels(y);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = illustration->margin.valuePixels(y);
    }
  }

  for (int i = 0; i < rightWrongList.size(); i++) {
    RightWrong *rightWrong = rightWrongList[i];
    if (rightWrong->tbl[y] < TblCsi) {
      top = rightWrong->margin.valuePixels(y);
    } else if (stepNumber.tbl[y] == TblCsi) {
      int margin = rightWrong->margin.valuePixels(y);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = rightWrong->margin.valuePixels(y);
    }
  }

  for (int i = 0; i < stickerList.size(); i++) {
    Sticker *sticker = stickerList[i];
    if (sticker->tbl[y] < TblCsi) {
      top = sticker->margin.valuePixels(y);
    } else if (stepNumber.tbl[y] == TblCsi) {
      int margin = sticker->margin.valuePixels(y);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = sticker->margin.valuePixels(y);
    }
  }
}

/***************************************************************************
 * This routine is used for tabular multi-steps.  It is used to determine
 * the location of CSI, PLI, stepNumber, rotateIcon and step relative Callouts
 * Illustrations, rightWrongs and Stickers
 ***************************************************************************/

void Step::placeit(
    int rows[],
    int margins[],
    int y,
    bool shared)
{

  /*********************************/
  /* record the origin of each row */
  /*********************************/

  int origin = 0;

  int origins[NumPlaces];

  for (int i = 0; i < NumPlaces; i++) {
      origins[i] = origin;
      if (rows[i]) {
          origin += rows[i] + margins[i];
        }
    }

  size[y] = origin;

  /*******************************************/
  /* Now place the components in pixel units */
  /*******************************************/

  csiPlacement.loc[y] = origins[TblCsi] + (rows[TblCsi] - csiPlacement.size[y])/2;
  pli.loc[y]          = origins[pli.tbl[y]];
  stepNumber.loc[y]   = origins[stepNumber.tbl[y]];
  subModel.loc[y]     = origins[subModel.tbl[y]];
  rotateIcon.loc[y]   = origins[rotateIcon.tbl[y]];
  oneToOne.loc[y]     = origins[oneToOne.tbl[y]];

  switch (y) {
    case XX:
      if ( ! shared) {
          pli.justifyX(origins[pli.tbl[y]],rows[pli.tbl[y]]);
        }
      if (showStepNumber && ! onlyChild()) {
          stepNumber.justifyX(origins[stepNumber.tbl[y]],rows[stepNumber.tbl[y]]);
        }
      if(displaySubModel){
          subModel.justifyX(origins[subModel.tbl[y]],rows[subModel.tbl[y]]);
        }
      if(placeRotateIcon){
          rotateIcon.justifyX(origins[rotateIcon.tbl[y]],rows[rotateIcon.tbl[y]]);
        }
      if(showOneToOne){
          oneToOne.justifyX(origins[oneToOne.tbl[y]],rows[oneToOne.tbl[y]]);
        }
      break;
    case YY:
      if ( ! shared) {
          pli.justifyY(origins[pli.tbl[y]],rows[pli.tbl[y]]);
        }
      if (showStepNumber && ! onlyChild()) {
          stepNumber.justifyY(origins[stepNumber.tbl[y]],rows[stepNumber.tbl[y]]);
        }
      if(displaySubModel){
          subModel.justifyY(origins[subModel.tbl[y]],rows[subModel.tbl[y]]);
        }
      if(placeRotateIcon){
          rotateIcon.justifyY(origins[rotateIcon.tbl[y]],rows[rotateIcon.tbl[y]]);
        }
      if(showOneToOne){
          oneToOne.justifyY(origins[oneToOne.tbl[y]],rows[oneToOne.tbl[y]]);
        }
      break;
    default:
      break;
    }

  /* place the Callouts that are relative to step components */

  for (int i = 0; i < calloutList.size(); i++) {
      Callout *callout = calloutList[i];
      PlacementData calloutPlacement = callout->placement.value();

      if (shared && callout->shared) {
          if (callout->size[y] > origins[TblCsi]) {
              int locY = callout->size[y] - origins[TblCsi] - margins[TblCsi];
              callout->loc[y] = locY;
            } else {
              int locY = origins[TblCsi] - callout->size[y] - margins[TblCsi];
              callout->loc[y] = locY;
            }
        } else {
          switch (calloutPlacement.relativeTo) {
            case CsiType:
            case PartsListType:
            case StepNumberType:
            case SubModelType:
            case RotateIconType:
            case OneToOneType:
              callout->loc[y] = origins[callout->tbl[y]];
              if (callout->shared) {
                  callout->loc[y] -= callout->margin.value(y) - 500;
                }

              if (y == YY) {
                  callout->justifyY(origins[callout->tbl[y]],
                      rows[callout->tbl[y]]);
                } else {
                  callout->justifyX(origins[callout->tbl[y]],
                      rows[callout->tbl[y]]);
                }
              break;
            default:
              break;
            }
        }
    }

  /* place the Illustrations that are relative to step components */

  for (int i = 0; i < illustrationList.size(); i++) {
      Illustration *illustration = illustrationList[i];
      PlacementData illustrationPlacement = illustration->placement.value();

      if (shared && illustration->shared) {
          if (illustration->size[y] > origins[TblCsi]) {
              int locY = illustration->size[y] - origins[TblCsi] - margins[TblCsi];
              illustration->loc[y] = locY;
            } else {
              int locY = origins[TblCsi] - illustration->size[y] - margins[TblCsi];
              illustration->loc[y] = locY;
            }
        } else {
          switch (illustrationPlacement.relativeTo) {
            case CsiType:
            case PartsListType:
            case StepNumberType:
            case SubModelType:
            case RotateIconType:
            case OneToOneType:
              illustration->loc[y] = origins[illustration->tbl[y]];
              if (illustration->shared) {
                  illustration->loc[y] -= illustration->margin.value(y) - 500;
                }

              if (y == YY) {
                  illustration->justifyY(origins[illustration->tbl[y]],
                      rows[illustration->tbl[y]]);
                } else {
                  illustration->justifyX(origins[illustration->tbl[y]],
                      rows[illustration->tbl[y]]);
                }
              break;
            default:
              break;
            }
        }
    }

  /* place the RightWrongs that are relative to step components */

  for (int i = 0; i < rightWrongList.size(); i++) {
      RightWrong *rightWrong = rightWrongList[i];
      PlacementData rightWrongPlacement = rightWrong->placement.value();

      if (shared && rightWrong->shared) {
          if (rightWrong->size[y] > origins[TblCsi]) {
              int locY = rightWrong->size[y] - origins[TblCsi] - margins[TblCsi];
              rightWrong->loc[y] = locY;
            } else {
              int locY = origins[TblCsi] - rightWrong->size[y] - margins[TblCsi];
              rightWrong->loc[y] = locY;
            }
        } else {
          switch (rightWrongPlacement.relativeTo) {
            case CsiType:
            case PartsListType:
            case StepNumberType:
            case SubModelType:
            case RotateIconType:
            case OneToOneType:
              rightWrong->loc[y] = origins[rightWrong->tbl[y]];
              if (rightWrong->shared) {
                  rightWrong->loc[y] -= rightWrong->margin.value(y) - 500;
                }

              if (y == YY) {
                  rightWrong->justifyY(origins[rightWrong->tbl[y]],
                      rows[rightWrong->tbl[y]]);
                } else {
                  rightWrong->justifyX(origins[rightWrong->tbl[y]],
                      rows[rightWrong->tbl[y]]);
                }
              break;
            default:
              break;
            }
        }
    }

  /* place the Stickers that are relative to step components */

  for (int i = 0; i < stickerList.size(); i++) {
      Sticker *sticker = stickerList[i];
      PlacementData stickerPlacement = sticker->placement.value();

      if (shared && sticker->shared) {
          if (sticker->size[y] > origins[TblCsi]) {
              int locY = sticker->size[y] - origins[TblCsi] - margins[TblCsi];
              sticker->loc[y] = locY;
            } else {
              int locY = origins[TblCsi] - sticker->size[y] - margins[TblCsi];
              sticker->loc[y] = locY;
            }
        } else {
          switch (stickerPlacement.relativeTo) {
            case CsiType:
            case PartsListType:
            case StepNumberType:
            case SubModelType:
            case RotateIconType:
            case OneToOneType:
              sticker->loc[y] = origins[sticker->tbl[y]];
              if (sticker->shared) {
                  sticker->loc[y] -= sticker->margin.value(y) - 500;
                }

              if (y == YY) {
                  sticker->justifyY(origins[sticker->tbl[y]],
                      rows[sticker->tbl[y]]);
                } else {
                  sticker->justifyX(origins[sticker->tbl[y]],
                      rows[sticker->tbl[y]]);
                }
              break;
            default:
              break;
            }
        }
    }
}

/*
 * The add graphics method is independent of Horizontal/Vertical
 * multi-step/callout/illustration/rightWrong/sticker
 * allocation, or tabular vs. freeform mode.
 */

void Step::addGraphicsItems(
    int             offsetX,
    int             offsetY,
    Meta           *meta,
    PlacementType   parentRelativeType,
    QGraphicsItem  *parent,
    bool            movable)
{
  offsetX += loc[XX];
  offsetY += loc[YY];
  
  // CSI

  csiItem = new CsiItem(this,
                        meta,
                        csiPixmap,
                        submodelLevel,
                        parent,
                        parentRelativeType);
  csiItem->assign(&csiPlacement);
  csiItem->setPos(offsetX + csiItem->loc[XX],
                  offsetY + csiItem->loc[YY]);
  csiItem->setFlag(QGraphicsItem::ItemIsMovable,movable);

  // PLI

  if (pli.tsize()) {
      pli.addPli(submodelLevel, parent);
      pli.setPos(offsetX + pli.loc[XX],
                 offsetY + pli.loc[YY]);
    }
  
  // Step Number

  if (stepNumber.number > 0 && ! onlyChild() && showStepNumber) {
      StepNumberItem *sn;
      sn = new StepNumberItem(this,
                              parentRelativeType,
                              numberPlacemetMeta,
                              "%d",
                              stepNumber.number,
                              parent);

      sn->setPos(offsetX + stepNumber.loc[XX],
                 offsetY + stepNumber.loc[YY]);

      sn->setFlag(QGraphicsItem::ItemIsMovable,movable);
    }

  // Sub Model

  if (displaySubModel){
      SubModelItem *sm;
      sm = new SubModelItem(this,
                              parentRelativeType,
                              subModel.subModelMeta,
                              parent);
      sm->setPos(offsetX + subModel.loc[XX],
                 offsetY + subModel.loc[YY]);

      sm->setFlag(QGraphicsItem::ItemIsMovable,movable);
    }

  // Rotate Icon

  if (placeRotateIcon){
      RotateIconItem *ri;
      ri = new RotateIconItem(this,
                              parentRelativeType,
                              rotateIcon.rotateIconMeta,
                              parent);
      ri->setPos(offsetX + rotateIcon.loc[XX],
                 offsetY + rotateIcon.loc[YY]);

      ri->setFlag(QGraphicsItem::ItemIsMovable,movable);
    }

  // One to One

  if (showOneToOne){
      if (oneToOne.tsize()){
          oneToOne.addOneToOne(submodelLevel, parent);
          oneToOne.setPos(offsetX + oneToOne.loc[XX],
                          offsetY + oneToOne.loc[YY]);
        }
    }

  // Callouts

  for (int i = 0; i < calloutList.size(); i++) {

      Callout *callout = calloutList[i];
      PlacementData placementData = callout->placement.value();

      QRect rect(csiItem->loc[XX],
                 csiItem->loc[YY],
                 csiItem->size[XX],
                 csiItem->size[YY]);

      if (placementData.relativeTo == CalloutType) {
          callout->addGraphicsItems(offsetX-loc[XX],offsetY-loc[YY],rect,parent, movable);
        } else {
          bool callout_movable = true /*movable*/;
          if (parentRelativeType == StepGroupType && placementData.relativeTo == StepGroupType) {
              callout_movable = true;
            }
          callout->addGraphicsItems(callout->shared ? 0 : offsetX,offsetY,rect,parent, callout_movable);
        }
      for (int i = 0; i < callout->pointerList.size(); i++) {
          Pointer *pointer = callout->pointerList[i];
          callout->parentStep = this;
          callout->addGraphicsPointerItem(pointer,callout->underpinnings);
        }
    }

  // Illustrations

  for (int i = 0; i < illustrationList.size(); i++) {

      Illustration *illustration = illustrationList[i];
      PlacementData placementData = illustration->placement.value();

      QRect rect(csiItem->loc[XX],
                 csiItem->loc[YY],
                 csiItem->size[XX],
                 csiItem->size[YY]);

      if (placementData.relativeTo == IllustrationType) {
          illustration->addGraphicsItems(offsetX-loc[XX],offsetY-loc[YY],rect,parent, movable);
        } else {
          bool illustration_movable = true /*movable*/;
          if (parentRelativeType == StepGroupType && placementData.relativeTo == StepGroupType) {
              illustration_movable = true;
            }
          illustration->addGraphicsItems(illustration->shared ? 0 : offsetX,offsetY,rect,parent, illustration_movable);
        }
      for (int i = 0; i < illustration->pointerList.size(); i++) {
          Pointer *pointer = illustration->pointerList[i];
          illustration->parentStep = this;
          illustration->addGraphicsPointerItem(pointer,illustration->underpinnings);
        }
    }

  // RightWrongs
  // RightWrongs

  for (int i = 0; i < rightWrongList.size(); i++) {

      RightWrong *rightWrong = rightWrongList[i];
      PlacementData placementData = rightWrong->placement.value();

      QRect rect(csiItem->loc[XX],
                 csiItem->loc[YY],
                 csiItem->size[XX],
                 csiItem->size[YY]);

      if (placementData.relativeTo == RightWrongType) {
          rightWrong->addGraphicsItems(offsetX-loc[XX],offsetY-loc[YY],rect,parent, movable);
        } else {
          bool rightWrong_movable = true /*movable*/;
          if (parentRelativeType == StepGroupType && placementData.relativeTo == StepGroupType) {
              rightWrong_movable = true;
            }
          rightWrong->addGraphicsItems(rightWrong->shared ? 0 : offsetX,offsetY,rect,parent, rightWrong_movable);
        }
      for (int i = 0; i < rightWrong->pointerList.size(); i++) {
          Pointer *pointer = rightWrong->pointerList[i];
          rightWrong->parentStep = this;
          rightWrong->addGraphicsPointerItem(pointer,rightWrong->underpinnings);
        }
    }
// NO POINTER
//
//  for (int i = 0; i < rightWrongList.size(); i++) {

//      RightWrong *rightWrong = rightWrongList[i];
//      PlacementData placementData = rightWrong->placement.value();

//      if (placementData.relativeTo == RightWrongType) {
//          rightWrong->addRightWrong(submodelLevel,parent);
//        } else {
//          bool rightWrong_movable = true /*movable*/;
//          if (parentRelativeType == StepGroupType && placementData.relativeTo == StepGroupType) {
//              rightWrong_movable = true;
//            }
//          rightWrong->addRightWrong(submodelLevel,parent);
//          rightWrong->setFlag(QGraphicsItem::ItemIsMovable,rightWrong_movable);
//        }
//    }

  // Stickers

  for (int i = 0; i < stickerList.size(); i++) {

      Sticker *sticker = stickerList[i];
      PlacementData placementData = sticker->placement.value();

      QRect rect(csiItem->loc[XX],
                 csiItem->loc[YY],
                 csiItem->size[XX],
                 csiItem->size[YY]);

      if (placementData.relativeTo == StickerType) {
          sticker->addGraphicsItems(offsetX-loc[XX],offsetY-loc[YY],rect,parent, movable);
        } else {
          bool sticker_movable = true /*movable*/;
          if (parentRelativeType == StepGroupType && placementData.relativeTo == StepGroupType) {
              sticker_movable = true;
            }
          sticker->addGraphicsItems(sticker->shared ? 0 : offsetX,offsetY,rect,parent, sticker_movable);
        }
      for (int i = 0; i < sticker->pointerList.size(); i++) {
          Pointer *pointer = sticker->pointerList[i];
          sticker->parentStep = this;
          sticker->addGraphicsPointerItem(pointer,sticker->underpinnings);
        }
    }
}

void Step::placeInside()
{
  // PLI

  if (pli.placement.value().preposition == Inside) {
      switch (pli.placement.value().relativeTo) {
        case CsiType:
          csiPlacement.placeRelative(&pli);
          break;
        case PartsListType:
          break;
        case StepNumberType:
          stepNumber.placeRelative(&pli);
          break;
        case SubModelType:
          subModel.placeRelative(&pli);
          break;
        case RotateIconType:
          rotateIcon.placeRelative(&pli);
          break;
        case OneToOneType:
          oneToOne.placeRelative(&pli);
          break;
        default:
          break;
        }
    }

  // Step Number

  if (stepNumber.placement.value().preposition == Inside) {
      switch (pli.placement.value().relativeTo) {
        case CsiType:
          csiPlacement.placeRelative(&stepNumber);
          break;
        case PartsListType:
          pli.placeRelative(&stepNumber);
          break;
        case StepNumberType:
          break;
        case SubModelType:
          subModel.placeRelative(&stepNumber);
          break;
        case RotateIconType:
          rotateIcon.placeRelative(&stepNumber);
          break;
        case OneToOneType:
          oneToOne.placeRelative(&stepNumber);
          break;
        default:
          break;
        }
    }

  // Sub Model

  if (subModel.placement.value().preposition == Inside) {
      switch (pli.placement.value().relativeTo) {
        case CsiType:
          csiPlacement.placeRelative(&subModel);
          break;
        case PartsListType:
          pli.placeRelative(&subModel);
          break;
        case StepNumberType:
          stepNumber.placeRelative(&subModel);
          break;
        case SubModelType:
          break;
        case RotateIconType:
          rotateIcon.placeRelative(&subModel);
          break;
        case OneToOneType:
          oneToOne.placeRelative(&subModel);
          break;
        default:
          break;
        }
    }

  // Rotate Icon

  if (rotateIcon.placement.value().preposition == Inside) {
      switch (pli.placement.value().relativeTo) {
        case CsiType:
          csiPlacement.placeRelative(&rotateIcon);
          break;
        case PartsListType:
          pli.placeRelative(&rotateIcon);
          break;
        case StepNumberType:
          stepNumber.placeRelative(&rotateIcon);
          break;
        case SubModelType:
          subModel.placeRelative(&rotateIcon);
          break;
        case RotateIconType:
          break;
        case OneToOneType:
          oneToOne.placeRelative(&rotateIcon);
          break;
        default:
          break;
        }
    }

  // One To One

  if (oneToOne.placement.value().preposition == Inside) {
      switch (pli.placement.value().relativeTo) {
        case CsiType:
          csiPlacement.placeRelative(&oneToOne);
          break;
        case PartsListType:
          pli.placeRelative(&oneToOne);
          break;
        case StepNumberType:
          stepNumber.placeRelative(&oneToOne);
          break;
        case SubModelType:
          subModel.placeRelative(&oneToOne);
          break;
        case RotateIconType:
          oneToOne.placeRelative(&oneToOne);
          break;
        case OneToOneType:
          break;
        default:
          break;
        }
    }

  // Callout

  for (int i = 0; i < calloutList.size(); i++) {

      Callout *callout = calloutList[i];
      PlacementData placementData = callout->placement.value();

      /* Offset Callouts */

      int relativeToSize[2];

      relativeToSize[XX] = 0;
      relativeToSize[YY] = 0;

      switch (placementData.relativeTo) {
        case CsiType:
          relativeToSize[XX] = csiPlacement.size[XX];
          relativeToSize[YY] = csiPlacement.size[YY];
          break;
        case PartsListType:
          relativeToSize[XX] = pli.size[XX];
          relativeToSize[YY] = pli.size[YY];
          break;
        case StepNumberType:
          relativeToSize[XX] = stepNumber.size[XX];
          relativeToSize[YY] = stepNumber.size[YY];
          break;
        case SubModelType:
          relativeToSize[XX] = subModel.size[XX];
          relativeToSize[YY] = subModel.size[YY];
          break;
        case RotateIconType:
          relativeToSize[XX] = rotateIcon.size[XX];
          relativeToSize[YY] = rotateIcon.size[YY];
          break;
        case OneToOneType:
          relativeToSize[XX] = oneToOne.size[XX];
          relativeToSize[YY] = oneToOne.size[YY];
          break;
        default:
          break;
        }
      callout->loc[XX] += relativeToSize[XX]*placementData.offsets[XX];
      callout->loc[YY] += relativeToSize[YY]*placementData.offsets[YY];
    }

  // Illustrations

  for (int i = 0; i < illustrationList.size(); i++) {

      Illustration *illustration = illustrationList[i];
      PlacementData placementData = illustration->placement.value();

      /* Offset Illustrations */

      int relativeToSize[2];

      relativeToSize[XX] = 0;
      relativeToSize[YY] = 0;

      switch (placementData.relativeTo) {
        case CsiType:
          relativeToSize[XX] = csiPlacement.size[XX];
          relativeToSize[YY] = csiPlacement.size[YY];
          break;
        case PartsListType:
          relativeToSize[XX] = pli.size[XX];
          relativeToSize[YY] = pli.size[YY];
          break;
        case StepNumberType:
          relativeToSize[XX] = stepNumber.size[XX];
          relativeToSize[YY] = stepNumber.size[YY];
          break;
        case SubModelType:
          relativeToSize[XX] = subModel.size[XX];
          relativeToSize[YY] = subModel.size[YY];
          break;
        case RotateIconType:
          relativeToSize[XX] = rotateIcon.size[XX];
          relativeToSize[YY] = rotateIcon.size[YY];
          break;
        case OneToOneType:
          relativeToSize[XX] = oneToOne.size[XX];
          relativeToSize[YY] = oneToOne.size[YY];
          break;
        default:
          break;
        }
      illustration->loc[XX] += relativeToSize[XX]*placementData.offsets[XX];
      illustration->loc[YY] += relativeToSize[YY]*placementData.offsets[YY];
    }

  // RightWrongs

  for (int i = 0; i < rightWrongList.size(); i++) {

      RightWrong *rightWrong = rightWrongList[i];
      PlacementData placementData = rightWrong->placement.value();

      /* Offset RightWrongs */

      int relativeToSize[2];

      relativeToSize[XX] = 0;
      relativeToSize[YY] = 0;

      switch (placementData.relativeTo) {
        case CsiType:
          relativeToSize[XX] = csiPlacement.size[XX];
          relativeToSize[YY] = csiPlacement.size[YY];
          break;
        case PartsListType:
          relativeToSize[XX] = pli.size[XX];
          relativeToSize[YY] = pli.size[YY];
          break;
        case StepNumberType:
          relativeToSize[XX] = stepNumber.size[XX];
          relativeToSize[YY] = stepNumber.size[YY];
          break;
        case SubModelType:
          relativeToSize[XX] = subModel.size[XX];
          relativeToSize[YY] = subModel.size[YY];
          break;
        case RotateIconType:
          relativeToSize[XX] = rotateIcon.size[XX];
          relativeToSize[YY] = rotateIcon.size[YY];
          break;
        case OneToOneType:
          relativeToSize[XX] = oneToOne.size[XX];
          relativeToSize[YY] = oneToOne.size[YY];
          break;
        default:
          break;
        }
      rightWrong->loc[XX] += relativeToSize[XX]*placementData.offsets[XX];
      rightWrong->loc[YY] += relativeToSize[YY]*placementData.offsets[YY];
    }

  // Stickers

  for (int i = 0; i < stickerList.size(); i++) {

      Sticker *sticker = stickerList[i];
      PlacementData placementData = sticker->placement.value();

      /* Offset Stickers */

      int relativeToSize[2];

      relativeToSize[XX] = 0;
      relativeToSize[YY] = 0;

      switch (placementData.relativeTo) {
        case CsiType:
          relativeToSize[XX] = csiPlacement.size[XX];
          relativeToSize[YY] = csiPlacement.size[YY];
          break;
        case PartsListType:
          relativeToSize[XX] = pli.size[XX];
          relativeToSize[YY] = pli.size[YY];
          break;
        case StepNumberType:
          relativeToSize[XX] = stepNumber.size[XX];
          relativeToSize[YY] = stepNumber.size[YY];
          break;
        case SubModelType:
          relativeToSize[XX] = subModel.size[XX];
          relativeToSize[YY] = subModel.size[YY];
          break;
        case RotateIconType:
          relativeToSize[XX] = rotateIcon.size[XX];
          relativeToSize[YY] = rotateIcon.size[YY];
          break;
        case OneToOneType:
          relativeToSize[XX] = oneToOne.size[XX];
          relativeToSize[YY] = oneToOne.size[YY];
          break;
        default:
          break;
        }
      sticker->loc[XX] += relativeToSize[XX]*placementData.offsets[XX];
      sticker->loc[YY] += relativeToSize[YY]*placementData.offsets[YY];
    }
}

/*********************************************************************
 *
 * This section implements a second, more freeform version of packing
 * steps into callouts and multiSteps.
 *
 * Steps being oriented into sub-columns or sub-rows with
 * step columns or rows, this rendering technique dues not necessarily
 * get you the most compact result.
 *
 *  In single step per page the placement algorithm is very flexible.
 * Anything can be placed relative to anything, as long as the placement
 * relationships lead everyone back to the page, then all things will
 * be placed.
 *
 * In free-form placement, some placement component is the root of all
 * placement (CSI, PLI, STEP_NUMBER, ROTATE_ICON).  All other placement components
 * are placed relative to the base, or placed relative to things that
 * are placed relative to the root.
 *
 ********************************************************************/

void Step::sizeitFreeform(
    int xx,
    int yy,
    int relativeBase,
    int relativeJustification,
    int &left,
    int &right)
{
  relativeJustification = relativeJustification;

  // Size up each Callout

  for (int i = 0; i < calloutList.size(); i++) {
      Callout *callout = calloutList[i];
      if (callout->meta.LPub.callout.freeform.value().mode) {
          callout->sizeitFreeform(xx,yy);
        } else {
          callout->sizeIt();
        }
    }

  // Place each Callout

  for (int i = 0; i < calloutList.size(); i++) {
      Callout *callout = calloutList[i];

      if (callout->meta.LPub.callout.freeform.value().mode) {
          if (callout->meta.LPub.callout.freeform.value().justification == Left ||
              callout->meta.LPub.callout.freeform.value().justification == Top) {
              callout->loc[xx] = callout->size[xx];
            }
        } else {
          callout->sizeIt();
        }
    }

  // Size up each Illustration

  for (int i = 0; i < illustrationList.size(); i++) {
      Illustration *illustration = illustrationList[i];
      if (illustration->meta.LPub.illustration.freeform.value().mode) {
          illustration->sizeitFreeform(xx,yy);
        } else {
          illustration->sizeIt();
        }
    }

  // Place each Illustration

  for (int i = 0; i < illustrationList.size(); i++) {
      Illustration *illustration = illustrationList[i];

      if (illustration->meta.LPub.illustration.freeform.value().mode) {
          if (illustration->meta.LPub.illustration.freeform.value().justification == Left ||
              illustration->meta.LPub.illustration.freeform.value().justification == Top) {
              illustration->loc[xx] = illustration->size[xx];
            }
        } else {
          illustration->sizeIt();
        }
    }

  // Size up each RightWrong

  for (int i = 0; i < rightWrongList.size(); i++) {
      RightWrong *rightWrong = rightWrongList[i];
      if (rightWrong->meta.LPub.rightWrong.freeform.value().mode) {
          rightWrong->sizeitFreeform(xx,yy);
        } else {
          rightWrong->sizeIt();
        }
    }

  // Place each RightWrong

  for (int i = 0; i < rightWrongList.size(); i++) {
      RightWrong *rightWrong = rightWrongList[i];

      if (rightWrong->meta.LPub.rightWrong.freeform.value().mode) {
          if (rightWrong->meta.LPub.rightWrong.freeform.value().justification == Left ||
              rightWrong->meta.LPub.rightWrong.freeform.value().justification == Top) {
              rightWrong->loc[xx] = rightWrong->size[xx];
            }
        } else {
          rightWrong->sizeIt();
        }
    }

  // Size up each Sticker

  for (int i = 0; i < stickerList.size(); i++) {
      Sticker *sticker = stickerList[i];
      if (sticker->meta.LPub.sticker.freeform.value().mode) {
          sticker->sizeitFreeform(xx,yy);
        } else {
          sticker->sizeIt();
        }
    }

  // Place each Sticker

  for (int i = 0; i < stickerList.size(); i++) {
      Sticker *sticker = stickerList[i];

      if (sticker->meta.LPub.sticker.freeform.value().mode) {
          if (sticker->meta.LPub.sticker.freeform.value().justification == Left ||
              sticker->meta.LPub.sticker.freeform.value().justification == Top) {
              sticker->loc[xx] = sticker->size[xx];
            }
        } else {
          sticker->sizeIt();
        }
    }

  // Size up the step number

  if (showStepNumber && ! onlyChild()) {
      stepNumber.sizeit();
    }

  // Size up the sub model

  if (displaySubModel){
      subModel.sizeit();
    }

  // Size up the rotateIcon

  if (placeRotateIcon){
      rotateIcon.sizeit();
    }

  //   Size up the oneToOne (Does not apply to oneToOne)

//  if (showOneToOne){
//      oneToOne.sizeit();
//    }

  // Place everything relative to the base (Page)

  int offsetX = 0, sizeX = 0;

  PlacementData placementData;

  switch (relativeBase) {
    case CsiType:
      placementData = csiPlacement.placement.value();
      placementData.relativeTo = PageType;
      csiPlacement.placement.setValue(placementData);
      csiPlacement.relativeTo(this);
      offsetX = csiPlacement.loc[xx];
      sizeX   = csiPlacement.size[yy];
      break;
    case PartsListType:
      placementData = pli.placement.value();
      placementData.relativeTo = PageType;
      pli.placement.setValue(placementData);
      pli.relativeTo(this);
      offsetX = pli.loc[xx];
      sizeX   = pli.size[yy];
      break;
    case StepNumberType:
      placementData = stepNumber.placement.value();
      placementData.relativeTo = PageType;
      stepNumber.placement.setValue(placementData);
      stepNumber.relativeTo(this);
      offsetX = stepNumber.loc[xx];
      sizeX   = stepNumber.size[xx];
      break;
    case SubModelType:
      placementData = subModel.placement.value();
      placementData.relativeTo = PageType;
      subModel.placement.setValue(placementData);
      subModel.relativeTo(this);
      offsetX = subModel.loc[xx];
      sizeX   = subModel.size[xx];
      break;
    case RotateIconType:
      placementData = rotateIcon.placement.value();
      placementData.relativeTo = PageType;
      rotateIcon.placement.setValue(placementData);
      rotateIcon.relativeTo(this);
      offsetX = rotateIcon.loc[xx];
      sizeX   = rotateIcon.size[xx];
      break;
    case OneToOneType:
      placementData = oneToOne.placement.value();
      placementData.relativeTo = PageType;
      oneToOne.placement.setValue(placementData);
      oneToOne.relativeTo(this);
      offsetX = oneToOne.loc[xx];
      sizeX   = oneToOne.size[xx];
      break;
    }

  // FIXME: when we get here for callouts that are placed to the left of the CSI
  // the outermost box is correctly placed, but within there the CSI is
  // in the upper left hand corner, even if it has a callout to the left of
  // it
  //
  // Have to determine the leftmost edge of any callouts
  //   Left of CSI
  //   Left edge of Top|Bottom Center or Right justified - we need place
  // size the step

  for (int dim = XX; dim <= YY; dim++) {

      int min = 500000;
      int max = 0;

      // CSI

      if (csiPlacement.loc[dim] < min) {
          min = csiPlacement.loc[dim];
        }
      if (csiPlacement.loc[dim] + csiPlacement.size[dim] > max) {
          max = csiPlacement.loc[dim] + csiPlacement.size[dim];
        }

      // PLI

      if (pli.loc[dim] < min) {
          min = pli.loc[dim];
        }
      if (pli.loc[dim] + pli.size[dim] > max) {
          max = pli.loc[dim] + pli.size[dim];
        }

      // Step Number

      if (stepNumber.loc[dim] < min) {
          min = stepNumber.loc[dim];
        }
      if (stepNumber.loc[dim] + stepNumber.size[dim] > max) {
          max = stepNumber.loc[dim] + stepNumber.size[dim];
        }

      // Sub Model

      if (subModel.loc[dim] < min) {
          min = subModel.loc[dim];
        }
      if (subModel.loc[dim] + subModel.size[dim] > max) {
          max = subModel.loc[dim] + subModel.size[dim];
        }

      // Rotate Icon

      if (rotateIcon.loc[dim] < min) {
          min = rotateIcon.loc[dim];
        }
      if (rotateIcon.loc[dim] + rotateIcon.size[dim] > max) {
          max = rotateIcon.loc[dim] + rotateIcon.size[dim];
        }

      // One To One

      if (oneToOne.loc[dim] < min) {
          min = oneToOne.loc[dim];
        }
      if (oneToOne.loc[dim] + oneToOne.size[dim] > max) {
          max = oneToOne.loc[dim] + oneToOne.size[dim];
        }

      // Callout

      for (int i = 0; i < calloutList.size(); i++) {
          Callout *callout = calloutList[i];
          if (callout->loc[dim] < min) {
              min = callout->loc[dim];
            }
          if (callout->loc[dim] + callout->size[dim] > max) {
              max = callout->loc[dim] + callout->size[dim];
            }
        }

      if (calledOut) {
          csiPlacement.loc[dim]  -= min;
          pli.loc[dim]           -= min;
          stepNumber.loc[dim]    -= min;
          subModel.loc[dim]      -= min;
          rotateIcon.loc[dim]    -= min;
          oneToOne.loc[dim]      -= min;

          for (int i = 0; i < calloutList.size(); i++) {
              Callout *callout = calloutList[i];
              callout->loc[dim] -= min;
            }
        }

      // Illustration

      for (int i = 0; i < illustrationList.size(); i++) {
          Illustration *illustration = illustrationList[i];
          if (illustration->loc[dim] < min) {
              min = illustration->loc[dim];
            }
          if (illustration->loc[dim] + illustration->size[dim] > max) {
              max = illustration->loc[dim] + illustration->size[dim];
            }
        }

      if (showIllustrations) {
          csiPlacement.loc[dim]  -= min;
          pli.loc[dim]           -= min;
          stepNumber.loc[dim]    -= min;
          rotateIcon.loc[dim]    -= min;
          subModel.loc[dim]      -= min;
          rotateIcon.loc[dim]    -= min;
          oneToOne.loc[dim]      -= min;

          for (int i = 0; i < illustrationList.size(); i++) {
              Illustration *illustration = illustrationList[i];
              illustration->loc[dim] -= min;
            }
        }

      // RightWrong

      for (int i = 0; i < rightWrongList.size(); i++) {
          RightWrong *rightWrong = rightWrongList[i];
          if (rightWrong->loc[dim] < min) {
              min = rightWrong->loc[dim];
            }
          if (rightWrong->loc[dim] + rightWrong->size[dim] > max) {
              max = rightWrong->loc[dim] + rightWrong->size[dim];
            }
        }

      if (showRightWrongs) {
          csiPlacement.loc[dim]  -= min;
          pli.loc[dim]           -= min;
          stepNumber.loc[dim]    -= min;
          subModel.loc[dim]      -= min;
          rotateIcon.loc[dim]    -= min;
          oneToOne.loc[dim]      -= min;

          for (int i = 0; i < rightWrongList.size(); i++) {
              RightWrong *rightWrong = rightWrongList[i];
              rightWrong->loc[dim] -= min;
            }
        }

      //Sticker

      for (int i = 0; i < stickerList.size(); i++) {
          Sticker *sticker = stickerList[i];
          if (sticker->loc[dim] < min) {
              min = sticker->loc[dim];
            }
          if (sticker->loc[dim] + sticker->size[dim] > max) {
              max = sticker->loc[dim] + sticker->size[dim];
            }
        }

      if (showStickers) {
          csiPlacement.loc[dim]  -= min;
          pli.loc[dim]           -= min;
          stepNumber.loc[dim]    -= min;
          subModel.loc[dim]      -= min;
          rotateIcon.loc[dim]    -= min;
          oneToOne.loc[dim]      -= min;

          for (int i = 0; i < stickerList.size(); i++) {
              Sticker *sticker = stickerList[i];
              sticker->loc[dim] -= min;
            }
        }

      // Size

      size[dim] = max - min;

      if (dim == XX) {
          left = min;
          right = max;
        }
    }

  /* Now make all things relative to the base */

  csiPlacement.loc[xx] -= offsetX + sizeX;
  pli.loc[xx]          -= offsetX + sizeX;
  stepNumber.loc[xx]   -= offsetX + sizeX;
  subModel.loc[xx]     -= offsetX + sizeX;
  rotateIcon.loc[xx]   -= offsetX + sizeX;
  oneToOne.loc[xx]     -= offsetX + sizeX;

  for (int i = 0; i < calloutList.size(); i++) {
      calloutList[i]->loc[xx] -= offsetX + sizeX;
    }
  for (int i = 0; i < illustrationList.size(); i++) {
      illustrationList[i]->loc[xx] -= offsetX + sizeX;
    }
  for (int i = 0; i < rightWrongList.size(); i++) {
      rightWrongList[i]->loc[xx] -= offsetX + sizeX;
    }
  for (int i = 0; i < stickerList.size(); i++) {
      stickerList[i]->loc[xx] -= offsetX + sizeX;
    }
}
