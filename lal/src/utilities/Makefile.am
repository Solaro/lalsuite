include $(top_srcdir)/gnuscripts/lalsuite_header_links.am

pkginclude_HEADERS = \
	CoarseGrainFrequencySeries.h \
	Dirichlet.h \
	Factorial.h \
	FindRoot.h \
	Integrate.h \
	Interpolate.h \
	LALAdaptiveRungeKutta4.h \
	LALRunningMedian.h \
	MatrixUtils.h \
	ODE.h \
	Random.h \
	RngMedBias.h \
	Sort.h \
	SphericalHarmonics.h \
	$(END_OF_LIST)

noinst_LTLIBRARIES = libutilities.la

libutilities_la_SOURCES = \
	CoarseGrainFrequencySeries.c \
	DetInverse.c \
	DetInverseInternal.c \
	Dirichlet.c \
	Eigen.c \
	EigenInternal.c \
	FindRoot.c \
	HeapSort.c \
	Integrate.c \
	Interpolate.c \
	LALAdaptiveRungeKutta4.c \
	LALRunningMedian.c \
	MatrixOps.c \
	MersenneRandom.c \
	ODE.c \
	Random.c \
	RngMedBias.c \
	SphericalHarmonics.c \
	$(END_OF_LIST)

noinst_HEADERS = \
	MatrixOps_source.c \
	$(END_OF_LIST)

EXTRA_DIST = \
	utilitiesCoarseGrain.png \
	$(END_OF_LIST)
