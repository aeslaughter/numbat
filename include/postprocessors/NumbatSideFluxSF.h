/*****************************************************************/
/*      NUMBAT - NUMerical Boussinesq Advective Transport        */
/*                                                               */
/*           All contents are licensed under MIT/BSD             */
/*              See LICENSE for full restrictions                */
/*****************************************************************/

#ifndef NUMBATSIDEFLUXSF_H
#define NUMBATSIDEFLUXSF_H

#include "SideIntegralVariablePostprocessor.h"

class NumbatSideFluxSF;

template <>
InputParameters validParams<NumbatSideFluxSF>();

/**
 * Calculates dimensionless flux over boundary for the streamfunction
 * formulation
 */
class NumbatSideFluxSF : public SideIntegralVariablePostprocessor
{
public:
  NumbatSideFluxSF(const InputParameters & parameters);

protected:
  virtual Real computeQpIntegral() override;
};

#endif // NUMBATSIDEFLUXSF_H
