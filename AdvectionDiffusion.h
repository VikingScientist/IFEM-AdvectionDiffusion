// $Id$
//==============================================================================
//!
//! \file AdvectionDiffusion.h
//!
//! \date Jun 12 2012
//!
//! \author Arne Morten Kvarving / SINTEF
//!
//! \brief Integrand implementations for Advection-Diffusion problems.
//!
//==============================================================================

#ifndef _ADVECTION_DIFFUSION_H
#define _ADVECTION_DIFFUSION_H

#include "IntegrandBase.h"
#include "ElmMats.h"
#include "EqualOrderOperators.h"
#include "ADFluidProperties.h"

class RealFunc;
class VecFunc;


/*!
  \brief Class representing the integrand of the Advection-Diffusion problem.
*/

class AdvectionDiffusion : public IntegrandBase
{
public:
  using WeakOps     = EqualOrderOperators::Weak;     //!< Convenience renaming
  using ResidualOps = EqualOrderOperators::Residual; //!< Convenience renaming

  //! \brief Enum defining the available stabilization methods.
  enum Stabilization { NONE, SUPG, GLS, MS };

  //! \brief Class representing the weak Dirichlet integrand.
  class WeakDirichlet : public IntegrandBase
  {
  public:
    //! \brief Default constructor.
    //! \param[in] n Number of spatial dimensions
    //! \param[in] CBI_ Model constant
    //! \param[in] gamma_ Adjoint factor
    WeakDirichlet(unsigned short int n, double CBI_ = 4, double gamma_ = 1.0);
    //! \brief The destructor deletes the advection field.
    virtual ~WeakDirichlet();

    //! \brief Returns that this integrand has no interior contributions.
    virtual bool hasInteriorTerms() const { return false; }

    //! \brief Defines which FE quantities are needed by the integrand.
    virtual int getIntegrandType() const { return ELEMENT_CORNERS; }

    using IntegrandBase::getLocalIntegral;
    //! \brief Returns a local integral contribution object for given element.
    //! \param[in] nen Number of nodes on element
    virtual LocalIntegral* getLocalIntegral(size_t nen, size_t, bool) const;

    using IntegrandBase::initElementBou;
    //! \brief Initializes current element for boundary integration.
    //! \param[in] MNPC Matrix of nodal point correspondance for current element
    //! \param elmInt Local integral for element
    virtual bool initElementBou(const std::vector<int>& MNPC,
                                LocalIntegral& elmInt);

    using IntegrandBase::evalBou;
    //! \brief Evaluates the integrand at a boundary point.
    //! \param elmInt The local integral object to receive the contributions
    //! \param[in] fe Finite element data of current integration point
    //! \param[in] X Cartesian coordinates of current integration point
    //! \param[in] normal Boundary normal vector at current integration point
    virtual bool evalBou(LocalIntegral& elmInt, const FiniteElement& fe,
                         const Vec3& X, const Vec3& normal) const;

    //! \brief Defines the advection field.
    void setAdvectionField(VecFunc* U) { Uad = U; }
    //! \brief Defines the flux function.
    void setFlux(RealFunc* f) { flux = f; }

    //! \brief Returns a reference to the fluid properties.
    AD::FluidProperties& getFluidProperties() { return props; }
    //! \brief Returns a const reference to the fluid properties.
    const AD::FluidProperties& getFluidProperties() const { return props; }

  protected:
    const double CBI;   //!< Model constant
    const double gamma; //!< Adjoint factor
    VecFunc*     Uad;   //!< Pointer to advection field
    RealFunc*    flux;  //!< Pointer to the flux field
    AD::FluidProperties props; //!< Fluid properties
  };

  //! \brief The default constructor initializes all pointers to zero.
  //! \param[in] n Number of spatial dimensions
  //! \param[in] s Stabilization option
  AdvectionDiffusion(unsigned short int n = 3, Stabilization s = NONE);

  //! \brief Class representing the Advection-Diffusion element matrices.
  class ElementInfo : public ElmMats
  {
  public:
    //! \brief Default constructor.
    explicit ElementInfo(bool lhs = true) : ElmMats(lhs), hk(0.0), iEl(0) {}
    //! \brief Empty destructor.
    virtual ~ElementInfo() {}

    //! \brief Returns the stabilization parameter.
    double getTau(double kappa, double Cinv, int p) const;

    Matrix eMs; //!< Stabilized matrix
    Vector eSs; //!< Stabilized vector
    Vector Cv;  //!< velocity + area

    double hk;  //!< element size
    size_t iEl; //!< element index
  };

  //! \brief The destructor deletes the advection field.
  virtual ~AdvectionDiffusion();

  //! \brief Defines the source function.
  void setSource(RealFunc* src) { source = src; }

  //! \brief Defines the Cinv stabilization parameter.
  void setCinv(double Cinv_) { Cinv = Cinv_; }
  //! \brief Returns the current Cinv value.
  double getCinv() const { return Cinv; }

  //! \brief Defines the stabilization type.
  void setStabilization(Stabilization s) { stab = s; }

  //! \brief Obtain the current stabilization type.
  Stabilization getStabilization() const { return stab; }

  //! \brief Defines the advection field.
  void setAdvectionField(VecFunc* U) { Uad = U; }

  //! \brief Defines the flux function.
  void setFlux(RealFunc* f) { flux = f; }

  //! \brief Defines the reaction field.
  void setReactionField(RealFunc* f) { reaction = f; }

  //! \brief Defines the global number of elements.
  void setElements(size_t els) { tauE.resize(els); }

  //! \brief Sets the basis order.
  void setOrder(int p) { order = p; }

  //! \brief Returns a previously calculated tau value for the given element.
  //! \brief param[in] e The element number
  //! \details Used with norm calculations
  double getElementTau(size_t e) const { return e > tauE.size() ? 0 : tauE(e); }

  //! \brief Defines which FE quantities are needed by the integrand.
  int getIntegrandType() const override;

  using IntegrandBase::getLocalIntegral;
  //! \brief Returns a local integral container for the given element.
  //! \param[in] nen Number of nodes on element
  //! \param[in] neumann Whether or not we are assembling Neumann BCs
  LocalIntegral* getLocalIntegral(size_t nen, size_t,
                                  bool neumann) const override;

  using IntegrandBase::finalizeElement;
  //! \brief Finalizes the element quantities after the numerical integration.
  //! \details This method is invoked once for each element, after the numerical
  //! integration loop over interior points is finished and before the resulting
  //! element quantities are assembled into their system level equivalents.
  bool finalizeElement(LocalIntegral& elmInt) override;

  using IntegrandBase::evalInt;
  //! \brief Evaluates the integrand at an interior point.
  //! \param elmInt The local integral object to receive the contributions
  //! \param[in] fe Finite element data of current integration point
  //! \param[in] X Cartesian coordinates of current integration point
  bool evalInt(LocalIntegral& elmInt, const FiniteElement& fe,
               const Vec3& X) const override;

  using IntegrandBase::evalBou;
  //! \brief Evaluates the integrand at a boundary point.
  //! \param elmInt The local integral object to receive the contributions
  //! \param[in] fe Finite element data of current integration point
  //! \param[in] X Cartesian coordinates of current integration point
  //! \param[in] normal Boundary normal vector at current integration point
  bool evalBou(LocalIntegral& elmInt, const FiniteElement& fe,
               const Vec3& X, const Vec3& normal) const override;

  using IntegrandBase::evalSol;
  //! \brief Evaluates the secondary solution at a result point.
  //! \param[out] s Array of solution field values at current point
  //! \param[in] fe Finite element data at current point
  //! \param[in] X Cartesian coordinates of current point
  //! \param[in] MNPC Nodal point correspondance for the basis function values
  bool evalSol(Vector& s, const FiniteElement& fe,
               const Vec3& X, const std::vector<int>& MNPC) const override;

  //! \brief Returns the number of primary/secondary solution field components.
  //! \param[in] fld which field set to consider (1=primary, 2=secondary)
  size_t getNoFields(int fld = 1) const override { return fld > 1 ? nsd : 1; }
  //! \brief Returns the name of the primary solution field.
  //! \param[in] prefix Name prefix
  std::string getField1Name(size_t, const char* prefix = 0) const override;
  //! \brief Returns the name of a secondary solution field component.
  //! \param[in] i Field component index
  //! \param[in] prefix Name prefix for all components
  std::string getField2Name(size_t i, const char* prefix = 0) const override;

  //! \brief Returns a pointer to an Integrand for solution norm evaluation.
  //! \note The Integrand object is allocated dynamically and has to be deleted
  //! manually when leaving the scope of the pointer variable receiving the
  //! returned pointer value.
  //! \param[in] asol Pointer to analytical solution (optional)
  NormBase* getNormIntegrand(AnaSol* asol = 0) const override;

  //! \brief Advances the integrand one time step forward.
  virtual void advanceStep() {}

  //! \brief Returns a reference to the fluid properties.
  AD::FluidProperties& getFluidProperties() { return props; }
  //! \brief Returns a const reference to the fluid properties.
  const AD::FluidProperties& getFluidProperties() const { return props; }

  //! \brief Defines the solution mode before the element assembly is started.
  //! \param[in] mode The solution mode to use
  void setMode(SIM::SolutionMode mode) override;

protected:
  VecFunc*  Uad;      //!< Pointer to advection field
  RealFunc* reaction; //!< Pointer to the reaction field
  RealFunc* source;   //!< Pointer to source field
  RealFunc* flux;     //!< Pointer to the flux field

  Vector tauE;  //!< Stored tau values - need for norm integration
  int    order; //!< Basis order

  AD::FluidProperties props; //!< Fluid properties

  Stabilization stab; //!< The type of stabilization used
  double        Cinv; //!< Stabilization parameter

  friend class AdvectionDiffusionNorm;
};


/*!
  \brief Class representing the integrand of Advection-Diffusion energy norms.
*/

class AdvectionDiffusionNorm : public NormBase
{
public:
  //! \brief The only constructor initializes its data members.
  //! \param[in] p The heat equation problem to evaluate norms for
  //! \param[in] a The analytical aolution (optional)
  AdvectionDiffusionNorm(AdvectionDiffusion& p, AnaSol* a = nullptr);
  //! \brief Empty destructor.
  virtual ~AdvectionDiffusionNorm() {}

  using NormBase::evalInt;
  //! \brief Evaluates the integrand at an interior point.
  //! \param elmInt The local integral object to receive the contributions
  //! \param[in] fe Finite element data of current integration point
  //! \param[in] X Cartesian coordinates of current integration point
  bool evalInt(LocalIntegral& elmInt, const FiniteElement& fe,
               const Vec3& X) const override;

  //! \brief Returns the number of norm groups or size of a specified group.
  //! \param[in] group The norm group to return the size of
  //! (if zero, return the number of groups)
  size_t getNoFields(int group = 0) const override;

  //! \brief Returns the name of a norm quantity.
  //! \param[in] i The norm group (one-based index)
  //! \param[in] j The norm number (one-based index)
  //! \param[in] prefix Common prefix for all norm names
  std::string getName(size_t i, size_t j, const char* prefix) const override;

  using NormBase::finalizeElement;
  //! \brief Finalizes the element norms after the numerical integration.
  //! \details This method is used to compute effectivity indices.
  //! \param elmInt The local integral object to receive the contributions
  bool finalizeElement(LocalIntegral&) override;

private:
  AnaSol* anasol; //!< Analytical solution
};

#endif
