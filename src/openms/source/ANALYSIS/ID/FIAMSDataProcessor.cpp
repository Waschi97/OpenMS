// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Svetlana Kutuzova, Douglas McCloskey $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/ID/FIAMSDataProcessor.h>
#include <OpenMS/ANALYSIS/ID/AccurateMassSearchEngine.h>
#include <OpenMS/FORMAT/MzTabFile.h>
#include <OpenMS/FILTERING/NOISEESTIMATION/SignalToNoiseEstimatorMedianRapid.h>
#include <OpenMS/ANALYSIS/OPENSWATH/SpectrumAddition.h>


namespace OpenMS {
  /// default constructor
  FIAMSDataProcessor::FIAMSDataProcessor() :
      DefaultParamHandler("FIAMSDataProcessor"),
      mzs_(),
      bin_sizes_(),
      sgfilter_(),
      picker_()
    {
    defaults_.setValue("filename", "fiams", "The filename to use for naming the output files");
    defaults_.setValue("dir_output", "", "The path to the directory where the output files will be placed");

    defaults_.setValue("resolution", 120000.0, "The instrument settings: resolution");

    defaults_.setValue("polarity", "positive", "The instrument settings: polarity");
    defaults_.setValidStrings("polarity", ListUtils::create<String>("positive,negative"));

    defaults_.setValue("max_mz", 1500, "Maximum mz");

    defaults_.setValue("bin_step", 20, "The size of the step to recalculated the bin size used for adding up spectra along the time axis");

    defaults_.setValue("db:mapping", ListUtils::create<String>("CHEMISTRY/HMDBMappingFile.tsv"), "For the accurate mass search. Database input file(s), containing three tab-separated columns of mass, formula, identifier. "
                                                                      "If 'mass' is 0, it is re-computed from the molecular sum formula. "
                                                                      "By default CHEMISTRY/HMDBMappingFile.tsv in OpenMS/share is used! If empty, the default will be used.");
    defaults_.setValue("db:struct", ListUtils::create<String>("CHEMISTRY/HMDB2StructMapping.tsv"), "For the accurate mass search. Database input file(s), containing four tab-separated columns of identifier, name, SMILES, INCHI."
                                                                        "The identifier should match with mapping file. SMILES and INCHI are reported in the output, but not used otherwise. "
                                                                        "By default CHEMISTRY/HMDB2StructMapping.tsv in OpenMS/share is used! If empty, the default will be used.");
    defaults_.setValue("positive_adducts", "CHEMISTRY/PositiveAdducts.tsv", "For the accurate mass search. This file contains the list of potential positive adducts that will be looked for in the database. "
                                                                                  "Edit the list if you wish to exclude/include adducts. "
                                                                                  "By default CHEMISTRY/PositiveAdducts.tsv in OpenMS/share is used! If empty, the default will be used.", ListUtils::create<String>("advanced"));
    defaults_.setValue("negative_adducts", "CHEMISTRY/NegativeAdducts.tsv", "For the accurate mass search. This file contains the list of potential negative adducts that will be looked for in the database. "
                                                                                  "Edit the list if you wish to exclude/include adducts. "
                                                                                  "By default CHEMISTRY/NegativeAdducts.tsv in OpenMS/share is used! If empty, the default will be used.", ListUtils::create<String>("advanced"));

    defaults_.setValue("store_progress", "true", "If the intermediate files should be stored in the output directory");
    
    defaults_.setValue("sgf:frame_length", 11, "SavitzkyGolayFilter parameter. The number of subsequent data points used for smoothing");
    defaults_.setValue("sgf:polynomial_order", 4, "SavitzkyGolayFilter parameter. Order or the polynomial that is fitted");
    defaultsToParam_();
  }

  /// Copy constructor
  FIAMSDataProcessor::FIAMSDataProcessor(const FIAMSDataProcessor& source) :
    DefaultParamHandler(source),
    mzs_(source.mzs_),
    bin_sizes_(source.bin_sizes_),
    sgfilter_(source.sgfilter_),
    picker_(source.picker_)
  {
  }

  /// assignment operator
  FIAMSDataProcessor& FIAMSDataProcessor::operator=(const FIAMSDataProcessor& rhs) {
    if (this == &rhs) return *this;
    mzs_ = rhs.mzs_;
    bin_sizes_ = rhs.bin_sizes_;
    sgfilter_ = rhs.sgfilter_;
    picker_ = rhs.picker_;
    return *this;
  }

  void FIAMSDataProcessor::updateMembers_() {
    float max_mz_ = param_.getValue("max_mz");
    float bin_step_ = param_.getValue("bin_step");
    float resolution_ = static_cast<float>(param_.getValue("resolution"));
    size_t n_bins = static_cast<int> (max_mz_ / bin_step_);
    mzs_.clear();
    bin_sizes_.clear();
    mzs_.reserve(n_bins);
    bin_sizes_.reserve(n_bins);
    for (size_t i = 0; i < n_bins; i++) {
        mzs_.push_back((i+1)*bin_step_);
        bin_sizes_.push_back(mzs_[i] / (resolution_*4.0));
    }
    Param p;
    p.setValue("frame_length", param_.getValue("sgf:frame_length"));
    p.setValue("polynomial_order", param_.getValue("sgf:polynomial_order"));
    sgfilter_.setParameters(p);
  }

  void FIAMSDataProcessor::cutForTime(const MSExperiment & experiment, const float & n_seconds, std::vector<MSSpectrum> & output) {
      for (const auto & s : experiment.getSpectra()) {
          if (s.getRT() < n_seconds) output.push_back(s);
      }
  }

  MSSpectrum FIAMSDataProcessor::mergeAlongTime(
    const std::vector<MSSpectrum> & input
    ) {
      MSSpectrum output;
      for (size_t i = 0; i < mzs_.size() - 1; i++) {
          OpenMS::MSSpectrum full_spectrum = OpenMS::SpectrumAddition::addUpSpectra(
              input, bin_sizes_[i], false
          );
          for (auto it = full_spectrum.begin(); it != full_spectrum.end(); ++it) {
              if (it->getMZ() > mzs_[i+1]) break;
              if (it->getMZ() >= mzs_[i]) output.push_back(*it);
          }
      }
      output.sortByPosition();
      return output;
  }

  MSSpectrum FIAMSDataProcessor::extractPeaks(const MSSpectrum & input) {
    MSSpectrum spectrum(input);
    sgfilter_.filter(spectrum);

    MSSpectrum picked;
    picker_.pick(spectrum, picked);

    return picked;
  }

  FeatureMap FIAMSDataProcessor::convertToFeatureMap(const MSSpectrum & input) {
    String polarity_ = param_.getValue("polarity");
    FeatureMap output;
    for (auto it = input.begin(); it != input.end(); ++it) {
        Feature f;
        f.setIntensity(it->getIntensity());
        f.setMZ(it->getMZ());
        f.setMetaValue("scan_polarity", polarity_);
        output.push_back(f);
    }
    return output;
  }

  void FIAMSDataProcessor::runAccurateMassSearch(FeatureMap & input, OpenMS::MzTab & output) {
    Param ams_param;
    ams_param.setValue("ionization_mode", "auto");
    ams_param.setValue("mass_error_value", 1e+06 / (static_cast<float>(param_.getValue("resolution"))*2));
    ams_param.setValue("db:mapping", param_.getValue("db:mapping"));
    ams_param.setValue("db:struct", param_.getValue("db:struct"));
    ams_param.setValue("positive_adducts", param_.getValue("positive_adducts"));
    ams_param.setValue("negative_adducts", param_.getValue("negative_adducts"));

    AccurateMassSearchEngine ams;
    ams.setParameters(ams_param);
    ams.init();

    ams.run(input, output);
  }

  MSSpectrum FIAMSDataProcessor::trackNoise(const MSSpectrum & input) {
    SignalToNoiseEstimatorMedianRapid sne(50);
    MSSpectrum output;
    if (input.size() == 0) {
      return output;
    }
    std::vector<double> mzs, intensities;
    mzs.reserve(input.size());
    intensities.reserve(input.size());
    for (auto it = input.begin(); it != input.end(); ++it)
    {
        mzs.push_back(it->getMZ());
        intensities.push_back(it->getIntensity());
    }
    SignalToNoiseEstimatorMedianRapid::NoiseEstimator e = sne.estimateNoise(mzs, intensities);
    
    for (auto it = input.begin(); it != input.end(); ++it)
    {
        Peak1D peak;
        peak.setMZ(it->getMZ());
        peak.setIntensity(e.get_noise_value(it->getMZ()));
        output.push_back(peak);
    }
    return output;
  }

  void FIAMSDataProcessor::run(const MSExperiment & experiment, const float & n_seconds, OpenMS::MzTab & output) {
    std::vector<MSSpectrum> output_cut;
    cutForTime(experiment, n_seconds, output_cut);
    MSSpectrum merged_spectrum = mergeAlongTime(output_cut);
    MSSpectrum picked_spectrum = extractPeaks(merged_spectrum);
    String postfix = String(static_cast<int>(n_seconds));
    String dir_output_ = param_.getValue("dir_output");
    String filename_ = param_.getValue("filename");
    if (param_.getValue("store_progress").toBool()) {
      storeSpectrum_(merged_spectrum, dir_output_ + "/" + filename_ + "_merged_" + postfix + ".mzML");
      storeSpectrum_(picked_spectrum, dir_output_ + "/" + filename_ + "_picked_" + postfix + ".mzML");
    }
    FeatureMap picked_features = convertToFeatureMap(picked_spectrum);
    MSSpectrum signal_to_noise = trackNoise(picked_spectrum);
    storeSpectrum_(signal_to_noise, dir_output_ + "/" + filename_ + "_signal_to_noise_" + postfix + ".mzML");
    runAccurateMassSearch(picked_features, output);
    OpenMS::MzTabFile mztab_outfile;
    mztab_outfile.store(dir_output_ + "/" + filename_ + "_" + postfix + ".mzTab", output);
  }

  void FIAMSDataProcessor::storeSpectrum_(const MSSpectrum & input, String filename) {
      MzMLFile mzml;
      MSExperiment exp;
      exp.addSpectrum(input);
      mzml.store(filename, exp);
  }

  /// Get mass-to-charge ratios to base the sliding window upon
  const std::vector<float> FIAMSDataProcessor::getMZs(){
    return mzs_;
  }

  /// Get the sliding bin sizes
  const std::vector<float> FIAMSDataProcessor::getBinSizes(){
    return bin_sizes_;
  }
}