/*
 * Copyright (C) 2008 J. Creighton, S. Fairhurst, B. Krishnan, L. Santamaria, D. Keppel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with with program; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

#include <math.h>

#include <gsl/gsl_const.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_odeiv.h>

#include <lal/LALSimInspiral.h>
#define LAL_USE_COMPLEX_SHORT_MACROS
#include <lal/LALComplex.h>
#include <lal/LALConstants.h>
#include <lal/LALStdlib.h>
#include <lal/TimeSeries.h>
#include <lal/Units.h>

#include "LALSimInspiraldEnergyFlux.c"
#include "LALSimInspiralPNCoefficients.c"
#include "check_series_macros.h"

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

NRCSID(LALSIMINSPIRALTAYLORT4C, "$Id$");

/**
 * This structure contains the intrinsic parameters and post-newtonian 
 * co-efficients for the energy and angular acceleration expansions. 
 * These are computed by XLALSimInspiralTaylorT4Setup routine.
 */

typedef struct
tagexpnCoeffsTaylorT4 {
   
   /*Angular velocity coefficient*/
   REAL8 av;

   /* Taylor expansion coefficents in domega/dt*/
   REAL8 aatN,aat2,aat3,aat4,aat5,aat6,aat6l,aat7;

   /* symmetric mass ratio, total mass, component masses*/
   REAL8 nu,m,m1,m2,mu;
}expnCoeffsTaylorT4;

typedef REAL8 (SimInspiralEnergy4)(
   REAL8 v, /**< post-Newtonian parameter */
   expnCoeffsdEnergyFlux *ak
);

typedef REAL8 (SimInspiralAngularAcceleration4)(
   REAL8 v, /**< post-Newtonian parameter */
   expnCoeffsTaylorT4 *ak
);

/**
 * This strucuture contains pointers to the functions for calculating
 * the post-newtonian terms at the desired order. They can be set by
 * XLALSimInspiralTaylorT4Setup by passing an appropriate PN order. 
 */

typedef struct
tagexpnFuncTaylorT4
{
   SimInspiralEnergy4 *energy4;
   SimInspiralAngularAcceleration4 *angacc4;
} expnFuncTaylorT4;


/**
 * Computes the rate of increase of the orbital frequency for a post-Newtonian
 * inspiral.
 *
 * Implements Equation 3.6 of: Alessandra Buonanno, Bala R Iyer, Evan
 * Ochsner, Yi Pan, and B S Sathyaprakash, "Comparison of post-Newtonian
 * templates for compact binary inspiral signals in gravitational-wave
 * detectors", Phys. Rev. D 80, 084043 (2009), arXiv:0907.0700v1
 */

static REAL8 
XLALSimInspiralAngularAcceleration4_0PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v9;
    
	v9 = pow(v, 9.0);

	ans = ak->aatN * v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_2PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v9;
    
	v2 = v*v;
	v9 = pow(v, 9.0);

	ans = ak->aatN * (1.
		+ ak->aat2*v2);

	ans *= v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_3PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v3,v9;
    
	v2 = v*v;
	v3 = v2*v;
	v9 = v3*v3*v3;

	ans = ak->aatN * (1.
		+ ak->aat2*v2
		+ ak->aat3*v3);

	ans *= v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_4PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v3,v4,v9;
    
	v2 = v*v;
	v3 = v2*v;
	v4 = v3*v;
	v9 = v4*v4*v;

	ans = ak->aatN * (1.
		+ ak->aat2*v2
		+ ak->aat3*v3
		+ ak->aat4*v4);

	ans *= v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_5PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v3,v4,v5,v9;
    
	v2 = v*v;
	v3 = v2*v;
	v4 = v3*v;
	v5 = v4*v;
	v9 = v5*v4;

	ans = ak->aatN * (1.
		+ ak->aat2*v2
		+ ak->aat3*v3
		+ ak->aat4*v4
		+ ak->aat5*v5);

	ans *= v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_6PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v3,v4,v5,v6,v9;
    
	v2 = v*v;
	v3 = v2*v;
	v4 = v3*v;
	v5 = v4*v;
	v6 = v5*v;
	v9 = v6*v3;

	ans = ak->aatN * (1.
		+ ak->aat2*v2
		+ ak->aat3*v3
		+ ak->aat4*v4
		+ ak->aat5*v5
		+ (ak->aat6 + ak->aat6l*log(16.0*v2))*v6);

	ans *= v9;

	return ans;
}

static REAL8 
XLALSimInspiralAngularAcceleration4_7PN(
	REAL8 v,		/**< post-Newtonian parameter */
	expnCoeffsTaylorT4 *ak	/**< PN co-efficients and intrinsic parameters */
	)
{
	REAL8 ans;
	REAL8 v2,v3,v4,v5,v6,v7,v9;
    
	v2 = v*v;
	v3 = v2*v;
	v4 = v3*v;
	v5 = v4*v;
	v6 = v5*v;
	v7 = v6*v;
	v9 = v7*v2;

	ans = ak->aatN * (1.
		+ ak->aat2*v2
		+ ak->aat3*v3
		+ ak->aat4*v4
		+ ak->aat5*v5
		+ (ak->aat6 + ak->aat6l*log(16.0*v2))*v6
		+ ak->aat7*v7);

	ans *= v9;

	return ans;
}

typedef struct
{
    REAL8 (*func)(REAL8 v, expnCoeffsTaylorT4 *ak);
    expnCoeffsTaylorT4 ak;
}XLALSimInspiralTaylorT4PNEvolveOrbitParams;

/** 
 * This function is used in the call to the GSL integrator.
 */
static int 
XLALSimInspiralTaylorT4PNEvolveOrbitIntegrand(double UNUSED t, const double y[], double ydot[], void *params)
{
	XLALSimInspiralTaylorT4PNEvolveOrbitParams* p = (XLALSimInspiralTaylorT4PNEvolveOrbitParams*)params;
	ydot[0] = p->func(y[0],&p->ak);
	ydot[1] = y[0]*y[0]*y[0]*p->ak.av;
	t = 0.0;
	return GSL_SUCCESS;
}


/**
 * Set up the expnCoeffsTaylorT4 and expnFuncTaylorT4 structures for
 * generating a TaylorT4 waveform and select the post-newtonian
 * functions corresponding to the desired order. 
 *
 * Inputs given in SI units.
 */
static int 
XLALSimInspiralTaylorT4Setup(
    expnCoeffsTaylorT4 *ak,		/**< coefficients for TaylorT4 evolution [modified] */
    expnFuncTaylorT4 *f,		/**< functions for TaylorT4 evolution [modified] */
    expnCoeffsdEnergyFlux *akdEF,	/**< coefficients for Energy calculation [modified] */
    REAL8 m1,				/**< mass of companion 1 */
    REAL8 m2,				/**< mass of companion 2 */
    int O				/**< twice post-Newtonian order */
)
{
    ak->m1 = m1;
    ak->m2 = m2;
    ak->m = ak->m1 + ak->m2;
    ak->mu = m1 * m2 / ak->m;
    ak->nu = ak->mu/ak->m;

    /* Angular velocity co-efficient */
    ak->av = pow(LAL_C_SI, 3.0)/(LAL_G_SI*ak->m);

    /* PN co-efficients for energy */
    akdEF->ETaN = XLALSimInspiralPNEnergy_0PNCoeff(ak->nu);
    akdEF->ETa1 = XLALSimInspiralPNEnergy_2PNCoeff(ak->nu);
    akdEF->ETa2 = XLALSimInspiralPNEnergy_4PNCoeff(ak->nu);
    akdEF->ETa3 = XLALSimInspiralPNEnergy_6PNCoeff(ak->nu);
    
    /* PN co-efficients for angular acceleration */
    ak->aatN = XLALSimInspiralTaylorT4AngularAccel_0PNCoeff(LAL_G_SI*ak->m/pow(LAL_C_SI, 3.0), ak->nu);
    ak->aat2 = XLALSimInspiralTaylorT4AngularAccel_2PNCoeff(ak->nu);
    ak->aat3 = XLALSimInspiralTaylorT4AngularAccel_3PNCoeff(ak->nu);
    ak->aat4 = XLALSimInspiralTaylorT4AngularAccel_4PNCoeff(ak->nu);
    ak->aat5 = XLALSimInspiralTaylorT4AngularAccel_5PNCoeff(ak->nu);
    ak->aat6 = XLALSimInspiralTaylorT4AngularAccel_6PNCoeff(ak->nu);
    ak->aat7 = XLALSimInspiralTaylorT4AngularAccel_7PNCoeff(ak->nu);
    ak->aat6l = XLALSimInspiralTaylorT4AngularAccel_6PNLogCoeff(ak->nu);

    switch (O)
    {
        case 0:
            f->energy4 = &XLALSimInspiralEt0;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_0PN;
            break;
        case 1:
            XLALPrintError("XLAL Error - %s: PN approximant not supported for PN order %d\n", __func__,O);
            XLAL_ERROR(XLAL_EINVAL);
            break;
        case 2:
            f->energy4 = &XLALSimInspiralEt2;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_2PN;
            break;
        case 3:
            f->energy4 = &XLALSimInspiralEt2;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_3PN;
            break;
        case 4:
            f->energy4 = &XLALSimInspiralEt4;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_4PN;
            break;
        case 5:
            f->energy4 = &XLALSimInspiralEt4;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_5PN;
            break;
        case 6:
            f->energy4 = &XLALSimInspiralEt6;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_6PN;
            break;
        case 7:
        case -1:
            f->energy4 = &XLALSimInspiralEt6;
            f->angacc4 = &XLALSimInspiralAngularAcceleration4_7PN;
            break;
        case 8:
            XLALPrintError("XLAL Error - %s: PN approximant not supported for PN order %d\n", __func__,O);
            XLAL_ERROR(XLAL_EINVAL);
            break;
        default:
            XLALPrintError("XLAL Error - %s: Unknown PN order in switch\n", __func__);
            XLAL_ERROR(XLAL_EINVAL);
    }
  
  return 0;
}


/**
 * Evolves a post-Newtonian orbit using the Taylor T4 method.
 *
 * See:
 * Michael Boyle, Duncan A. Brown, Lawrence E. Kidder, Abdul H. Mroue,
 * Harald P. Pfeiﬀer, Mark A. Scheel, Gregory B. Cook, and Saul A. Teukolsky
 * "High-accuracy comparison of numerical relativity simulations with
 * post-Newtonian expansions"
 * <a href="http://arxiv.org/abs/0710.0158v2">arXiv:0710.0158v2</a>.
 */
int XLALSimInspiralTaylorT4PNEvolveOrbit(
		REAL8TimeSeries **v,   /**< post-Newtonian parameter [returned] */
		REAL8TimeSeries **phi, /**< orbital phase [returned] */
		REAL8 phic,            /**< coalescence phase */
		REAL8 deltaT,          /**< sampling interval */
		REAL8 m1,              /**< mass of companion 1 */
		REAL8 m2,              /**< mass of companion 2 */
		REAL8 f_min,           /**< start frequency */
		int O                  /**< twice post-Newtonian order */
		)
{
	const UINT4 blocklen = 1024;
	const REAL8 visco = 1./sqrt(6.);
	XLALSimInspiralTaylorT4PNEvolveOrbitParams params;
	expnFuncTaylorT4 expnfunc;
	expnCoeffsTaylorT4 ak;
	expnCoeffsdEnergyFlux akdEF;

	if(XLALSimInspiralTaylorT4Setup(&ak,&expnfunc,&akdEF,m1,m2,O))
		XLAL_ERROR(XLAL_EFUNC);

	params.func=expnfunc.angacc4;
	params.ak=ak;

	REAL8 E;
	UINT4 j;
	LIGOTimeGPS tc = LIGOTIMEGPSZERO;
	double y[2];
	double yerr[2];
	const gsl_odeiv_step_type *T = gsl_odeiv_step_rk4;
	gsl_odeiv_step *s;
	gsl_odeiv_system sys;

	/* setup ode system */
	sys.function = XLALSimInspiralTaylorT4PNEvolveOrbitIntegrand;
	sys.jacobian = NULL;
	sys.dimension = 2;
	sys.params = &params;

	/* allocate memory */
	*v = XLALCreateREAL8TimeSeries( "ORBITAL_VELOCITY_PARAMETER", &tc, 0., deltaT, &lalDimensionlessUnit, blocklen );
	*phi = XLALCreateREAL8TimeSeries( "ORBITAL_PHASE", &tc, 0., deltaT, &lalDimensionlessUnit, blocklen );
	if ( !v || !phi )
		XLAL_ERROR(XLAL_EFUNC);

	y[0] = (*v)->data->data[0] = cbrt(LAL_PI*LAL_G_SI*ak.m*f_min)/LAL_C_SI;
	y[1] = (*phi)->data->data[0] = 0.;
	E = expnfunc.energy4(y[0],&akdEF);
	if (XLALIsREAL8FailNaN(E))
		XLAL_ERROR(XLAL_EFUNC);
	j = 0;

	s = gsl_odeiv_step_alloc(T, 2);
	while (1) {
		++j;
		gsl_odeiv_step_apply(s, j*deltaT, deltaT, y, yerr, NULL, NULL, &sys);
		/* ISCO termination condition for quadrupole, 1pN, 2.5pN */
		if ( y[0] > visco ) {
			XLALPrintInfo("XLAL Info - %s: PN inspiral terminated at ISCO\n", __func__);
			break;
		}
		if ( j >= (*v)->data->length ) {
			if ( ! XLALResizeREAL8TimeSeries(*v, 0, (*v)->data->length + blocklen) )
				XLAL_ERROR(XLAL_EFUNC);
			if ( ! XLALResizeREAL8TimeSeries(*phi, 0, (*phi)->data->length + blocklen) )
				XLAL_ERROR(XLAL_EFUNC);
		}
		(*v)->data->data[j] = y[0];
		(*phi)->data->data[j] = y[1];
	}
	gsl_odeiv_step_free(s);

	/* make the correct length */
	if ( ! XLALResizeREAL8TimeSeries(*v, 0, j) )
		XLAL_ERROR(XLAL_EFUNC);
	if ( ! XLALResizeREAL8TimeSeries(*phi, 0, j) )
		XLAL_ERROR(XLAL_EFUNC);

	/* adjust to correct time */
	XLALGPSAdd(&(*v)->epoch, -1.0*j*deltaT);
	XLALGPSAdd(&(*phi)->epoch, -1.0*j*deltaT);

	/* phi here is the orbital phase = 1/2 * GW phase.
	 * End GW phase specified on command line.
	 * Adjust phase so phi = phi_end/2 at the end */

	phic /= 2.;
	phic -= (*phi)->data->data[j-1];
	for (j = 0; j < (*phi)->data->length; ++j)
		(*phi)->data->data[j] += phic;

	return (int)(*v)->data->length;
}


/**
 * Driver routine to compute the post-Newtonian inspiral waveform.
 *
 * This routine allows the user to specify different pN orders
 * for phasing calcuation vs. amplitude calculations.
 */
int XLALSimInspiralTaylorT4PNGenerator(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 v0,                 /**< tail-term gauge choice thing (default = 1) */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int amplitudeO,           /**< twice post-Newtonian amplitude order */
	       	int phaseO                /**< twice post-Newtonian phase order */
		)
{
	REAL8TimeSeries *v;
	REAL8TimeSeries *phi;
	int status;
	int n;
	n = XLALSimInspiralTaylorT4PNEvolveOrbit(&v, &phi, phic, deltaT, m1, m2, f_min, phaseO);
	if ( n < 0 )
		XLAL_ERROR(XLAL_EFUNC);
	status = XLALSimInspiralPNPolarizationWaveforms(hplus, hcross, v, phi, v0, m1, m2, r, i, amplitudeO);
	XLALDestroyREAL8TimeSeries(phi);
	XLALDestroyREAL8TimeSeries(v);
	if ( status < 0 )
		XLAL_ERROR(XLAL_EFUNC);
	return n;
}


/**
 * Driver routine to compute the post-Newtonian inspiral waveform.
 *
 * This routine uses the same pN order for phasing and amplitude
 * (unless the order is -1 in which case the highest available
 * order is used for both of these -- which might not be the same).
 *
 * Constant log term in amplitude set to 1.  This is a gauge choice.
 */
int XLALSimInspiralTaylorT4PN(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int O                     /**< twice post-Newtonian order */
		)
{
	/* set v0 to default value 1 */
	return XLALSimInspiralTaylorT4PNGenerator(hplus, hcross, phic, 1.0, deltaT, m1, m2, f_min, r, i, O, O);
}


/**
 * Driver routine to compute the restricted post-Newtonian inspiral waveform.
 *
 * This routine computes the phasing to the specified order, but
 * only computes the amplitudes to the Newtonian (quadrupole) order.
 *
 * Constant log term in amplitude set to 1.  This is a gauge choice.
 */
int XLALSimInspiralTaylorT4PNRestricted(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int O                     /**< twice post-Newtonian phase order */
		)
{
	/* use Newtonian order for amplitude */
	/* set v0 to default value 1 */
	return XLALSimInspiralTaylorT4PNGenerator(hplus, hcross, phic, 1.0, deltaT, m1, m2, f_min, r, i, 0, O);
}


#if 0
#include <lal/PrintFTSeries.h>
#include <lal/PrintFTSeries.h>
extern int lalDebugLevel;
int main(void)
{
	LIGOTimeGPS tc = { 888888888, 222222222 };
	REAL8 phic = 1.0;
	REAL8 deltaT = 1.0/16384.0;
	REAL8 m1 = 1.4*LAL_MSUN_SI;
	REAL8 m2 = 1.4*LAL_MSUN_SI;
	REAL8 r = 1e6*LAL_PC_SI;
	REAL8 i = 0.5*LAL_PI;
	REAL8 f_min = 100.0;
	int O = -1;
	REAL8TimeSeries *hplus;
	REAL8TimeSeries *hcross;
	lalDebugLevel = 7;
	XLALSimInspiralTaylorT4PN(&hplus, &hcross, &tc, phic, deltaT, m1, m2, f_min, r, i, O);
	LALDPrintTimeSeries(hplus, "hp.dat");
	LALDPrintTimeSeries(hcross, "hc.dat");
	XLALDestroyREAL8TimeSeries(hplus);
	XLALDestroyREAL8TimeSeries(hcross);
	LALCheckMemoryLeaks();
	return 0;
}
#endif
