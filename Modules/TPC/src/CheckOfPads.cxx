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
/// \file   CheckOfPads.cxx
/// \author Laura Serksnyte, Maximilian Horst
///

#include "TPC/CheckOfPads.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPad.h>
#include <TList.h>
#include <TPaveText.h>

#include <iostream>

namespace o2::quality_control_modules::tpc
{
void CheckOfPads::configure()
{
  if (auto param = mCustomParameters.find("mediumQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mMediumQualityLimit = std::atof(param->second.c_str());
  } else {
    mMediumQualityLimit = 0.7;
    ILOG(Info, Support) << "Chosen check requires mediumQualityPercentageOfWorkingPads which is not given. Setting to default 0.7 ." << ENDM;
  }
  if (auto param = mCustomParameters.find("badQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mBadQualityLimit = std::atof(param->second.c_str());
  } else {
    mBadQualityLimit = 0.3;
    ILOG(Info, Support) << "Chosen check requires badQualityPercentageOfWorkingPads which is not given. Setting to default 0.3 ." << ENDM;
  }
  // check if expected Value Check is wished for:
  if (auto param = mCustomParameters.find("CheckChoice"); param != mCustomParameters.end()) {
    std::string CheckChoiceString = param->second.c_str();
    if (size_t finder = CheckChoiceString.find("ExpectedValue"); finder != std::string::npos) {
      mCheckChoice = "ExpectedValue";
      //ILOG(Warning, Support) << "Found expected Value in the CheckChoice String" << ENDM;
    }
    if (size_t finder = CheckChoiceString.find("Mean"); finder != std::string::npos) {
      //ILOG(Warning, Support) << "Found Mean in the CheckChoice String" << ENDM;
      if (mCheckChoice == "ExpectedValue") {
        mCheckChoice = "Both";
        //ILOG(Warning, Support) << "Setting CheckChoice to Both" << ENDM;
      } else {
        mCheckChoice = "Mean";
        //ILOG(Warning, Support) << "Setting CheckChoice to Mean" << ENDM;
      }
    }
    if (size_t finder = CheckChoiceString.find("Empty"); finder != std::string::npos) {
      mEmptyCheck = true;
      //ILOG(Warning, Support) << "Found Empty in the CheckChoice String. setting mEmptyCheck to " << mEmptyCheck << ENDM;
    }
    if (mCheckChoice == "NULL") {
      if (mEmptyCheck == false) {
        mCheckChoice = "Both";
        ILOG(Warning, Support) << "This Check requires a CheckChoice. The given value is wrong or not readable. Chose between 'ExpectedValue'(Compare the pad mean to an expectedValue), 'Mean' (compare pad mean to global mean) or Both (='Mean,ExpectedValue'). As a default 'Both' was selected." << ENDM;
      }
    }
  } else {
    mCheckChoice = "Both";
    ILOG(Warning, Support) << "This Check requires a CheckChoice, but no parameter was given. Chose between 'ExpectedValue'(Compare the pad mean to an expectedValue), 'Mean' (compare pad mean to global mean) or 'Both'(='Mean,ExpectedValue'). As a default 'Both' was selected." << ENDM;
  }
  if (mCheckChoice == "ExpectedValue" || mCheckChoice == "Both") {
    // load expectedValue
    if (auto param = mCustomParameters.find("ExpectedValue"); param != mCustomParameters.end()) {
      mExpectedValue = std::atof(param->second.c_str());
      //ILOG(Warning, Support) << " Expected Value: " << mExpectedValue << ENDM;
    } else {
      mExpectedValue = 1.0;
      ILOG(Info, Support) << "Chosen check requires ExpectedValue which is not given. Setting to default 1 ." << ENDM;
    }
    // load expectedValue Sigma Medium
    if (auto param = mCustomParameters.find("ExpectedValueSigmaMedium"); param != mCustomParameters.end()) {
      mExpectedValueMediumSigmas = std::atof(param->second.c_str());
    } else {
      mExpectedValueMediumSigmas = 3.;
      ILOG(Info, Support) << "Chosen check requires ExpectedValueSigmaMedium which is not given. Setting to default 3 sigma ." << ENDM;
    }
    // load expectedValue Sigma Bad
    if (auto param = mCustomParameters.find("ExpectedValueSigmaBad"); param != mCustomParameters.end()) {
      mExpectedValueBadSigmas = std::atof(param->second.c_str());
    } else {
      mExpectedValueBadSigmas = 6.;
      ILOG(Info, Support) << "Chosen check requires ExpectedValueSigmaBad which is not given. Setting to default 6 sigma ." << ENDM;
    }
  }
  // Cehck if Mean comparison is wished for:
  if (mCheckChoice == "Mean" || mCheckChoice == "Both") {
    // load Mean Sigma Medium
    if (auto param = mCustomParameters.find("MeanSigmaMedium"); param != mCustomParameters.end()) {
      mMeanMediumSigmas = std::atof(param->second.c_str());
    } else {
      mMeanMediumSigmas = 3;
      ILOG(Info, Support) << "Chosen check requires MeanSigmaMedium which is not given. Setting to default 3 sigma ." << ENDM;
    }
    // load Mean Sigma Bad
    if (auto param = mCustomParameters.find("MeanSigmaBad"); param != mCustomParameters.end()) {
      mMeanBadSigmas = std::atof(param->second.c_str());
    } else {
      mMeanBadSigmas = 6;
      ILOG(Info, Support) << "Chosen check requires MeanSigmaBad which is not given. Setting to default 6 sigma ." << ENDM;
    }
  }


  if (auto param = mCustomParameters.find("MOsNames2D"); param != mCustomParameters.end()) {
    auto temp = param->second.c_str();
    std::istringstream ss(temp);
    std::string token;
    while (std::getline(ss, token, ',')) {
      mMOsToCheck2D.emplace_back(token);
    }
  }
}

//______________________________________________________________________________
Quality CheckOfPads::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result_EV = Quality::Null;
  Quality result_Mean = Quality::Null;
  Quality result_Global = Quality::Null;
  Quality result_Empty = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
    }
    auto moName = mo->getName();
    std::string histName, histNameS;
    if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
      size_t end = moName.find("_2D");
      auto histSubName = moName.substr(7, end - 7);
      result_EV = Quality::Good;
      result_Mean = Quality::Good;

      auto* canv = (TCanvas*)mo->getObject();
      if (!canv)
        continue;
      // Check all histograms in the canvas

      mTotalMean = 0.;

      for (int tpads = 1; tpads <= 72; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        const auto histName = fmt::format("h_{:s}_ROC_{:02d}", histSubName, tpads - 1);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          mSectorsName.push_back("notitle");
          mSectorsQuality_EV.push_back(Quality::Null);
          continue;
        }
        TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          mSectorsName.push_back("notitle");
          mSectorsQuality_EV.push_back(Quality::Null);
          continue;
        }
        const std::string titleh = h->GetTitle();

        mSectorsName.push_back(titleh);
        // check if we are dealing with IROC or OROC
        float totalPads = 0;
        int MaximumXBin = 0;
        int MaximumYBin = 110;
        if (titleh.find("IROC") != std::string::npos) {
          totalPads = 5280;
          MaximumXBin = 62;
        } else if (titleh.find("OROC") != std::string::npos) {
          totalPads = 9280;
          MaximumXBin = 88;
        } else {
          return Quality::Null;
        }
        if (mEmptyCheck) {
          const int NX = h->GetNbinsX();
          const int NY = h->GetNbinsY();
          // Check how many of the pads are non zero
          float sum = 0.;
          for (int i = 1; i <= NX; i++) {
            for (int j = 1; j <= NY; j++) {
              float val = h->GetBinContent(i, j);
              if (val > 0.) {
                sum += 1;
              }
            }
          }
          // ILOG(Error, Support) << "Medium: " << mMediumQualityLimit * totalPads << " Bad: " << mBadQualityLimit * totalPads << ENDM;
          if (sum > mMediumQualityLimit * totalPads) {
            result_Empty = Quality::Good;
          } else if (sum < mBadQualityLimit * totalPads) {
            result_Empty = Quality::Bad;
          } else {
            result_Empty = Quality::Medium;
          }
          mSectorsQuality_Empty.push_back(result_Empty);
          mEmptyPadPercent.push_back(1. - (float)sum / totalPads);
        }

        float PadSum = 0.;
        int PadsCount = 0;
        float PadStdev = 0.;
        float PadMean = 0.;

        /////// Anchor: from here, access to full Pad. Calculate Means and such.
        // Run twice to get the mean and the standard deviation
        for (int RunNo = 1; RunNo <= 2; RunNo++) {
          // Run1: calculate single Pad total->Mean
          // Run2: calculate standardDeviation from mean
          for (int xBin = 1; xBin < MaximumXBin; xBin++) {
            for (int yBin = 1; yBin < MaximumYBin; yBin++) {
              float Binvalue = h->GetBinContent(xBin, yBin);
              if (Binvalue != 0) {
                if (RunNo == 1) {
                  PadSum += Binvalue;
                  PadsCount++;
                } else {
                  PadStdev += pow(Binvalue - PadMean, 2);
                }
              }
            }
          }

          if (RunNo == 1) {
            PadMean = PadSum / PadsCount;
          }
        }

        PadStdev = sqrt(PadStdev / (PadsCount - 1));
        // ILOG(Warning, Support) << "Pad: " << titleh << " Mean: " << PadMean << " Stdev: " << PadStdev << " Expected: " << mExpectedValue << " Sigma Med: " << mExpectedValueMediumSigmas << " Sigma Bad: " << mExpectedValueBadSigmas << ENDM;
        //  ILOG(Warning, Support) << "Medium Threshold: " << PadStdev * mExpectedValueMediumSigmas << " Bad Threshold: " << PadStdev * mExpectedValueBadSigmas << ENDM;
        mPadMeans.push_back(PadMean);
        mPadStdev.push_back(PadStdev);
      }
      float SumOfWeights = 0.;
      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads
        // calculate the total mean and standard deviation
        mTotalMean += mPadMeans[it] / mPadStdev[it];
        SumOfWeights += 1 / mPadStdev[it];
      }
      mTotalStdev = sqrt(1 / SumOfWeights); // standard deviation of the weighted average.
      mTotalMean /= SumOfWeights;           // Weighted average (by standard deviation) of the total mean
      // calculate the Qualities:

      for (size_t it = 0; it < mPadMeans.size(); it++) { // loop over all pads

        if (mCheckChoice == "ExpectedValue" || mCheckChoice == "Both") {

          if (fabs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueMediumSigmas) {
            result_EV = Quality::Good;
            // ILOG(Warning, Support) << "Good Quality! " << fabs(mPadMeans[it] - mExpectedValue) << " < " << mPadStdev[it] * mExpectedValueMediumSigmas << ENDM;
          } else if (fabs(mPadMeans[it] - mExpectedValue) >= mPadStdev[it] * mExpectedValueMediumSigmas && fabs(mPadMeans[it] - mExpectedValue) < mPadStdev[it] * mExpectedValueBadSigmas) {
            result_EV = Quality::Medium;
            // ILOG(Warning, Support) << "Medium Quality! " << fabs(mPadMeans[it] - mExpectedValue) << " > " << mPadStdev[it] * mExpectedValueMediumSigmas << ENDM;
          } else {
            result_EV = Quality::Bad;
            // ILOG(Warning, Support) << "Bad Quality! " << fabs(mPadMeans[it] - mExpectedValue) << " > " << mPadStdev[it] * mExpectedValueBadSigmas << ENDM;
          }
          // ILOG(Warning, Support) << "Pad: " << titleh << " Mean: " << mPadMeans[it] << " Stdev: " << mPadStdev[it] << " Expected: " << mExpectedValue << " Sigma Med: " << mExpectedValueMediumSigmas << " Sigma Bad: " << mExpectedValueBadSigmas << " Result: " << result_EV << ENDM;
        }

        mSectorsQuality_EV.push_back(result_EV);
        if (mCheckChoice == "Mean" || mCheckChoice == "Both") {

          if (fabs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanMediumSigmas) {
            result_Mean = Quality::Good;
            // ILOG(Warning, Support) << "Good Quality! " << fabs(mPadMeans[it] - mTotalMean) << " < " << mPadStdev[it] * mMeanMediumSigmas << ENDM;
          } else if (fabs(mPadMeans[it] - mTotalMean) >= mPadStdev[it] * mMeanMediumSigmas && fabs(mPadMeans[it] - mTotalMean) < mPadStdev[it] * mMeanBadSigmas) {
            result_Mean = Quality::Medium;
            // ILOG(Warning, Support) << "Medium Quality! " << fabs(mPadMeans[it] - mTotalMean) << " > " << mPadStdev[it] * mMeanMediumSigmas << ENDM;
          } else {
            result_Mean = Quality::Bad;
            // ILOG(Warning, Support) << "Bad Quality! " << fabs(mPadMeans[it] - mTotalMean) << " > " << mPadStdev[it] * mMeanBadSigmas << ENDM;
          }
          // ILOG(Warning, Support) << "Pad: " << titleh << " Mean: " << mPadMeans[it] << " Stdev: " << mPadStdev[it] << " Global Mean: " << mTotalMean << " Sigma Med: " << mMeanMediumSigmas << " Sigma Bad: " << mMeanBadSigmas << " Result: " << result_Mean << ENDM;
        }

        mSectorsQuality_Mean.push_back(result_Mean);

        ILOG(Warning, Support) << mSectorsQuality_Empty[it] << " " << result_Mean << " " << result_EV << ENDM;

        if (mCheckChoice == "Both") { // compare the total mean to the expected value. This is returned as the quality object
          if (fabs(mTotalMean - mExpectedValue) < mTotalStdev * mExpectedValueMediumSigmas) {
            result_Global = Quality::Good;
          } else if (fabs(mTotalMean - mExpectedValue) > mTotalStdev * mExpectedValueBadSigmas) {
            result_Global = Quality::Bad;
          } else {
            result_Global = Quality::Medium;
          }
          // compare Qualities of mean, expected value and (if applicable) empty
          if (result_Mean.isWorseThan(result_EV)) {
            // ILOG(Error, Support) << result_Mean << " is Worse than " << mSectorsQuality_EV[tpads - 1] << ENDM;
            if (mEmptyCheck) {
              if (result_Mean.isWorseThan(mSectorsQuality_Empty[it])) {

                mSectorsQuality.push_back(result_Mean);
              } else {
                mSectorsQuality.push_back(mSectorsQuality_Empty[it]);
              }
            } else {
              mSectorsQuality.push_back(result_Mean);
            }
          } else {
            if (mEmptyCheck) {
              if (result_EV.isWorseThan(mSectorsQuality_Empty[it])) {

                mSectorsQuality.push_back(result_EV);
              } else {
                mSectorsQuality.push_back(mSectorsQuality_Empty[it]);
              }
            } else {
              mSectorsQuality.push_back(result_EV);
            }
            // ILOG(Error, Support) << result_Mean << " is Better than or Equal to " << mSectorsQuality_EV[tpads - 1] << ENDM;
            // ILOG(Warning,Support)<<"Adding Quality: " << mSectorsQuality_EV[tpads - 1] <<ENDM;
          }
        } // if mCheckChoice==Both
        else if (mCheckChoice == "Mean") {
          if (mEmptyCheck) {
            if (result_Mean.isWorseThan(mSectorsQuality_Empty[it])) {
              mSectorsQuality.push_back(result_Mean);
            } else {
              mSectorsQuality.push_back(mSectorsQuality_Empty[it]);
            }
          } else {
            mSectorsQuality.push_back(result_Mean);
          }
        } else if (mCheckChoice == "ExpectedValue") {
          if (mEmptyCheck) {
            if (result_EV.isWorseThan(mSectorsQuality_Empty[it])) {

              mSectorsQuality.push_back(result_EV);
            } else {
              mSectorsQuality.push_back(mSectorsQuality_Empty[it]);
            }
          } else {
            mSectorsQuality.push_back(result_EV);
          }
        } else if (mEmptyCheck) {
          mSectorsQuality.push_back(mSectorsQuality_Empty[it]);
        } else {
          ILOG(Fatal, Support) << "No Quality object found! Check Choice was: " << mCheckChoice << " and EmptyCheck was set to: " << mEmptyCheck << ENDM;
        }
      } // for it in vectors (pads)

    } // if MO exists
  }   // MO-map loop
  return result_Global;
} // end of loop over moMap

//______________________________________________________________________________
std::string CheckOfPads::getAcceptedType() { return "TCanvas"; }

//______________________________________________________________________________
void CheckOfPads::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  // ILOG(Warning, Support) << "Now onto beautify!" << ENDM;
  auto moName = mo->getName();
  // ILOG(Warning, Support) << "Checking MO: " << moName << ENDM;
  if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {

    // ILOG(Warning, Support) << "In MO loop!" << ENDM;
    int padsTotal = 0, padsstart = 1000;
    auto* tcanv = (TCanvas*)mo->getObject();
    std::string histNameS, histName;
    padsstart = 1;
    padsTotal = 72;
    size_t end = moName.find("_2D");
    auto histSubName = moName.substr(7, end - 7);
    histNameS = fmt::format("h_{:s}_ROC", histSubName);
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {

      // ILOG(Warning, Support) << "In pad loop! pad:" << tpads - 1 << ENDM;
      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      // ILOG(Warning, Support) << "Found pad " << tpads - 1 << ENDM;
      pad->cd();
      TH1F* h = nullptr;
      histName = fmt::format("{:s}_{:02d}", histNameS, tpads - 1);
      h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
      if (!h) {
        continue;
      }
      // ILOG(Warning, Support) << "Found histogram " << histName << ENDM;
      const std::string titleh = h->GetTitle();
      // ILOG(Warning, Support) << "With title " << titleh << ENDM;
      auto it = std::find(mSectorsName.begin(), mSectorsName.end(), titleh);
      if (it == mSectorsName.end()) {
        continue;
      }

      const int index = std::distance(mSectorsName.begin(), it);
      TPaveText* msgQuality = new TPaveText(0.1, 0.88, 0.81, 0.95, "NDC");
      msgQuality->SetBorderSize(1);
      Quality qualitySpecial = mSectorsQuality[index];
      ILOG(Warning, Support) << "Pad No. " << tpads - 1 << " Has quality: " << qualitySpecial << ENDM;
      msgQuality->SetName(Form("%s_msg", mo->GetName()));
      if (qualitySpecial == Quality::Good) {
        msgQuality->Clear();
        msgQuality->AddText("Good");
        msgQuality->SetFillColor(kGreen);
      } else if (qualitySpecial == Quality::Bad) {
        msgQuality->Clear();
        msgQuality->AddText("Bad");
        msgQuality->SetFillColor(kRed);
      } else if (qualitySpecial == Quality::Medium) {
        msgQuality->Clear();
        msgQuality->AddText("Medium");
        msgQuality->SetFillColor(kOrange);
      } else if (qualitySpecial == Quality::Null) {
        h->SetFillColor(0);
      }
      if (mCheckChoice == "Both") {
        msgQuality->AddText(fmt::format("Global Mean: {:f}, Pad Mean: {:f}, Expected Value: {:f}", mTotalMean, mPadMeans[index], mExpectedValue).data());
      }
      if (mCheckChoice == "Mean") {
        msgQuality->AddText(fmt::format("Global Mean: {:f}, Pad Mean: {:f}", mTotalMean, mPadMeans[index]).data());
      }
      if (mCheckChoice == "ExpectedValue") {
        msgQuality->AddText(fmt::format("Pad Mean: {:f}, Expected Value: {:f}", mPadMeans[index], mExpectedValue).data());
      }
      if (mEmptyCheck) {
        // msgQuality->AddText(fmt::format("{:f} percent Empty pads. Med: {:f}, bad: {:f}", mEmptyPadPercent[index], mMediumQualityLimit, mBadQualityLimit).data());
        msgQuality->AddText(fmt::format("{:f} percent Empty pads.", mEmptyPadPercent[index]).data());
      }
      h->SetLineColor(kBlack);
      msgQuality->Draw("same");
    }
    // auto savefileNAme = fmt::format("/home/ge56luj/Desktop/ThisIsBeautifyObject{:s}.pdf", moName);
    // tcanv->SaveAs(savefileNAme.c_str());
    mSectorsName.clear();
    mSectorsQuality.clear();
  }
}

} // namespace o2::quality_control_modules::tpc
