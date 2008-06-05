/********************************************************************************
** Form generated from reading ui file 'ImageVis3D.ui'
**
** Created: Wed 4. Jun 14:14:29 2008
**      by: Qt User Interface Compiler version 4.4.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef IMAGEVIS3D_AUTO_H
#define IMAGEVIS3D_AUTO_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDockWidget>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMdiArea>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionWerwer;
    QAction *actionCreate_New_Project;
    QAction *actionOpen_Existing_Project;
    QAction *actionLast_Used_Projects;
    QAction *actionSave_Current_Project;
    QAction *actionLoad_Dataset;
    QAction *actionSave_Dataset;
    QAction *actionQuit;
    QAction *actionClose_Dataset;
    QAction *actionClose_Project;
    QAction *action2_x_2_View;
    QAction *action1_x_3_Viw;
    QAction *actionSinge_View;
    QAction *actionDirect_Volume_Rendering;
    QAction *actionIsosurface_Rendering;
    QAction *actionCrop;
    QAction *actionMask;
    QAction *actionBox;
    QAction *actionPoly_Line;
    QAction *actionSelect_All;
    QAction *actionDelete_Selection;
    QAction *actionInvert_Selection;
    QAction *actionHistorgram;
    QAction *actionStastistcs;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionGenerate_Movie;
    QAction *actionREcord_Image;
    QAction *actionGo_Fullscreen;
    QAction *actionRecord_Movie;
    QAction *actionMedian;
    QAction *actionGaussian;
    QAction *actionEdge_Detection;
    QAction *actionDesign_Convolution;
    QAction *actionLoad_Convolution;
    QAction *actionSave_Convolution;
    QAction *actionAnd;
    QAction *actionOr;
    QAction *actionNot;
    QAction *actionXor;
    QAction *actionLoad_Transfer_Function;
    QAction *actionAbout;
    QAction *actionOpen_Online_Help;
    QAction *actionLoad;
    QAction *actionSave;
    QAction *actionNew_1D_Transfer_Function;
    QAction *action2D_Transfer_Function_Editor;
    QAction *actionToggle_between_1D_and_2D;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_2;
    QMdiArea *mdiArea;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_View;
    QMenu *menu_Tools;
    QMenu *menuBitwise_Operations;
    QMenu *menu_Filters;
    QMenu *menu_Help;
    QMenu *menuSelection;
    QMenu *menuRecording;
    QMenu *menuTransfer_Functions;
    QStatusBar *statusbar;
    QDockWidget *dockWidget;
    QWidget *dockWidgetContents;
    QHBoxLayout *horizontalLayout;
    QTabWidget *tabWidget;
    QWidget *Seite;
    QGridLayout *gridLayout;
    QListWidget *listWidget;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QWidget *Seite_2;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QPushButton *pushButton_5;
    QPushButton *pushButton_6;
    QGroupBox *groupBox_2;
    QPushButton *pushButton_7;
    QPushButton *pushButton_8;
    QPushButton *pushButton_9;
    QPushButton *pushButton_10;
    QLabel *label;
    QPushButton *pushButton_11;
    QPushButton *pushButton_12;
    QPushButton *pushButton_13;
    QDockWidget *dockWidget_2;
    QWidget *dockWidgetContents_2;

    void setupUi(QMainWindow *MainWindow)
    {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
    MainWindow->resize(1164, 814);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
    MainWindow->setSizePolicy(sizePolicy);
    MainWindow->setMouseTracking(false);
    MainWindow->setDockNestingEnabled(false);
    MainWindow->setDockOptions(QMainWindow::AllowTabbedDocks|QMainWindow::AnimatedDocks);
    actionWerwer = new QAction(MainWindow);
    actionWerwer->setObjectName(QString::fromUtf8("actionWerwer"));
    actionCreate_New_Project = new QAction(MainWindow);
    actionCreate_New_Project->setObjectName(QString::fromUtf8("actionCreate_New_Project"));
    actionOpen_Existing_Project = new QAction(MainWindow);
    actionOpen_Existing_Project->setObjectName(QString::fromUtf8("actionOpen_Existing_Project"));
    actionLast_Used_Projects = new QAction(MainWindow);
    actionLast_Used_Projects->setObjectName(QString::fromUtf8("actionLast_Used_Projects"));
    actionSave_Current_Project = new QAction(MainWindow);
    actionSave_Current_Project->setObjectName(QString::fromUtf8("actionSave_Current_Project"));
    actionLoad_Dataset = new QAction(MainWindow);
    actionLoad_Dataset->setObjectName(QString::fromUtf8("actionLoad_Dataset"));
    actionSave_Dataset = new QAction(MainWindow);
    actionSave_Dataset->setObjectName(QString::fromUtf8("actionSave_Dataset"));
    actionQuit = new QAction(MainWindow);
    actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
    actionClose_Dataset = new QAction(MainWindow);
    actionClose_Dataset->setObjectName(QString::fromUtf8("actionClose_Dataset"));
    actionClose_Project = new QAction(MainWindow);
    actionClose_Project->setObjectName(QString::fromUtf8("actionClose_Project"));
    action2_x_2_View = new QAction(MainWindow);
    action2_x_2_View->setObjectName(QString::fromUtf8("action2_x_2_View"));
    action1_x_3_Viw = new QAction(MainWindow);
    action1_x_3_Viw->setObjectName(QString::fromUtf8("action1_x_3_Viw"));
    actionSinge_View = new QAction(MainWindow);
    actionSinge_View->setObjectName(QString::fromUtf8("actionSinge_View"));
    actionDirect_Volume_Rendering = new QAction(MainWindow);
    actionDirect_Volume_Rendering->setObjectName(QString::fromUtf8("actionDirect_Volume_Rendering"));
    actionIsosurface_Rendering = new QAction(MainWindow);
    actionIsosurface_Rendering->setObjectName(QString::fromUtf8("actionIsosurface_Rendering"));
    actionCrop = new QAction(MainWindow);
    actionCrop->setObjectName(QString::fromUtf8("actionCrop"));
    actionMask = new QAction(MainWindow);
    actionMask->setObjectName(QString::fromUtf8("actionMask"));
    actionBox = new QAction(MainWindow);
    actionBox->setObjectName(QString::fromUtf8("actionBox"));
    actionPoly_Line = new QAction(MainWindow);
    actionPoly_Line->setObjectName(QString::fromUtf8("actionPoly_Line"));
    actionSelect_All = new QAction(MainWindow);
    actionSelect_All->setObjectName(QString::fromUtf8("actionSelect_All"));
    actionDelete_Selection = new QAction(MainWindow);
    actionDelete_Selection->setObjectName(QString::fromUtf8("actionDelete_Selection"));
    actionInvert_Selection = new QAction(MainWindow);
    actionInvert_Selection->setObjectName(QString::fromUtf8("actionInvert_Selection"));
    actionHistorgram = new QAction(MainWindow);
    actionHistorgram->setObjectName(QString::fromUtf8("actionHistorgram"));
    actionStastistcs = new QAction(MainWindow);
    actionStastistcs->setObjectName(QString::fromUtf8("actionStastistcs"));
    actionUndo = new QAction(MainWindow);
    actionUndo->setObjectName(QString::fromUtf8("actionUndo"));
    actionRedo = new QAction(MainWindow);
    actionRedo->setObjectName(QString::fromUtf8("actionRedo"));
    actionGenerate_Movie = new QAction(MainWindow);
    actionGenerate_Movie->setObjectName(QString::fromUtf8("actionGenerate_Movie"));
    actionREcord_Image = new QAction(MainWindow);
    actionREcord_Image->setObjectName(QString::fromUtf8("actionREcord_Image"));
    actionGo_Fullscreen = new QAction(MainWindow);
    actionGo_Fullscreen->setObjectName(QString::fromUtf8("actionGo_Fullscreen"));
    actionRecord_Movie = new QAction(MainWindow);
    actionRecord_Movie->setObjectName(QString::fromUtf8("actionRecord_Movie"));
    actionMedian = new QAction(MainWindow);
    actionMedian->setObjectName(QString::fromUtf8("actionMedian"));
    actionGaussian = new QAction(MainWindow);
    actionGaussian->setObjectName(QString::fromUtf8("actionGaussian"));
    actionEdge_Detection = new QAction(MainWindow);
    actionEdge_Detection->setObjectName(QString::fromUtf8("actionEdge_Detection"));
    actionDesign_Convolution = new QAction(MainWindow);
    actionDesign_Convolution->setObjectName(QString::fromUtf8("actionDesign_Convolution"));
    actionLoad_Convolution = new QAction(MainWindow);
    actionLoad_Convolution->setObjectName(QString::fromUtf8("actionLoad_Convolution"));
    actionSave_Convolution = new QAction(MainWindow);
    actionSave_Convolution->setObjectName(QString::fromUtf8("actionSave_Convolution"));
    actionAnd = new QAction(MainWindow);
    actionAnd->setObjectName(QString::fromUtf8("actionAnd"));
    actionOr = new QAction(MainWindow);
    actionOr->setObjectName(QString::fromUtf8("actionOr"));
    actionNot = new QAction(MainWindow);
    actionNot->setObjectName(QString::fromUtf8("actionNot"));
    actionXor = new QAction(MainWindow);
    actionXor->setObjectName(QString::fromUtf8("actionXor"));
    actionLoad_Transfer_Function = new QAction(MainWindow);
    actionLoad_Transfer_Function->setObjectName(QString::fromUtf8("actionLoad_Transfer_Function"));
    actionAbout = new QAction(MainWindow);
    actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
    actionOpen_Online_Help = new QAction(MainWindow);
    actionOpen_Online_Help->setObjectName(QString::fromUtf8("actionOpen_Online_Help"));
    actionLoad = new QAction(MainWindow);
    actionLoad->setObjectName(QString::fromUtf8("actionLoad"));
    actionSave = new QAction(MainWindow);
    actionSave->setObjectName(QString::fromUtf8("actionSave"));
    actionNew_1D_Transfer_Function = new QAction(MainWindow);
    actionNew_1D_Transfer_Function->setObjectName(QString::fromUtf8("actionNew_1D_Transfer_Function"));
    action2D_Transfer_Function_Editor = new QAction(MainWindow);
    action2D_Transfer_Function_Editor->setObjectName(QString::fromUtf8("action2D_Transfer_Function_Editor"));
    actionToggle_between_1D_and_2D = new QAction(MainWindow);
    actionToggle_between_1D_and_2D->setObjectName(QString::fromUtf8("actionToggle_between_1D_and_2D"));
    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    centralwidget->setGeometry(QRect(84, 22, 778, 771));
    horizontalLayout_2 = new QHBoxLayout(centralwidget);
    horizontalLayout_2->setSpacing(6);
    horizontalLayout_2->setMargin(0);
    horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
    mdiArea = new QMdiArea(centralwidget);
    mdiArea->setObjectName(QString::fromUtf8("mdiArea"));

    horizontalLayout_2->addWidget(mdiArea);

    MainWindow->setCentralWidget(centralwidget);
    menubar = new QMenuBar(MainWindow);
    menubar->setObjectName(QString::fromUtf8("menubar"));
    menubar->setGeometry(QRect(0, 0, 1164, 22));
    menu_File = new QMenu(menubar);
    menu_File->setObjectName(QString::fromUtf8("menu_File"));
    menu_View = new QMenu(menubar);
    menu_View->setObjectName(QString::fromUtf8("menu_View"));
    menu_Tools = new QMenu(menubar);
    menu_Tools->setObjectName(QString::fromUtf8("menu_Tools"));
    menuBitwise_Operations = new QMenu(menu_Tools);
    menuBitwise_Operations->setObjectName(QString::fromUtf8("menuBitwise_Operations"));
    menu_Filters = new QMenu(menubar);
    menu_Filters->setObjectName(QString::fromUtf8("menu_Filters"));
    menu_Help = new QMenu(menubar);
    menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
    menuSelection = new QMenu(menubar);
    menuSelection->setObjectName(QString::fromUtf8("menuSelection"));
    menuRecording = new QMenu(menubar);
    menuRecording->setObjectName(QString::fromUtf8("menuRecording"));
    menuTransfer_Functions = new QMenu(menubar);
    menuTransfer_Functions->setObjectName(QString::fromUtf8("menuTransfer_Functions"));
    MainWindow->setMenuBar(menubar);
    statusbar = new QStatusBar(MainWindow);
    statusbar->setObjectName(QString::fromUtf8("statusbar"));
    statusbar->setGeometry(QRect(0, 793, 1164, 21));
    MainWindow->setStatusBar(statusbar);
    dockWidget = new QDockWidget(MainWindow);
    dockWidget->setObjectName(QString::fromUtf8("dockWidget"));
    dockWidget->setGeometry(QRect(866, 22, 298, 771));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(dockWidget->sizePolicy().hasHeightForWidth());
    dockWidget->setSizePolicy(sizePolicy1);
    dockWidget->setFloating(false);
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    dockWidgetContents = new QWidget();
    dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    dockWidgetContents->setGeometry(QRect(0, 22, 298, 749));
    horizontalLayout = new QHBoxLayout(dockWidgetContents);
    horizontalLayout->setSpacing(6);
    horizontalLayout->setMargin(0);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
    tabWidget = new QTabWidget(dockWidgetContents);
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setMinimumSize(QSize(100, 0));
    tabWidget->setTabShape(QTabWidget::Rounded);
    Seite = new QWidget();
    Seite->setObjectName(QString::fromUtf8("Seite"));
    Seite->setGeometry(QRect(0, 0, 274, 704));
    gridLayout = new QGridLayout(Seite);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(0);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    listWidget = new QListWidget(Seite);
    listWidget->setObjectName(QString::fromUtf8("listWidget"));

    gridLayout->addWidget(listWidget, 0, 0, 1, 2);

    pushButton_3 = new QPushButton(Seite);
    pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));

    gridLayout->addWidget(pushButton_3, 1, 0, 1, 1);

    pushButton_4 = new QPushButton(Seite);
    pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));

    gridLayout->addWidget(pushButton_4, 1, 1, 1, 1);

    tabWidget->addTab(Seite, QString());
    Seite_2 = new QWidget();
    Seite_2->setObjectName(QString::fromUtf8("Seite_2"));
    Seite_2->setGeometry(QRect(0, 0, 274, 704));
    verticalLayout_2 = new QVBoxLayout(Seite_2);
    verticalLayout_2->setSpacing(6);
    verticalLayout_2->setMargin(0);
    verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
    groupBox = new QGroupBox(Seite_2);
    groupBox->setObjectName(QString::fromUtf8("groupBox"));
    groupBox->setMinimumSize(QSize(0, 51));
    groupBox->setMaximumSize(QSize(16777215, 51));
    groupBox->setFlat(false);
    pushButton_5 = new QPushButton(groupBox);
    pushButton_5->setObjectName(QString::fromUtf8("pushButton_5"));
    pushButton_5->setGeometry(QRect(10, 20, 75, 23));
    pushButton_6 = new QPushButton(groupBox);
    pushButton_6->setObjectName(QString::fromUtf8("pushButton_6"));
    pushButton_6->setGeometry(QRect(100, 20, 75, 23));

    verticalLayout_2->addWidget(groupBox);

    groupBox_2 = new QGroupBox(Seite_2);
    groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
    pushButton_7 = new QPushButton(groupBox_2);
    pushButton_7->setObjectName(QString::fromUtf8("pushButton_7"));
    pushButton_7->setGeometry(QRect(10, 20, 181, 23));
    pushButton_8 = new QPushButton(groupBox_2);
    pushButton_8->setObjectName(QString::fromUtf8("pushButton_8"));
    pushButton_8->setGeometry(QRect(10, 80, 181, 23));
    pushButton_9 = new QPushButton(groupBox_2);
    pushButton_9->setObjectName(QString::fromUtf8("pushButton_9"));
    pushButton_9->setGeometry(QRect(10, 50, 181, 23));
    pushButton_10 = new QPushButton(groupBox_2);
    pushButton_10->setObjectName(QString::fromUtf8("pushButton_10"));
    pushButton_10->setGeometry(QRect(10, 130, 181, 23));
    label = new QLabel(groupBox_2);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(10, 110, 61, 16));
    pushButton_11 = new QPushButton(groupBox_2);
    pushButton_11->setObjectName(QString::fromUtf8("pushButton_11"));
    pushButton_11->setGeometry(QRect(10, 170, 181, 23));
    pushButton_12 = new QPushButton(groupBox_2);
    pushButton_12->setObjectName(QString::fromUtf8("pushButton_12"));
    pushButton_12->setGeometry(QRect(10, 200, 181, 23));
    pushButton_13 = new QPushButton(groupBox_2);
    pushButton_13->setObjectName(QString::fromUtf8("pushButton_13"));
    pushButton_13->setGeometry(QRect(10, 240, 181, 23));

    verticalLayout_2->addWidget(groupBox_2);

    tabWidget->addTab(Seite_2, QString());

    horizontalLayout->addWidget(tabWidget);

    dockWidget->setWidget(dockWidgetContents);
    MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(2), dockWidget);
    dockWidget_2 = new QDockWidget(MainWindow);
    dockWidget_2->setObjectName(QString::fromUtf8("dockWidget_2"));
    dockWidget_2->setGeometry(QRect(0, 22, 80, 771));
    dockWidgetContents_2 = new QWidget();
    dockWidgetContents_2->setObjectName(QString::fromUtf8("dockWidgetContents_2"));
    dockWidgetContents_2->setGeometry(QRect(0, 22, 80, 749));
    dockWidget_2->setWidget(dockWidgetContents_2);
    MainWindow->addDockWidget(static_cast<Qt::DockWidgetArea>(1), dockWidget_2);

    menubar->addAction(menu_File->menuAction());
    menubar->addAction(menuSelection->menuAction());
    menubar->addAction(menu_View->menuAction());
    menubar->addAction(menu_Tools->menuAction());
    menubar->addAction(menu_Filters->menuAction());
    menubar->addAction(menuRecording->menuAction());
    menubar->addAction(menu_Help->menuAction());
    menubar->addAction(menuTransfer_Functions->menuAction());
    menu_File->addAction(actionCreate_New_Project);
    menu_File->addAction(actionOpen_Existing_Project);
    menu_File->addAction(actionLast_Used_Projects);
    menu_File->addAction(actionSave_Current_Project);
    menu_File->addAction(actionClose_Project);
    menu_File->addSeparator();
    menu_File->addAction(actionLoad_Dataset);
    menu_File->addAction(actionSave_Dataset);
    menu_File->addAction(actionClose_Dataset);
    menu_File->addSeparator();
    menu_File->addAction(actionQuit);
    menu_View->addAction(actionGo_Fullscreen);
    menu_View->addSeparator();
    menu_View->addAction(action2_x_2_View);
    menu_View->addAction(action1_x_3_Viw);
    menu_View->addAction(actionSinge_View);
    menu_View->addSeparator();
    menu_View->addAction(actionDirect_Volume_Rendering);
    menu_View->addAction(actionIsosurface_Rendering);
    menu_Tools->addAction(actionCrop);
    menu_Tools->addAction(menuBitwise_Operations->menuAction());
    menu_Tools->addAction(actionHistorgram);
    menuBitwise_Operations->addAction(actionAnd);
    menuBitwise_Operations->addAction(actionOr);
    menuBitwise_Operations->addAction(actionNot);
    menuBitwise_Operations->addAction(actionXor);
    menu_Filters->addAction(actionMedian);
    menu_Filters->addSeparator();
    menu_Filters->addAction(actionGaussian);
    menu_Filters->addAction(actionEdge_Detection);
    menu_Filters->addSeparator();
    menu_Filters->addAction(actionDesign_Convolution);
    menu_Filters->addAction(actionLoad_Convolution);
    menu_Filters->addAction(actionSave_Convolution);
    menu_Help->addAction(actionAbout);
    menu_Help->addAction(actionOpen_Online_Help);
    menuSelection->addAction(actionBox);
    menuSelection->addAction(actionPoly_Line);
    menuSelection->addAction(actionSelect_All);
    menuSelection->addSeparator();
    menuSelection->addAction(actionDelete_Selection);
    menuSelection->addAction(actionInvert_Selection);
    menuSelection->addSeparator();
    menuSelection->addAction(actionStastistcs);
    menuSelection->addSeparator();
    menuSelection->addAction(actionUndo);
    menuSelection->addAction(actionRedo);
    menuRecording->addAction(actionREcord_Image);
    menuRecording->addAction(actionRecord_Movie);
    menuTransfer_Functions->addAction(actionLoad);
    menuTransfer_Functions->addAction(actionSave);
    menuTransfer_Functions->addSeparator();
    menuTransfer_Functions->addAction(actionNew_1D_Transfer_Function);
    menuTransfer_Functions->addAction(action2D_Transfer_Function_Editor);
    menuTransfer_Functions->addSeparator();
    menuTransfer_Functions->addAction(actionToggle_between_1D_and_2D);

    retranslateUi(MainWindow);

    tabWidget->setCurrentIndex(0);


    QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
    MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
    actionWerwer->setText(QApplication::translate("MainWindow", "werwer", 0, QApplication::UnicodeUTF8));
    actionCreate_New_Project->setText(QApplication::translate("MainWindow", "Create New Project", 0, QApplication::UnicodeUTF8));
    actionOpen_Existing_Project->setText(QApplication::translate("MainWindow", "Open Existing Project", 0, QApplication::UnicodeUTF8));
    actionLast_Used_Projects->setText(QApplication::translate("MainWindow", "Last Used Projects", 0, QApplication::UnicodeUTF8));
    actionSave_Current_Project->setText(QApplication::translate("MainWindow", "Save Current Project", 0, QApplication::UnicodeUTF8));
    actionLoad_Dataset->setText(QApplication::translate("MainWindow", "Load Dataset", 0, QApplication::UnicodeUTF8));
    actionSave_Dataset->setText(QApplication::translate("MainWindow", "Save Dataset", 0, QApplication::UnicodeUTF8));
    actionQuit->setText(QApplication::translate("MainWindow", "Quit", 0, QApplication::UnicodeUTF8));
    actionClose_Dataset->setText(QApplication::translate("MainWindow", "Close Dataset", 0, QApplication::UnicodeUTF8));
    actionClose_Project->setText(QApplication::translate("MainWindow", "Close Project", 0, QApplication::UnicodeUTF8));
    action2_x_2_View->setText(QApplication::translate("MainWindow", "2 x 2 View", 0, QApplication::UnicodeUTF8));
    action1_x_3_Viw->setText(QApplication::translate("MainWindow", "1 x 3 View", 0, QApplication::UnicodeUTF8));
    actionSinge_View->setText(QApplication::translate("MainWindow", "Singe View", 0, QApplication::UnicodeUTF8));
    actionDirect_Volume_Rendering->setText(QApplication::translate("MainWindow", "Direct Volume Rendering", 0, QApplication::UnicodeUTF8));
    actionIsosurface_Rendering->setText(QApplication::translate("MainWindow", "Isosurface Rendering", 0, QApplication::UnicodeUTF8));
    actionCrop->setText(QApplication::translate("MainWindow", "Crop", 0, QApplication::UnicodeUTF8));
    actionMask->setText(QApplication::translate("MainWindow", "Mask", 0, QApplication::UnicodeUTF8));
    actionBox->setText(QApplication::translate("MainWindow", "Box Select", 0, QApplication::UnicodeUTF8));
    actionPoly_Line->setText(QApplication::translate("MainWindow", "Poly Line Select", 0, QApplication::UnicodeUTF8));
    actionSelect_All->setText(QApplication::translate("MainWindow", "Select All", 0, QApplication::UnicodeUTF8));
    actionDelete_Selection->setText(QApplication::translate("MainWindow", "Delete Selection", 0, QApplication::UnicodeUTF8));
    actionInvert_Selection->setText(QApplication::translate("MainWindow", "Invert Selection", 0, QApplication::UnicodeUTF8));
    actionHistorgram->setText(QApplication::translate("MainWindow", "Historgram", 0, QApplication::UnicodeUTF8));
    actionStastistcs->setText(QApplication::translate("MainWindow", "Stastistcs", 0, QApplication::UnicodeUTF8));
    actionUndo->setText(QApplication::translate("MainWindow", "Undo", 0, QApplication::UnicodeUTF8));
    actionRedo->setText(QApplication::translate("MainWindow", "Redo", 0, QApplication::UnicodeUTF8));
    actionGenerate_Movie->setText(QApplication::translate("MainWindow", "Generate Movie", 0, QApplication::UnicodeUTF8));
    actionREcord_Image->setText(QApplication::translate("MainWindow", "Record Image", 0, QApplication::UnicodeUTF8));
    actionGo_Fullscreen->setText(QApplication::translate("MainWindow", "Go Fullscreen", 0, QApplication::UnicodeUTF8));
    actionRecord_Movie->setText(QApplication::translate("MainWindow", "Record Movie", 0, QApplication::UnicodeUTF8));
    actionMedian->setText(QApplication::translate("MainWindow", "Median", 0, QApplication::UnicodeUTF8));
    actionGaussian->setText(QApplication::translate("MainWindow", "Gaussian", 0, QApplication::UnicodeUTF8));
    actionEdge_Detection->setText(QApplication::translate("MainWindow", "Edge Detection", 0, QApplication::UnicodeUTF8));
    actionDesign_Convolution->setText(QApplication::translate("MainWindow", "Design Convolution", 0, QApplication::UnicodeUTF8));
    actionLoad_Convolution->setText(QApplication::translate("MainWindow", "Load Convolution", 0, QApplication::UnicodeUTF8));
    actionSave_Convolution->setText(QApplication::translate("MainWindow", "Save Convolution", 0, QApplication::UnicodeUTF8));
    actionAnd->setText(QApplication::translate("MainWindow", "and", 0, QApplication::UnicodeUTF8));
    actionOr->setText(QApplication::translate("MainWindow", "or", 0, QApplication::UnicodeUTF8));
    actionNot->setText(QApplication::translate("MainWindow", "not", 0, QApplication::UnicodeUTF8));
    actionXor->setText(QApplication::translate("MainWindow", "xor", 0, QApplication::UnicodeUTF8));
    actionLoad_Transfer_Function->setText(QApplication::translate("MainWindow", "Load Transfer Function", 0, QApplication::UnicodeUTF8));
    actionAbout->setText(QApplication::translate("MainWindow", "About", 0, QApplication::UnicodeUTF8));
    actionOpen_Online_Help->setText(QApplication::translate("MainWindow", "Open Online Help", 0, QApplication::UnicodeUTF8));
    actionLoad->setText(QApplication::translate("MainWindow", "Load", 0, QApplication::UnicodeUTF8));
    actionSave->setText(QApplication::translate("MainWindow", "Save Current", 0, QApplication::UnicodeUTF8));
    actionNew_1D_Transfer_Function->setText(QApplication::translate("MainWindow", "1D Transfer Function Editor", 0, QApplication::UnicodeUTF8));
    action2D_Transfer_Function_Editor->setText(QApplication::translate("MainWindow", "2D Transfer Function Editor", 0, QApplication::UnicodeUTF8));
    actionToggle_between_1D_and_2D->setText(QApplication::translate("MainWindow", "Toggle between 1D and 2D", 0, QApplication::UnicodeUTF8));
    menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0, QApplication::UnicodeUTF8));
    menu_View->setTitle(QApplication::translate("MainWindow", "&View", 0, QApplication::UnicodeUTF8));
    menu_Tools->setTitle(QApplication::translate("MainWindow", "&Tools", 0, QApplication::UnicodeUTF8));
    menuBitwise_Operations->setTitle(QApplication::translate("MainWindow", "Bitwise Operations", 0, QApplication::UnicodeUTF8));
    menu_Filters->setTitle(QApplication::translate("MainWindow", "&Filters", 0, QApplication::UnicodeUTF8));
    menu_Help->setTitle(QApplication::translate("MainWindow", "&Help", 0, QApplication::UnicodeUTF8));
    menuSelection->setTitle(QApplication::translate("MainWindow", "Edit", 0, QApplication::UnicodeUTF8));
    menuRecording->setTitle(QApplication::translate("MainWindow", "Recording", 0, QApplication::UnicodeUTF8));
    menuTransfer_Functions->setTitle(QApplication::translate("MainWindow", "Transfer Functions", 0, QApplication::UnicodeUTF8));
    dockWidget->setWindowTitle(QApplication::translate("MainWindow", "Tools", 0, QApplication::UnicodeUTF8));
    pushButton_3->setText(QApplication::translate("MainWindow", "Undo", 0, QApplication::UnicodeUTF8));
    pushButton_4->setText(QApplication::translate("MainWindow", "Redo", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(Seite), QApplication::translate("MainWindow", "History", 0, QApplication::UnicodeUTF8));
    groupBox->setTitle(QApplication::translate("MainWindow", "Image", 0, QApplication::UnicodeUTF8));
    pushButton_5->setText(QApplication::translate("MainWindow", "Single ", 0, QApplication::UnicodeUTF8));
    pushButton_6->setText(QApplication::translate("MainWindow", "Sequence", 0, QApplication::UnicodeUTF8));
    groupBox_2->setTitle(QApplication::translate("MainWindow", "Movie", 0, QApplication::UnicodeUTF8));
    pushButton_7->setText(QApplication::translate("MainWindow", "Start", 0, QApplication::UnicodeUTF8));
    pushButton_8->setText(QApplication::translate("MainWindow", "Stop", 0, QApplication::UnicodeUTF8));
    pushButton_9->setText(QApplication::translate("MainWindow", "Pause", 0, QApplication::UnicodeUTF8));
    pushButton_10->setText(QApplication::translate("MainWindow", "Play", 0, QApplication::UnicodeUTF8));
    label->setText(QApplication::translate("MainWindow", "Camera Path", 0, QApplication::UnicodeUTF8));
    pushButton_11->setText(QApplication::translate("MainWindow", "Add new View", 0, QApplication::UnicodeUTF8));
    pushButton_12->setText(QApplication::translate("MainWindow", "Delete Last View", 0, QApplication::UnicodeUTF8));
    pushButton_13->setText(QApplication::translate("MainWindow", "Clear All Views", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(Seite_2), QApplication::translate("MainWindow", "Recorder", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IMAGEVIS3D_AUTO_H
