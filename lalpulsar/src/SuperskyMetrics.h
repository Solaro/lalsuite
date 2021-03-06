//
// Copyright (C) 2014, 2015 Karl Wette
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with with program; see the file COPYING. If not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
// MA 02111-1307 USA
//

#ifndef _SUPERSKYMETRICS_H
#define _SUPERSKYMETRICS_H

#include <gsl/gsl_matrix.h>
#include <lal/LALStdlib.h>
#include <lal/UniversalDopplerMetric.h>
#include <lal/LatticeTiling.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// \defgroup SuperskyMetrics_h Header SuperskyMetrics.h
/// \ingroup lalpulsar_metric
/// \author Karl Wette
/// \brief Compute the supersky metrics and coordinate transforms of \cite WettePrix2013a .
///
/// @{
///

///
/// Coordinate systems associated with the supersky metrics.
///
typedef enum {
  SSC_PHYSICAL,					///< Physical: right ascension, declination, frequency and spindowns
  SSC_SUPER_SKY,				///< (Full) supersky: 3-dimensional sky (equatorial coordinates), spindowns and frequency
  SSC_REDUCED_SUPER_SKY,			///< Reduced supersky: 2-dimensional sky (reduced supersky coordinates), reduced spindowns and frequency
  SSC_MAX
} SuperskyCoordinates;

///
/// Compute the reduced supersky metric (2-dimensional sky), and/or the (full) supersky metric
/// (3-dimensional sky), for individual segments and/or appropriately averaged over a segment list.
///
#ifdef SWIG // SWIG interface directives
SWIGLAL( INOUT_STRUCTS( gsl_matrix **, out_rssky_metric, out_rssky_transf, out_ssky_metric, out_rssky_metric_seg, out_rssky_transf_seg, out_ssky_metric_seg ) );
#endif
int XLALComputeSuperskyMetrics(
  gsl_matrix **out_rssky_metric,		///< [out] Output reduced supersky metric, appropriately averaged over segments
  gsl_matrix **out_rssky_transf,		///< [out] Output reduced supersky metric transform data
  gsl_matrix **out_ssky_metric,			///< [out] Output (full) supersky metric, appropriately averaged over segments
  gsl_matrix **out_rssky_metric_seg,		///< [out] Output reduced supersky metrics for each segment, concatenated by column: [rssky_metric_1, rssky_metric_2, ...]
  gsl_matrix **out_rssky_transf_seg,		///< [out] Output reduced supersky metric transform data for each segment, concatenated by column: [rssky_transf_1, rssky_transf_2, ...]
  gsl_matrix **out_ssky_metric_seg,		///< [out] Output (full) supersky metrics for each segment, concatenated by column: [ssky_metric_1, ssky_metric_2, ...]
  const size_t spindowns,			///< [in] Number of frequency+spindown coordinates
  const LIGOTimeGPS *ref_time,			///< [in] Reference time for the metrics
  const LALSegList *segments,			///< [in] List of segments to average metrics over
  const double fiducial_freq,			///< [in] Fiducial frequency for sky-position coordinates
  const MultiLALDetector *detectors,		///< [in] List of detectors to average metrics over
  const MultiNoiseFloor *detector_weights,	///< [in] Weights used to combine single-detector metrics (default: unit weights)
  const DetectorMotionType detector_motion,	///< [in] Which detector motion to use
  const EphemerisData *ephemerides		///< [in] Earth/Sun ephemerides
  );

///
/// Convert a series of points between supersky coordinate systems.
///
#ifdef SWIG // SWIG interface directives
SWIGLAL( INOUT_STRUCTS( gsl_matrix **, out_points ) );
#endif
int XLALConvertSuperskyCoordinates(
  const SuperskyCoordinates out,		///< [in] Coordinate system of the output points
  gsl_matrix **out_points,			///< [in/out] Matrix whose columns are the output points
  const SuperskyCoordinates in,			///< [in] Coordinate system of the input points
  const gsl_matrix *in_points,			///< [in] Matrix whose columns are the input points
  const gsl_matrix *rssky_transf		///< [in] Reduced supersky coordinate transform data
  );

///
/// Convert a single point from physical to supersky coordinates.
///
int XLALConvertPhysicalToSupersky(
  const SuperskyCoordinates out,		///< [in] Coordinate system of the output point
  gsl_vector *out_point,			///< [in/out] Output point in supersky coordinates
  const PulsarDopplerParams *in_phys,		///< [in] Input point in physical coordinates
  const gsl_matrix *rssky_transf		///< [in] Reduced supersky coordinate transform data
  );

///
/// Convert a single point from supersky to physical coordinates.
///
int XLALConvertSuperskyToPhysical(
  PulsarDopplerParams *out_phys,		///< [in/out] Output point in physical coordinates
  const SuperskyCoordinates in,			///< [in] Coordinate system of the input point
  const gsl_vector *in_point,			///< [in] Input point in supersky coordinates
  const gsl_matrix *rssky_transf		///< [in] Reduced supersky coordinate transform data
  );

///
/// Set all-sky parameter-space bounds on a lattice tiling using the reduced supersky metric.
///
int XLALSetSuperskyLatticeTilingAllSkyBounds(
  LatticeTiling *tiling				///< [in] Lattice tiling
  );

///
/// Set a sky point parameter-space bound on a lattice tiling using the reduced supersky metric.
///
int XLALSetSuperskyLatticeTilingSkyPointBounds(
  LatticeTiling *tiling,			///< [in] Lattice tiling
  const gsl_matrix *rssky_transf,		///< [in] Reduced supersky coordinate transform data
  const double alpha,				///< [in] Sky point right ascension
  const double delta				///< [in] Sky point declination
  );

///
/// Set parameter-space bounds on the physical frequency/spindowns \f$f^{(s)}\f$ for a lattice
/// tiling using the reduced supersky metric.
///
int XLALSetSuperskyLatticeTilingPhysicalSpinBound(
  LatticeTiling *tiling,			///< [in] Lattice tiling
  const gsl_matrix *rssky_transf,		///< [in] Reduced supersky coordinate transform data
  const size_t s,				///< [in] Spindown order; 0=frequency, 1=first spindown, etc.
  const double bound1,				///< [in] First bound on frequency/spindown
  const double bound2				///< [in] Second bound on frequency/spindown
  );

///
/// Set parameter-space bounds on the reduced supersky frequency/spindown coordinates \f$\nu^{(s)}\f$
/// for a lattice tiling using the reduced supersky metric. These coordinates are related to the
/// physical frequency/spindowns by \f$\nu^{(s)} = f^{(s)} + \vec\Delta^s \cdot \vec n\f$.
///
int XLALSetSuperskyLatticeTilingCoordinateSpinBound(
  LatticeTiling *tiling,			///< [in] Lattice tiling.
  const gsl_matrix *rssky_transf,		///< [in] Reduced supersky coordinate transform data
  const size_t s,				///< [in] Spindown order; 0=frequency, 1=first spindown, etc.
  const double bound1,				///< [in] First bound on frequency/spindown
  const double bound2				///< [in] Second bound on frequency/spindown
  );

/// @}

#ifdef __cplusplus
}
#endif

#endif // _SUPERSKYMETRICS_H
