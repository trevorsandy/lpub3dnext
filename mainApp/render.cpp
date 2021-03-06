/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2015 - 2020 Trevor SANDY. All rights reserved.
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
 * This class encapsulates the external renderers.  For now, this means
 * only ldglite.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QtWidgets>
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QImageReader>
#include <QtConcurrent>

#include "lpub.h"
#include "render.h"
#include "resolution.h"
#include "meta.h"
#include "math.h"
#include "lpub_preferences.h"
#include "application.h"

#include <LDVQt/LDVWidget.h>
#include <LDVQt/LDVImageMatte.h>

#include "paths.h"

#include "lc_file.h"
#include "project.h"
#include "pieceinf.h"
#include "lc_model.h"
#include "view.h"
#include "lc_qhtmldialog.h"
#include "lc_partselectionwidget.h"

#include "lc_library.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

Render *renderer;

LDGLite ldglite;
LDView  ldview;
POVRay  povray;
Native  native;

//#define LduDistance 5729.57
//#define _CA "-ca0.01"
#define LP3D_CA 0.01
#define LP3D_CDF 1.0
#define USE_ALPHA "+UA"

#define SNAPSHOTS_LIST_THRESHOLD 3

static double pi = 4*atan(1.0);

// the default camera distance for real size
static float LduDistance = float(10.0/tan(0.005*pi/180));

// renderer timeout in milliseconds
int Render::rendererTimeout(){
    if (Preferences::rendererTimeout == -1)
        return -1;
    else
        return Preferences::rendererTimeout*60*1000;
}

const QString Render::fixupDirname(const QString &dirNameIn) {
#ifdef Q_OS_WIN
    long     length = 0;
    TCHAR*   buffer = nullptr;
//  30/11/2014 Generating "invalid conversion from const ushort to const wchar" compile error:
//  LPCWSTR dirNameWin = dirNameIn.utf16();
    LPCWSTR dirNameWin = reinterpret_cast<LPCWSTR>(dirNameIn.utf16());

// First obtain the size needed by passing nullptr and 0.

    length = GetShortPathName(dirNameWin, nullptr, 0);
    if (length == 0){
                QString message = QString("Couldn't get length of short path name length, lastError is %1, trying long path name").arg(GetLastError());
#ifdef QT_DEBUG_MODE
                qDebug() << message << "\n";
#else
                emit gui->messageSig(LOG_INFO, message);
#endif
                return dirNameIn;
     }
// Dynamically allocate the correct size
// (terminating null char was included in length)

    buffer = new TCHAR[length];

// Now simply call again using same long path.

    length = GetShortPathName(dirNameWin, buffer, length);
    if (length == 0){
                QString message = QString("Couldn't get length of short path name length, lastError is %1, trying long path name").arg(GetLastError());
#ifdef QT_DEBUG_MODE
                qDebug() << message << "\n";
#else
                emit gui->messageSig(LOG_INFO, message);
#endif
        return dirNameIn;
    }

    QString dirNameOut = QString::fromWCharArray(buffer);

    delete [] buffer;
        return dirNameOut;
#else
        return dirNameIn;
#endif
}

QString const Render::getRenderer()
{
  if (renderer == &ldglite)
  {
    return RENDERER_LDGLITE;
  }
  else
  if (renderer == &ldview)
  {
    return RENDERER_LDVIEW;
  }
  else
  if (renderer == &povray)
  {
    return RENDERER_POVRAY;
  }
  else
  {
    return RENDERER_NATIVE;
  }
}

int Render::getRendererIndex()
{
    int renderer = 0; // RENDERER_NATIVE, RENDERER_LDGLITE
    if (getRenderer() == RENDERER_POVRAY)      // 1
        renderer = 1;
    else if (getRenderer() == RENDERER_LDVIEW) // 2
        renderer = 2;
    return renderer;
}

void Render::setRenderer(QString const &name)
{
  if (name == RENDERER_LDGLITE)
  {
    renderer = &ldglite;
  }
  else
  if (name == RENDERER_LDVIEW)
  {
    renderer = &ldview;
  }
  else
  if (name == RENDERER_POVRAY)
  {
    renderer = &povray;
  }
  else
  {
    renderer = &native;
  }
}

const QString Render::getRotstepMeta(RotStepMeta &rotStep, bool isKey /*false*/){
  QString rotstepString;
  if (isKey) {
      rotstepString = QString("%1_%2_%3_%4")
                              .arg(qRound(rotStep.value().rots[0]))
                              .arg(qRound(rotStep.value().rots[1]))
                              .arg(qRound(rotStep.value().rots[2]))
                              .arg(rotStep.value().type.isEmpty() ? "REL" :
                                   rotStep.value().type.trimmed());
  } else {
      rotstepString = QString("0 // ROTSTEP %1 %2 %3 %4")
                              .arg(rotStep.value().type.isEmpty() ? "REL" :
                                   rotStep.value().type.trimmed())
                              .arg(rotStep.value().rots[0])
                              .arg(rotStep.value().rots[1])
                              .arg(rotStep.value().rots[2]);
  }

  //gui->messageSig(LOG_DEBUG,QString("ROTSTEP String: %1").arg(rotstepString));

  return rotstepString;
}

void Render::setLDrawHeaderAndFooterMeta(QStringList &parts, const QString &modelName, int imageType, bool displayOnly) {

    QStringList tokens;
    QString baseName = QFileInfo(modelName).completeBaseName().toLower();
    bool isMPD       = imageType == Options::Mt::SMP;  // always MPD if imageType is SMP
    baseName         = QString("%1").arg(baseName.replace(baseName.indexOf(baseName.at(0)),1,baseName.at(0).toUpper()));

    // Test for MPD
    if (!isMPD) {
        for (int i = 0; i < parts.size(); i++) {
            QString line = parts.at(i);
            split(line, tokens);
            if ((isMPD = tokens[0] == "1" && tokens.size() == 15 && gui->isSubmodel(tokens[14]))) {
                break;
            }
        }
    }

    // special case where the modelName will match the line type name so we append '_Preview' to the modelName
    if (imageType == Options::Mt::SMP) {
         baseName = baseName.append("_Preview");
    }

    // special case where model file is a display model or final step in fade step document
    if (displayOnly) {
        baseName = baseName.append("_Display_Model");
    }

    parts.prepend(QString("0 Name: %1").arg(modelName));
    parts.prepend(QString("0 %1").arg(baseName));

    if (isMPD) {
        parts.prepend(QString("0 FILE %1").arg(modelName));
        parts.append("0 NOFILE");
    }
}

bool Render::useLDViewSCall(){
    return (Preferences::preferredRenderer == RENDERER_LDVIEW &&
            Preferences::enableLDViewSingleCall);
}

bool Render::useLDViewSList(){
    return (Preferences::preferredRenderer == RENDERER_LDVIEW &&
            Preferences::enableLDViewSnaphsotList);
}

bool Render::clipImage(QString const &pngName) {

    QImage toClip(QDir::toNativeSeparators(pngName));
    QRect clipBox;

    int minX = toClip.width(); int maxX = 0;
    int minY = toClip.height();int maxY = 0;

    for(int x=0; x < toClip.width(); x++)
        for(int y=0; y < toClip.height(); y++)
            if (qAlpha(toClip.pixel(x, y)))
            {
                minX = qMin(x, minX);
                minY = qMin(y, minY);
                maxX = qMax(x, maxX);
                maxY = qMax(y, maxY);
            }

    if (minX > maxX || minY > maxY) {
        emit gui->messageSig(LOG_STATUS, qPrintable("No opaque content in " + pngName));
        return false;
    } else {
        clipBox.setCoords(minX, minY, maxX, maxY);
    }

    //save clipBox;
    QImage clippedImage = toClip.copy(clipBox);
    QString clipMsg = QString("%1 (w:%2 x h:%3)")
                              .arg(pngName)
                              .arg(clippedImage.width())
                              .arg(clippedImage.height());

    QImageWriter Writer(QDir::toNativeSeparators(pngName));
    if (Writer.format().isEmpty())
            Writer.setFormat("PNG");

    if (Writer.write(clippedImage)) {
        emit gui->messageSig(LOG_STATUS, QString("Clipped image saved '%1'")
                             .arg(clipMsg));
    } else {
        emit gui->messageSig(LOG_ERROR, QString("Failed to save clipped image '%1': %2")
                             .arg(clipMsg)
                             .arg(Writer.errorString()));
        return false;
    }
    return true;
 }

void Render::addArgument(
        QStringList   &arguments,
        const QString &newArg,
        const QString &argChk,
        const int      povGenerator,
        const int      additionalArgs) {

    if (!additionalArgs) {
        arguments.append(newArg);
        return;
    }

    auto isMatch = [&newArg] (QString &oldArg) {
        QStringList flags = QStringList()
            << "-ca" << "-cg"
                ;
        if (getRenderer() == RENDERER_LDGLITE) {
            flags
            << "-J" << "-j" << "-v" << "-o"
            << "-w"  << "-mF" << "-ldcF" << "-i2"
            << "-fh" << "-q"    << "-2g,2x"
            << "-N"  << "-2g" << "-l3"   << "-fe"
            << "-fr" << "-ff" << "-x"    << "-ms"
            << "-mS" << "-e"  << "-p"    << "-ld"
            << "-z"  << "-Z"  << "-cc"   << "-co"
            << "-cla"<< "-cu" << "-lc"   << "-lC"
               ;
        } else {
            flags
            << "-vv" << "-q" << "-qq"
               ;
        }
        for (int i = 0; i < flags.size(); i++){
            if (newArg.startsWith(flags[i]) && oldArg.startsWith(flags[i]))
               return true;
        }
        return false;
    };

    int insertIndex = -1;
    for (int i = 0; i < arguments.size(); i++) {
        if (arguments[i] != "" && arguments[i] != " ") {
            if (argChk.isEmpty()) {
                if (!(getRenderer() == RENDERER_POVRAY && !povGenerator)){
                    if (isMatch(arguments[i]) ||
                        arguments[i].startsWith(newArg.left(newArg.indexOf("=")))) {
                        insertIndex = arguments.indexOf(arguments[i]);
                        break;
                    }
                }
            } else
            if (arguments[i].contains(argChk)) {
                insertIndex = arguments.indexOf(arguments[i]);
                break;
            }
        }
    }

    if (insertIndex < 0) {
        arguments.append(newArg);
    } else {
        arguments.replace(insertIndex,newArg);
    }
}

// Shared calculations
float stdCameraDistance(Meta &meta, float scale) {
    float onexone;
    float factor;

    // Do the math in pixels
    onexone  = 20*meta.LPub.resolution.ldu(); // size of 1x1 in units
    onexone *= meta.LPub.resolution.value();  // size of 1x1 in pixels
    onexone *= scale;
    factor   = gui->pageSize(meta.LPub.page, 0)/onexone; // in pixels;
//#ifdef QT_DEBUG_MODE
//    logTrace() << "\n" << QString("DEBUG - STANDARD CAMERA DISTANCE")
//               << "\n" << QString("PI [4*atan(1.0)]                    : %1").arg(double(pi))
//               << "\n" << QString("LduDistance [10.0/tan(0.005*pi/180)]: %1").arg(double(LduDistance))
//               << "\n" << QString("Page Width [pixels]                 : %1").arg(gui->pageSize(meta.LPub.page, 0))
//               << "\n" << QString("Resolution [LDU]                    : %1").arg(QString::number(double(meta.LPub.resolution.ldu()), 'f' ,10))
//               << "\n" << QString("Resolution [pixels]                 : %1").arg(double(meta.LPub.resolution.value()))
//               << "\n" << QString("Scale                               : %1").arg(double(scale))
//               << "\n" << QString("1x1 [20*res.ldu*res.pix*scale]      : %1").arg(QString::number(double(onexone), 'f' ,10))
//               << "\n" << QString("Factor [Page size/OnexOne]          : %1").arg(QString::number(double(factor), 'f' ,10))
//               << "\n" << QString("Cam Distance [Factor*LduDistance]   : %1").arg(QString::number(double(factor*LduDistance), 'f' ,10))
//                  ;
//#endif
    return factor*LduDistance;
}

float Render::getPovrayRenderCameraDistance(const QString &cdKeys){
    enum Cda {
       K_IMAGEWIDTH = 0,
       K_IMAGEHEIGHT,
       K_MODELSCALE,
       K_RESOLUTION,
       K_RESOLUTIONTYPE
    };

    QStringList cdKey = cdKeys.split(" ");

#ifdef QT_DEBUG_MODE
    QString message = QString("DEBUG STRING IN - ResType: %1, Resolution: %2, Width: %3, Height: %4, Scale: %5")
            .arg(cdKey.at(K_RESOLUTIONTYPE) == "DPI" ? "DPI" : "DPCM")
            .arg(cdKey.at(K_RESOLUTION).toDouble())
            .arg(cdKey.at(K_IMAGEWIDTH).toDouble()).arg(cdKey.at(K_IMAGEHEIGHT).toDouble())
            .arg(cdKey.at(K_MODELSCALE).toDouble());
    emit gui->messageSig(LOG_TRACE, message);
#endif

    float scale = cdKey.at(K_MODELSCALE).toFloat();
    ResolutionType ResType = cdKey.at(K_RESOLUTIONTYPE) == "DPI" ? DPI : DPCM;

    Meta meta;
    meta.LPub.resolution.setValue(ResType,cdKey.at(K_RESOLUTION).toFloat());
    meta.LPub.page.size.setValuesPixels(cdKey.at(K_IMAGEWIDTH).toFloat(),cdKey.at(K_IMAGEHEIGHT).toFloat());

#ifdef QT_DEBUG_MODE
    message = QString("DEBUG META OUT - Resolution: %1, Width: %2, Height: %3")
            .arg(double(meta.LPub.resolution.value()))
            .arg(double(meta.LPub.page.size.value(0)))
            .arg(double(meta.LPub.page.size.value(1)));
    emit gui->messageSig(LOG_TRACE, message);
#endif

    // calculate LDView camera distance settings
    return stdCameraDistance(meta,scale);
}

const QStringList Render::getImageAttributes(const QString &nameKey)
{
    QFileInfo fileInfo(nameKey);
    QString cleanString = fileInfo.completeBaseName();
    if (Preferences::enableFadeSteps && nameKey.endsWith(FADE_SFX))
        cleanString.chop(QString(FADE_SFX).size());
    else
    if (Preferences::enableHighlightStep && nameKey.endsWith(HIGHLIGHT_SFX))
        cleanString.chop(QString(HIGHLIGHT_SFX).size());

    return cleanString.split("_");
}

bool Render::compareImageAttributes(
    const QStringList &attributes,
    const QString &compareKey,
    bool pare)
{
    bool result;
    QString message;
    QStringList attributesList = attributes;
    if (attributesList.size() >= nBaseAttributes) {
        if (!compareKey.endsWith("SUB") &&
             attributes.endsWith("SUB"))
           attributesList.removeLast();
        if (pare) {
            attributesList.removeAt(nResType);
            attributesList.removeAt(nResolution);
            attributesList.removeAt(nPageWidth);
            attributesList.removeAt(nColorCode);
            attributesList.removeAt(nType);
        }
        const QString attributesKey = attributesList.join("_");
        result = compareKey != attributesKey;
        if (Preferences::debugLogging) {
            message = QString("Attributes compare: [%1], attributesKey [%2], compareKey [%3]")
                              .arg(result ? "No Match - usingSnapshotArgs (attributes)" : "Match" )
                              .arg(attributesKey).arg(compareKey);
            gui->messageSig(LOG_DEBUG, message);
        }
    } else {
        result = false;
        message           = QString("Malformed image file attributes list [%1]")
                                    .arg(attributesList.join("_"));
        gui->messageSig(LOG_NOTICE, message);
    }
    return result;
}

bool Render::createSnapshotsList(
    const QStringList &ldrNames,
    const QString &SnapshotsList)
{
    QFile SnapshotsListFile(SnapshotsList);
    if ( ! SnapshotsListFile.open(QFile::WriteOnly | QFile::Text)) {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Failed to create LDView (Single Call) PLI Snapshots list file!"));
        return false;
    }

    QTextStream out(&SnapshotsListFile);

    for (int i = 0; i < ldrNames.size(); i++) {
        QString smLine = ldrNames[i];
        if (QFileInfo(smLine).exists()) {
            out << smLine << endl;
            if (Preferences::debugLogging)
                emit gui->messageSig(LOG_DEBUG, QString("Wrote %1 to PLI Snapshots list").arg(smLine));
        } else {
            emit gui->messageSig(LOG_ERROR, QString("Error %1 not written to Snapshots list - file does not exist").arg(smLine));
        }
    }
    SnapshotsListFile.close();
    return true;
}

int Render::executeLDViewProcess(QStringList &arguments, Options::Mt module) {

  QString message = QString("LDView %1 %2 Arguments: %3 %4")
                            .arg(useLDViewSCall() ? "(SingleCall)" : "(Default)")
                            .arg(module == Options::CSI ? "CSI" : "PLI")
                            .arg(Preferences::ldviewExe)
                            .arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
  qDebug() << qPrintable(message);
#else
  emit gui->messageSig(LOG_INFO, message);
#endif

  QProcess ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldview");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldview");

  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished(rendererTimeout())) {
      if (ldview.exitCode() != 0 || 1) {
          QByteArray status = ldview.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView %1 %2 render failed with code %2 %3")
                               .arg(useLDViewSCall() ? "(SingleCall)" : "(Default)")
                               .arg(module == Options::CSI ? "CSI" : "PLI")
                               .arg(ldview.exitCode())
                               .arg(str));
          return -1;
        }
    }

  bool usingInputFileList = false;
  foreach(QString argument, arguments){
      if (argument.startsWith("-CommandLinesList=") ||
          argument.startsWith("-SaveSnapshotsList=")) {
          usingInputFileList = true;
          break;
      }
  }

  if (!usingInputFileList) {
      QFile outputImageFile(arguments.last());
      if (! outputImageFile.exists()) {
          emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView %1 image generation failed for %2 with message %3")
                               .arg(module == Options::CSI ? "CSI" : "PLI")
                               .arg(outputImageFile.fileName())
                               .arg(outputImageFile.errorString()));
          return -1;
      }
  }

  return 0;
}

/***************************************************************************
 *
 * The math for zoom factor.  1.0 is true size.
 *
 * 1 LDU is 1/64 of an inch
 *
 * LDGLite produces 72 DPI
 *
 * Camera default FoV angle is 0.01
 *
 * What distance do we need to put the camera, given a user chosen DPI,
 * to get zoom factor of 1.0?
 *
 **************************************************************************/


/***************************************************************************
 *
 * POVRay renderer
 *
 **************************************************************************/

float POVRay::cameraDistance(
    Meta &meta,
    float scale)
{
  return stdCameraDistance(meta, scale)*0.455f;
}

int POVRay::renderCsi(
    const QString     &addLine,
    const QStringList &csiParts,
    const QStringList &csiKeys,
    const QString     &pngName,
    Meta              &meta,
    int                nType)
{
  Q_UNUSED(csiKeys)
  Q_UNUSED(nType)

  /* Create the CSI DAT file */
  QString message;
  QString ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
  QString povName = ldrName + ".pov";

  // RotateParts #2 - 8 parms
  int rc;
  if ((rc = rotateParts(addLine, meta.rotStep, csiParts, ldrName, QString(),meta.LPub.assem.cameraAngles,false/*ldv*/,Options::Mt::CSI)) < 0) {
      return rc;
   }

  // Populate render attributes
  QStringList ldviewParmslist = meta.LPub.assem.ldviewParms.value().split(' ');
  QString transform  = meta.rotStep.value().type;
  bool noCA          = Preferences::applyCALocally || transform == "ABS";
  bool pp            = Preferences::perspectiveProjection;
  int studLogo       = meta.LPub.assem.studLogo.value() ? meta.LPub.assem.studLogo.value() : -1;
  float modelScale   = meta.LPub.assem.modelScale.value();
  float cameraFoV    = meta.LPub.assem.cameraFoV.value();
  float cameraAngleX = noCA ? 0.0f : meta.LPub.assem.cameraAngles.value(0);
  float cameraAngleY = noCA ? 0.0f : meta.LPub.assem.cameraAngles.value(1);
  xyzVector target   = xyzVector(meta.LPub.assem.target.x(),meta.LPub.assem.target.y(),meta.LPub.assem.target.z());

  /* determine camera distance */
  int cd = int(meta.LPub.assem.cameraDistance.value());
  if (!cd)
      cd = int(cameraDistance(meta,modelScale)*1700/1000);

  // set page size
  bool useImageSize = meta.LPub.assem.imageSize.value(0) > 0;
  int width  = useImageSize ? int(meta.LPub.assem.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  int height = useImageSize ? int(meta.LPub.assem.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

  // projection settings
  QString CA, cg;

  // parameter arguments;
  QStringList parmsArgs;

  auto getRendererSettings = [
          &pp,
          &cd,
          &target,
          &modelScale,
          &cameraFoV,
          &cameraAngleX,
          &cameraAngleY,
          &ldviewParmslist,
          &useImageSize] (
      QString     &CA,
      QString     &cg,
      QStringList &parmsArgs)
  {
      // additional LDView parameters;
      qreal cdf = LP3D_CDF;
      QString dz, dl, df = QString("-FOV=%1").arg(double(cameraFoV));
      bool pd = false, pl = false, pf = false, pz = false;
      for (int i = 0; i < ldviewParmslist.size(); i++) {
          if (ldviewParmslist[i] != "" && ldviewParmslist[i] != " ") {
              if (pp) {
                  if ((pl = ldviewParmslist[i].contains("-DefaultLatLong=")))
                       dl = ldviewParmslist[i];
                  if ((pz = ldviewParmslist[i].contains("-DefaultZoom=")))
                       dz = ldviewParmslist[i];
                  if ((pf = ldviewParmslist[i].contains("-FOV=")))
                       df = ldviewParmslist[i];
                  if ((pd = ldviewParmslist[i].contains("-LDViewPerspectiveDistanceFactor="))) {
                      bool ok;
                      qreal _cdf = QStringList(ldviewParmslist[i].split("=")).last().toDouble(&ok);
                      if (ok && _cdf < LP3D_CDF)
                          cdf = _cdf;
                  }
              }
              if (!pd && !pl && !pf && !pz) {
                addArgument(parmsArgs, ldviewParmslist[i]);    // 10. ldviewParms [usually empty]
              }
          }
      }

      // Set camera angle and camera globe and update arguments with perspective projection settings
      if (pp && pl && !pz)
          dz = QString("-DefaultZoom=%1").arg(double(modelScale));

      CA = pp ? df : QString("-ca%1") .arg(LP3D_CA);              // replace CA with FOV

      // Set alternate target position or use specified image size
      QString _mc;
      if (target.isPopulated())
          _mc = QString("-ModelCenter=%1,%2,%3 ").arg(double(target.x)).arg(double(target.y)).arg(double(target.z));
      if ((!_mc.isEmpty() && !pl) || (useImageSize && _mc.isEmpty())){
          // Set model center
          QString _dl = QString("-DefaultLatLong=%1,%2")
                                .arg(double(cameraAngleX))
                                .arg(double(cameraAngleY));
          QString _dz = QString("-DefaultZoom=%1").arg(double(modelScale));
          // Set zoom to fit when use image size specified
          QString _sz;
          if (useImageSize && _mc.isEmpty())
              _sz = QString(" -SaveZoomToFit=1");
          cg = QString("%1%2 %3%4").arg(_mc.isEmpty() ? "" : _mc).arg(_dl).arg(_dz).arg(_sz.isEmpty() ? "" : _sz);
      } else {
          cg = pp ? pl ? QString("-DefaultLatLong=%1 %2")
                                 .arg(dl)
                                 .arg(dz)                         // replace Camera Globe with DefaultLatLon and add DefaultZoom
                       : QString("-cg%1,%2,%3")
                                 .arg(double(cameraAngleX))
                                 .arg(double(cameraAngleY))
                                 .arg(QString::number(cd * cdf,'f',0) )
                  : QString("-cg%1,%2,%3")
                            .arg(double(cameraAngleX))
                            .arg(double(cameraAngleY))
                            .arg(cd);
      }

      // additional LDView parameters;
      if (parmsArgs.size()){
          emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDView additional POV-Ray PLI renderer parameters: %1")
                                                        .arg(parmsArgs.join(" ")));
          cg.append(QString(" %1").arg(parmsArgs.join(" ")));
      }
  };

  getRendererSettings(CA, cg, parmsArgs);

  QString s  = studLogo >= 0 ? QString("-StudLogo=%1").arg(studLogo) : "";
  QString w  = QString("-SaveWidth=%1") .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-ExportFile=%1") .arg(povName);
  QString l  = QString("-LDrawDir=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldrawLibPath)));
  QString o  = QString("-HaveStdOut=1");
  QString v  = QString("-vv");

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << s;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;

  if (!Preferences::altLDConfigPath.isEmpty()) {
     QString altldc = "-LDConfig=" + Preferences::altLDConfigPath;
     addArgument(arguments, altldc, "-LDConfig", 0, parmsArgs.size());
  }

  // LDView block begin
  if (Preferences::povFileGenerator == RENDERER_LDVIEW) {

      addArgument(arguments, o, "-HaveStdOut", 0, parmsArgs.size());
      addArgument(arguments, v, "-vv", 0, parmsArgs.size());

//      if (Preferences::enableFadeSteps)
//        arguments <<  QString("-SaveZMap=1");

      bool hasLDViewIni = Preferences::ldviewPOVIni != "";
      if(hasLDViewIni){
          QString ini  = QString("-IniFile=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldviewPOVIni)));
          addArgument(arguments, ini, "-IniFile", 0, parmsArgs.size());
        }

      arguments << QDir::toNativeSeparators(ldrName);

      removeEmptyStrings(arguments);

      emit gui->messageSig(LOG_STATUS, "LDView POV CSI file generation...");

      QProcess    ldview;
      ldview.setEnvironment(QProcess::systemEnvironment());
      ldview.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
      ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldviewpov");
      ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldviewpov");

      message = QString("LDView POV file generate CSI Arguments: %1 %2").arg(Preferences::ldviewExe).arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
      qDebug() << qPrintable(message);
#else
      emit gui->messageSig(LOG_INFO, message);
#endif

      ldview.start(Preferences::ldviewExe,arguments);
      if ( ! ldview.waitForFinished(rendererTimeout())) {
          if (ldview.exitCode() != 0 || 1) {
              QByteArray status = ldview.readAll();
              QString str;
              str.append(status);
              emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView POV file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
              return -1;
          }
      }
  }
  else
  // Native POV Generator block
  if (Preferences::povFileGenerator == RENDERER_NATIVE) {

      QString workingDirectory = QDir::currentPath();

      arguments << QDir::toNativeSeparators(ldrName);

      removeEmptyStrings(arguments);

      emit gui->messageSig(LOG_STATUS, "Native POV CSI file generation...");

      bool retError = false;
      ldvWidget = new LDVWidget(nullptr,NativePOVIni,true);
      if (! ldvWidget->doCommand(arguments))  {
          emit gui->messageSig(LOG_ERROR, QString("Failed to generate CSI POV file for command: %1").arg(arguments.join(" ")));
          retError = true;
      }

      // ldvWidget changes the Working directory so we must reset
      if (! QDir::setCurrent(workingDirectory)) {
          emit gui->messageSig(LOG_ERROR, QString("Failed to restore CSI POV working directory %1").arg(workingDirectory));
          retError = true;
      }
      if (retError)
          return -1;
  }

  QStringList povArguments;
  if (Preferences::povrayDisplay){
      povArguments << QString("+d");
  } else {
      povArguments << QString("-d");
  }

  QString O = QString("+O\"%1\"").arg(QDir::toNativeSeparators(pngName));
  QString W = QString("+W%1").arg(width);
  QString H = QString("+H%1").arg(height);

  povArguments << O;
  povArguments << W;
  povArguments << H;
  povArguments << USE_ALPHA;
  povArguments << getPovrayRenderQuality();

  bool hasSTL       = Preferences::lgeoStlLib;
  bool hasLGEO      = Preferences::lgeoPath != "";
  bool hasPOVRayIni = Preferences::povrayIniPath != "";
  bool hasPOVRayInc = Preferences::povrayIncPath != "";

  if(hasPOVRayInc){
      QString povinc = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIncPath)));
      povArguments << povinc;
  }
  if(hasPOVRayIni){
      QString povini = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIniPath)));
      povArguments << povini;
  }
  if(hasLGEO){
      QString lgeoLg = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg")));
      QString lgeoAr = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar")));
      povArguments << lgeoLg;
      povArguments << lgeoAr;
      if (hasSTL){
          QString lgeoStl = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/stl")));
          povArguments << lgeoStl;
      }
  }

  QString I = QString("+I\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(povName)));
  povArguments.insert(2,I);

  parmsArgs = meta.LPub.assem.povrayParms.value().split(' ');
  for (int i = 0; i < parmsArgs.size(); i++) {
      if (parmsArgs[i] != "" && parmsArgs[i] != " ") {
          addArgument(povArguments, parmsArgs[i], QString(), true);
        }
    }
  if (parmsArgs.size())
      emit gui->messageSig(LOG_INFO,QMessageBox::tr("POV-Ray additional CSI renderer parameters: %1")
                           .arg(parmsArgs.join(" ")));

//#ifndef __APPLE__
//  povArguments << "/EXIT";
//#endif

  removeEmptyStrings(povArguments);

  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing POVRay %1 CSI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  QProcess povray;
  QStringList povEnv = QProcess::systemEnvironment();
  povEnv.prepend("POV_IGNORE_SYSCONF_MSG=1");
  povray.setEnvironment(povEnv);
  povray.setWorkingDirectory(QDir::currentPath()+ "/" + Paths::assemDir); // pov win console app will not write to dir different from cwd or source file dir
  povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
  povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");

  message = QString("POVRay CSI Arguments: %1 %2").arg(Preferences::povrayExe).arg(povArguments.join(" "));
#ifdef QT_DEBUG_MODE
  qDebug() << qPrintable(message);
#else
  emit gui->messageSig(LOG_INFO, message);
#endif

  povray.start(Preferences::povrayExe,povArguments);
  if ( ! povray.waitForFinished(rendererTimeout())) {
      if (povray.exitCode() != 0) {
          QByteArray status = povray.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(LOG_ERROR,QMessageBox::tr("POVRay CSI render failed with code %1\n%2").arg(povray.exitCode()) .arg(str));
          return -1;
        }
    }

  if (clipImage(pngName))
    return 0;
  else
    return -1;
}

int POVRay::renderPli(
    const QStringList &ldrNames ,
    const QString     &pngName,
    Meta    	      &meta,
    int                pliType,
    int                keySub)
{
  // Select meta type
  PliMeta &metaType = pliType == SUBMODEL ? static_cast<PliMeta&>(meta.LPub.subModel) :
                      pliType == BOM ? meta.LPub.bom : meta.LPub.pli;

  // test ldrNames
  QFileInfo fileInfo(ldrNames.first());
  if ( ! fileInfo.exists()) {
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("POV PLI render input file was not found at the specified path [%1]")
                                                    .arg(ldrNames.first()));
    return -1;
  }

  //  QStringList list;
  QString message;
  QString povName = ldrNames.first() +".pov";

  // Populate render attributes
  QStringList ldviewParmslist = metaType.ldviewParms.value().split(' ');
  QString transform  = metaType.rotStep.value().type;
  bool noCA          = pliType == SUBMODEL ? Preferences::applyCALocally || transform == "ABS" : transform == "ABS";
  bool pp            = Preferences::perspectiveProjection;
  int studLogo       = metaType.studLogo.value() ? metaType.studLogo.value() : -1;
  float modelScale   = metaType.modelScale.value();
  float cameraFoV    = metaType.cameraFoV.value();
  float cameraAngleX = noCA ? 0.0f : metaType.cameraAngles.value(0);
  float cameraAngleY = noCA ? 0.0f : metaType.cameraAngles.value(1);
  xyzVector target   = xyzVector(metaType.target.x(),metaType.target.y(),metaType.target.z());

  /* determine camera distance */
  int cd = int(metaType.cameraDistance.value());
  if (!cd)
      cd = int(cameraDistance(meta,modelScale)*1700/1000);

  //qDebug() << "LDView (Default) Camera Distance: " << cd;

  // set page size
  bool useImageSize = metaType.imageSize.value(0) > 0;
  int width  = useImageSize ? int(metaType.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  int height = useImageSize ? int(metaType.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

  // projection settings
  QString CA, cg;

  // parameter arguments;
  QStringList parmsArgs;

  auto getRendererSettings = [
          &keySub,
          &pp,
          &cd,
          &pngName,
          &target,
          &modelScale,
          &cameraFoV,
          &cameraAngleX,
          &cameraAngleY,
          &ldviewParmslist,
          &useImageSize] (
      QString     &CA,
      QString     &cg,
      bool        &noCA,
      QStringList &parmsArgs)
  {
      // additional LDView parameters;
      qreal cdf = LP3D_CDF;
      QString dz, dl, df = QString("-FOV=%1").arg(double(cameraFoV));
      bool pd = false, pl = false, pf = false, pz = false;
      for (int i = 0; i < ldviewParmslist.size(); i++) {
          if (ldviewParmslist[i] != "" && ldviewParmslist[i] != " ") {
              if (pp) {
                  if ((pl = ldviewParmslist[i].contains("-DefaultLatLong=")))
                       dl = ldviewParmslist[i];
                  if ((pz = ldviewParmslist[i].contains("-DefaultZoom=")))
                       dz = ldviewParmslist[i];
                  if ((pf = ldviewParmslist[i].contains("-FOV=")))
                       df = ldviewParmslist[i];
                  if ((pd = ldviewParmslist[i].contains("-LDViewPerspectiveDistanceFactor="))) {
                      bool ok;
                      qreal _cdf = QStringList(ldviewParmslist[i].split("=")).last().toDouble(&ok);
                      if (ok && _cdf < LP3D_CDF)
                          cdf = _cdf;
                  }
              }
              if (!pd && !pl && !pf && !pz) {
                addArgument(parmsArgs, ldviewParmslist[i]);    // 10. ldviewParms [usually empty]
              }
          }
      }

      // Set camera angle and camera globe and update arguments with perspective projection settings
      if (pp && pl && !pz)
          dz = QString("-DefaultZoom=%1").arg(double(modelScale));

      // Process substitute part attributes
      if (keySub) {
        QStringList attributes = getImageAttributes(pngName);
        bool hr;
        if ((hr = attributes.size() == nHasRotstep) || attributes.size() == nHasTargetAndRotstep)
          noCA = attributes.at(hr ? nRotTrans : nRot_Trans) == "ABS";
        if (attributes.size() >= nHasTarget)
          target = xyzVector(attributes.at(nTargetX).toFloat(),attributes.at(nTargetY).toFloat(),attributes.at(nTargetZ).toFloat());
        if (keySub > PliBeginSub2Rc) {
          modelScale   = attributes.at(nModelScale).toFloat();
          cameraFoV    = attributes.at(nCameraFoV).toFloat();
          cameraAngleX = attributes.at(nCameraAngleXX).toFloat();
          cameraAngleY = attributes.at(nCameraAngleYY).toFloat();
        }
      }

      CA = pp ? df : QString("-ca%1") .arg(LP3D_CA);              // replace CA with FOV

      // Set alternate target position or use specified image size
      QString _mc;
      if (target.isPopulated())
          _mc = QString("-ModelCenter=%1,%2,%3 ").arg(double(target.x)).arg(double(target.y)).arg(double(target.z));
      if ((!_mc.isEmpty() && !pl) || (useImageSize && _mc.isEmpty())){
          // Set model center
          QString _dl = QString("-DefaultLatLong=%1,%2")
                                .arg(double(cameraAngleX))
                                .arg(double(cameraAngleY));
          QString _dz = QString("-DefaultZoom=%1").arg(double(modelScale));
          // Set zoom to fit when use image size specified
          QString _sz;
          if (useImageSize && _mc.isEmpty())
              _sz = QString(" -SaveZoomToFit=1");
          cg = QString("%1%2 %3%4").arg(_mc.isEmpty() ? "" : _mc).arg(_dl).arg(_dz).arg(_sz.isEmpty() ? "" : _sz);
      } else {
          cg = pp ? pl ? QString("-DefaultLatLong=%1 %2")
                                 .arg(dl)
                                 .arg(dz)                         // replace Camera Globe with DefaultLatLon and add DefaultZoom
                       : QString("-cg%1,%2,%3")
                                 .arg(double(cameraAngleX))
                                 .arg(double(cameraAngleY))
                                 .arg(QString::number(cd * cdf,'f',0) )
                  : QString("-cg%1,%2,%3")
                            .arg(double(cameraAngleX))
                            .arg(double(cameraAngleY))
                            .arg(cd);
      }

      // additional LDView parameters;
      if (parmsArgs.size()){
          emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDView additional POV-Ray PLI renderer parameters: %1")
                                                        .arg(parmsArgs.join(" ")));
          cg.append(QString(" %1").arg(parmsArgs.join(" ")));
      }
  };

  getRendererSettings(CA, cg, noCA, parmsArgs);

  QString s  = studLogo >= 0 ? QString("-StudLogo=%1").arg(studLogo) : "";
  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-ExportFile=%1") .arg(povName);  // -ExportSuffix not required
  QString l  = QString("-LDrawDir=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldrawLibPath)));
  QString o  = QString("-HaveStdOut=1");
  QString v  = QString("-vv");

  QStringList arguments;
  arguments << CA;             // Camera Angle (i.e. Field of Veiw)
  arguments << cg.split(" ");  // Camera Globe, Target and Additional Parameters when specified
  arguments << s;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;

  if (!Preferences::altLDConfigPath.isEmpty()) {
     QString altldc = "-LDConfig=" + Preferences::altLDConfigPath;
     addArgument(arguments, altldc, "-LDConfig", 0, parmsArgs.size());
  }

  // LDView block begin
  if (Preferences::povFileGenerator == RENDERER_LDVIEW) {

      addArgument(arguments, o, "-HaveStdOut", 0, parmsArgs.size());
      addArgument(arguments, v, "-vv", 0, parmsArgs.size());

      bool hasLDViewIni = Preferences::ldviewPOVIni != "";
      if(hasLDViewIni){
          QString ini  = QString("-IniFile=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldviewPOVIni)));
          addArgument(arguments, ini, "-IniFile", 0, parmsArgs.size());
        }

      arguments << QDir::toNativeSeparators(ldrNames.first());

      removeEmptyStrings(arguments);

      emit gui->messageSig(LOG_STATUS, "LDView POV PLI file generation...");

      QProcess    ldview;
      ldview.setEnvironment(QProcess::systemEnvironment());
      ldview.setWorkingDirectory(QDir::currentPath());
      ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldviewpov");
      ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldviewpov");

      message = QString("LDView POV file generate PLI Arguments: %1 %2").arg(Preferences::ldviewExe).arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
      qDebug() << qPrintable(message);
#else
      emit gui->messageSig(LOG_INFO, message);
#endif

      ldview.start(Preferences::ldviewExe,arguments);
      if ( ! ldview.waitForFinished()) {
          if (ldview.exitCode() != 0) {
              QByteArray status = ldview.readAll();
              QString str;
              str.append(status);
              emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView POV file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
              return -1;
          }
      }
  }
  else
  // Native POV Generator block
  if (Preferences::povFileGenerator == RENDERER_NATIVE) {

      QString workingDirectory = QDir::currentPath();

      arguments << QDir::toNativeSeparators(ldrNames.first());

      removeEmptyStrings(arguments);

      emit gui->messageSig(LOG_STATUS, "Native POV PLI file generation...");

      bool retError = false;
      ldvWidget = new LDVWidget(nullptr,NativePOVIni,true);
      if (! ldvWidget->doCommand(arguments)) {
          emit gui->messageSig(LOG_ERROR, QString("Failed to generate PLI POV file for command: %1").arg(arguments.join(" ")));
          retError = true;
      }

      // ldvWidget changes the Working directory so we must reset
      if (! QDir::setCurrent(workingDirectory)) {
          emit gui->messageSig(LOG_ERROR, QString("Failed to restore PLI POV working directory %1").arg(workingDirectory));
          retError = true;
      }
      if (retError)
        return -1;
  }

  QStringList povArguments;
  if (Preferences::povrayDisplay){
      povArguments << QString("+d");
  } else {
      povArguments << QString("-d");
  }

  QString O = QString("+O\"%1\"").arg(QDir::toNativeSeparators(pngName));
  QString W = QString("+W%1").arg(width);
  QString H = QString("+H%1").arg(height);

  povArguments << O;
  povArguments << W;
  povArguments << H;
  povArguments << USE_ALPHA;
  povArguments << getPovrayRenderQuality();

  bool hasSTL       = Preferences::lgeoStlLib;
  bool hasLGEO      = Preferences::lgeoPath != "";
  bool hasPOVRayIni = Preferences::povrayIniPath != "";
  bool hasPOVRayInc = Preferences::povrayIncPath != "";

  if(hasPOVRayInc){
      QString povinc = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIncPath)));
      povArguments << povinc;
  }
  if(hasPOVRayIni){
      QString povini = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIniPath)));
      povArguments << povini;
  }
  if(hasLGEO){
      QString lgeoLg = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg")));
      QString lgeoAr = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar")));
      povArguments << lgeoLg;
      povArguments << lgeoAr;
      if (hasSTL){
          QString lgeoStl = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/stl")));
          povArguments << lgeoStl;
        }
    }

  QString I = QString("+I\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(povName)));
  povArguments.insert(2,I);

  parmsArgs = meta.LPub.assem.povrayParms.value().split(' ');
  for (int i = 0; i < parmsArgs.size(); i++) {
      if (parmsArgs[i] != "" && parmsArgs[i] != " ") {
          addArgument(povArguments, parmsArgs[i], QString(), true);
        }
    }
  if (parmsArgs.size())
      emit gui->messageSig(LOG_INFO,QMessageBox::tr("POV-Ray additional PLI renderer parameters: %1")
                           .arg(parmsArgs.join(" ")));

//#ifndef __APPLE__
//  povArguments << "/EXIT";
//#endif

  removeEmptyStrings(povArguments);

  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing POVRay %1 PLI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  QProcess povray;
  QStringList povEnv = QProcess::systemEnvironment();
  povEnv.prepend("POV_IGNORE_SYSCONF_MSG=1");
  QString workingDirectory = pliType == SUBMODEL ? Paths::submodelDir : Paths::partsDir;
  povray.setEnvironment(povEnv);
  povray.setWorkingDirectory(QDir::currentPath()+ "/" + workingDirectory); // pov win console app will not write to dir different from cwd or source file dir
  povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
  povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");

  message = QString("POVRay PLI Arguments: %1 %2").arg(Preferences::povrayExe).arg(povArguments.join(" "));
#ifdef QT_DEBUG_MODE
  //qDebug() << qPrintable(message);
  emit gui->messageSig(LOG_DEBUG, message);
#else
  emit gui->messageSig(LOG_INFO, message);
#endif

  povray.start(Preferences::povrayExe, povArguments);
  if ( ! povray.waitForFinished(rendererTimeout())) {
      if (povray.exitCode() != 0) {
          QByteArray status = povray.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(LOG_ERROR,QMessageBox::tr("POVRay PLI render failed with code %1\n%2") .arg(povray.exitCode()) .arg(str));
          return -1;
      }
  }

  if (clipImage(pngName))
    return 0;
  else
    return -1;
}


/***************************************************************************
 *
 * LDGLite renderer
 *
 **************************************************************************/

float LDGLite::cameraDistance(
  Meta &meta,
  float scale)
{
    return stdCameraDistance(meta,scale);
}

int LDGLite::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QStringList &csiKeys,
  const QString     &pngName,
        Meta        &meta,
  int                nType)
{
  Q_UNUSED(csiKeys)
  Q_UNUSED(nType)

  /* Create the CSI DAT file */
  QString ldrPath, ldrName, ldrFile;
  int rc;
  ldrName = "csi.ldr";
  ldrPath = QDir::currentPath() + "/" + Paths::tmpDir;
  ldrFile = ldrPath + "/" + ldrName;
  // RotateParts #2 - 8 parms
  if ((rc = rotateParts(addLine, meta.rotStep, csiParts, ldrFile,QString(),meta.LPub.assem.cameraAngles,false/*ldv*/,Options::Mt::CSI)) < 0) {
     return rc;
  }

  /* determine camera distance */
  int cd = int(meta.LPub.assem.cameraDistance.value());
  if (!cd)
      cd = int(cameraDistance(meta,meta.LPub.assem.modelScale.value()));

  /* apply camera angle */

  bool noCA  = Preferences::applyCALocally;
  bool pp    = Preferences::perspectiveProjection;

  bool useImageSize = meta.LPub.assem.imageSize.value(0) > 0;
  int width  = useImageSize ? int(meta.LPub.assem.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  int height = useImageSize ? int(meta.LPub.assem.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);

  int lineThickness = int(resolution()/150+0.5f);
  if (lineThickness == 0) {
    lineThickness = 1;
  }
  QString w  = QString("-W%1")      .arg(lineThickness); // ldglite always deals in 72 DPI

  QString CA = QString("-ca%1") .arg(pp ? double(meta.LPub.assem.cameraFoV.value()) : LP3D_CA);

  QString cg;
  if (meta.LPub.assem.target.isPopulated()){
      cg = QString("-co%1,%2,%3")
               .arg(double(meta.LPub.assem.target.x()))
               .arg(double(meta.LPub.assem.target.y()))
               .arg(double(meta.LPub.assem.target.z()));
  } else {
      cg = QString("-cg%1,%2,%3") .arg(noCA ? 0.0 : double(meta.LPub.assem.cameraAngles.value(0)))
                                  .arg(noCA ? 0.0 : double(meta.LPub.assem.cameraAngles.value(1)))
                                  .arg(cd);
  }

  QString s  = meta.LPub.assem.studLogo.value() ?
                         QString("-sl%1")
                                 .arg(meta.LPub.assem.studLogo.value()) : QString();

  QString J  = QString("-%1").arg(pp ? "J" : "j");

  QStringList arguments;
  arguments << CA;                  // camera FOV in degrees
  arguments << cg;                  // camera globe - scale factor or model origin for the camera to look at
  arguments << J;                   // projection
  arguments << v;                   // display in X wide by Y high window
  arguments << o;                   // changes the centre X across and Y down
  arguments << w;                   // line thickness
  arguments << s;                   // stud logo

  QStringList list;
  // First, load parms from meta if any
  list = meta.LPub.assem.ldgliteParms.value().split(' ');
  for (int i = 0; i < list.size(); i++) {
     if (list[i] != "" && list[i] != " ") {
         addArgument(arguments, list[i]);
      }
  }
  if (list.size())
      emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDGlite additional CSI renderer parameters: %1") .arg(list.join(" ")));

  // Add ini arguments if not already in additional parameters
  for (int i = 0; i < Preferences::ldgliteParms.size(); i++) {
      if (list.indexOf(QRegExp("^" + QRegExp::escape(Preferences::ldgliteParms[i]))) < 0) {
        addArgument(arguments, Preferences::ldgliteParms[i], "", 0, list.size());
      }
  }

  // Add custom color file if exist
  if (!Preferences::altLDConfigPath.isEmpty()) {
    addArgument(arguments, QString("-ldcF%1").arg(Preferences::altLDConfigPath), "-ldcF", 0, list.size());
  }

  arguments << QDir::toNativeSeparators(mf);                  // .png file name
  arguments << QDir::toNativeSeparators(ldrFile);             // csi.ldr (input file)

  removeEmptyStrings(arguments);

  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing LDGLite %1 CSI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawLibPath;
  //emit gui->messageSig(LOG_DEBUG,qPrintable("LDRAWDIR=" + Preferences::ldrawLibPath));

  if (!Preferences::ldgliteSearchDirs.isEmpty()) {
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;
    //emit gui->messageSig(LOG_DEBUG,qPrintable("LDSEARCHDIRS: " + Preferences::ldgliteSearchDirs));
  }

  ldglite.setEnvironment(env);
  //emit gui->messageSig(LOG_DEBUG,qPrintable("ENV: " + env.join(" ")));

  ldglite.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr-ldglite");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout-ldglite");

  QString message = QString("LDGLite CSI Arguments: %1 %2").arg(Preferences::ldgliteExe).arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
  qDebug() << qPrintable(message);
#else
  emit gui->messageSig(LOG_INFO, message);
#endif

  ldglite.start(Preferences::ldgliteExe,arguments);
  if ( ! ldglite.waitForFinished(rendererTimeout())) {
    if (ldglite.exitCode() != 0) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }

  QFile outputImageFile(pngName);
  if (! outputImageFile.exists()) {
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDGLite CSI image generation failed for %1 with message %2")
                           .arg(outputImageFile.fileName()).arg(outputImageFile.errorString()));
      return -1;
  }

  return 0;
}


int LDGLite::renderPli(
  const QStringList &ldrNames,
  const QString     &pngName,
  Meta              &meta,
  int                pliType,
  int                keySub)
{
  // Select meta type
  PliMeta &metaType = pliType == SUBMODEL ? static_cast<PliMeta&>(meta.LPub.subModel) :
                      pliType == BOM ? meta.LPub.bom : meta.LPub.pli;

  // Populate render attributes
  QString transform  = metaType.rotStep.value().type;
  bool  noCA         = transform  == "ABS";
  bool pp            = Preferences::perspectiveProjection;
  float modelScale   = metaType.modelScale.value();
  float cameraFoV    = metaType.cameraFoV.value();
  float cameraAngleX = metaType.cameraAngles.value(0);
  float cameraAngleY = metaType.cameraAngles.value(1);
  xyzVector target   = xyzVector(metaType.target.x(),metaType.target.y(),metaType.target.z());

  // Process substitute part attributes
  if (keySub) {
    QStringList attributes = getImageAttributes(pngName);
    bool hr;
    if ((hr = attributes.size() == nHasRotstep) || attributes.size() == nHasTargetAndRotstep)
      noCA = attributes.at(hr ? nRotTrans : nRot_Trans) == "ABS";
    if (attributes.size() >= nHasTarget)
      target = xyzVector(attributes.at(nTargetX).toFloat(),attributes.at(nTargetY).toFloat(),attributes.at(nTargetZ).toFloat());
    if (keySub > PliBeginSub2Rc) {
      modelScale   = attributes.at(nModelScale).toFloat();
      cameraFoV    = attributes.at(nCameraFoV).toFloat();
      cameraAngleX = attributes.at(nCameraAngleXX).toFloat();
      cameraAngleY = attributes.at(nCameraAngleYY).toFloat();
    }
  }

  /* determine camera distance */
  int cd = int(metaType.cameraDistance.value());
  if (!cd)
      cd = int(cameraDistance(meta,modelScale));

  bool useImageSize = metaType.imageSize.value(0) > 0;
  int width  = useImageSize ? int(metaType.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  int height = useImageSize ? int(metaType.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

  int lineThickness = int(double(resolution())/72.0+0.5);

  if (pliType == SUBMODEL)
      noCA   = Preferences::applyCALocally || noCA;

  QString CA = QString("-ca%1") .arg(double(cameraFoV));

  QString cg;
  if (target.isPopulated()) {
      cg = QString("-co%1,%2,%3")
                   .arg(double(target.x))
                   .arg(double(target.y))
                   .arg(double(target.z));
  } else {
      cg = QString("-cg%1,%2,%3") .arg(noCA ? 0.0 : double(cameraAngleX))
                                  .arg(noCA ? 0.0 : double(cameraAngleY))
                                  .arg(cd);
  }

  QString J  = QString("-J");
  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
  QString w  = QString("-W%1")      .arg(lineThickness);  // ldglite always deals in 72 DPI

  QString s  = metaType.studLogo.value() ?
                         QString("-sl%1")
                                 .arg(metaType.studLogo.value()) : QString();

  QStringList arguments;
  arguments << CA;                  // Camera FOV in degrees
  arguments << cg;                  // camera globe - scale factor or model origin for the camera to look at
  arguments << J;                   // Perspective projection
  arguments << v;                   // display in X wide by Y high window
  arguments << o;                   // changes the centre X across and Y down
  arguments << w;                   // line thickness
  arguments << s ;                  // stud logo

  QStringList list;
  // First, load additional parms from meta if any
  list = metaType.ldgliteParms.value().split(' ');
  for (int i = 0; i < list.size(); i++) {
     if (list[i] != "" && list[i] != " ") {
         addArgument(arguments, list[i]);
      }
  }
  if (list.size())
      emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDGlite additional PLI renderer parameters %1") .arg(list.join(" ")));

  // Add ini parms if not already added from meta
  for (int i = 0; i < Preferences::ldgliteParms.size(); i++) {
      if (list.indexOf(QRegExp("^" + QRegExp::escape(Preferences::ldgliteParms[i]))) < 0) {
        addArgument(arguments, Preferences::ldgliteParms[i], "", 0, list.size());
      }
  }

  // add custom color file if exist
  if (!Preferences::altLDConfigPath.isEmpty()) {
    addArgument(arguments, QString("-ldcF%1").arg(Preferences::altLDConfigPath), "-ldcF", 0, list.size());
  }

  arguments << QDir::toNativeSeparators(mf);
  arguments << QDir::toNativeSeparators(ldrNames.first());

  removeEmptyStrings(arguments);

  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing LDGLite %1 PLI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawLibPath;
  //emit gui->messageSig(LOG_DEBUG,qPrintable("LDRAWDIR=" + Preferences::ldrawLibPath));

  if (!Preferences::ldgliteSearchDirs.isEmpty()){
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;
    //emit gui->messageSig(LOG_DEBUG,qPrintable("LDSEARCHDIRS: " + Preferences::ldgliteSearchDirs));
  }

  ldglite.setEnvironment(env);
  ldglite.setWorkingDirectory(QDir::currentPath());
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr-ldglite");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout-ldglite");

  QString message = QString("LDGLite PLI Arguments: %1 %2").arg(Preferences::ldgliteExe).arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
  qDebug() << qPrintable(message);
#else
  emit gui->messageSig(LOG_INFO, message);
#endif

  ldglite.start(Preferences::ldgliteExe,arguments);
  if (! ldglite.waitForFinished(rendererTimeout())) {
    if (ldglite.exitCode()) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }

  QFile outputImageFile(pngName);
  if (! outputImageFile.exists()) {
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDGLite PLI image generation failed for %1 with message %2")
                           .arg(outputImageFile.fileName()).arg(outputImageFile.errorString()));
      return -1;
  }

  return 0;
}


/***************************************************************************
 *
 * LDView renderer
 *                                  6x6                    5990
 *      LDView               LDView    LDGLite       LDView
 * 0.1    8x5     8x6         32x14    40x19  0.25  216x150    276x191  0.28
 * 0.2   14x10   16x10                              430x298    552x381
 * 0.3   20x14   20x15                              644x466    824x571
 * 0.4   28x18   28x19                              859x594   1100x762
 * 0.5   34x22   36x22                             1074x744   1376x949  0.28
 * 0.6   40x27   40x28                             1288x892
 * 0.7   46x31   48x32                            1502x1040
 * 0.8   54x35   56x37
 * 0.9   60x40   60x41
 * 1.0   66x44   68x46       310x135  400x175 0.29
 * 1.1   72x48
 * 1.2   80x53
 * 1.3   86x57
 * 1.4   92x61
 * 1.5   99x66
 * 1.6  106x70
 * 2.0  132x87  132x90       620x270  796x348 0.28
 * 3.0  197x131 200x134      930x404 1169x522
 * 4.0  262x174 268x178     1238x539 1592x697 0.29
 * 5.0  328x217 332x223     1548x673
 *
 *
 **************************************************************************/

float LDView::cameraDistance(
  Meta &meta,
  float scale)
{
    return stdCameraDistance(meta, scale)*0.775f;
}

int LDView::renderCsi(
        const QString     &addLine,
        const QStringList &csiParts,   // this is ldrNames when useLDViewSCall = true
        const QStringList &csiKeys,
        const QString     &pngName,
        Meta              &meta,
        int                nType)
{
    Q_UNUSED(nType)

    // paths
    QString tempPath  = QDir::toNativeSeparators(QDir::currentPath() + "/" + Paths::tmpDir);
    QString assemPath = QDir::toNativeSeparators(QDir::currentPath() + "/" + Paths::assemDir);

    // Populate render attributes
    QStringList ldviewParmslist;
    if (useLDViewSCall())
        ;
    else
        ldviewParmslist = meta.LPub.assem.ldviewParms.value().split(' ');
    QString transform  = meta.rotStep.value().type;
    bool noCA          = Preferences::applyCALocally || transform == "ABS";
    bool pp            = Preferences::perspectiveProjection;
    int studLogo       = meta.LPub.assem.studLogo.value() ? meta.LPub.assem.studLogo.value() : -1;
    float modelScale   = meta.LPub.assem.modelScale.value();
    float cameraFoV    = meta.LPub.assem.cameraFoV.value();
    float cameraAngleX = noCA ? 0.0f : meta.LPub.assem.cameraAngles.value(0);
    float cameraAngleY = noCA ? 0.0f : meta.LPub.assem.cameraAngles.value(1);
    xyzVector target   = xyzVector(meta.LPub.assem.target.x(),meta.LPub.assem.target.y(),meta.LPub.assem.target.z());

    // Assemble compareKey and test csiParts if Single Call
    QString compareKey;
    if (useLDViewSCall()){

        // test first csiParts
        QFileInfo fileInfo(csiParts.first());
        if ( ! fileInfo.exists()) {
          emit gui->messageSig(LOG_ERROR,QMessageBox::tr("CSI render input file was not found at the specified path [%1]")
                                                         .arg(csiParts.first()));
          return -1;
        }

        compareKey = QString("%1_%2_%3_%4")
                             .arg(double(modelScale))                    // 1
                             .arg(double(cameraFoV))                     // 2
                             .arg(double(cameraAngleX))                  // 3
                             .arg(double(cameraAngleY));                 // 4
        // append target vector if specified
        if (meta.LPub.assem.target.isPopulated())
            compareKey.append(QString("_%1_%2_%3")
                              .arg(double(meta.LPub.assem.target.x()))   // 5
                              .arg(double(meta.LPub.assem.target.y()))   // 6
                              .arg(double(meta.LPub.assem.target.z()))); // 7
        // append rotate type if specified
        if (meta.rotStep.isPopulated())
            compareKey.append(QString("_%1")                             // 8
                              .arg(transform.isEmpty() ? "REL" : transform));
    }

    /* determine camera distance */
    int cd = int(meta.LPub.assem.cameraDistance.value());
    if (!cd)
        cd = int(cameraDistance(meta,modelScale)*1700/1000);

    // set page size
    bool useImageSize = meta.LPub.assem.imageSize.value(0) > 0;
    int width  = useImageSize ? int(meta.LPub.assem.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
    int height = useImageSize ? int(meta.LPub.assem.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

    // arguments settings
    bool usingSnapshotArgs = false;
    QStringList attributes;;

    // projection settings
    QString CA, cg;

    // parameter arguments;
    QStringList ldviewParmsArgs;

    auto getRendererSettings = [
            &pp,
            &cd,
            &target,
            &modelScale,
            &cameraFoV,
            &cameraAngleX,
            &cameraAngleY,
            &ldviewParmslist,
            &useImageSize] (
        QString     &CA,
        QString     &cg,
        QStringList &ldviewParmsArgs)
    {
        // additional LDView parameters;
        qreal cdf = LP3D_CDF;
        QString dz, dl, df = QString("-FOV=%1").arg(double(cameraFoV));
        bool pd = false, pl = false, pf = false, pz = false;
        for (int i = 0; i < ldviewParmslist.size(); i++) {
            if (ldviewParmslist[i] != "" && ldviewParmslist[i] != " ") {
                if (pp) {
                    if ((pl = ldviewParmslist[i].contains("-DefaultLatLong=")))
                         dl = ldviewParmslist[i];
                    if ((pz = ldviewParmslist[i].contains("-DefaultZoom=")))
                         dz = ldviewParmslist[i];
                    if ((pf = ldviewParmslist[i].contains("-FOV=")))
                         df = ldviewParmslist[i];
                    if ((pd = ldviewParmslist[i].contains("-LDViewPerspectiveDistanceFactor="))) {
                        bool ok;
                        qreal _cdf = QStringList(ldviewParmslist[i].split("=")).last().toDouble(&ok);
                        if (ok && _cdf < LP3D_CDF)
                            cdf = _cdf;
                    }
                }
                if (!pd && !pl && !pf && !pz) {
                  addArgument(ldviewParmsArgs, ldviewParmslist[i]);    // 10. ldviewParms [usually empty]
                }
            }
        }

        // Set camera angle and camera globe and update arguments with perspective projection settings
        if (pp && pl && !pz)
            dz = QString("-DefaultZoom=%1").arg(double(modelScale));

        CA = pp ? df : QString("-ca%1") .arg(LP3D_CA);              // replace CA with FOV

        // Set alternate target position or use specified image size
        QString _mc;
        if (target.isPopulated())
            _mc = QString("-ModelCenter=%1,%2,%3 ").arg(double(target.x)).arg(double(target.y)).arg(double(target.z));
        if ((!_mc.isEmpty() && !pl) || (useImageSize && _mc.isEmpty())){
            // Set model center
            QString _dl = QString("-DefaultLatLong=%1,%2")
                                  .arg(double(cameraAngleX))
                                  .arg(double(cameraAngleY));
            QString _dz = QString("-DefaultZoom=%1").arg(double(modelScale));
            // Set zoom to fit when use image size specified
            QString _sz;
            if (useImageSize && _mc.isEmpty())
                _sz = QString(" -SaveZoomToFit=1");
            cg = QString("%1%2 %3%4").arg(_mc.isEmpty() ? "" : _mc).arg(_dl).arg(_dz).arg(_sz.isEmpty() ? "" : _sz);
        } else {
            cg = pp ? pl ? QString("-DefaultLatLong=%1 %2")
                                   .arg(dl)
                                   .arg(dz)                         // replace Camera Globe with DefaultLatLon and add DefaultZoom
                         : QString("-cg%1,%2,%3")
                                   .arg(double(cameraAngleX))
                                   .arg(double(cameraAngleY))
                                   .arg(QString::number(cd * cdf,'f',0) )
                    : QString("-cg%1,%2,%3")
                              .arg(double(cameraAngleX))
                              .arg(double(cameraAngleY))
                              .arg(cd);
        }

        // additional LDView parameters;
        if (ldviewParmsArgs.size()){
            emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDView additional CSI renderer parameters: %1")
                                                          .arg(ldviewParmsArgs.join(" ")));
            cg.append(QString(" %1").arg(ldviewParmsArgs.join(" ")));
        }
    };

    auto processAttributes = [this, &meta, &usingSnapshotArgs, &getRendererSettings] (
        QStringList &attributes,
        xyzVector   &target,
        bool        &noCA,
        int         &cd,
        QString     &CA,
        QString     &cg,
        QStringList &ldviewParmsArgs,
        float       &modelScale,
        float       &cameraFoV,
        float       &cameraAngleX,
        float       &cameraAngleY)
    {
        if (usingSnapshotArgs) {
            // set scale FOV and camera angles
            bool hr;
            if ((hr = attributes.size() == nHasRotstep) || attributes.size() == nHasTargetAndRotstep)
                noCA = attributes.at(hr ? nRotTrans : nRot_Trans) == "ABS";
            // set target attribute
            if (attributes.size() >= nHasTarget)
                target = xyzVector(attributes.at(nTargetX).toFloat(),attributes.at(nTargetY).toFloat(),attributes.at(nTargetZ).toFloat());
            // set scale FOV and camera angles
            modelScale   = attributes.at(nModelScale).toFloat();
            cameraFoV    = attributes.at(nCameraFoV).toFloat();
            cameraAngleX = noCA ? 0.0f : attributes.at(nCameraAngleXX).toFloat();
            cameraAngleY = noCA ? 0.0f : attributes.at(nCameraAngleYY).toFloat();
            cd = int(cameraDistance(meta,modelScale)*1700/1000);
            getRendererSettings(CA, cg, ldviewParmsArgs);
        }
    };

    /* Create the CSI DAT file(s) */

    QString f, snapshotArgsKey, imageMatteArgsKey;
    bool usingListCmdArg     = false;
    bool usingDefaultArgs    = true;
    bool usingSingleSetArgs  =false;
    bool snapshotArgsChanged = false;
    bool enableIM            = false;
    QStringList ldrNames, ldrNamesIM, snapshotLdrs;
    if (useLDViewSCall()) {  // Use LDView SingleCall
        // populate ldrNames
        if (Preferences::enableFadeSteps && Preferences::enableImageMatting){  // ldrName entries (IM ON)
            enableIM = true;
            foreach(QString csiEntry, csiParts){              // csiParts are ldrNames under LDViewSingleCall
                QString csiFile = QString("%1/%2").arg(assemPath).arg(QFileInfo(QString(csiEntry).replace(".ldr",".png")).fileName());
                if (LDVImageMatte::validMatteCSIImage(csiFile)) {
                    ldrNamesIM << csiEntry;                   // ldrName entries that ARE IM
                } else {
                    ldrNames << csiEntry;                     // ldrName entries that ARE NOT IM
                }
            }
        } else {                                              // ldrName entries (IM off)
            ldrNames = csiParts;
        }

        // process part attributes
        QString snapshotsCmdLineArgs,snapshotArgs;
        QStringList snapShotsListArgs,keys;
        for (int i = 0; i < ldrNames.size(); i++) {
            QString ldrName = ldrNames.at(i);

            if (!QFileInfo(ldrName).exists()) {
                emit gui->messageSig(LOG_ERROR, QString("LDR file %1 not found.").arg(ldrName));
                continue;
            }

            // split snapshot, imageMatte and additional renderer keys,
            keys              = csiKeys.at(i).split("|");
            snapshotArgsKey   = keys.at(0);
            imageMatteArgsKey = keys.at(1);
            if (keys.size() == 3)
                ldviewParmslist = keys.at(2).split(" ");
            attributes = snapshotArgsKey.split("_");
            if (attributes.size() > 2)
                attributes.replace(1,"0");

            // attributes are different from default
            usingSnapshotArgs = compareImageAttributes(attributes, compareKey, usingDefaultArgs);
            if (usingSnapshotArgs){
                processAttributes(attributes, target, noCA, cd, CA, cg, ldviewParmsArgs,
                                  modelScale, cameraFoV, cameraAngleX, cameraAngleY);
                snapshotArgsChanged = !usingDefaultArgs;
                usingDefaultArgs    = usingDefaultArgs ? false: usingDefaultArgs;
                usingSnapshotArgs   = false;                // reset
                compareKey          = attributes.join("_");

                if (!snapshotArgsChanged) {
                    snapshotArgs = QString("%1 %2").arg(CA).arg(cg);
                }
            } else {
                getRendererSettings(CA, cg, ldviewParmsArgs);
            }

            snapshotLdrs.append(ldrName);

            if (!usingDefaultArgs) {

                QString saveArgs = QString("-SaveSnapShots=1 %1").arg(ldrName);
                saveArgs.prepend(QString("%1 %2 ").arg(CA).arg(cg));
                snapShotsListArgs.append(QString(" %1").arg(saveArgs));
            }
        }

        // using same snapshot args for all parts
        usingSnapshotArgs = !usingDefaultArgs && !snapshotArgsChanged;

        if (snapshotLdrs.size() ) {
            // using default args or same snapshot args for all parts
            if ((usingSingleSetArgs = usingDefaultArgs || usingSnapshotArgs)) {

                // using same snapshot args for all parts
                if (usingSnapshotArgs) {
                    keys = snapshotArgs.split(" ");
                    CA   = keys.at(0);
                    cg   = keys.mid(1).join(" ");
                }

                // use single line snapshots command
                if (snapshotLdrs.size() < SNAPSHOTS_LIST_THRESHOLD || !useLDViewSList()) {

                    // use single line snapshots command
                    snapshotsCmdLineArgs = QString("-SaveSnapShots=1");

                }
                // create snapshot list
                else {

                    usingListCmdArg = true;
                    QString SnapshotsList = tempPath + QDir::separator() + "pliSnapshotsList.lst";
                    if (!createSnapshotsList(snapshotLdrs,SnapshotsList))
                        return -1;
                    snapshotsCmdLineArgs = QString("-SaveSnapshotsList=%1").arg(SnapshotsList);
                }

                f  = snapshotsCmdLineArgs;
            }
            // create a command lines list - we have subSnapShotsListArgs or not usingDefaultArgs
            else
            {
                usingListCmdArg = true;
                QString CommandLinesList = tempPath + QDir::separator() + "pliCommandLinesList.lst";
                QFile CommandLinesListFile(CommandLinesList);
                if ( ! CommandLinesListFile.open(QFile::WriteOnly | QFile::Text)) {
                    emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Failed to create LDView (Single Call) PLI CommandLines list file!"));
                    return -1;
                }

                QTextStream out(&CommandLinesListFile);
                // add normal snapshot lines
                if (snapshotLdrs.size()) {
                    foreach (QString argsLine,snapShotsListArgs) {
                        out << argsLine << endl;
                        if (Preferences::debugLogging)
                            emit gui->messageSig(LOG_DEBUG, QString("Wrote %1 to CSI Command line list").arg(argsLine));
                    }
                }
                CommandLinesListFile.close();

                f  = QString("-CommandLinesList=%1").arg(CommandLinesList);    // run in renderCsi
            }
        }

    } else { // End Use SingleCall

        usingSingleSetArgs = usingDefaultArgs;

        int rc;
        QString csiKey = QString();
        if (Preferences::enableFadeSteps && Preferences::enableImageMatting &&
                LDVImageMatte::validMatteCSIImage(csiKeys.first())) {                    // ldrName entries (IM ON)
            enableIM = true;
            csiKey = csiKeys.first();
        }

        ldrNames << QDir::fromNativeSeparators(tempPath + "/csi.ldr");

        getRendererSettings(CA, cg, ldviewParmsArgs);

        // RotateParts #2 - 8 parms
        if ((rc = rotateParts(addLine, meta.rotStep, csiParts, ldrNames.first(), csiKey, meta.LPub.assem.cameraAngles,false/*ldv*/,Options::Mt::CSI)) < 0) {
            emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView (Single Call) CSI rotate parts failed!"));
            return rc;
        } else
          // recheck csiKey - may have been deleted by rotateParts if IM files not created.
          if (enableIM) {
            enableIM = LDVImageMatte::validMatteCSIImage(csiKeys.first());
        }

        f  = QString("-SaveSnapShot=%1") .arg(pngName);
    }

    bool haveLdrNames   = !ldrNames.isEmpty();
    bool haveLdrNamesIM = !ldrNamesIM.isEmpty();

    QString s  = studLogo >= 0 ? QString("-StudLogo=%1").arg(studLogo) : "";
    QString w  = QString("-SaveWidth=%1")  .arg(width);
    QString h  = QString("-SaveHeight=%1") .arg(height);
    QString l  = QString("-LDrawDir=%1")   .arg(Preferences::ldrawLibPath);
    QString o  = QString("-HaveStdOut=1");
    QString v  = QString("-vv");

    QStringList arguments;
    if (usingSingleSetArgs){
        arguments << CA;             // Camera Angle (i.e. Field of Veiw)
        arguments << cg.split(" ");  // Camera Globe, Target and Additional Parameters when specified
    }

    arguments << f; // -CommandLinesList | -SaveSnapshotsList | -SaveSnapShots | -SaveSnapShot
    arguments << s ;// -StudLogo
    arguments << w; // -SaveWidth
    arguments << h; // -SaveHeight
    arguments << l; // -LDrawDir
    arguments << o; // -HaveStdOut
    arguments << v; // -vv (Verbose)

    QString ini;
    if(Preferences::ldviewIni != ""){
        ini = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
        addArgument(arguments, ini, "-IniFile", 0, ldviewParmsArgs.size());
    }

    QString altldc;
    if (!Preferences::altLDConfigPath.isEmpty()) {
        altldc = QString("-LDConfig=%1").arg(Preferences::altLDConfigPath);
        addArgument(arguments, altldc, "-LDConfig", 0, ldviewParmsArgs.size());
    }

    if (haveLdrNames) {
        if (useLDViewSCall()) {
            //-SaveSnapShots=1
            if ((!useLDViewSList() && !usingListCmdArg) ||
                (useLDViewSList() && snapshotLdrs.size() < SNAPSHOTS_LIST_THRESHOLD))
                arguments = arguments + snapshotLdrs;  // 13. LDR input file(s)
        } else {
            // SaveSnapShot=1
            arguments << QDir::toNativeSeparators(ldrNames.first());

        }

        removeEmptyStrings(arguments);

        emit gui->messageSig(LOG_INFO_STATUS, QString("Executing LDView %1 CSI render - please wait...")
                                                      .arg(pp ? "Perspective" : "Orthographic"));

        // execute LDView process
        if (executeLDViewProcess(arguments, Options::CSI) != 0) // ldrName entries that ARE NOT IM exist - e.g. first step
            return -1;
    }

    // Build IM arguments and process IM [Not implemented - not updated with perspective 'pp' routines]
    QStringList im_arguments;
    if (enableIM && haveLdrNamesIM) {
        QString a  = QString("-AutoCrop=0");
        im_arguments << CA;                         // 00. Camera FOV in degrees
        im_arguments << cg.split(" ");              // 01. Camera globe
        im_arguments << a;                          // 02. AutoCrop off - to create same size IM pair files
        im_arguments << w;                          // 03. SaveWidth
        im_arguments << h;                          // 04. SaveHeight
        im_arguments << f;                          // 05. SaveSnapshot/SaveSnapshots/SaveSnapshotsList
        im_arguments << l;                          // 06. LDrawDir
        im_arguments << o;                          // 07. HaveStdOut
        im_arguments << v;                          // 09. Verbose
        for (int i = 0; i < ldviewParmslist.size(); i++) {
            if (ldviewParmslist[i] != "" &&
                    ldviewParmslist[i] != " ") {
                im_arguments << ldviewParmslist[i]; // 10. ldviewParms [usually empty]
            }
        }
        im_arguments << ini;                        // 11. LDView.ini
        if (!altldc.isEmpty())
            im_arguments << altldc;                 // 12.Alternate LDConfig

        removeEmptyStrings(arguments);

        if (useLDViewSCall()){

            if (enableIM) {
                if (haveLdrNamesIM) {
                    // IM each ldrNameIM file
                    emit gui->messageSig(LOG_STATUS, "Executing LDView render Image Matte CSI - please wait...");

                    foreach(QString ldrNameIM, ldrNamesIM){
                        QFileInfo pngFileInfo(QString("%1/%2").arg(assemPath).arg(QFileInfo(QString(ldrNameIM).replace(".ldr",".png")).fileName()));
                        QString csiKey = LDVImageMatte::getMatteCSIImage(pngFileInfo.absoluteFilePath());
                        if (!csiKey.isEmpty()) {
                            if (!LDVImageMatte::matteCSIImage(im_arguments, csiKey))
                                return -1;
                        }
                    }
                }
            }

        } else {

            // image matte - LDView Native csiKeys.first()
            if (enableIM) {
                QString csiFile = LDVImageMatte::getMatteCSIImage(csiKeys.first());
                if (!csiFile.isEmpty())
                    if (!LDVImageMatte::matteCSIImage(im_arguments, csiFile))
                        return -1;
            }
        }
    }

    // move generated CSI images to assem subfolder
    if (useLDViewSCall()){
        foreach(QString ldrName, ldrNames){
            QString pngFileTmpPath = ldrName.replace(".ldr",".png");
            QString pngFilePath = QString("%1/%2").arg(assemPath).arg(QFileInfo(pngFileTmpPath).fileName());
            QFile destinationFile(pngFilePath);
            QFile sourceFile(pngFileTmpPath);
            if (! destinationFile.exists() || destinationFile.remove()) {
                if (! sourceFile.rename(destinationFile.fileName()))
                    emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView CSI image move failed for %1").arg(pngFilePath));
            } else {
                emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView could not remove old CSI image file %1").arg(pngFilePath));
            }
        }
    }

    return 0;
}

int LDView::renderPli(
  const QStringList &ldrNames,
  const QString     &pngName,
  Meta              &meta,
  int                pliType,
  int                keySub)
{
  // Select meta type
  PliMeta &metaType = pliType == SUBMODEL ? static_cast<PliMeta&>(meta.LPub.subModel) :
                      pliType == BOM ? meta.LPub.bom : meta.LPub.pli;

  // test ldrNames
  QFileInfo fileInfo(ldrNames.first());
  if ( ! fileInfo.exists()) {
      emit gui->messageSig(LOG_ERROR,QMessageBox::tr("PLI render input file was not found at the specified path [%1]")
                                                    .arg(ldrNames.first()));
    return -1;
  }

  // paths
  QString tempPath  = QDir::toNativeSeparators(QDir::currentPath() + "/" + Paths::tmpDir);
  QString partsPath = QDir::toNativeSeparators(QDir::currentPath() + "/" + (pliType == SUBMODEL ? Paths::submodelDir : Paths::partsDir));

  // Populate render attributes
  QStringList ldviewParmslist = metaType.ldviewParms.value().split(' ');
  QString transform  = metaType.rotStep.value().type;
  bool noCA          = pliType == SUBMODEL ? Preferences::applyCALocally || transform == "ABS" : transform == "ABS";
  bool pp            = Preferences::perspectiveProjection;
  int studLogo       = metaType.studLogo.value() ? metaType.studLogo.value() : -1;
  float modelScale   = metaType.modelScale.value();
  float cameraFoV    = metaType.cameraFoV.value();
  float cameraAngleX = noCA ? 0.0f : metaType.cameraAngles.value(0);
  float cameraAngleY = noCA ? 0.0f : metaType.cameraAngles.value(1);
  xyzVector target   = xyzVector(metaType.target.x(),metaType.target.y(),metaType.target.z());

  // Assemble compareKey if Single Call
  QString compareKey;
  if (useLDViewSCall()){
      compareKey = QString("%1_%2_%3_%4")
                           .arg(double(modelScale))             // 1
                           .arg(double(cameraFoV))              // 2
                           .arg(double(cameraAngleX))           // 3
                           .arg(double(cameraAngleY));          // 4
      // append target vector if specified
      if (metaType.target.isPopulated())
          compareKey.append(QString("_%1_%2_%3")
                            .arg(double(metaType.target.x()))   // 5
                            .arg(double(metaType.target.y()))   // 6
                            .arg(double(metaType.target.z()))); // 7
      // append rotate type if specified
      if (keySub > PliBeginSub5Rc || metaType.rotStep.isPopulated())
          compareKey.append(QString("_%1")                      // 8
                            .arg(transform.isEmpty() ? "REL" : transform));
  }

  /* determine camera distance */
  int cd = int(metaType.cameraDistance.value());
  if (!cd)
      cd = int(cameraDistance(meta,modelScale)*1700/1000);

  //qDebug() << "LDView (Default) Camera Distance: " << cd;

  // set page size
  bool useImageSize = metaType.imageSize.value(0) > 0;
  int width  = useImageSize ? int(metaType.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  int height = useImageSize ? int(metaType.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);

  // arguments settings
  bool usingListCmdArg   = false;
  bool usingSnapshotArgs = false;
  QStringList attributes, snapshotLdrs;

  // projection settings
  qreal cdf = LP3D_CDF;
  bool pd = false, pl = false, pf = false, pz = false;
  QString dz, dl, df = QString("-FOV=%1").arg(double(cameraFoV));
  QString CA, cg;

  // additional LDView parameters;
  QStringList ldviewParmsArgs;

  for (int i = 0; i < ldviewParmslist.size(); i++) {
      if (ldviewParmslist[i] != "" && ldviewParmslist[i] != " ") {
          if (pp) {
              if ((pl = ldviewParmslist[i].contains("-DefaultLatLong=")))
                  dl = ldviewParmslist[i];
              if ((pz = ldviewParmslist[i].contains("-DefaultZoom=")))
                  dz = ldviewParmslist[i];
              if ((pf = ldviewParmslist[i].contains("-FOV=")))
                  df = ldviewParmslist[i];
              if ((pd = ldviewParmslist[i].contains("-LDViewPerspectiveDistanceFactor="))) {
                  bool ok;
                  qreal _cdf = QStringList(ldviewParmslist[i].split("=")).last().toDouble(&ok);
                  if (ok && _cdf < LP3D_CDF)
                      cdf = _cdf;
              }
          }
          if (!pd && !pl && !pf && !pz)
              addArgument(ldviewParmsArgs, ldviewParmslist[i]);    // 10. ldviewParms [usually empty]
      }
  }

  // Set camera angle and camera globe and update arguments with perspective projection settings
  if (pp && pl && !pz)
      dz = QString("-DefaultZoom=%1").arg(double(modelScale));

  auto getRendererSettings = [
          &pp,
          &dl,
          &df,
          &dz,
          &pl,
          &cd,
          &cdf,
          &target,
          &modelScale,
          &cameraAngleX,
          &cameraAngleY,
          &useImageSize] (
          QString &CA,
          QString &cg)
  {
      CA = pp ? df : QString("-ca%1") .arg(LP3D_CA);              // replace CA with FOV
      // Set alternate target position or use specified image size
      QString _mc;
      if (target.isPopulated())
          _mc = QString("-ModelCenter=%1,%2,%3").arg(double(target.x)).arg(double(target.y)).arg(double(target.z));
      if ((!_mc.isEmpty() && !pl) || (useImageSize && _mc.isEmpty())){
          // Set model center
          QString _dl = QString("-DefaultLatLong=%1,%2")
                                .arg(double(cameraAngleX))
                                .arg(double(cameraAngleY));
          QString _dz = QString("-DefaultZoom=%1").arg(double(modelScale));
          // Set zoom to fit when use image size specified
          QString _sz;
          if (useImageSize && _mc.isEmpty())
              _sz = QString("-SaveZoomToFit=1");
          cg = QString("%1 %2 %3%4").arg(_mc).arg(_dl).arg(_dz).arg(_sz.isEmpty() ? "" : _sz);
      } else {
          cg = pp ? pl ? QString("-DefaultLatLong=%1 %2")
                         .arg(dl)
                         .arg(dz)                             // replace Camera Globe with DefaultLatLon and add DefaultZoom
                       : QString("-cg%1,%2,%3")
                         .arg(double(cameraAngleX))
                         .arg(double(cameraAngleY))
                         .arg(QString::number(cd * cdf,'f',0) )
                       : QString("-cg%1,%2,%3")
                         .arg(double(cameraAngleX))
                         .arg(double(cameraAngleY))
                         .arg(cd);
      }
  };

  auto processAttributes = [
          this,
          &meta,
          &keySub,
          &usingSnapshotArgs,
          &getRendererSettings] (
      QStringList &attributes,
      xyzVector   &target,
      bool        &noCA,
      int         &cd,
      QString     &CA,
      QString     &cg,
      float       &modelScale,
      float       &cameraFoV,
      float       &cameraAngleX,
      float       &cameraAngleY)
  {
      if (keySub > PliBeginSub2Rc || usingSnapshotArgs) {
          // set scale FOV and camera angles
          bool hr;
          if ((hr = attributes.size() == nHasRotstep) || attributes.size() == nHasTargetAndRotstep)
              noCA = attributes.at(hr ? nRotTrans : nRot_Trans) == "ABS";
          // set target attribute
          if (attributes.size() >= nHasTarget)
              target = xyzVector(attributes.at(nTargetX).toFloat(),attributes.at(nTargetY).toFloat(),attributes.at(nTargetZ).toFloat());
          // set scale FOV and camera angles

          modelScale   = attributes.at(nModelScale).toFloat();
          cameraFoV    = attributes.at(nCameraFoV).toFloat();
          cameraAngleX = noCA ? 0.0f : attributes.at(nCameraAngleXX).toFloat();
          cameraAngleY = noCA ? 0.0f : attributes.at(nCameraAngleYY).toFloat();
          cd = int(cameraDistance(meta,modelScale)*1700/1000);
          getRendererSettings(CA,cg);
      }
  };

  /* Create the PLI DAT file(s) */

  QString f;
  bool hasSubstitutePart   = false;
  bool usingDefaultArgs    = true;
  bool usingSingleSetArgs  = false;
  bool snapshotArgsChanged = false;
  if (useLDViewSCall() && pliType != SUBMODEL) {  // Use LDView SingleCall

      // process part attributes
      QString snapshotsCmdLineArgs,snapshotArgs;
      QStringList snapShotsListArgs, subSnapShotsListArgs;

      foreach (QString ldrName,ldrNames) {
          if (!QFileInfo(ldrName).exists()) {
              emit gui->messageSig(LOG_ERROR, QString("LDR file %1 not found.").arg(ldrName));
              continue;
          }

          // get attribues from ldrName key
          attributes = getImageAttributes(ldrName);

          // determine if is substitute part
          hasSubstitutePart = keySub && attributes.endsWith("SUB");

          // attributes are different from default
          usingSnapshotArgs = compareImageAttributes(attributes, compareKey, usingDefaultArgs);
          if (usingSnapshotArgs){
              processAttributes(attributes, target, noCA, cd, CA, cg, modelScale, cameraFoV, cameraAngleX, cameraAngleY);
              snapshotArgsChanged = !usingDefaultArgs;
              usingDefaultArgs    = usingDefaultArgs ? false : usingDefaultArgs;
              usingSnapshotArgs   = false;                // reset
              compareKey          = attributes.join("_");

              if (!snapshotArgsChanged)
                  snapshotArgs = QString("%1 %2").arg(CA).arg(cg);

          } else {
              getRendererSettings(CA,cg);
          }

          // if substitute, trigger command list
          if (hasSubstitutePart) {

             usingDefaultArgs = false;
             QString pngName = QString(ldrName).replace("_SUB.ldr",".png");

             // use command list as pngName and ldrName must be specified
             subSnapShotsListArgs.append(QString("%1 %2 -SaveSnapShot=%3 %4").arg(CA).arg(cg).arg(pngName).arg(ldrName));

          } else {

             // if using different snapshot args, trigger command list
             if (!usingDefaultArgs) {

                 QString saveArgs = QString("-SaveSnapShots=1 %1").arg(ldrName);
                 snapShotsListArgs.append(QString("%1 %2 %3").arg(CA).arg(cg).arg(saveArgs));
             }
          }

          snapshotLdrs.append(ldrName);
      }

      // using same snapshot args for all parts
      usingSnapshotArgs = !usingDefaultArgs && !snapshotArgsChanged && !hasSubstitutePart;

      if (snapshotLdrs.size()) {
          // using default args or same snapshot args for all parts
          if ((usingSingleSetArgs = usingDefaultArgs || usingSnapshotArgs)) {

              // using same snapshot args for all parts
              if (usingSnapshotArgs) {
                  QStringList keys = snapshotArgs.split(" ");
                  CA = keys.at(0);
                  cg = keys.mid(1).join(" ");
              }

              // use single line snapshots command
              if (snapshotLdrs.size() < SNAPSHOTS_LIST_THRESHOLD || !useLDViewSList()) {

                  snapshotsCmdLineArgs = QString("-SaveSnapShots=1");

              }
              // create snapshot list
              else {

                  usingListCmdArg = true;
                  QString SnapshotsList = tempPath + QDir::separator() + "pliSnapshotsList.lst";
                  if (!createSnapshotsList(snapshotLdrs,SnapshotsList))
                      return -1;
                  snapshotsCmdLineArgs = QString("-SaveSnapshotsList=%1").arg(SnapshotsList);
              }

              f  = snapshotsCmdLineArgs;
          }
          // create a command lines list - we have subSnapShotsListArgs or not usingDefaultArgs
          else
          {
              usingListCmdArg = true;
              QString CommandLinesList = tempPath + QDir::separator() + "pliCommandLinesList.lst";
              QFile CommandLinesListFile(CommandLinesList);
              if ( ! CommandLinesListFile.open(QFile::WriteOnly | QFile::Text)) {
                  emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Failed to create LDView (Single Call) PLI CommandLines list file!"));
                  return -1;
              }

              QTextStream out(&CommandLinesListFile);
              if (snapshotLdrs.size()) {
                  // add normal snapshot lines
                  foreach (QString argsLine,snapShotsListArgs) {
                      out << argsLine << endl;
                      if (Preferences::debugLogging)
                          emit gui->messageSig(LOG_DEBUG, QString("Wrote %1 to PLI Command line list").arg(argsLine));
                  }
                  // add substitute snapshot lines
                  foreach (QString argsLine,subSnapShotsListArgs) {
                      out << argsLine << endl;
                      if (Preferences::debugLogging)
                          emit gui->messageSig(LOG_DEBUG, QString("Wrote %1 to PLI Substitute Command line list").arg(argsLine));
                  }
              }
              CommandLinesListFile.close();

              f  = QString("-CommandLinesList=%1").arg(CommandLinesList);    // run in renderCsi
          }
      }

  } else { // End Use SingleCall

      usingSingleSetArgs = usingDefaultArgs;

      if (keySub) {
          // process substitute attributes
          attributes = getImageAttributes(pngName);
          hasSubstitutePart = attributes.endsWith("SUB");
          processAttributes(attributes, target, noCA, cd, CA, cg, modelScale, cameraFoV, cameraAngleX, cameraAngleY);
      } else {
          getRendererSettings(CA,cg);
      }

      f  = QString("-SaveSnapShot=%1") .arg(pngName);
  }

  QString s  = studLogo >= 0 ? QString("-StudLogo=%1").arg(studLogo) : "";
  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString l  = QString("-LDrawDir=%1")   .arg(Preferences::ldrawLibPath);
  QString o  = QString("-HaveStdOut=1");
  QString v  = QString("-vv");

  QStringList arguments;
  if (usingSingleSetArgs){
      arguments << CA;             // Camera Angle (i.e. Field of Veiw)
      arguments << cg.split(" ");  // Camera Globe, Target and Additional Parameters when specified
  }

  // append additional LDView parameters
  if (ldviewParmsArgs.size()) {
    for (int i = 0; i < ldviewParmsArgs.size(); i++) {
      addArgument(arguments, ldviewParmsArgs[i]);
    }
    emit gui->messageSig(LOG_INFO,QMessageBox::tr("LDView additional PLI renderer parameters: %1")
                         .arg(ldviewParmsArgs.join(" ")));
  }

  arguments << f; // -CommandLinesList | -SaveSnapshotsList | -SaveSnapShots | -SaveSnapShot
  arguments << s ;// -StudLogo
  arguments << w; // -SaveWidth
  arguments << h; // -SaveHeight
  arguments << l; // -LDrawDir
  arguments << o; // -HaveStdOut
  arguments << v; // -vv (Verbose)

  if(!Preferences::ldviewIni.isEmpty()){
      QString ini;
      ini = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
      addArgument(arguments, ini, "-IniFile", 0, ldviewParmsArgs.size());
  }

  QString altldc;
  if (!Preferences::altLDConfigPath.isEmpty()) {
      altldc = QString("-LDConfig=%1").arg(Preferences::altLDConfigPath);
      addArgument(arguments, altldc, "-LDConfig", 0, ldviewParmsArgs.size());
  }

  if (useLDViewSCall() && pliType != SUBMODEL) {
      //-SaveSnapShots=1
      if (!hasSubstitutePart &&
           ((!useLDViewSList() && !usingListCmdArg) ||
            (useLDViewSList() && snapshotLdrs.size() < SNAPSHOTS_LIST_THRESHOLD)))
          arguments = arguments + snapshotLdrs;  // 13. LDR input file(s)
  } else {
      //-SaveSnapShot=%1
      arguments << QDir::toNativeSeparators(ldrNames.first());
  }

  removeEmptyStrings(arguments);

  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing LDView %1 PLI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  // execute LDView process
  if (executeLDViewProcess(arguments, Options::PLI) != 0)
      return -1;

  // move generated PLI images to parts subfolder
  if (useLDViewSCall() && pliType != SUBMODEL){
      foreach(QString ldrName, ldrNames){
          QString pngFileTmpPath = ldrName.endsWith("_SUB.ldr") ?
                                   ldrName.replace("_SUB.ldr",".png") :
                                   ldrName.replace(".ldr",".png");
          QString pngFilePath = partsPath + QDir::separator() + QFileInfo(pngFileTmpPath).fileName();
          QFile destinationFile(pngFilePath);
          QFile sourceFile(pngFileTmpPath);
          if (! destinationFile.exists() || destinationFile.remove()) {
              if (! sourceFile.rename(destinationFile.fileName()))
                  emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView PLI image move failed for %1").arg(pngFilePath));
          } else {
              emit gui->messageSig(LOG_ERROR,QMessageBox::tr("LDView could not remove old PLI image file %1").arg(pngFilePath));
          }
      }
  }

  return 0;
}

/***************************************************************************
 *
 * Native renderer
 *
 **************************************************************************/

float Native::cameraDistance(
    Meta &meta,
    float scale)
{
  return stdCameraDistance(meta,scale);
}

int Native::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QStringList &csiKeys,
  const QString     &pngName,
        Meta        &meta,
  int                nType)
{
  QString ldrName     = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
  float lineThickness = (float(resolution()/Preferences::highlightStepLineWidth));

  // process native settings
  int studLogo         = meta.LPub.assem.studLogo.value();
  float camDistance    = meta.LPub.assem.cameraDistance.value();
  float cameraAngleX   = meta.LPub.assem.cameraAngles.value(0);
  float cameraAngleY   = meta.LPub.assem.cameraAngles.value(1);
  float modelScale     = meta.LPub.assem.modelScale.value();
  float cameraFoV      = meta.LPub.assem.cameraFoV.value();
  bool  isOrtho        = meta.LPub.assem.isOrtho.value();
  QString cameraName   = meta.LPub.assem.cameraName.value();
  xyzVector target     = xyzVector(meta.LPub.assem.target.x(),meta.LPub.assem.target.y(),meta.LPub.assem.target.z());
  if (nType == NTypeCalledOut) {
    studLogo           = meta.LPub.callout.csi.studLogo.value();
    camDistance        = meta.LPub.callout.csi.cameraDistance.value();
    cameraAngleX       = meta.LPub.callout.csi.cameraAngles.value(0);
    cameraAngleY       = meta.LPub.callout.csi.cameraAngles.value(1);
    modelScale         = meta.LPub.callout.csi.modelScale.value();
    cameraFoV          = meta.LPub.callout.csi.cameraFoV.value();
//    zNear              = meta.LPub.callout.csi.znear.value();
//    zFar               = meta.LPub.callout.csi.zfar.value();
    isOrtho            = meta.LPub.callout.csi.isOrtho.value();
    cameraName         = meta.LPub.callout.csi.cameraName.value();
    target             = xyzVector(meta.LPub.callout.csi.target.x(),meta.LPub.callout.csi.target.y(),meta.LPub.callout.csi.target.z());
  } else if (nType == NTypeMultiStep) {
    studLogo           = meta.LPub.multiStep.csi.studLogo.value();
    camDistance        = meta.LPub.multiStep.csi.cameraDistance.value();
    cameraAngleX       = meta.LPub.multiStep.csi.cameraAngles.value(0);
    cameraAngleY       = meta.LPub.multiStep.csi.cameraAngles.value(1);
    modelScale         = meta.LPub.multiStep.csi.modelScale.value();
    cameraFoV          = meta.LPub.multiStep.csi.cameraFoV.value();
//    zNear              = meta.LPub.multiStep.csi.znear.value();
//    zFar               = meta.LPub.multiStep.csi.zfar.value();
    isOrtho            = meta.LPub.multiStep.csi.isOrtho.value();
    cameraName         = meta.LPub.multiStep.csi.cameraName.value();
    target             = xyzVector(meta.LPub.multiStep.csi.target.x(),meta.LPub.multiStep.csi.target.y(),meta.LPub.multiStep.csi.target.z());
  }

  // Camera Angles always passed to Native renderer except if ABS rotstep
  QString rotstepType = meta.rotStep.value().type;
  bool noCA           = rotstepType == "ABS";
  bool pp             = Preferences::perspectiveProjection;
  bool useImageSize   = meta.LPub.assem.imageSize.value(0) > 0;

  // Renderer options
  NativeOptions *Options    = new NativeOptions();
  Options->ImageType         = Options::CSI;
  Options->InputFileName     = ldrName;
  Options->OutputFileName    = pngName;
  Options->StudLogo          = studLogo;
  Options->Resolution        = resolution();
  Options->ImageWidth        = useImageSize ? int(meta.LPub.assem.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  Options->ImageHeight       = useImageSize ? int(meta.LPub.assem.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);
  Options->PageWidth         = gui->pageSize(meta.LPub.page, 0);
  Options->PageHeight        = gui->pageSize(meta.LPub.page, 1);
  Options->IsOrtho           = isOrtho;
  Options->CameraName        = cameraName;
  Options->FoV               = cameraFoV;
  Options->Latitude          = noCA ? 0.0 : cameraAngleX;
  Options->Longitude         = noCA ? 0.0 : cameraAngleY;
  Options->Target            = target;
  Options->ModelScale        = modelScale;
  Options->CameraDistance    = camDistance > 0 ? camDistance : cameraDistance(meta,modelScale);
  Options->LineWidth         = lineThickness;
  Options->HighlightNewParts = gui->suppressColourMeta(); //Preferences::enableHighlightStep;

  // Set CSI project
  Project* CsiImageProject = new Project();
  gApplication->SetProject(CsiImageProject);

  // Render image
  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing Native %1 CSI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  if (gui->exportingObjects()) {
      if (csiKeys.size()) {
          emit gui->messageSig(LOG_INFO_STATUS, "Rendering CSI Objects...");
          QString baseName = csiKeys.first();
          QString outPath  = gui->saveDirectoryName;
          bool ldvExport   = true;

          switch (gui->exportMode){
          case EXPORT_3DS_MAX:
              Options->ExportMode = int(EXPORT_3DS_MAX);
              Options->ExportFileName = QDir::toNativeSeparators(outPath+"/"+baseName+".3ds");
              break;
          case EXPORT_STL:
              Options->ExportMode = int(EXPORT_STL);
              Options->ExportFileName = QDir::toNativeSeparators(outPath+"/"+baseName+".stl");
              break;
          case EXPORT_POVRAY:
              Options->ExportMode = int(EXPORT_POVRAY);
              Options->ExportFileName = QDir::toNativeSeparators(outPath+"/"+baseName+".pov");
              break;
          case EXPORT_COLLADA:
              Options->ExportMode = int(EXPORT_COLLADA);
              Options->ExportFileName = QDir::toNativeSeparators(outPath+"/"+baseName+".dae");
              ldvExport = false;
              break;
          case EXPORT_WAVEFRONT:
              Options->ExportMode = int(EXPORT_WAVEFRONT);
              Options->ExportFileName = QDir::toNativeSeparators(outPath+"/"+baseName+".obj");
              ldvExport = false;
              break;
          default:
              emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Invalid CSI Object export option."));
              delete CsiImageProject;
              return -1;
          }
          // These exports are performed by the Native LDV module (LDView).
          if (ldvExport) {
              if (gui->exportMode == EXPORT_POVRAY     ||
                  gui->exportMode == EXPORT_STL        ||
                  gui->exportMode == EXPORT_HTML_PARTS ||
                  gui->exportMode == EXPORT_3DS_MAX) {
                  Options->IniFlag = gui->exportMode == EXPORT_POVRAY ? NativePOVIni :
                                     gui->exportMode == EXPORT_STL ? NativeSTLIni : Native3DSIni;
                  /*  Options->IniFlag = gui->exportMode == EXPORT_POVRAY ? NativePOVIni :
                                         gui->exportMode == EXPORT_STL ? NativeSTLIni : EXPORT_HTML_PARTS; */
              }

              ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/exportcsi.ldr";

              // RotateParts #2 - 8 parms, rotate parts for ldvExport - apply camera angles
              int rc;
              if ((rc = rotateParts(addLine, meta.rotStep, csiParts, ldrName, QString(),meta.LPub.assem.cameraAngles,ldvExport,Options::Mt::CSI)) < 0) {
                  return rc;
              }

              /* determine camera distance */
              int cd = int(meta.LPub.assem.cameraDistance.value());
              if (!cd)
                  cd = int(cameraDistance(meta,modelScale)*1700/1000);

              /* apply camera angles */
              noCA  = Preferences::applyCALocally || noCA;

              QString df = QString("-FOV=%1").arg(double(cameraFoV));
              QString CA = pp ? df : QString("-ca%1") .arg(LP3D_CA);              // replace CA with FOV

              qreal cdf = LP3D_CDF;
              QString cg = QString("-cg%1,%2,%3")
                                   .arg(noCA ? 0.0 : double(cameraAngleX))
                                   .arg(noCA ? 0.0 : double(cameraAngleY))
                                   .arg(pp ? QString::number(cd * cdf,'f',0) : QString::number(cd) );

              QString s  = studLogo > 0 ? QString("-StudLogo=%1").arg(studLogo) : "";
              QString w  = QString("-SaveWidth=%1") .arg(double(Options->ImageWidth));
              QString h  = QString("-SaveHeight=%1") .arg(double(Options->ImageHeight));
              QString f  = QString("-ExportFile=%1") .arg(Options->ExportFileName);
              QString l  = QString("-LDrawDir=%1") .arg(QDir::toNativeSeparators(Preferences::ldrawLibPath));
              QString o  = QString("-HaveStdOut=1");
              QString v  = QString("-vv");

              QStringList arguments;
              arguments << CA;
              arguments << cg;
              arguments << s;
              arguments << w;
              arguments << h;
              arguments << f;
              arguments << l;

              if (!Preferences::altLDConfigPath.isEmpty()) {
                  arguments << "-LDConfig=" + Preferences::altLDConfigPath;
              }

              arguments << QDir::toNativeSeparators(ldrName);

              removeEmptyStrings(arguments);

              Options->ExportArgs = arguments;
          }
      } else {
          Options->ExportMode = EXPORT_NONE;
      }
  }

  if (!RenderNativeImage(Options)) {
      return -1;
  }

  return 0;
}

int Native::renderPli(
  const QStringList &ldrNames,
  const QString     &pngName,
  Meta              &meta,
  int               pliType,
  int               keySub)
{
  // Select meta type
  PliMeta &metaType = pliType == SUBMODEL ? static_cast<PliMeta&>(meta.LPub.subModel) :
                      pliType == BOM ? meta.LPub.bom : meta.LPub.pli;

  // Populate render attributes
  int studLogo         = metaType.studLogo.value();
  float camDistance    = metaType.cameraDistance.value();
  float modelScale     = metaType.modelScale.value();
  float cameraAngleX   = metaType.cameraAngles.value(0);
  float cameraAngleY   = metaType.cameraAngles.value(1);
  float cameraFoV      = metaType.cameraFoV.value();
  bool  isOrtho        = metaType.isOrtho.value();
  QString cameraName   = metaType.cameraName.value();
  xyzVector target     = xyzVector(metaType.target.x(),metaType.target.y(),metaType.target.z());
  bool useImageSize    = metaType.imageSize.value(0) > 0;

  // Camera Angles always passed to Native renderer except if ABS rotstep
  bool noCA            = metaType.rotStep.value().type  == "ABS";
  bool pp              = Preferences::perspectiveProjection;

  // Process substitute part attributes
  if (keySub) {
    QStringList attributes = getImageAttributes(pngName);
    bool hr;
    if ((hr = attributes.size() == nHasRotstep) || attributes.size() == nHasTargetAndRotstep)
      noCA = attributes.at(hr ? nRotTrans : nRot_Trans) == "ABS";
    if (attributes.size() >= nHasTarget)
      target = xyzVector(attributes.at(nTargetX).toFloat(),attributes.at(nTargetY).toFloat(),attributes.at(nTargetZ).toFloat());
    if (keySub > PliBeginSub2Rc) {
      modelScale   = attributes.at(nModelScale).toFloat();
      cameraFoV    = attributes.at(nCameraFoV).toFloat();
      cameraAngleX = attributes.at(nCameraAngleXX).toFloat();
      cameraAngleY = attributes.at(nCameraAngleYY).toFloat();
    }
  }

  // Renderer options
  NativeOptions *Options = new NativeOptions();
  Options->ImageType      = Options::PLI;
  Options->InputFileName  = ldrNames.first();
  Options->OutputFileName = pngName;
  Options->StudLogo       = studLogo;
  Options->Resolution     = resolution();
  Options->ImageWidth     = useImageSize ? int(metaType.imageSize.value(0)) : gui->pageSize(meta.LPub.page, 0);
  Options->ImageHeight    = useImageSize ? int(metaType.imageSize.value(1)) : gui->pageSize(meta.LPub.page, 1);
  Options->PageWidth      = gui->pageSize(meta.LPub.page, 0);
  Options->PageHeight     = gui->pageSize(meta.LPub.page, 1);
  Options->IsOrtho        = isOrtho;
  Options->CameraName     = cameraName;
  Options->FoV            = cameraFoV;
  Options->Latitude       = noCA ? 0.0 : cameraAngleX;
  Options->Longitude      = noCA ? 0.0 : cameraAngleY;
  Options->Target         = target;
  Options->ModelScale     = modelScale;
  Options->CameraDistance = camDistance > 0 ? camDistance : cameraDistance(meta,modelScale);
  Options->LineWidth      = HIGHLIGHT_LINE_WIDTH_DEFAULT;

  // Set PLI project
  Project* PliImageProject = new Project();
  gApplication->SetProject(PliImageProject);

  // Render image
  emit gui->messageSig(LOG_INFO_STATUS, QString("Executing Native %1 PLI render - please wait...")
                                                .arg(pp ? "Perspective" : "Orthographic"));

  if (!RenderNativeImage(Options)) {
      return -1;
  }

  return 0;
}

float Render::ViewerCameraDistance(
  Meta &meta,
  float scale)
{
    float distance = stdCameraDistance(meta,scale);
    if (Preferences::preferredRenderer == RENDERER_POVRAY)      // 1
        distance = (distance*0.455f)*1700.0f/1000.0f;
    else if (Preferences::preferredRenderer == RENDERER_LDVIEW) // 2
        distance = (distance*0.775f)*1700.0f/1000.0f;
    return distance;
}

bool Render::ExecuteViewer(const NativeOptions *O, bool RenderImage/*false*/){

    lcGetActiveProject()->SetRenderAttributes(
                O->ImageType,
                O->ImageWidth,
                O->ImageHeight,
                O->PageWidth,
                O->PageHeight,
                getRendererIndex(),
                O->ImageFileName,
                O->Resolution);

    gui->SetStudLogo(O->StudLogo,true);

    if (!RenderImage) {
        gui->enableApplyLightAction();
        gui->GetPartSelectionWidget()->SetDefaultPart();
    }

    View* ActiveView = gui->GetActiveView();

    lcModel* ActiveModel = ActiveView->GetActiveModel();

    lcCamera* Camera =  ActiveView->mCamera;

    // Switch Y and Z axis with -Y(LC -Z) in the up direction (Reset)
    lcVector3 Target = lcVector3(O->Target.x,O->Target.z,O->Target.y);

    bool DefaultCamera  = O->CameraName.isEmpty();
    bool IsOrtho        = DefaultCamera ? gui->GetPreferences().mNativeProjection : O->IsOrtho;
    bool ZoomExtents    = false; // TODO - Set as menu item? (!RenderImage && IsOrtho);
    bool UsingViewpoint = gui->GetPreferences().mNativeViewpoint <= 6;

    if (UsingViewpoint) {      // ViewPoints (Front, Back, Top, Bottom, Left, Right, Home)

        ActiveView->SetViewpoint(lcViewpoint(gui->GetPreferences().mNativeViewpoint));

    } else {                   // Default View (Angles + Distance + Perspective|Orthographic)

        Camera->m_fovy  = O->FoV + Camera->m_fovy - CAMERA_FOV_DEFAULT;

        ActiveView->SetProjection(IsOrtho);
        ActiveView->SetCameraGlobe(O->Latitude, O->Longitude, O->CameraDistance, Target, ZoomExtents);
    }

    if (Preferences::debugLogging){
        QStringList arguments;
        if (RenderImage) {
            arguments << (O->InputFileName.isEmpty()   ? QString() : QString("InputFileName: %1").arg(O->InputFileName));
            arguments << (O->OutputFileName.isEmpty()  ? QString() : QString("OutputFileName: %1").arg(O->OutputFileName));
            arguments << (O->ExportFileName.isEmpty()  ? QString() : QString("ExportFileName: %1").arg(O->ExportFileName));
            arguments << (O->IniFlag == -1             ? QString() : QString("IniFlag: %1").arg(iniFlagNames[O->IniFlag]));
            arguments << (O->ExportMode == EXPORT_NONE ? QString() : QString("ExportMode: %1").arg(nativeExportNames[O->ExportMode]));
            arguments << (O->ExportArgs.size() == 0    ? QString() : QString("ExportArgs: %1").arg(O->ExportArgs.join(" ")));
            arguments << QString("LineWidth: %1").arg(double(O->LineWidth));
            arguments << QString("TransBackground: %1").arg(O->TransBackground ? "True" : "False");
            arguments << QString("HighlightNewParts: %1").arg(O->HighlightNewParts ? "True" : "False");
        } else {
            arguments << QString("ViewerStepKey: %1").arg(O->ViewerStepKey);
            arguments << (O->ImageFileName.isEmpty() ? QString() : QString("ImageFileName: %1").arg(O->ImageFileName));
            arguments << QString("RotStep: X(%1) Y(%2) Z(%3) %4").arg(double(O->RotStep.x)).arg(double(O->RotStep.y)).arg(double(O->RotStep.z)).arg(O->RotStepType);
        }
        arguments << QString("StudLogo: %1").arg(O->StudLogo);
        arguments << QString("Resolution: %1").arg(double(O->Resolution));
        arguments << QString("ImageWidth: %1").arg(O->ImageWidth);
        arguments << QString("ImageHeight: %1").arg(O->ImageHeight);
        arguments << QString("PageWidth: %1").arg(O->PageWidth);
        arguments << QString("PageHeight: %1").arg(O->PageHeight);
        arguments << QString("CameraFoV: %1").arg(double(Camera->m_fovy));
        arguments << QString("CameraZNear: %1").arg(double(Camera->m_zNear));
        arguments << QString("CameraZFar: %1").arg(double(Camera->m_zFar));
        arguments << QString("CameraDistance (Scale %1): %2").arg(double(O->ModelScale)).arg(double(O->CameraDistance),0,'f',0);
        arguments << QString("CameraName: %1").arg(DefaultCamera ? "Default" : O->CameraName);
        arguments << QString("CameraProjection: %1").arg(IsOrtho ? "Orthographic" : "Perspective");
        arguments << QString("UsingViewpoint: %1").arg(UsingViewpoint ? "True" : "False");
        arguments << QString("ZoomExtents: %1").arg(ZoomExtents ? "True" : "False");
        arguments << QString("CameraLatitude: %1").arg(double(O->Latitude));
        arguments << QString("CameraLongitude: %1").arg(double(O->Longitude));
        arguments << QString("CameraTarget: X(%1) Y(%2) Z(%3)").arg(double(O->Target.x)).arg(double(O->Target.y)).arg(double(O->Target.z));

        removeEmptyStrings(arguments);

        QString message = QString("%1 %2 Arguments: %3")
                                .arg(RenderImage ? "Native Renderer" : "3DViewer")
                                .arg(O->ImageType == Options::CSI ? "CSI" : O->ImageType == Options::PLI ? "PLI" : "SMP")
                                .arg(arguments.join(" "));
#ifdef QT_DEBUG_MODE
      qDebug() << qPrintable(message) << "\n";
#else
      emit gui->messageSig(LOG_INFO, message);
      emit gui->messageSig(LOG_INFO, QString());
#endif
    }

    bool rc = true;

    // generate image
    if (RenderImage) {

        const bool UseImageSize = O->ImageWidth != O->PageWidth || O->ImageHeight != O->PageHeight;
        const int ImageWidth  = int(O->PageWidth);
        const int ImageHeight = int(UseImageSize ? O->PageHeight / 2 : O->PageHeight);
        QString ImageType     = O->ImageType == Options::CSI ? "CSI" : O->ImageType == Options::CSI ? "PLI" : "SMP";

        lcStep ImageStep      = 1;
        lcStep CurrentStep    = ActiveModel->GetCurrentStep();

        if (ZoomExtents)
            ActiveModel->ZoomExtents(Camera, float(ImageWidth) / float(ImageHeight));

        ActiveView->MakeCurrent();
        lcContext* Context = ActiveView->mContext;
        View View(ActiveModel);
        View.SetCamera(Camera, false);
        View.SetContext(Context);

        if ((rc = View.BeginRenderToImage(ImageWidth, ImageHeight))) {

            struct NativeImage
            {
                QImage RenderedImage;
                QRect Bounds;
            };
            NativeImage Image;

            ActiveModel->SetTemporaryStep(ImageStep);

            View.OnDraw();

            Image.RenderedImage = View.GetRenderImage();

            View.EndRenderToImage();

            Context->ClearResources();

            ActiveModel->SetTemporaryStep(CurrentStep);

            if (!ActiveModel->IsActive())
                ActiveModel->CalculateStep(LC_STEP_MAX);

            auto CalculateImageBounds = [&O, &UseImageSize](NativeImage& Image)
            {
                QImage& RenderedImage = Image.RenderedImage;
                int Width  = RenderedImage.width();
                int Height = RenderedImage.height();

                if (UseImageSize)
                {
                    int AdjX = (Width - O->ImageWidth) / 2;
                    int AdjY = (Height - O->ImageHeight) / 2;
                    Image.Bounds = QRect(QPoint(AdjX, AdjY), QPoint(QPoint(Width, Height) - QPoint(AdjX, AdjY)));

                } else {

                    int MinX = Width;
                    int MinY = Height;
                    int MaxX = 0;
                    int MaxY = 0;

                    for (int x = 0; x < Width; x++)
                    {
                        for (int y = 0; y < Height; y++)
                        {
                            if (qAlpha(RenderedImage.pixel(x, y)))
                            {
                                MinX = qMin(x, MinX);
                                MinY = qMin(y, MinY);
                                MaxX = qMax(x, MaxX);
                                MaxY = qMax(y, MaxY);
                            }
                        }
                    }

                    Image.Bounds = QRect(QPoint(MinX, MinY), QPoint(MaxX, MaxY));
                }
            };

            CalculateImageBounds(Image);

            QImageWriter Writer(O->OutputFileName);

            if (Writer.format().isEmpty())
                Writer.setFormat("PNG");

            if (!Writer.write(QImage(Image.RenderedImage.copy(Image.Bounds))))
            {
                emit gui->messageSig(LOG_ERROR,QString("Could not write to Native %1 %2 file:<br>[%3].<br>Reason: %4.<br>"
                                                       "Ensure Field of View (default is 30) and Camera Distance Factor <br>"
                                                       "are configured for the Native Renderer")
                                     .arg(ImageType)
                                     .arg(O->ExportMode == EXPORT_NONE ?
                                              QString("image") :
                                              QString("%1 object")
                                              .arg(nativeExportNames[gui->exportMode]))
                        .arg(O->OutputFileName)
                        .arg(Writer.errorString()));
                rc = false;
            }
            else
            {
                emit gui->messageSig(LOG_INFO,QMessageBox::tr("Native %1 image file rendered '%2'")
                                     .arg(ImageType).arg(O->OutputFileName));
            }

            lcGetActiveProject()->SetImageSize(Image.Bounds.width(), Image.Bounds.height());

        } else {
            emit gui->messageSig(LOG_ERROR,QMessageBox::tr("BeginRenderToImage for Native %1 image returned code %2.<br>"
                                                           "Render framebuffer is not valid").arg(ImageType).arg(rc));
        }

        if (O->ExportMode != EXPORT_NONE) {
            if (!NativeExport(O)) {
                emit gui->messageSig(LOG_ERROR,QMessageBox::tr("%1 Objects render failed.").arg(ImageType));
                rc = false;
            }
        }
    }

    return rc;
}

bool Render::RenderNativeImage(const NativeOptions *Options)
{
    if (! gui->OpenProject(Options->InputFileName))
        return false;

    return ExecuteViewer(Options,true/*exportImage*/);
}

bool Render::LoadViewer(const ViewerOptions *Options){

    gui->setViewerStepKey(Options->ViewerStepKey, Options->ImageType);

    Project* StepProject = new Project();
    if (LoadStepProject(StepProject, Options)){
        gApplication->SetProject(StepProject);
        gui->UpdateAllViews();
    }
    else
    {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Could not load 3DViewer model file %1.")
                             .arg(Options->ViewerStepKey));
        gui->setViewerStepKey(QString(),0);
        delete StepProject;
        return false;
    }

    NativeOptions *derived = new NativeOptions(*Options);
    if (derived)
        return ExecuteViewer(derived);
    else
        return false;
}

bool Render::LoadStepProject(Project* StepProject, const ViewerOptions *O)
{
    QString FileName = gui->getViewerStepFilePath(O->ViewerStepKey);

    if (FileName.isEmpty())
    {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Did not receive 3DViewer CSI path for %1.").arg(FileName));
        return false;
    }

    QStringList Content = gui->getViewerStepRotatedContents(O->ViewerStepKey);
    if (Content.isEmpty())
    {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Did not receive 3DViewer CSI content for %1.").arg(FileName));
        return false;
    }

#ifdef QT_DEBUG_MODE
    // viewerStepKey - 3 elements:
    // CSI: 0=modelName, 1=lineNumber,   2=stepNumber [_dm (displayModel)]
    // SMP: 0=modelName, 1=lineNumber,   2=stepNumber [_Preview (Submodel Preview)]
    // PLI: 0=partName,  1=colourNumber, 2=stepNumber
    QFileInfo outFileInfo(FileName);
    QString imageType   = outFileInfo.completeBaseName().replace(".ldr","");
    bool pliPart        = imageType.toLower() == "pli";
    QStringList keys    = gui->getViewerStepKeys(true/*get Name*/, pliPart);
    QString outfileName = QString("%1/viewer_%2_%3.ldr")
                                  .arg(outFileInfo.absolutePath())
                                  .arg(imageType)
                                  .arg(QString("%1_%2_%3")
                                               .arg(keys[0])
                                               .arg(keys[1])
                                               .arg(keys[2]));
    QFile file(outfileName);
    if ( ! file.open(QFile::WriteOnly | QFile::Text)) {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Cannot open 3DViewer file %1 for writing: %2")
                             .arg(outfileName) .arg(file.errorString()));
    }
    QTextStream out(&file);
    for (int i = 0; i < Content.size(); i++) {
        QString line = Content[i];
        out << line << endl;
    }
    file.close();
#endif

    StepProject->DeleteAllModels();
    StepProject->SetFileName(FileName);

    QByteArray QBA;
    foreach(QString line, Content){
        QBA.append(line);
        QBA.append(QString("\n"));
    }

    if (StepProject->GetFileName().isEmpty())
    {
        emit gui->messageSig(LOG_ERROR,QMessageBox::tr("3DViewer file name not set!"));
        return false;
    }
    QFileInfo FileInfo(StepProject->GetFileName());

    QBuffer Buffer(&QBA);
    Buffer.open(QIODevice::ReadOnly);

    while (!Buffer.atEnd())
    {
        lcModel* Model = new lcModel(QString());
        Model->SplitMPD(Buffer);

        if (StepProject->GetModels().IsEmpty() || !Model->GetProperties().mFileName.isEmpty())
        {
            StepProject->AddModel(Model);
            Model->CreatePieceInfo(StepProject);
        }
        else
            delete Model;
    }

    Buffer.seek(0);

    for (int ModelIdx = 0; ModelIdx < StepProject->GetModels().GetSize(); ModelIdx++)
    {
        lcModel* Model = StepProject->GetModels()[ModelIdx];
        Model->LoadLDraw(Buffer, StepProject);
        Model->SetSaved();
    }

    if (StepProject->GetModels().IsEmpty())
        return false;  

    if (StepProject->GetModels().GetSize() == 1)
    {
        lcModel* Model = StepProject->GetModels()[0];

        if (Model->GetProperties().mFileName.isEmpty())
        {
            Model->SetFileName(FileInfo.fileName());
            lcGetPiecesLibrary()->RenamePiece(Model->GetPieceInfo(), FileInfo.fileName().toLatin1());
        }
    }

    std::vector<lcModel*> UpdatedModels;

    for (lcModel* Model : StepProject->GetModels())
    {
        Model->UpdateMesh();
        Model->UpdatePieceInfo(UpdatedModels);
    }

    StepProject->SetModified(false);

    return true;
}

int Render::getViewerPieces()
{
    if (gui->GetActiveModel())
        return gui->GetActiveModel()->GetPieces().GetSize();
    return 0;
}

bool Render::NativeExport(const NativeOptions *Options) {

    QString exportModeName = nativeExportNames[Options->ExportMode];

    Project* NativeExportProject = new Project();

    if (Options->ExportMode == EXPORT_HTML_STEPS ||
        Options->ExportMode == EXPORT_WAVEFRONT  ||
        Options->ExportMode == EXPORT_COLLADA    ||
        Options->ExportMode == EXPORT_CSV        ||
        Options->ExportMode == EXPORT_BRICKLINK /*||
        Options->ExportMode == EXPORT_3DS_MAX*/) {

        emit gui->messageSig(LOG_STATUS, QString("Native CSI %1 Export...").arg(exportModeName));
        gApplication->SetProject(NativeExportProject);
        if (Options->ExportMode != EXPORT_HTML_STEPS) {
            if (! gui->OpenProject(Options->InputFileName)) {
                emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Failed to open CSI %1 Export project")
                                                               .arg(exportModeName));
                delete NativeExportProject;
                return false;
            }
        }
    }
    else
    {
        return doLDVCommand(Options->ExportArgs, Options->ExportMode);
    }

    if (Options->ExportMode == EXPORT_CSV)
    {
        lcGetActiveProject()->ExportCSV();
    }
    else
    if (Options->ExportMode == EXPORT_BRICKLINK)
    {
        lcGetActiveProject()->ExportBrickLink();
    }
    else
    if (Options->ExportMode == EXPORT_WAVEFRONT)
    {
        lcGetActiveProject()->ExportWavefront(Options->ExportFileName);
    }
    else
    if (Options->ExportMode == EXPORT_COLLADA)
    {
        lcGetActiveProject()->ExportCOLLADA(Options->ExportFileName);
    }
    else
    if (Options->ExportMode == EXPORT_HTML_STEPS)
    {
        bool rc = true;
        bool exportCancelled = false;

        lcHTMLExportOptions HTMLOptions(lcGetActiveProject());

        HTMLOptions.PathName = Options->ExportFileName;

        lcQHTMLDialog Dialog(gui, &HTMLOptions);
        if ((exportCancelled = Dialog.exec() != QDialog::Accepted))
            rc = true;

        HTMLOptions.SaveDefaults();

        if (! exportCancelled) {

            if (! gui->OpenProject(Options->InputFileName)) {
                emit gui->messageSig(LOG_ERROR,QMessageBox::tr("Failed to open CSI %1 Export project")
                                                               .arg(exportModeName));
                delete NativeExportProject;
                rc = false;
            }

            lcGetActiveProject()->ExportHTML(HTMLOptions);

            QString htmlIndex = QDir::fromNativeSeparators(
                                HTMLOptions.PathName + "/" +
                                QFileInfo(Options->InputFileName).baseName() +
                                "-index.html");

            gui->setExportedFile(htmlIndex);

            gui->showExportedFile();
        }

        return rc;
    }
/*
    // These are executed through the LDV Native renderer
    else
    if (Options->ExportMode == EXPORT_POVRAY)
    {
        lcGetActiveProject()->ExportPOVRay(Options->ExportFileName);
    }
    else
    if (Options->ExportMode == EXPORT_3DS_MAX)
    {
        lcGetActiveProject()->Export3DStudio(Options->ExportFileName);
    }
*/

    return true;
}

void Render::showLdvExportSettings(int iniFlag){
    ldvWidget = new LDVWidget(nullptr,IniFlag(iniFlag),true);
    ldvWidget->showLDVExportOptions();
}

void Render::showLdvLDrawPreferences(int iniFlag){
    ldvWidget = new LDVWidget(nullptr,IniFlag(iniFlag),true);
    ldvWidget->showLDVPreferences();
}

bool Render::doLDVCommand(const QStringList &args, int exportMode, int iniFlag){
    QString exportModeName = nativeExportNames[exportMode];
    bool exportHTML = exportMode == EXPORT_HTML_PARTS;
    QStringList arguments = args;

    if (exportMode == EXPORT_NONE && iniFlag == NumIniFiles) {
        emit gui->messageSig(LOG_ERROR, QString("Invalid export mode and ini flag codes specified."));
        return false;
    }

    switch (exportMode){
    case EXPORT_HTML_PARTS:
        iniFlag = NativePartList;
        break;
    case EXPORT_POVRAY:
        iniFlag = NativePOVIni;
        break;
    case POVRAY_RENDER:
        iniFlag = POVRayRender;
        break;
    case EXPORT_STL:
        iniFlag = NativeSTLIni;
        break;
    case EXPORT_3DS_MAX:
        iniFlag = Native3DSIni;
        break;
    default:
        if (iniFlag == NumIniFiles)
            return false;
        break;
    }

    QString workingDirectory = QDir::currentPath();
    emit gui->messageSig(LOG_TRACE, QString("Native CSI %1 Export for command: %2")
                                             .arg(exportModeName)
                                             .arg(arguments.join(" ")));
    ldvWidget = new LDVWidget(nullptr,IniFlag(iniFlag),true);
    if (exportHTML)
        gui->connect(ldvWidget, SIGNAL(loadBLCodesSig()), gui, SLOT(loadBLCodes()));
    if (! ldvWidget->doCommand(arguments))  {
        emit gui->messageSig(LOG_ERROR, QString("Failed to generate CSI %1 Export for command: %2")
                                                .arg(exportModeName)
                                                .arg(arguments.join(" ")));
        return false;
    }
    if (! QDir::setCurrent(workingDirectory)) {
        emit gui->messageSig(LOG_ERROR, QString("Failed to restore CSI %1 export working directory: %2")
                                                .arg(exportModeName)
                                                .arg(workingDirectory));
        return false;
    }
    if (exportHTML)
        gui->disconnect(ldvWidget, SIGNAL(loadBLCodesSig()), gui, SLOT(loadBLCodes()));

    return true;
}

const QString Render::getPovrayRenderQuality(int quality)
{
    // POV-Ray 3.2 Command-Line and INI-File Options
    if (quality == -1)
        quality = Preferences::povrayRenderQuality;

    QStringList Arguments;

    switch (quality)
    {
    case 0:
        Arguments << QString("+Q11");
        Arguments << QString("+R3");
        Arguments << QString("+A0.1");
        Arguments << QString("+J0.5");
        break;

    case 1:
        Arguments << QString("+Q5");
        Arguments << QString("+A0.1");
        break;

    case 2:
        break;
    }

    return Arguments.join(" ");
}

const QString Render::getRenderImageFile(int renderType)
{
    QDir renderDir(QString("%1/%2").arg(QDir::currentPath())
                   .arg(renderType == POVRAY_RENDER ? Paths::povrayRenderDir : Paths::blenderRenderDir));
    if (!renderDir.exists()) {
        if (renderType == POVRAY_RENDER)
            Paths::mkPovrayDir();
        else
            Paths::mkBlenderDir();
    }

    QString fileName = gui->getViewerConfigKey(gui->getViewerStepKey()).replace(";","_");

    if (fileName.isEmpty()){
        fileName = renderType == POVRAY_RENDER ? "povray_image_render" : "blender_image_render";
        emit gui->messageSig(LOG_NOTICE, QString("Failed to receive model file name - using [%1]").arg(fileName));
    }

    QString imageFile = QDir::toNativeSeparators(QString("%1/%2.png")
                       .arg(renderDir.absolutePath())
                       .arg(fileName));

    return imageFile;

}

const QString Render::getRenderModelFile(int renderType) {

    QString modelFile;
    QString filePath = QDir::currentPath() + QDir::separator() + Paths::tmpDir + QDir::separator();

    if (renderType == POVRAY_RENDER) {

        modelFile = QDir::toNativeSeparators(filePath + "csi_povray.ldr");

    } else if (renderType == BLENDER_RENDER) {

        modelFile = QDir::toNativeSeparators(filePath + "csi_blender.ldr");

        gui->saveCurrent3DViewerModel(modelFile);
    }
    return modelFile;
}

// create Native version of the CSI/PLI file - consolidate subfiles and parts into single file
int Render::createNativeModelFile(
    QStringList &csiRotatedParts,
    bool         doFadeStep,
    bool         doHighlightStep)
{
  QStringList csiSubModels;
  QStringList csiSubModelParts;
  QStringList csiParts = csiRotatedParts;

  QStringList argv;
  int         rc;

  if (csiRotatedParts.size() > 0) {

      /* Parse the rotated parts looking for subModels,
       * renaming fade and highlight step parts (not sure this is used here)
       * merging and formatting submodels by calling mergeNativeCSISubModels and
       * returning all parts by reference
      */
      for (int index = 0; index < csiRotatedParts.size(); index++) {

          QString csiLine = csiRotatedParts[index];
          split(csiLine, argv);
          if (argv.size() == 15 && argv[0] == "1") {

              /* process subfiles in csiRotatedParts */
              QString type = argv[argv.size()-1];

              bool isCustomSubModel = false;
              bool isCustomPart = false;
              QString customType;

              // Custom part types
              if (doFadeStep) {
                  QString fadeSfx = QString("%1.").arg(FADE_SFX);
                  bool isFadedItem = type.contains(fadeSfx);
                  // Fade file
                  if (isFadedItem) {
                      customType = type;
                      customType = customType.replace(fadeSfx,".");
                      isCustomSubModel = gui->isSubmodel(customType);
                      isCustomPart = gui->isUnofficialPart(customType);
                    }
                }

              if (doHighlightStep) {
                  QString highlightSfx = QString("%1.").arg(HIGHLIGHT_SFX);
                  bool isHighlightItem = type.contains(highlightSfx);
                  // Highlight file
                  if (isHighlightItem) {
                      customType = type;
                      customType = customType.replace(highlightSfx,".");
                      isCustomSubModel = gui->isSubmodel(customType);
                      isCustomPart = gui->isUnofficialPart(customType);
                    }
                }

              if (gui->isSubmodel(type) || gui->isUnofficialPart(type) || isCustomSubModel || isCustomPart) {
                  /* capture subfiles (full string) to be processed when finished */
                  if (!csiSubModels.contains(type.toLower()))
                       csiSubModels << type.toLower();
                }
            }
        } //end for

      /* process extracted submodels and unofficial files */
      if (csiSubModels.size() > 0){
          if (csiSubModels.size() > 2)
              csiSubModels.removeDuplicates();
          if ((rc = mergeNativeCSISubModels(csiSubModels, csiSubModelParts, doFadeStep, doHighlightStep)) != 0){
              emit gui->messageSig(LOG_ERROR,QString("Failed to process viewer CSI submodels"));
              return rc;
            }
        }

      /* add sub model content to csiRotatedParts file */
      if (! csiSubModelParts.empty())
        {
          for (int i = 0; i < csiSubModelParts.size(); i++) {
              QString smLine = csiSubModelParts[i];
              csiParts << smLine;
            }
        }

      /* return rotated parts by reference */
      csiRotatedParts = csiParts;
    }

  return 0;
}

int Render::mergeNativeCSISubModels(QStringList &subModels,
                                  QStringList &subModelParts,
                                  bool doFadeStep,
                                  bool doHighlightStep)
{
  QStringList csiSubModels        = subModels;
  QStringList csiSubModelParts    = subModelParts;
  QStringList newSubModels;

  QStringList argv;
  int         rc;

  if (csiSubModels.size() > 0) {

      /* read in all detected sub model file content */
      for (int index = 0; index < csiSubModels.size(); index++) {

          QString ldrName(QDir::currentPath() + "/" +
                          Paths::tmpDir + "/" +
                          csiSubModels[index]);

          /* initialize the working submodel file - define header. */
          QString modelName = QFileInfo(csiSubModels[index]).completeBaseName().toLower();
          modelName = modelName.replace(
                      modelName.indexOf(modelName.at(0)),1,modelName.at(0).toUpper());

          csiSubModelParts << QString("0 FILE %1").arg(csiSubModels[index]);
          csiSubModelParts << QString("0 %1").arg(modelName);
          csiSubModelParts << QString("0 Name: %1").arg(csiSubModels[index]);
          csiSubModelParts << QString("0 !LPUB MODEL NAME %1").arg(modelName);

          /* read the actual submodel file */
          QFile ldrfile(ldrName);
          if ( ! ldrfile.open(QFile::ReadOnly | QFile::Text)) {
              emit gui->messageSig(LOG_ERROR,QString("Could not read CSI submodel file %1: %2")
                                   .arg(ldrName)
                                   .arg(ldrfile.errorString()));
              return -1;
            }
          /* populate file contents into working submodel csi parts */
          QTextStream in(&ldrfile);
          while ( ! in.atEnd()) {
              QString csiLine = in.readLine(0);
              split(csiLine, argv);

              if (argv.size() == 15 && argv[0] == "1") {
                  /* check and process any subfiles in csiRotatedParts */
                  QString type = argv[argv.size()-1];

                  bool isCustomSubModel = false;
                  bool isCustomPart = false;
                  QString customType;

                  // Custom part types
                  if (doFadeStep) {
                      QString fadeSfx = QString("%1.").arg(FADE_SFX);
                      bool isFadedItem = type.contains(fadeSfx);
                      // Fade file
                      if (isFadedItem) {
                          customType = type;
                          customType = customType.replace(fadeSfx,".");
                          isCustomSubModel = gui->isSubmodel(customType);
                          isCustomPart = gui->isUnofficialPart(customType);
                        }
                    }

                  if (doHighlightStep) {
                      QString highlightSfx = QString("%1.").arg(HIGHLIGHT_SFX);
                      bool isHighlightItem = type.contains(highlightSfx);
                      // Highlight file
                      if (isHighlightItem) {
                          customType = type;
                          customType = customType.replace(highlightSfx,".");
                          isCustomSubModel = gui->isSubmodel(customType);
                          isCustomPart = gui->isUnofficialPart(customType);
                        }
                    }

                  if (gui->isSubmodel(type) || gui->isUnofficialPart(type) || isCustomSubModel || isCustomPart) {
                      /* capture all subfiles (full string) to be processed when finished */
                      if (!newSubModels.contains(type.toLower()))
                              newSubModels << type.toLower();
                    }
                }
              if (isGhost(csiLine))
                  argv.prepend(GHOST_META);
              csiLine = argv.join(" ");
              csiSubModelParts << csiLine;
            }
          csiSubModelParts << "0 NOFILE";
        }

      /* recurse and process any identified submodel files */
      if (newSubModels.size() > 0){
          if (newSubModels.size() > 2)
              newSubModels.removeDuplicates();
          if ((rc = mergeNativeCSISubModels(newSubModels, csiSubModelParts, doFadeStep, doHighlightStep)) != 0){
              emit gui->messageSig(LOG_ERROR,QString("Failed to recurse viewer CSI submodels"));
              return rc;
            }
        }
      subModelParts = csiSubModelParts;
    }
  return 0;
}
