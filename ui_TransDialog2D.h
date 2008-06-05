/********************************************************************************
** Form generated from reading ui file 'TransDialog2D.ui'
**
** Created: Thu 5. Jun 10:02:41 2008
**      by: Qt User Interface Compiler version 4.4.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_TRANSDIALOG2D_H
#define UI_TRANSDIALOG2D_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>

QT_BEGIN_NAMESPACE

class Ui_TransDialog2D
{
public:
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TransDialog2D)
    {
    if (TransDialog2D->objectName().isEmpty())
        TransDialog2D->setObjectName(QString::fromUtf8("TransDialog2D"));
    TransDialog2D->resize(561, 465);
    buttonBox = new QDialogButtonBox(TransDialog2D);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(210, 420, 341, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    retranslateUi(TransDialog2D);
    QObject::connect(buttonBox, SIGNAL(accepted()), TransDialog2D, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), TransDialog2D, SLOT(reject()));

    QMetaObject::connectSlotsByName(TransDialog2D);
    } // setupUi

    void retranslateUi(QDialog *TransDialog2D)
    {
    TransDialog2D->setWindowTitle(QApplication::translate("TransDialog2D", "Dialog", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(TransDialog2D);
    } // retranslateUi

};

namespace Ui {
    class TransDialog2D: public Ui_TransDialog2D {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TRANSDIALOG2D_H
