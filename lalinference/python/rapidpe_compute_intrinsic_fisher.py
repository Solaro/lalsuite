#!/usr/bin/env python
#
# Copyright (C) 2012 Evan Ochsner
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Compute the likelihood of parameters of a GW signal given some data
that has been marginalized over extrinsic parameters. Creates a dag workflow
to perform this calculation.
"""

import os
import select
import sys
import stat
from functools import partial
from optparse import OptionParser, OptionGroup

import numpy as np

import lal
import lalsimulation as lalsim

import glue.lal
from glue.ligolw import utils, ligolw, lsctables, table, ilwd
from glue.ligolw.utils import process
from glue import pipeline

from lalinference.rapid_pe import lalsimutils as lsu
from lalinference.rapid_pe import effectiveFisher as eff
from lalinference.rapid_pe import dagutils

__author__ = "Evan Ochsner <evano@gravity.phys.uwm.edu>, Chris Pankow <pankow@gravity.phys.uwm.edu>, R. O'Shaughnessy"

optp = OptionParser()
# Options needed by this program only.
optp.add_option("-X", "--mass-points-xml", action="store_true", help="Output mass points as a sim_inspiral table.")
optp.add_option("-N", "--N-mass-pts", type=int, default=200, help="Number of intrinsic parameter (mass) values at which to compute marginalized likelihood. Default is 200.")
optp.add_option("--linear-spoked", action="store_true", help="Place mass pts along spokes linear in radial distance (if omitted placement will be random and uniform in volume")
optp.add_option("--match-value", type=float, default=0.97, help="Use this as the minimum match value. Default is 0.97")
optp.add_option("--save-ellipsoid-data", action="store_true", help="Save the parameters and eigenvalues of the ellipsoid.")

# Options transferred to ILE
optp.add_option("-C", "--channel-name", action="append", help="instrument=channel-name, e.g. H1=FAKE-STRAIN. Can be given multiple times for different instruments.")
optp.add_option("-x", "--coinc-xml", help="gstlal_inspiral XML file containing coincidence information.")
optp.add_option("-s", "--sim-xml", help="XML file containing injected event information.")

#
# Add the intrinsic parameters
#
intrinsic_params = OptionGroup(optp, "Intrinsic Parameters", "Intrinsic parameters (e.g component mass) to use.")
intrinsic_params.add_option("--mass1", type=float, help="Value of first component mass, in solar masses. Required if not providing coinc tables.")
intrinsic_params.add_option("--mass2", type=float, help="Value of second component mass, in solar masses. Required if not providing coinc tables.")
intrinsic_params.add_option("--event-time", type=float, help="Event coalescence GPS time.")
intrinsic_params.add_option("--approximant", help="Waveform family approximant to use. Required.")
optp.add_option_group(intrinsic_params)

opts, args = optp.parse_args()

#
# Get trigger information from coinc xml file
#

# Get end time from coinc inspiral table or command line
xmldoc = None
if opts.coinc_xml is not None:
    xmldoc = utils.load_filename(opts.coinc_xml)
    coinc_table = table.get_table(xmldoc, lsctables.CoincInspiralTable.tableName)
    assert len(coinc_table) == 1
    coinc_row = coinc_table[0]
    event_time = coinc_row.get_end()
    print "Coinc XML loaded, event time: %s" % str(coinc_row.get_end())
elif opts.event_time is not None:
    event_time = glue.lal.LIGOTimeGPS(opts.event_time)
    print "Event time from command line: %s" % str(event_time)
else:
    raise ValueError("Either --coinc-xml or --event-time must be provided to parse event time.")

# get masses from sngl_inspiral_table
if opts.mass1 is not None and opts.mass2 is not None:
    m1, m2 = opts.mass1, opts.mass2
elif xmldoc is not None:
    sngl_inspiral_table = table.get_table(xmldoc, lsctables.SnglInspiralTable.tableName)
    assert len(sngl_inspiral_table) == len(coinc_row.ifos.split(","))
    m1, m2 = None, None
    for sngl_row in sngl_inspiral_table:
        # NOTE: gstlal is exact match, but other pipelines may not be
        assert m1 is None or (sngl_row.mass1 == m1 and sngl_row.mass2 == m2)
        m1, m2 = sngl_row.mass1, sngl_row.mass2
    event_time = glue.lal.LIGOTimeGPS(opts.event_time)
else:
    raise ValueError("Need either --mass1 --mass2 or --coinc-xml to retrieve masses.")

m1_SI = m1 * lal.MSUN_SI
m2_SI = m2 * lal.MSUN_SI
print "Computing marginalized likelihood in a neighborhood about intrinsic parameters mass 1: %f, mass 2 %f" % (m1, m2)

#
# FIXME: Hardcoded values - eventually promote to command line arguments
#
template_min_freq = 40.
ip_min_freq = 40.
eff_fisher_psd = lal.LIGOIPsd
analyticPSD_Q = True
# The next 4 lines set the maximum size of the region to explore
min_mc_factor = 0.9
max_mc_factor = 1.1
min_eta = 0.05
max_eta = 0.25
# Control evaluation of the effective Fisher grid
NMcs = 11
NEtas = 11
match_cntr = opts.match_value # Fill an ellipsoid of this match
wide_match = 1-(1-opts.match_value)**(2/3.0)
fit_cntr = match_cntr # Do the effective Fisher fit with pts above this match
Nrandpts = opts.N_mass_pts # Requested number of pts to put inside the ellipsoid

#
# Setup signal and IP class
#
param_names = ['Mc', 'eta']
McSIG = lsu.mchirp(m1_SI, m2_SI)
etaSIG = lsu.symRatio(m1_SI, m2_SI)
PSIG = lsu.ChooseWaveformParams(
        m1=m1_SI, m2=m2_SI,
        fmin=template_min_freq,
        approx=lalsim.GetApproximantFromString(opts.approximant)
        )
# Find a deltaF sufficient for entire range to be explored
PTEST = PSIG.copy()

# Check the waveform generated in the corners for the 
# longest possible waveform
PTEST.m1, PTEST.m2 = lsu.m1m2(McSIG*min_mc_factor, min_eta)
deltaF_1 = lsu.findDeltaF(PTEST)
PTEST.m1, PTEST.m2 = lsu.m1m2(McSIG*min_mc_factor, max_eta)
deltaF_2 = lsu.findDeltaF(PTEST)
# set deltaF accordingly
PSIG.deltaF = min(deltaF_1, deltaF_2)

PTMPLT = PSIG.copy()

IP = lsu.Overlap(fLow = ip_min_freq,
        deltaF = PSIG.deltaF,
        psd = eff_fisher_psd,
        analyticPSD_Q = analyticPSD_Q
        )

hfSIG = lsu.norm_hoff(PSIG, IP)

# Find appropriate parameter ranges
min_mc = McSIG * min_mc_factor
max_mc = McSIG * max_mc_factor
param_ranges = eff.find_effective_Fisher_region(PSIG, IP, wide_match,
        param_names, [[min_mc, max_mc],[min_eta, max_eta]])
print "Computing amibiguity function in the range:"
for i, param in enumerate(param_names):
    if param=='Mc' or param=='m1' or param=='m2': # rescale output by MSUN
        print "\t", param, ":", np.array(param_ranges[i])/lal.MSUN_SI,\
                "(Msun)"
    else:
        print "\t", param, ":", param_ranges[i]

# setup uniform parameter grid for effective Fisher
pts_per_dim = [NMcs, NEtas]
Mcpts, etapts = eff.make_regular_1d_grids(param_ranges, pts_per_dim)
etapts = map(lsu.sanitize_eta, etapts)
McMESH, etaMESH = eff.multi_dim_meshgrid(Mcpts, etapts)
McFLAT, etaFLAT = eff.multi_dim_flatgrid(Mcpts, etapts)
dMcMESH = McMESH - McSIG
detaMESH = etaMESH - etaSIG
dMcFLAT = McFLAT - McSIG
detaFLAT = etaFLAT - etaSIG
grid = eff.multi_dim_grid(Mcpts, etapts)

# Change units on Mc
dMcFLAT_MSUN = dMcFLAT / lal.MSUN_SI
dMcMESH_MSUN = dMcMESH / lal.MSUN_SI
McMESH_MSUN = McMESH / lal.MSUN_SI
McSIG_MSUN = McSIG / lal.MSUN_SI

# Evaluate ambiguity function on the grid
rhos = np.array(eff.evaluate_ip_on_grid(hfSIG, PTMPLT, IP, param_names, grid))
rhogrid = rhos.reshape(NMcs, NEtas)

# Fit to determine effective Fisher matrix
cut = rhos > fit_cntr
fitgamma = eff.effectiveFisher(eff.residuals2d, rhos[cut], dMcFLAT_MSUN[cut],
        detaFLAT[cut])
# Find the eigenvalues/vectors of the effective Fisher matrix
gam = eff.array_to_symmetric_matrix(fitgamma)
evals, evecs, rot = eff.eigensystem(gam)

# Print information about the effective Fisher matrix
# and its eigensystem
print "Least squares fit finds g_Mc,Mc = ", fitgamma[0]
print "                        g_Mc,eta = ", fitgamma[1]
print "                        g_eta,eta = ", fitgamma[2]

print "\nFisher matrix:"
print "eigenvalues:", evals
print "eigenvectors:"
print evecs
print "rotation taking eigenvectors into Mc, eta basis:"
print rot

#
# Distribute points inside predicted ellipsoid of certain level of overlap
#
r1 = np.sqrt(2.*(1.-match_cntr)/np.real(evals[0])) # ellipse radii ...
r2 = np.sqrt(2.*(1.-match_cntr)/np.real(evals[1])) # ... along eigendirections
# Get pts. inside an ellipsoid oriented along eigenvectors
Nrad = 10 # N.B. hard-coded to assume 10 pts per radial spoke!
Nspokes = int(Nrandpts/Nrad)
# This angle ensures a spoke will be placed along the equal mass line.
# N.B. Nspokes must be even to ensure spokes along the equal mass line
# pointing both directions from the center!
ph0 = np.arctan(np.abs(r1) * (rot[0,1])/(np.abs(r2) * rot[0,0]) )
if opts.linear_spoked:
	print "Doing linear spoked placement"
	cart_grid, sph_grid = eff.linear_spoked_ellipsoid(Nrad,Nspokes,[ph0],r1,r2)
elif opts.uniform_spoked:
	print "Doing uniform spoked placement"
	cart_grid, sph_grid = eff.uniform_spoked_ellipsoid(Nrad,Nspokes,[ph0],r1,r2)
else: # do random, uniform placement
	print "Doing uniform random placement"
	cart_grid, sph_grid = eff.uniform_random_ellipsoid(Nrandpts, r1, r2)
# Rotate to get coordinates in parameter basis
cart_grid = np.array([ np.real( np.dot(rot, cart_grid[i]))
    for i in xrange(len(cart_grid)) ])
# Put in convenient units,
# change from parameter differential (i.e. dtheta)
# to absolute parameter value (i.e. theta = theta_true + dtheta)
rand_dMcs_MSUN, rand_detas = tuple(np.transpose(cart_grid)) # dMc, deta
rand_Mcs = rand_dMcs_MSUN * lal.MSUN_SI + McSIG # Mc (kg)
rand_etas = rand_detas + etaSIG # eta

# Prune points with unphysical values of eta from cart_grid
rand_etas = np.array(map(partial(lsu.sanitize_eta, exception=np.NAN), rand_etas))
cart_grid = np.transpose((rand_Mcs,rand_etas))
phys_cut = ~np.isnan(cart_grid).any(1) # cut to remove unphysical pts
cart_grid = cart_grid[phys_cut]
print "Requested", Nrandpts, "points inside the ellipsoid of",\
        match_cntr, "match."
print "Kept", len(cart_grid), "points with physically allowed parameters."

# Output Cartesian and spherical coordinates of intrinsic grid
indices = np.arange(len(cart_grid))
Mcs_MSUN, etas = np.transpose(cart_grid)
Mcs_MSUN = Mcs_MSUN / lal.MSUN_SI
radii, angles = np.transpose(sph_grid[phys_cut])
outgrid = np.transpose((indices,Mcs_MSUN,etas,radii,angles))
# If clusters get upgraded, add this header to output:
# header='index Mc eta radius angle'
np.savetxt('intrinsic_grid.dat', outgrid)

# Output information about the intrinsic ellipsoid
area = np.pi * r1 * r2
frac_area = area * len(cart_grid) / Nrandpts
gam = np.real(gam)
test = np.concatenate((np.array([[McSIG/lal.MSUN_SI, etaSIG]]), gam,np.array([[r1,r2]]),np.array([[area, frac_area]]), np.array([[match_cntr,match_cntr]]) ))
# If clusters get upgraded, add this header to output:
#   header='grid center; 2x2 effective Fisher matrix, 4rd row: ellipsoid axes, 5th row: total ellipse area, estimated physical area'
if opts.save_ellipsoid_data:
    np.savetxt('ellipsoid.dat', test)

# Convert to m1, m2
m1m2_grid = np.array([lsu.m1m2(cart_grid[i][0], cart_grid[i][1])
        for i in xrange(len(cart_grid))])
m1m2_grid /= lal.MSUN_SI

if opts.mass_points_xml:
    xmldoc = ligolw.Document()
    xmldoc.appendChild(ligolw.LIGO_LW())
    procrow = process.append_process(xmldoc, program=sys.argv[0])
    procid = procrow.process_id
    process.append_process_params(xmldoc, procrow, process.process_params_from_dict(opts.__dict__))
    
    sim_insp_tbl = lsctables.New(lsctables.SimInspiralTable, ["simulation_id", "process_id", "numrel_data", "mass1", "mass2"])
    for itr, (m1, m2) in enumerate(m1m2_grid):
        sim_insp = sim_insp_tbl.RowType()
        sim_insp.numrel_data = "MASS_SET_%d" % itr
        sim_insp.simulation_id = ilwd.ilwdchar("sim_inspiral:sim_inspiral_id:%d" % itr)
        sim_insp.process_id = procid
        sim_insp.mass1, sim_insp.mass2 = m1, m2
        sim_insp_tbl.append(sim_insp)
    xmldoc.childNodes[0].appendChild(sim_insp_tbl)
    ifos = "".join([o.split("=")[0][0] for o in opts.channel_name])
    start = int(event_time)
    fname = "%s-MASS_POINTS-%d-1.xml.gz" % (ifos, start)
    utils.write_filename(xmldoc, fname, gz=True)