#include "../Tuvok/StdTuvokDefines.h"
#include <sstream>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "BasicSettingsDlg.h"

using namespace tuvok;

BasicSettingsDlg::BasicSettingsDlg(MasterController& mcontroller,
                                   enum PerformanceLevel defaultLevel,
                                   QWidget* parent /* = 0 */,
                                   Qt::WindowFlags flags /* = 0 */) :
  QDialog(parent, flags),
  ctlr(mcontroller),
  lDesc(NULL),
  slPerf(NULL),
  useAdvanced(false)
{
  setupUI(this, defaultLevel);
}

BasicSettingsDlg::~BasicSettingsDlg(void) { }

void BasicSettingsDlg::setupUI(QDialog *dlg, enum PerformanceLevel lvl) {
  // Erase old layout/info.
  qDeleteAll(dlg->children());

  dlg->setWindowTitle("ImageVis3D Settings");
  dlg->setObjectName(QString::fromUtf8("BasicSettingsDlg"));
  dlg->setWindowModality(Qt::ApplicationModal);
  dlg->resize(300, 150);
  dlg->setContextMenuPolicy(Qt::DefaultContextMenu);
  dlg->setSizeGripEnabled(true);
  dlg->setModal(true);

  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->setObjectName(QString::fromUtf8("vlayout"));

  // perf / slider / and a description of the current setting
  QHBoxLayout* hPerf = new QHBoxLayout();
  QLabel* lPerf = new QLabel("Performance:");
  this->slPerf = new QSlider();
  this->slPerf->setOrientation(Qt::Horizontal);
  this->slPerf->setMinimum(MAX_RESPONSIVENESS);
  this->slPerf->setMaximum(MAX_PERFORMANCE);
  this->slPerf->setMinimumSize(QSize(75, 20));
  this->slPerf->setTickPosition(QSlider::TicksBelow);
  this->slPerf->setTickInterval(1);
  this->slPerf->setValue(int(lvl));
  this->lDesc = new QLabel("Favor more responsive behavior.");
  QSizePolicy lblPol(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
  lblPol.setHorizontalStretch(1);
  lblPol.setVerticalStretch(1);
  this->lDesc->setSizePolicy(lblPol);
  this->lDesc->setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
  this->lDesc->setWordWrap(true);
  hPerf->addWidget(lPerf);
  hPerf->addWidget(this->slPerf);
  hPerf->addWidget(this->lDesc);

  // buttons: we put them in a layout so we can add an 'advanced' one.
  QHBoxLayout* hbuttons = new QHBoxLayout();
  QDialogButtonBox* bbox = new QDialogButtonBox();
  QPushButton* btnAdvanced = new QPushButton("Advanced");
  bbox->setOrientation(Qt::Horizontal);
  bbox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  hbuttons->addWidget(btnAdvanced);
  hbuttons->addWidget(bbox);

  vlayout->addLayout(hPerf);
  vlayout->addLayout(hbuttons);
  this->setLayout(vlayout);

  connect(slPerf, SIGNAL(valueChanged(int)), dlg, SLOT(PerfLevelChanged(int)));
  connect(bbox, SIGNAL(accepted()), dlg, SLOT(accept()));
  connect(bbox, SIGNAL(rejected()), dlg, SLOT(reject()));
  connect(btnAdvanced, SIGNAL(clicked()), dlg, SLOT(AdvancedFeatures()));
  this->PerfLevelChanged(int(lvl));
}

enum PerformanceLevel BasicSettingsDlg::GetPerformanceLevel() const {
  return static_cast<PerformanceLevel>(this->slPerf->value());
}

bool BasicSettingsDlg::GetUseAdvancedSettings() const {
  return useAdvanced;
}

void BasicSettingsDlg::PerfLevelChanged(int level) {
  std::ostringstream help;
  help << level << ": ";
  PerformanceLevel pflvl = static_cast<PerformanceLevel>(level);
  switch(pflvl) {
    case MAX_RESPONSIVENESS:
      help << "Favor fast response time and display more intermediate results."
              "  This makes ImageVis3D feel more 'lightweight', but it will "
              "take more time to reach the highest resolution version.";
      break;
    case MIDDLE_1:
      help << "Provide an even balance between response time and performance.";
      break;
    case MAX_PERFORMANCE:
      help << "Favor maximum performance over intermediate results.  This "
              "minimizes the time ImageVis3D takes to reach the final, "
              "maximum-resolution rendering, but ImageVis3D may feel more "
              "sluggish while e.g. changing the transfer function.";
      break;
  }
  this->lDesc->setText(help.str().c_str());
}
void BasicSettingsDlg::AdvancedFeatures() {
  this->useAdvanced = true;
  this->reject();
}

/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/
