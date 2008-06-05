/********************************************************************************
** Form generated from reading ui file 'TransDialog1D.ui'
**
** Created: Thu 5. Jun 10:02:41 2008
**      by: Qt User Interface Compiler version 4.4.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_TRANSDIALOG1D_H
#define UI_TRANSDIALOG1D_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGraphicsView>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QRadioButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_TransDialog1D
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox_Components;
    QCheckBox *checkBox_Red;
    QCheckBox *checkBox_Green;
    QCheckBox *checkBox_Blue;
    QCheckBox *checkBox_Alpha;
    QGraphicsView *graphicsView_Histogram;
    QSlider *verticalSlider_ScaleHistogram;
    QGroupBox *groupBox_Groups;
    QRadioButton *radioButton_User;
    QRadioButton *radioButton_Luminance;
    QRadioButton *radioButton_Intensity;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TransDialog1D)
    {
    if (TransDialog1D->objectName().isEmpty())
        TransDialog1D->setObjectName(QString::fromUtf8("TransDialog1D"));
    TransDialog1D->setWindowModality(Qt::NonModal);
    TransDialog1D->resize(585, 356);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(TransDialog1D->sizePolicy().hasHeightForWidth());
    TransDialog1D->setSizePolicy(sizePolicy);
    TransDialog1D->setContextMenuPolicy(Qt::NoContextMenu);
    TransDialog1D->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    TransDialog1D->setSizeGripEnabled(false);
    TransDialog1D->setModal(false);
    gridLayout = new QGridLayout(TransDialog1D);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    groupBox_Components = new QGroupBox(TransDialog1D);
    groupBox_Components->setObjectName(QString::fromUtf8("groupBox_Components"));
    groupBox_Components->setMinimumSize(QSize(131, 141));
    groupBox_Components->setMaximumSize(QSize(131, 141));
    groupBox_Components->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    groupBox_Components->setFlat(false);
    groupBox_Components->setCheckable(false);
    checkBox_Red = new QCheckBox(groupBox_Components);
    checkBox_Red->setObjectName(QString::fromUtf8("checkBox_Red"));
    checkBox_Red->setGeometry(QRect(10, 20, 72, 18));
    checkBox_Green = new QCheckBox(groupBox_Components);
    checkBox_Green->setObjectName(QString::fromUtf8("checkBox_Green"));
    checkBox_Green->setGeometry(QRect(10, 50, 72, 18));
    checkBox_Blue = new QCheckBox(groupBox_Components);
    checkBox_Blue->setObjectName(QString::fromUtf8("checkBox_Blue"));
    checkBox_Blue->setGeometry(QRect(10, 80, 72, 18));
    checkBox_Alpha = new QCheckBox(groupBox_Components);
    checkBox_Alpha->setObjectName(QString::fromUtf8("checkBox_Alpha"));
    checkBox_Alpha->setGeometry(QRect(10, 110, 72, 18));

    gridLayout->addWidget(groupBox_Components, 0, 0, 1, 1);

    graphicsView_Histogram = new QGraphicsView(TransDialog1D);
    graphicsView_Histogram->setObjectName(QString::fromUtf8("graphicsView_Histogram"));
    graphicsView_Histogram->setMinimumSize(QSize(309, 307));
    graphicsView_Histogram->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    graphicsView_Histogram->setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);

    gridLayout->addWidget(graphicsView_Histogram, 0, 1, 3, 1);

    verticalSlider_ScaleHistogram = new QSlider(TransDialog1D);
    verticalSlider_ScaleHistogram->setObjectName(QString::fromUtf8("verticalSlider_ScaleHistogram"));
    verticalSlider_ScaleHistogram->setOrientation(Qt::Vertical);

    gridLayout->addWidget(verticalSlider_ScaleHistogram, 0, 2, 3, 1);

    groupBox_Groups = new QGroupBox(TransDialog1D);
    groupBox_Groups->setObjectName(QString::fromUtf8("groupBox_Groups"));
    groupBox_Groups->setMinimumSize(QSize(131, 111));
    groupBox_Groups->setMaximumSize(QSize(131, 111));
    radioButton_User = new QRadioButton(groupBox_Groups);
    radioButton_User->setObjectName(QString::fromUtf8("radioButton_User"));
    radioButton_User->setGeometry(QRect(10, 20, 84, 18));
    radioButton_User->setChecked(true);
    radioButton_Luminance = new QRadioButton(groupBox_Groups);
    radioButton_Luminance->setObjectName(QString::fromUtf8("radioButton_Luminance"));
    radioButton_Luminance->setGeometry(QRect(10, 50, 84, 18));
    radioButton_Intensity = new QRadioButton(groupBox_Groups);
    radioButton_Intensity->setObjectName(QString::fromUtf8("radioButton_Intensity"));
    radioButton_Intensity->setGeometry(QRect(10, 80, 84, 18));

    gridLayout->addWidget(groupBox_Groups, 1, 0, 1, 1);

    verticalSpacer = new QSpacerItem(128, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout->addItem(verticalSpacer, 2, 0, 1, 1);

    buttonBox = new QDialogButtonBox(TransDialog1D);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    gridLayout->addWidget(buttonBox, 3, 0, 1, 3);

    QWidget::setTabOrder(checkBox_Red, checkBox_Green);
    QWidget::setTabOrder(checkBox_Green, checkBox_Blue);
    QWidget::setTabOrder(checkBox_Blue, checkBox_Alpha);
    QWidget::setTabOrder(checkBox_Alpha, radioButton_User);
    QWidget::setTabOrder(radioButton_User, radioButton_Luminance);
    QWidget::setTabOrder(radioButton_Luminance, radioButton_Intensity);
    QWidget::setTabOrder(radioButton_Intensity, graphicsView_Histogram);
    QWidget::setTabOrder(graphicsView_Histogram, verticalSlider_ScaleHistogram);
    QWidget::setTabOrder(verticalSlider_ScaleHistogram, buttonBox);

    retranslateUi(TransDialog1D);
    QObject::connect(buttonBox, SIGNAL(accepted()), TransDialog1D, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), TransDialog1D, SLOT(reject()));
    QObject::connect(checkBox_Red, SIGNAL(clicked()), TransDialog1D, SLOT(SetUserMode()));
    QObject::connect(checkBox_Green, SIGNAL(clicked()), TransDialog1D, SLOT(SetUserMode()));
    QObject::connect(checkBox_Blue, SIGNAL(clicked()), TransDialog1D, SLOT(SetUserMode()));
    QObject::connect(checkBox_Alpha, SIGNAL(clicked()), TransDialog1D, SLOT(SetUserMode()));
    QObject::connect(radioButton_Intensity, SIGNAL(clicked()), TransDialog1D, SLOT(SetIntensityMode()));
    QObject::connect(radioButton_Luminance, SIGNAL(clicked()), TransDialog1D, SLOT(SetLuminanceMode()));

    QMetaObject::connectSlotsByName(TransDialog1D);
    } // setupUi

    void retranslateUi(QDialog *TransDialog1D)
    {
    TransDialog1D->setWindowTitle(QApplication::translate("TransDialog1D", "1D Transfer Function Editor", 0, QApplication::UnicodeUTF8));
    groupBox_Components->setTitle(QApplication::translate("TransDialog1D", "Color Components", 0, QApplication::UnicodeUTF8));
    checkBox_Red->setText(QApplication::translate("TransDialog1D", "Red", 0, QApplication::UnicodeUTF8));
    checkBox_Green->setText(QApplication::translate("TransDialog1D", "Green", 0, QApplication::UnicodeUTF8));
    checkBox_Blue->setText(QApplication::translate("TransDialog1D", "Blue", 0, QApplication::UnicodeUTF8));
    checkBox_Alpha->setText(QApplication::translate("TransDialog1D", "Alpha", 0, QApplication::UnicodeUTF8));
    groupBox_Groups->setTitle(QApplication::translate("TransDialog1D", "Groups", 0, QApplication::UnicodeUTF8));
    radioButton_User->setText(QApplication::translate("TransDialog1D", "User", 0, QApplication::UnicodeUTF8));
    radioButton_Luminance->setText(QApplication::translate("TransDialog1D", "Luminance", 0, QApplication::UnicodeUTF8));
    radioButton_Intensity->setText(QApplication::translate("TransDialog1D", "Intensity", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(TransDialog1D);
    } // retranslateUi

};

namespace Ui {
    class TransDialog1D: public Ui_TransDialog1D {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TRANSDIALOG1D_H
