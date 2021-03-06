/****************************************************************************
**
** Copyright (C) 2015 - 2020 Trevor SANDY. All rights reserved.
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
#ifndef APPLICATION_H
#define APPLICATION_H

#include <QScopedPointer>
#include <QException>
#include <QtWidgets>
#include <QFile>
#include <string>
#include <vector>

#include "QsLog.h"
#include "name.h"

#ifdef Q_OS_WIN
  #include <Windows.h>
#endif

class InitException: public QException
{
public:
    void raise() const {throw InitException{};}
};

/// The Application class is responsible for further initialization of the app
/// and provides acessors to the current instance and internal resources. It
/// also take cares of shutdown cleanup. An Application class must be
/// instantiaded only once.
class Application : public QObject
{
     Q_OBJECT

public:
    /// Creates the Application.
    Application(int& argc, char **argv);

    /// Returns a pointer to the current Application instance;
    static Application* instance();

    /// Returns the command line arguments
    QStringList arguments();

    /// Initialize the Application and process the command line arguments.
    void initialize();

    /// This is the equivalent of the main function.
    void mainApp();

    /// Runs the Application and returns the exit code.
    int run();

    /// Return applicaion launch mode
    bool modeGUI();

    /// Sets the theme
    void setTheme(bool appStarted = true);

    /// Gets the theme
    QString getTheme();

#ifdef Q_OS_WIN
    /// Console redirection for Windows
    void RedirectIOToConsole();
    /// Windows dmp file generation
    static void lcSehInit();
    static LONG WINAPI lcSehHandler(PEXCEPTION_POINTERS exceptionPointers);
#endif

    /// Initialize the splash screen
    QSplashScreen *splash;

public slots:
    /// Splash message function to display message updates during startup
    void splashMsg(QString message)
    {
      if (m_console_mode)
      {
          logInfo() << message;
          return;
      }
#ifdef QT_DEBUG_MODE
      logInfo() << message;
#else
      logStatus() << message;
#endif
      splash->showMessage(QSplashScreen::tr(message.toLatin1().constData()),Qt::AlignBottom | Qt::AlignLeft, QColor(QString(SPLASH_FONT_COLOUR)));
      m_application.processEvents();
    }

signals:
    /// Splash message signal to pass messages
    void splashMsgSig(QString message);

private:
    /// Qt application
    QApplication m_application;

    /// Current application instance
    static Application* m_instance;

    /// Command console mode flag;
    bool m_console_mode;

    /// Print details flag
    bool m_print_output;

    /// Redirect input/output to console
    bool m_redirect_io_to_console;

    /// File specified on via commandline
    QString m_commandline_file;

    /// Theme set at startup
    QString m_theme;

#ifdef Q_OS_WIN
    /// Windows console information
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    /// Windows standard output handle
    HANDLE ConsoleOutput;
    FILE *COutputHandle;

    /// Windows standard error handle
    HANDLE ConsoleError;
    FILE *CErrorHandle;

    /// Windows standard input handle
    HANDLE ConsoleInput;
    FILE *CInputHandle;

    /// Windows console save attributes
    WORD m_currentConsoleAttr;

    /// indicate if using allocated console
    BOOL m_allocated_console;
#endif
};

/// ENTRY_POINT is a macro that implements the main function.
#if !defined(Q_OS_MAC) && QT_VERSION >= QT_VERSION_CHECK(5,6,0)
#define ENTRY_POINT \
    int main(int argc, char** argv) \
    { \
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL); \
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling); \
        QScopedPointer<Application> app(new Application(argc, argv)); \
        try \
        { \
            app->initialize(); \
        } \
        catch(const InitException &ex) \
        { \
           fprintf(stdout, "Could not initialize the application."); \
        } \
        catch(...) \
        { \
           fprintf(stdout, "A fatal error ocurred."); \
        } \
        return app->run(); \
    }
#else
#define ENTRY_POINT \
    int main(int argc, char** argv) \
    { \
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL); \
        QScopedPointer<Application> app(new Application(argc, argv)); \
        try \
        { \
            app->initialize(); \
        } \
        catch(const InitException &ex) \
        { \
           fprintf(stdout, "Could not initialize the application."); \
        } \
        catch(...) \
        { \
           fprintf(stdout, "A fatal error ocurred."); \
        } \
        return app->run(); \
    }
#endif

void clearCustomPartCache(bool silent = false);
void clearAndReloadModelFileFromSettings();
void clearAndReloadModelFile();
void reloadCurrentPage();
void restartApplication();
#endif // APPLICATION_H
