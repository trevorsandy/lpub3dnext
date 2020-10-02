
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
 * This class encapsulates the external renderers.  For now, this means
 * only ldglite.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "lpub.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "render.h"
#include "resolution.h"
#include "meta.h"
#include "math.h"
#include "lpub_preferences.h"

#include "paths.h"
//**3D
#include "lc_mainwindow.h"
//**

#ifndef __APPLE__
#include <windows.h>
#endif

Render *renderer;

LDGLite ldglite;
LDView  ldview;
POVRay povray;

//#define LduDistance 5729.57
#define CA "-ca0.01"
#define USE_ALPHA "+UA"

static double pi = 4*atan(1.0);
// the default camera distance for real size
static float LduDistance = 10.0/tan(0.005*pi/180);

// renderer timeout in milliseconds
static int rendererTimeout(){
    if (Preferences::rendererTimeout == -1)
        return -1;
    else
        return Preferences::rendererTimeout*60*1000;
}

QString const Render::getRenderer()
{
  if (renderer == &ldglite) {
    return "LDGLite";
  } else if (renderer == &ldview){
    return "LDView";
  } else {
      return "POVRay";
  }
}

void Render::setRenderer(QString const &name)
{
  if (name == "LDGLite") {
    renderer = &ldglite;
  } else if (name == "LDView") {
    renderer = &ldview;
  } else {
      renderer = &povray;
  }
}

bool Render::useLDViewSCall(bool override){
  if (override)
    return override;
  else
    return Preferences::useLDViewSingleCall;
}

void clipImage(QString const &pngName){
	//printf("\n");
	QImage toClip(QDir::toNativeSeparators(pngName));
	QRect clipBox = toClip.rect();
	
	//printf("clipping %s from %d x %d at (%d,%d)\n",qPrintable(QDir::toNativeSeparators(pngName)),clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	int x,y;
	int initLeft = clipBox.left();
	int initTop = clipBox.top();
	int initRight = clipBox.right();
	int initBottom = clipBox.bottom();
	for(x = initLeft; x < initRight; x++){
		for(y = initTop; y < initBottom; y++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (y != initBottom) {
			clipBox.setLeft(x);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	
	initLeft = clipBox.left();
	for(x = initRight; x >= initLeft; x--){
		for(y = initTop; y < initBottom; y++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (y != initBottom) {
			clipBox.setRight(x);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	initRight = clipBox.right();
	for(y = initTop; y < initBottom; y++){
		for(x = initLeft; x < initRight; x++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (x != initRight) {
			clipBox.setTop(y);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	initTop = clipBox.top();
	for(y = initBottom; y >= initTop; y--){
		for(x = initLeft; x < initRight; x++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (x != initRight) {
			clipBox.setBottom(y);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	QImage clipped = toClip.copy(clipBox);
	//toClip.save(QDir::toNativeSeparators(pngName+"-orig.png"));
	clipped.save(QDir::toNativeSeparators(pngName));
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
	
	return factor*LduDistance;
}

/***************************************************************************
 *
 * The math for zoom factor.  1.0 is true size.
 *
 * 1 LDU is 1/64 of an inch
 *
 * LDGLite produces 72 DPI
 *
 * Camera angle is 0.01
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
float POVRay::cameraDistance(Meta &meta, float scale){

    if (Preferences::ldviewPOVRayFileGenerator)
        return stdCameraDistance(meta, scale)*0.455;
    else
        return stdCameraDistance(meta, scale);
}

int POVRay::renderCsi(
          const QString     &addLine,
          const QStringList &csiParts,
          const QString     &pngName,
          Meta              &meta){
	
	
	/* Create the CSI DAT file */
	QString ldrName;
	int rc;
	ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
	QString povName = ldrName +".pov";
	if ((rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName)) < 0) {
		return rc;
	}
	
	QStringList arguments;
    QStringList list;
    bool hasLGEO = Preferences::lgeoPath != "";

  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

    if (Preferences::ldviewPOVRayFileGenerator) {
        // LDView pov file generator

        /* determine camera distance */
        int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value())*1700/1000;
        //qDebug() << "LDView (Native) Camera Distance: " <<

        QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.assem.angle.value(0))
                                            .arg(meta.LPub.assem.angle.value(1))
                                            .arg(cd);

        QString w  = QString("-SaveWidth=%1") .arg(width);
        QString h  = QString("-SaveHeight=%1") .arg(height);
        QString s  = QString("-ExportFile=%1") .arg(povName);

        arguments << CA;
        arguments << cg;
        arguments << "-PovExporter/Floor=0";
        arguments << "-ShowHighlightLines=1";
        arguments << "-ConditionalHighlights=1";
        arguments << "-SaveZoomToFit=0";
        arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
  arguments << "-SnapshotSuffix=.png";
  arguments << w;
  arguments << h;
  arguments << s;

        list = meta.LPub.assem.ldviewParms.value().split("\\s+");
        for (int i = 0; i < list.size(); i++) {
          if (list[i] != "" && list[i] != " ") {
            arguments << list[i];
          }
        }
        arguments << ldrName;

        emit gui->messageSig(true, "POVRay (LDView) render CSI...");

        QProcess    ldview;
        ldview.setEnvironment(QProcess::systemEnvironment());
        ldview.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
        //qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";
        ldview.start(Preferences::ldviewExe,arguments);
        if ( ! ldview.waitForFinished(rendererTimeout())) {
          if (ldview.exitCode() != 0 || 1) {
            QByteArray status = ldview.readAll();
            QString str;
            str.append(status);
            emit gui->messageSig(false,QMessageBox::tr("LDView POV-RAY file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
            return -1;
          }
        }
        // LDView end
    } else {
        // L3P pov file generator
        int cd = cameraDistance(meta, meta.LPub.assem.modelScale.value());
        float ar = width/(float)height;

        QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.assem.angle.value(0))
                                            .arg(meta.LPub.assem.angle.value(1))
                                            .arg(cd);
        QString car = QString("-car%1").arg(ar);
        QString ldd = QString("-ldd%1").arg(QDir::toNativeSeparators(Preferences::ldrawPath));
        arguments << CA;
        arguments << cg;
        arguments << "-ld";
        if(hasLGEO){
            QString lgd = QString("-lgd%1").arg(QDir::toNativeSeparators(Preferences::lgeoPath));
            arguments << "-lgeo";
            arguments << lgd;
        }
        arguments << car;
        arguments << "-o";
        arguments << ldd;

        list = meta.LPub.assem.l3pParms.value().split("\\s+");
        for (int i = 0; i < list.size(); i++) {
            if (list[i] != "" && list[i] != " ") {
                arguments << list[i];
            }
        }

        arguments << QDir::toNativeSeparators(ldrName);
        arguments << QDir::toNativeSeparators(povName);

        emit gui->messageSig(true, "POVRay (L3P) render CSI...");

        QProcess l3p;
        QStringList env = QProcess::systemEnvironment();
        l3p.setEnvironment(env);
        l3p.setWorkingDirectory(QDir::currentPath() +"/"+Paths::tmpDir);
        l3p.setStandardErrorFile(QDir::currentPath() + "/stderr");
        l3p.setStandardOutputFile(QDir::currentPath() + "/stdout");
        //qDebug() << qPrintable(Preferences::l3pExe + " " + arguments.join(" ")) << "\n";
        l3p.start(Preferences::l3pExe,arguments);
        if ( ! l3p.waitForFinished(rendererTimeout())) {
            if (l3p.exitCode() != 0) {
                QByteArray status = l3p.readAll();
                QString str;
                str.append(status);
                emit gui->messageSig(false,QMessageBox::tr("L3P POV-RAY file generation failed with code %1\n%2").arg(l3p.exitCode()) .arg(str));
                return -1;
            }
        }
        // L3P end
    }

	QStringList povArguments;
    QString O = QString("+O%1").arg(QDir::toNativeSeparators(pngName));
    QString I = QString("+I%1").arg(QDir::toNativeSeparators(povName));
	QString W = QString("+W%1").arg(width);
	QString H = QString("+H%1").arg(height);
	
	povArguments << I;
	povArguments << O;
	povArguments << W;
	povArguments << H;
	povArguments << USE_ALPHA;
	if(hasLGEO){
        QString lgeoLg = QString("+L%1").arg(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg"));
        QString lgeoAr = QString("+L%2").arg(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar"));
		povArguments << lgeoLg;
		povArguments << lgeoAr;
	}
#ifndef __APPLE__
	povArguments << "/EXIT";
#endif
	
    list = meta.LPub.assem.povrayParms.value().split("\\s+");
	for (int i = 0; i < list.size(); i++) {
		if (list[i] != "" && list[i] != " ") {
			povArguments << list[i];
		}
	}
	
    emit gui->messageSig(true, "POV-RAY render CSI...");

	QProcess povray;
	QStringList povEnv = QProcess::systemEnvironment();
	povray.setEnvironment(povEnv);
	povray.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
	povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
	povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");
    //qDebug() << qPrintable(Preferences::povrayExe + " " + povArguments.join(" ")) << "\n";
	povray.start(Preferences::povrayExe,povArguments);
    if ( ! povray.waitForFinished(rendererTimeout())) {
		if (povray.exitCode() != 0) {
			QByteArray status = povray.readAll();
			QString str;
			str.append(status);
            emit gui->messageSig(false,QMessageBox::tr("POV-RAY failed with code %1\n%2").arg(povray.exitCode()) .arg(str));
			return -1;
		}
	}
	
	clipImage(pngName);

	return 0;
	
}

int POVRay::renderPli(
           const QStringList &ldrNames,
		   const QString &pngName,
		   Meta    &meta,
		   bool     bom){
	
    QString povName = ldrNames.first() +".pov";

    QStringList arguments;
    QStringList list;
    bool hasLGEO = Preferences::lgeoPath != "";

  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

    if (Preferences::ldviewPOVRayFileGenerator) {
        // LDView pov file generator

        PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

        /* determine camera distance */
        int cd = cameraDistance(meta,pliMeta.modelScale.value())*1700/1000;

        QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                .arg(pliMeta.angle.value(1))
                .arg(cd);

        QString w  = QString("-SaveWidth=%1")  .arg(width);
        QString h  = QString("-SaveHeight=%1") .arg(height);
        QString s  = QString("-ExportFile=%1") .arg(povName);

        arguments << CA;
        arguments << cg;
        arguments << "-PovExporter/Floor=0";
        arguments << "-ShowHighlightLines=1";
        arguments << "-ConditionalHighlights=1";
        arguments << "-SaveZoomToFit=0";
        arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
  arguments << "-SnapshotSuffix=.png";
  arguments << w;
  arguments << h;
  arguments << s;

        list = meta.LPub.pli.ldviewParms.value().split("\\s+");
        for (int i = 0; i < list.size(); i++) {
            if (list[i] != "" && list[i] != " ") {
                arguments << list[i];
            }
        }
        arguments << ldrNames.first();

        emit gui->messageSig(true, "POV-RAY (LDView) render PLI...");

        QProcess    ldview;
        ldview.setEnvironment(QProcess::systemEnvironment());
        ldview.setWorkingDirectory(QDir::currentPath());
        //qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";
        ldview.start(Preferences::ldviewExe,arguments);
        if ( ! ldview.waitForFinished()) {
            if (ldview.exitCode() != 0) {
                QByteArray status = ldview.readAll();
                QString str;
                str.append(status);
                emit gui->messageSig(false,QMessageBox::tr("LDView POV-RAY file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
                return -1;
            }
        }
        // LDView end
    } else {
        // L3P pov file generator

        float ar = width/(float)height;

        /* determine camera distance */

        PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

        int cd = cameraDistance(meta,pliMeta.modelScale.value());

        QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                .arg(pliMeta.angle.value(1))
                .arg(cd);

        QString car = QString("-car%1").arg(ar);
        QString ldd = QString("-ldd%1").arg(QDir::toNativeSeparators(Preferences::ldrawPath));

        arguments << CA;
        arguments << cg;
        arguments << "-ld";
        if(hasLGEO){
            QString lgd = QString("-lgd%1").arg(QDir::toNativeSeparators(Preferences::lgeoPath));
            arguments << "-lgeo";
            arguments << lgd;
        }
        arguments << car;
        arguments << "-o";
        arguments << ldd;

        QStringList list;
        list = meta.LPub.assem.l3pParms.value().split("\\s+");
        for (int i = 0; i < list.size(); i++) {
            if (list[i] != "" && list[i] != " ") {
                arguments << list[i];
            }
        }

        arguments << QDir::toNativeSeparators(ldrNames.first());
        arguments << QDir::toNativeSeparators(povName);

        emit gui->messageSig(true, "POV-RAY (L3P) render PLI...");

        QProcess    l3p;
        QStringList env = QProcess::systemEnvironment();
        l3p.setEnvironment(env);
        l3p.setWorkingDirectory(QDir::currentPath());
        l3p.setStandardErrorFile(QDir::currentPath() + "/stderr");
        l3p.setStandardOutputFile(QDir::currentPath() + "/stdout");
        //qDebug() << qPrintable(Preferences::l3pExe + " " + arguments.join(" ")) << "\n";
        l3p.start(Preferences::l3pExe,arguments);
        if (! l3p.waitForFinished()) {
            if (l3p.exitCode()) {
                QByteArray status = l3p.readAll();
                QString str;
                str.append(status);
                emit gui->messageSig(false,QMessageBox::tr("L3P POV-RAY file generation failed with exit code %1\n%2") .arg(l3p.exitCode()) .arg(str));
                return -1;
            }
        }
    }

	QStringList povArguments;
    QString O = QString("+O%1").arg(QDir::toNativeSeparators(pngName));
    QString I = QString("+I%1").arg(QDir::toNativeSeparators(povName));
	QString W = QString("+W%1").arg(width);
	QString H = QString("+H%1").arg(height);
	
	povArguments << I;
	povArguments << O;
	povArguments << W;
	povArguments << H;
	povArguments << USE_ALPHA;
	if(hasLGEO){
        QString lgeoLg = QString("+L%1").arg(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg"));
        QString lgeoAr = QString("+L%2").arg(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar"));
		povArguments << lgeoLg;
		povArguments << lgeoAr;
	}
#ifndef __APPLE__
	povArguments << "/EXIT";
#endif
	
    list = meta.LPub.assem.povrayParms.value().split("\\s+");
	for (int i = 0; i < list.size(); i++) {
		if (list[i] != "" && list[i] != " ") {
			povArguments << list[i];
		}
	}

    emit gui->statusMessage(true, "Execute command: POV-RAY render PLI.");

	QProcess povray;
	QStringList povEnv = QProcess::systemEnvironment();
	povray.setEnvironment(povEnv);
	povray.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
	povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
	povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");
    //qDebug() << qPrintable(Preferences::povrayExe + " " + povArguments.join(" ")) << "\n";
	povray.start(Preferences::povrayExe,povArguments);
    if ( ! povray.waitForFinished(rendererTimeout())) {
		if (povray.exitCode() != 0) {
			QByteArray status = povray.readAll();
			QString str;
			str.append(status);
            emit gui->messageSig(false,QMessageBox::tr("POV-RAY failed with code %1\n%2") .arg(povray.exitCode()) .arg(str));
			return -1;
		}
	}
	
	clipImage(pngName);
	
	return 0;	
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
  const QString     &pngName,
        Meta        &meta)
{
	/* Create the CSI DAT file */
	QString ldrName;
	int rc;
	ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
	if ((rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName)) < 0) {
		return rc;
	}

  /* determine camera distance */
  
  QStringList arguments;

  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value());

  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
  
  int lineThickness = resolution()/150+0.5;
  if (lineThickness == 0) {
    lineThickness = 1;
  }
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(lineThickness);

  QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.assem.angle.value(0))
                                      .arg(meta.LPub.assem.angle.value(1))
                                      .arg(cd);

  arguments << "-l3";               // use l3 parser
  arguments << "-i2";               // image type 2=.png
  arguments << CA;                  // camera FOV angle in degrees
  arguments << cg;                  // camera globe - scale factor
  arguments << "-J";                // perspective projection
  arguments << v;                   // display in X wide by Y high window
  arguments << o;                   // changes the center X across and Y down
  arguments << w;                   // line thickness
  arguments << "-q";                // Anti Aliasing (Quality Lines)

  QStringList list;                 // -fh = Turns on shading mode
  list = meta.LPub.assem.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }

  arguments << mf;                  // .png file name
  arguments << ldrName;             // csi.ldr (input file)

  emit gui->messageSig(true, "LDGLite render CSI...");

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  if (!Preferences::ldgliteSearchDirs.isEmpty())
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;

//  logDebug() << "ldglite CSI Arguments:" << arguments;
//  logDebug() << "LDSEARCHDIRS:" << Preferences::ldgliteSearchDirs;
//  logDebug() << "ENV:" << env;

  ldglite.setEnvironment(env);
  ldglite.setWorkingDirectory(QDir::currentPath() + "/"+Paths::tmpDir);
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout");
  ldglite.start(Preferences::ldgliteExe,arguments);
  if ( ! ldglite.waitForFinished(rendererTimeout())) {
    if (ldglite.exitCode() != 0) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }

  return 0;
}

  
int LDGLite::renderPli(
  const QStringList &ldrNames,
  const QString     &pngName,
  Meta    &meta,
  bool     bom)
{
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  /* determine camera distance */

  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  int cd = cameraDistance(meta,pliMeta.modelScale.value());

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(int(resolution()/72.0+0.5));

  QStringList arguments;
  arguments << "-l3";
  arguments << "-i2";
  arguments << CA;
  arguments << cg;
  arguments << "-J";
  arguments << v;
  arguments << o;
  arguments << w;
  arguments << "-q";          //Anti Aliasing (Quality Lines)

  QStringList list;
  list = meta.LPub.pli.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
	  if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
	  }
  }
  arguments << mf;
  arguments << ldrNames.first();
  
  emit gui->messageSig(true, "LDGLite render PLI...");

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  if (!Preferences::ldgliteSearchDirs.isEmpty())
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;

//  qDebug() << "ldglite PLI Arguments: " << arguments;
//  qDebug() << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;

  ldglite.setEnvironment(env);
  ldglite.setWorkingDirectory(QDir::currentPath());
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout");
  ldglite.start(Preferences::ldgliteExe,arguments);
  if (! ldglite.waitForFinished()) {
    if (ldglite.exitCode()) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
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
	return stdCameraDistance(meta, scale)*0.775;
}

int LDView::renderCsi(
  const QString     &addLine,
  const QStringList &content,
  const QString     &pngName,
        Meta        &meta)
{
  QStringList ldrNames;

  /* determine camera distance */
  
  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.assem.angle.value(0))
                                      .arg(meta.LPub.assem.angle.value(1))
                                      .arg(cd);
  QString w  = QString("-SaveWidth=%1") .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s;
  if (useLDViewSCall()) {
      s  = QString("-SaveSnapShots=1");
      ldrNames = content;
  } else {
      s  = QString("-SaveSnapShot=%1") .arg(pngName);
      /* Create the CSI DAT file */
      int rc;
      ldrNames << QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
      if ((rc = rotateParts(addLine,meta.rotStep, content, ldrNames.first())) < 0) {
          return rc;
      }
  }

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << "-SaveAlpha=1";
  arguments << "-AutoCrop=1";
  arguments << "-ShowHighlightLines=1";
  arguments << "-ConditionalHighlights=1";
  arguments << "-SaveZoomToFit=0";
  arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
//  arguments << "-SnapshotSuffix=.png";
  arguments << w;
  arguments << h;
  arguments << s;

  QStringList list;
  list = meta.LPub.assem.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments = arguments + ldrNames;

  emit gui->messageSig(true, "LDView render CSI...");
  
  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
  ldview.start(Preferences::ldviewExe,arguments);
  //qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";
  if ( ! ldview.waitForFinished(rendererTimeout())) {
    if (ldview.exitCode() != 0 || 1) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDView failed\n%1") .arg(str));
      return -1;
    }
  }

  if (useLDViewSCall()){
    // move generated CSI images
    QString ldrName;
    QDir dir(QDir::currentPath() + "/" + Paths::tmpDir);
    foreach(ldrName, ldrNames){
        QFileInfo fInfo(ldrName.replace(".ldr",".png"));
        QString imageFilePath = QDir::currentPath() + "/" +
            Paths::assemDir + "/" + fInfo.fileName();
        if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
            //in case failure because file exist
            QFile pngFile(imageFilePath);
            if (! pngFile.exists()){
                emit gui->messageSig(false,QMessageBox::tr("LDView CSI image file move failed for\n%1")
                                     .arg(imageFilePath));
                return -1;
              } else {
                //file exist so delete and retry
                QFile pngFile(imageFilePath);
                if (pngFile.remove()) {
                    //retry
                    if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
                        emit gui->messageSig(false,QMessageBox::tr("LDView CSI image file move failed AGAIN for\n%1")
                                             .arg(imageFilePath));
                        return -1;
                      }
                  } else {
                    emit gui->messageSig(false,QMessageBox::tr("LDView could not remove old CSI image file \n%1")
                                         .arg(imageFilePath));
                    return -1;
                  }
              }
          }
      }
  }

  return 0;
}

int LDView::renderPli(
  const QStringList &ldrNames,
  const QString &pngName,
  Meta    &meta,
  bool     bom)
{
  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  /* determine camera distance */

  int cd = cameraDistance(meta,pliMeta.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);
  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s;
  if (useLDViewSCall())
      s  = QString("-SaveSnapShots=1");
  else
      s  = QString("-SaveSnapShot=%1") .arg(pngName);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << "-SaveAlpha=1";
  arguments << "-AutoCrop=1";
  arguments << "-ShowHighlightLines=1";
  arguments << "-ConditionalHighlights=1";
  arguments << "-SaveZoomToFit=0";
  arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
//  arguments << "-SnapshotSuffix=.png";
  arguments << w;
  arguments << h;
  arguments << s;

  QStringList list;
  list = meta.LPub.pli.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments = arguments + ldrNames;

  emit gui->messageSig(true, "LDView render PLI...");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath());
  //qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";
  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished()) {
    if (ldview.exitCode() != 0) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDView failed\n%1") .arg(str));
      return -1;
    }
  }

  if (useLDViewSCall()){
    // move generated CSI images
    QString ldrName;
    QDir dir(QDir::currentPath() + "/" + Paths::tmpDir);
    foreach(ldrName, ldrNames){
        QFileInfo fInfo(ldrName.replace(".ldr",".png"));
        QString imageFilePath = QDir::currentPath() + "/" +
            Paths::partsDir + "/" + fInfo.fileName();
        if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
            //in case failure because file exist
            QFile pngFile(imageFilePath);
            if (! pngFile.exists()){
                emit gui->messageSig(false,QMessageBox::tr("LDView PLI image file move failed for\n%1")
                           .arg(imageFilePath));
                return -1;
              }else {
                //file exist so delete and retry
                QFile pngFile(imageFilePath);
                if (pngFile.remove()) {
                    //retry
                    if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
                        emit gui->messageSig(false,QMessageBox::tr("LDView PLI image file move failed AGAIN for\n%1")
                                             .arg(imageFilePath));
                        return -1;
                      }
                  } else {
                    emit gui->messageSig(false,QMessageBox::tr("LDView could not remove old PLI image file \n%1")
                                         .arg(imageFilePath));
                    return -1;
                  }
              }
          }
      }
  }

  return 0;
}
