/********************************************************************************
** Form generated from reading ui file '1DTrans.ui'
**
** Created: Wed 4. Jun 17:05:17 2008
**      by: Qt User Interface Compiler version 4.4.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_1DTRANS_H
#define UI_1DTRANS_H

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
    QGroupBox *groupBox;
    QCheckBox *checkBox_3;
    QCheckBox *checkBox_4;
    QCheckBox *checkBox_5;
    QCheckBox *checkBox_6;
    QGraphicsView *graphicsView;
    QSlider *verticalSlider;
    QGroupBox *groupBox_2;
    QRadioButton *radioButton;
    QRadioButton *radioButton_2;
    QRadioButton *radioButton_3;
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
    groupBox = new QGroupBox(TransDialog1D);
    groupBox->setObjectName(QString::fromUtf8("groupBox"));
    groupBox->setMinimumSize(QSize(131, 141));
    groupBox->setMaximumSize(QSize(131, 141));
    groupBox->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    groupBox->setFlat(false);
    groupBox->setCheckable(false);
    checkBox_3 = new QCheckBox(groupBox);
    checkBox_3->setObjectName(QString::fromUtf8("checkBox_3"));
    checkBox_3->setGeometry(QRect(10, 20, 72, 18));
    checkBox_4 = new QCheckBox(groupBox);
    checkBox_4->setObjectName(QString::fromUtf8("checkBox_4"));
    checkBox_4->setGeometry(QRect(10, 50, 72, 18));
    checkBox_5 = new QCheckBox(groupBox);
    checkBox_5->setObjectName(QString::fromUtf8("checkBox_5"));
    checkBox_5->setGeometry(QRect(10, 80, 72, 18));
    checkBox_6 = new QCheckBox(groupBox);
    checkBox_6->setObjectName(QString::fromUtf8("checkBox_6"));
    checkBox_6->setGeometry(QRect(10, 110, 72, 18));

    gridLayout->addWidget(groupBox, 0, 0, 1, 1);

    graphicsView = new QGraphicsView(TransDialog1D);
    graphicsView->setObjectName(QString::fromUtf8("graphicsView"));
    graphicsView->setMinimumSize(QSize(309, 307));
    graphicsView->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    graphicsView->setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);

    gridLayout->addWidget(graphicsView, 0, 1, 3, 1);

    verticalSlider = new QSlider(TransDialog1D);
    verticalSlider->setObjectName(QString::fromUtf8("verticalSlider"));
    verticalSlider->setOrientation(Qt::Vertical);

    gridLayout->addWidget(verticalSlider, 0, 2, 3, 1);

    groupBox_2 = new QGroupBox(TransDialog1D);
    groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
    groupBox_2->setMinimumSize(QSize(131, 111));
    groupBox_2->setMaximumSize(QSize(131, 111));
    radioButton = new QRadioButton(groupBox_2);
    radioButton->setObjectName(QString::fromUtf8("radioButton"));
    radioButton->setGeometry(QRect(10, 20, 84, 18));
    radioButton->setChecked(true);
    radioButton_2 = new QRadioButton(groupBox_2);
    radioButton_2->setObjectName(QString::fromUtf8("radioButton_2"));
    radioButton_2->setGeometry(QRect(10, 50, 84, 18));
    radioButton_3 = new QRadioButton(groupBox_2);
    radioButton_3->setObjectName(QString::fromUtf8("radioButton_3"));
    radioButton_3->setGeometry(QRect(10, 80, 84, 18));

    gridLayout->addWidget(groupBox_2, 1, 0, 1, 1);

    verticalSpacer = new QSpacerItem(128, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    gridLayout->addItem(verticalSpacer, 2, 0, 1, 1);

    buttonBox = new QDialogButtonBox(TransDialog1D);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    gridLayout->addWidget(buttonBox, 3, 0, 1, 3);

    QWidget::setTabOrder(checkBox_3, checkBox_4);
    QWidget::setTabOrder(checkBox_4, checkBox_5);
    QWidget::setTabOrder(checkBox_5, checkBox_6);
    QWidget::setTabOrder(checkBox_6, radioButton);
    QWidget::setTabOrder(radioButton, radioButton_2);
    QWidget::setTabOrder(radioButton_2, radioButton_3);
    QWidget::setTabOrder(radioButton_3, graphicsView);
    QWidget::setTabOrder(graphicsView, verticalSlider);
    QWidget::setTabOrder(verticalSlider, buttonBox);

    retranslateUi(TransDialog1D);
    QObject::connect(buttonBox, SIGNAL(accepted()), TransDialog1D, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), TransDialog1D, SLOT(reject()));

    QMetaObject::connectSlotsByName(TransDialog1D);
    } // setupUi

    void retranslateUi(QDialog *TransDialog1D)
    {
    TransDialog1D->setWindowTitle(QApplication::translate("TransDialog1D", "1D Transfer Function Editor", 0, QApplication::UnicodeUTF8));
    groupBox->setTitle(QApplication::translate("TransDialog1D", "Color Components", 0, QApplication::UnicodeUTF8));
    checkBox_3->setText(QApplication::translate("TransDialog1D", "Red", 0, QApplication::UnicodeUTF8));
    checkBox_4->setText(QApplication::translate("TransDialog1D", "Green", 0, QApplication::UnicodeUTF8));
    checkBox_5->setText(QApplication::translate("TransDialog1D", "Blue", 0, QApplication::UnicodeUTF8));
    checkBox_6->setText(QApplication::translate("TransDialog1D", "Alpha", 0, QApplication::UnicodeUTF8));
    groupBox_2->setTitle(QApplication::translate("TransDialog1D", "Groups", 0, QApplication::UnicodeUTF8));
    radioButton->setText(QApplication::translate("TransDialog1D", "User", 0, QApplication::UnicodeUTF8));
    radioButton_2->setText(QApplication::translate("TransDialog1D", "Luminance", 0, QApplication::UnicodeUTF8));
    radioButton_3->setText(QApplication::translate("TransDialog1D", "Intensity", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(TransDialog1D);
    } // retranslateUi

};

namespace Ui {
    class TransDialog1D: public Ui_TransDialog1D {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_1DTRANS_H
