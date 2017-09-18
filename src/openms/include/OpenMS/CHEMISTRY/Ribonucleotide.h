// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2017.
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
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------
//

#ifndef OPENMS_CHEMISTRY_RIBONUCLEOTIDE_H
#define OPENMS_CHEMISTRY_RIBONUCLEOTIDE_H

#include <OpenMS/DATASTRUCTURES/String.h>

#include <iostream>

namespace OpenMS
{
  /** @ingroup Chemistry

      @brief Representation of a ribonucleotide (modified or unmodified)

      The available information is based on the Modomics database (http://modomics.genesilico.pl/modifications/).

      @see RibonucleotideDB
  */
  class OPENMS_DLLAPI Ribonucleotide
  {
  public:
    Ribonucleotide():
      origin('X'), mono_mass(0.0), avg_mass(0.0)
    {
    }

    enum RiboNucleotideType
    {                 // NB: Not all fragments types are valid for all residue types, this class should probably get split
      Full = 0,       // with N-terminus and C-terminus
      Internal,       // internal, without any termini
      FivePrime,      // only N-terminus
      ThreePrime,      // only C-terminus
      AIon,           // MS:1001229 N-terminus up to the C-alpha/carbonyl carbon bond
      BIon,           // MS:1001224 N-terminus up to the peptide bond
      CIon,           // MS:1001231 N-terminus up to the amide/C-alpha bond
      XIon,           // MS:1001228 amide/C-alpha bond up to the C-terminus
      YIon,           // MS:1001220 peptide bond up to the C-terminus
      ZIon,           // MS:1001230 C-alpha/carbonyl carbon bond
      Precursor,      // MS:1001523 Precursor ion
      BIonMinusH20,   // MS:1001222 b ion without water
      YIonMinusH20,   // MS:1001223 y ion without water
      BIonMinusNH3,   // MS:1001232 b ion without ammonia
      YIonMinusNH3,   // MS:1001233 y ion without ammonia
      NonIdentified,  // MS:1001240 Non-identified ion
      Unannotated,    // no stored annotation
      WIon,             //W ion, added for nucleic acid support
      AminusB,        //A ion with base loss, added for nucleic acid support
      DIon,           // D-ion for nucleic acid support
      SizeOfResidueType
    };
    enum NucleicAcidType
    {
        DNA = 0,
        RNA,
        Undefined
    };

    String name;
    String code; // short name
    String new_code;
    String html_code; // RNAMods code
    String formula;
    char origin;
    double mono_mass;
    double avg_mass;
  };

  OPENMS_DLLAPI std::ostream& operator<<(std::ostream& os, const Ribonucleotide& ribo);
}

#endif
