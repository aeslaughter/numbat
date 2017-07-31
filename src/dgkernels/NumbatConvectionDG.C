/*****************************************************************/
/*      NUMBAT - NUMerical Boussinesq Advective Transport        */
/*                                                               */
/*           All contents are licensed under MIT/BSD             */
/*              See LICENSE for full restrictions                */
/*****************************************************************/

#include "NumbatConvectionDG.h"
#include "MooseMesh.h"

template <>
InputParameters
validParams<NumbatConvectionDG>()
{
  InputParameters params = validParams<DGKernel>();
  RealVectorValue g(0, 0, -9.81);
  params.addParam<RealVectorValue>(
      "gravity", g, "Gravity vector. Defaults to -9.81 in y direction for 2D, z direction for 3D");
  params.addRequiredCoupledVar("pressure", "Pressure variable");
  params.addClassDescription("Discontinuous Galerkin upwinded convection");
  return params;
}

NumbatConvectionDG::NumbatConvectionDG(const InputParameters & parameters)
  : DerivativeMaterialInterface<DGKernel>(parameters),
    _concentration_name(_var.name()),
    _grad_pressure(coupledGradient("pressure")),
    _pvar(coupled("pressure")),
    _porosity(getMaterialProperty<Real>("porosity")),
    _density(getMaterialProperty<Real>("density")),
    _ddensity_dc(getMaterialPropertyDerivative<Real>("density", _concentration_name)),
    _gravity(getParam<RealVectorValue>("gravity")),
    _viscosity(getMaterialProperty<Real>("viscosity")),
    _permeability(getMaterialProperty<RealTensorValue>("permeability"))
{
  // Numbat only works in 2 or 3 dimensions
  if (_mesh.dimension() == 1)
    mooseError("Numbat only works for 2D or 3D meshes!");

  // If gravity hasn't been provided, use a value of -9.81 in the y direction
  // for 2D, or in the z direction for 3D (which is the default)
  if (!parameters.isParamSetByUser("gravity"))
    if (_mesh.dimension() == 2)
      _gravity = RealVectorValue(0, -9.81, 0);
}

Real
NumbatConvectionDG::computeQpResidual(Moose::DGResidualType type)
{
  Real residual = 0.0;

  RealVectorValue velocity =
      -_permeability[_qp] / _viscosity[_qp] * (_grad_pressure[_qp] - _density[_qp] * _gravity);
  Real vdotn = velocity * _normals[_qp];

  switch (type)
  {
    case Moose::Element:
      if (vdotn >= 0)
        residual += vdotn * _u[_qp] * _test[_i][_qp];
      else
        residual += vdotn * _u_neighbor[_qp] * _test[_i][_qp];
      break;

    case Moose::Neighbor:
      if (vdotn >= 0)
        residual -= vdotn * _u[_qp] * _test_neighbor[_i][_qp];
      else
        residual -= vdotn * _u_neighbor[_qp] * _test_neighbor[_i][_qp];
      break;
  }

  return residual;
}

Real
NumbatConvectionDG::computeQpJacobian(Moose::DGJacobianType type)
{
  Real jacobian = 0.0;

  RealVectorValue velocity =
      -_permeability[_qp] / _viscosity[_qp] * (_grad_pressure[_qp] - _density[_qp] * _gravity);
  Real vdotn = velocity * _normals[_qp];

  switch (type)
  {
    case Moose::ElementElement:
      if (vdotn >= 0)
        jacobian += vdotn * _phi[_j][_qp] * _test[_i][_qp];
      break;

    case Moose::ElementNeighbor:
      if (vdotn < 0)
        jacobian += vdotn * _phi_neighbor[_j][_qp] * _test[_i][_qp];
      break;

    case Moose::NeighborElement:
      if (vdotn >= 0)
        jacobian -= vdotn * _phi[_j][_qp] * _test_neighbor[_i][_qp];
      break;

    case Moose::NeighborNeighbor:
      if (vdotn < 0)
        jacobian -= vdotn * _phi_neighbor[_j][_qp] * _test_neighbor[_i][_qp];
      break;
  }

  return jacobian;
}

Real
NumbatConvectionDG::computeQpOffDiagJacobian(Moose::DGJacobianType type, unsigned int jvar)
{
  Real offdiagjacobian = 0.0;

  if (jvar == _pvar)
  {
    RealVectorValue velocity =
        -_permeability[_qp] / _viscosity[_qp] * (_grad_pressure[_qp] - _density[_qp] * _gravity);
    Real vdotn = velocity * _normals[_qp];

    RealVectorValue dvelocity = -_permeability[_qp] / _viscosity[_qp] * (_grad_phi[_j][_qp]);
    Real dvdotn = dvelocity * _normals[_qp];

    RealVectorValue dvelocity_neighbor =
        -_permeability[_qp] / _viscosity[_qp] * (_grad_phi_neighbor[_j][_qp]);
    Real dvdotn_neighbor = dvelocity_neighbor * _normals[_qp];

    switch (type)
    {
      case Moose::ElementElement:
        if (vdotn >= 0)
          offdiagjacobian += dvdotn * _u[_qp] * _test[_i][_qp];
        break;

      case Moose::ElementNeighbor:
        if (vdotn < 0)
          offdiagjacobian -= dvdotn_neighbor * _u[_qp] * _test_neighbor[_i][_qp];
        break;

      case Moose::NeighborElement:
        if (vdotn >= 0)
          offdiagjacobian -= dvdotn * _u[_qp] * _test_neighbor[_i][_qp];
        break;

      case Moose::NeighborNeighbor:
        if (vdotn < 0)
          offdiagjacobian -= dvdotn_neighbor * _u[_qp] * _test_neighbor[_i][_qp];
        break;
    }
  }

  return offdiagjacobian;
}