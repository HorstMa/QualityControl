// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckOfPads.h
/// \author Laura Serksnyte
///

#ifndef QC_MODULE_TPC_CheckOfPads_H
#define QC_MODULE_TPC_CheckOfPads_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Check whether the cluster number for a track is smaller than 40 or 20 in Track task.
///
/// \author Laura Serksnyte
class CheckOfPads : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  CheckOfPads() = default;
  /// Destructor
  ~CheckOfPads() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(CheckOfPads, 1);
  std::vector<std::string> mSectorsName_EV;
  std::vector<Quality> mSectorsQuality_EV;
  std::vector<std::string> mSectorsName_Mean;
  std::vector<Quality> mSectorsQuality_Mean;
  std::vector<Quality> mSectorsQuality_Empty;
  std::vector<std::string> mSectorsName;
  std::vector<Quality> mSectorsQuality;
  std::vector<std::string> mMOsToCheck2D;
  std::string mCheckChoice="NULL";
  static constexpr std::string_view CheckChoiceMean = "Mean";
  static constexpr std::string_view CheckChoiceExpectedValue = "ExpectedValue";
  static constexpr std::string_view CheckChoiceBoth = "Both";
  float mMediumQualityLimit;
  float mBadQualityLimit;
  float mExpectedValue;
  float mExpectedValueMediumSigmas;
  float mExpectedValueBadSigmas;
  float mMeanMediumSigmas;
  float mMeanBadSigmas;
  float mTotalMean;
  float mTotalStdev;
  bool mEmptyCheck=false;
  std::vector<float> mPadMeans;
  std::vector<float> mPadStdev;
  std::vector<float> mEmptyPadPercent;
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_CheckOfPads_H