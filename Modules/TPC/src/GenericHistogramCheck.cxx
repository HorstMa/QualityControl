// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   GenericHistogramCheck.cxx
/// \author My Name
///

#include "TPC/GenericHistogramCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>
#include <THn.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tpc
{

void GenericHistogramCheck::configure()
{

  // Read in dimensionality of histogram
  // optional (2D,3D,..) axis (x,y,z,...) or full mean
  // Read in expectation value
}

Quality GenericHistogramCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  ILOG(Warning, Support) << "Check started....?" << ENDM;
  Quality result = Quality::Null;

  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;

      ILOG(Error, Support) << "No MO found" << ENDM;
    }
    ILOG(Warning, Support) << "MO: " << mo->getName() << ENDM;
    auto* h = (TH1F*)(mo->getObject());
    // const int HistDimension = h->GetNdimensions();

    ILOG(Warning, Support) << "Our Dimensionality: " << mDimension << ENDM;
    result = Quality::Good;

    mMean = h->ComputeIntegral() / (float)h->GetXaxis()->GetNbins();

    ILOG(Warning, Support) << "Mean: " << mMean << ENDM;
  } // for Mo map

  return result;
}

std::string GenericHistogramCheck::getAcceptedType() { return "TH1"; }

void GenericHistogramCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Warning, Support) << "BEAUTIFYY" << ENDM;
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::tpc
