/*----------------------------------------------------------------------- 
 * 
 * File Name: LIGOMetadataUtils.h
 *
 * Author: Brown, D. A.
 * 
 * Revision: $Id$
 * 
 *-----------------------------------------------------------------------
 */

#if 0
<lalVerbatim file="LIGOMetadataUtilsHV">
Author: Brown, D. A.
$Id$
</lalVerbatim> 
<lalLaTeX>
\section{Header \texttt{LIGOMetadataUtils.h}}
\label{s:LIGOMetadataItils.h}

Provides functions for manipulating the LAL structures that correspond
to the LIGO metadata database tables defined in \texttt{LIGOMetadataTables.h}.

\subsection*{Synopsis}
\begin{verbatim}
#include <lal/LIGOMetadataUtils.h>
\end{verbatim}

\noindent This header provides prototypes for routines that perform processing
on the LAL structures that correspond to the LIGO metadata database tables
defined in \texttt{LIGOMetadataTables.h}, such as sorting and eliminating 
duplictaes. The functions specific to a particular metadata table (e.g. 
\texttt{sngl\_inspiral}, \texttt{sngl\_burst}, etc.) are all prototyped in
this header.

\subsection*{Types}

\noindent None.

</lalLaTeX>
#endif

#ifndef _LIGOMETADATAUTILS_H
#define _LIGOMETADATAUTILS_H

#ifdef  __cplusplus
extern "C" {
#pragma }
#endif

NRCSID( LIGOMETADATAUTILSH, "$Id$" );

#include <lal/LIGOMetadataTables.h>


/*
 *
 * inspiral specific structures 
 *
 */


typedef struct
tagSnglInspiralAccuracy
{
  INT4  match;
  REAL4 epsilon;
  REAL4 kappa;
  INT8  dt;
  REAL4 dm;
}
SnglInspiralAccuracy;


/*
 *
 * burst specific structures 
 *
 */


typedef struct
tagSnglBurstAccuracy
{
  INT4  match;
  REAL4 dRhoPlus;
  REAL4 dRhoMinus;
  INT8  dtime;
  REAL4 dm;

}
SnglBurstAccuracy;


/*
 *
 * inspiral specific functions
 *
 */


void
LALSortSnglInspiral (
    LALStatus          *status,
    SnglInspiralTable **eventHead,
    int(*comparfunc)    (const void *, const void *)
    );

int
LALCompareSnglInspiralByMass (
    const void *a,
    const void *b
    );

int
LALCompareSnglInspiralByTime (
    const void *a,
    const void *b
    );

void
LALCompareSnglInspiral (
    LALStatus                *status,
    SnglInspiralTable        *aPtr,
    SnglInspiralTable        *bPtr,
    SnglInspiralAccuracy     *params
    );


/*
 *
 * burst specific functions
 *
 */


void
LALSortSnglBurst(
    LALStatus          *status,
    SnglBurstTable **eventHead,
    int(*comparfunc)    (const void *, const void *)
    );

int
LALCompareSnglBurstByTime(
    const void *a,
    const void *b
    );

int
LALCompareSnglBurstByTimeAndFreq(
    const void *a,
    const void *b
    );

void
LALCompareSnglBurst(
    LALStatus             *status,
    SnglBurstTable        *aPtr,
    SnglBurstTable        *bPtr,
    SnglBurstAccuracy     *params
    );

void
LALClusterSnglBurstTable (
	      LALStatus        *status,
              SnglBurstTable   *burstEvent,
	      INT4             *nevents
	      );

#if 0
<lalLaTeX>
\vfill{\footnotesize\input{LIGOMetadataUtilsHV}}

\newpage
\input{SnglInspiralUtilsC}
</lalLaTeX>
#endif

#ifdef  __cplusplus
#pragma {
}
#endif

#endif /* _LIGOMETADATAUTILS_H */

