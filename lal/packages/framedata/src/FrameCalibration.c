/**** <lalVerbatim file="FrameCalibrationCV">
 * Author: Brown, D. A.
 * $Id$
 **** </lalVerbatim> */

/**** <lalLaTeX>
 *
 * \subsection{Module \texttt{FrameCalibration.c}}
 * \label{ss:FrameCalibration.c}
 *
 * This module contains code used to extract calibration information contained
 * in frame files, and to construct a response (or transfer) function.  This
 * is supposed to provide a high-level interface for search authors to obtain a
 * response function in the desired form.
 *
 * \subsection*{Prototypes}
 * \input{FrameCalibrationCP}
 *
 * The routine \texttt{LALFrameExtractResponse()} extracts the necessary
 * calibration information from the frames. The frames used to construct
 * the calibration are located using the specified catalog. The function
 * constructs a response function (as a frequency series) from this
 * information.  If the fourth argument is non-\texttt{NULL} then this
 * string specifies the detector (H1, H2, L1, etc.) for which the calibration
 * is required.  If the fifth argument is non-zero then the 
 * calibration will be averaged over the \texttt{duration}.  If the 
 * \texttt{duration} is set to zero, then the first calibration at or after
 * the start time is used.  Certain fields of the output should be set before 
 * this routine is called.  In particular: 
 * \begin{enumerate}
 * \item The epoch field of the frequency series should be set to the correct
 * epoch so that the routine can generate a response function tailored to that
 * time (accounting for calibration drifts). 
 *
 * \item The units of the response function should be set to be either
 * strain-per-count (for a response function) or count-per-strain (for a
 * transfer function); the routine will then return either the response
 * function or its inverse depending on the specified units.  Furthermore, the
 * power-of-ten field of the units is examined to scale the response function
 * accordingly. 
 *
 * \item The data vector should be allocated to the required length and the
 * frequency step size should be set to the required value so that the routine
 * can interpolate the response function to the required frequencies.
 * \end{enumerate}
 * The format of the frame catalog must be as follows.
 * \begin{enumerate}
 * \item It must contain an entry for one frame of type \verb+CAL_REF+ which 
 * contains the response and cavity gain frequency series that will be up
 * dated to the specified point in time. For example it must contain an entry
 * such as
 * \begin{verbatim}
 * L CAL_REF 715388533 64 file://localhost/path/to/L-CAL_REF-715388533-64.gwf
 * \end{verbatim}
 * where the frame file contains the channels \verb+L1:CAL-RESPONSE+ and
 * \verb+L1:CAL-CAV_GAIN+.
 *
 * \item It must also contain entries for the frames needed to update the
 * point calibration to the current time. These must contain the
 * \verb+L1:CAL-OLOOP_FAC+ and \verb+L1:CAL-CAV_FAC+ channels. The update
 * factor frames may either be SenseMon type frames, containing the factor
 * channels as \verb+real_8+ trend data or frames generated by the lalapps
 * program \verb+lalapps_mkcalfac+ which creates channels of type
 * \verb+complex_8+. The entries in the cache file must be of the format
 * \begin{verbatim}
 * L CAL_FAC 714240000 1369980 file://localhost/path/to/L-CAL_FAC-714240000-1369980.gwf
 * \end{verbatim}
 * for \verb+lalapps_mkcalfac+ type frames or
 * \begin{verbatim}
 * L SenseMonitor_L1_M 729925200 3600 file://localhost/path/to/L-SenseMonitor_L1_M-729925200-3600.gwf
 * \end{verbatim}
 * for SenseMon type frames.  If both types of frame are present in the cache,
 * SenseMon frames are used in preference.
 * \end{enumerate}
 *
 * \vfill{\footnotesize\input{FrameCalibrationCV}}
 *
 **** </lalLaTeX> */
#include <math.h>
#include <string.h>
#include <lal/LALStdlib.h>
#include <lal/LALStdio.h>
#include <lal/LALConstants.h>
#include <lal/AVFactories.h>
#include <lal/Calibration.h>
#include <lal/FrameStream.h>
#include <lal/FrameCalibration.h>

NRCSID( FRAMECALIBRATIONC, "$Id$" );

#define DURATION 256

#define RESPONSE_CHAN "CAL-RESPONSE"
#define CAV_GAIN_CHAN "CAL-CAV_GAIN"
#define OLOOP_FAC_CHAN "CAL-OLOOP_FAC"
#define CAV_FAC_CHAN "CAL-CAV_FAC"

#define REF_TYPE "CAL_REF"
#define FAC_TYPE "CAL_FAC"
#define SENSEMON_FAC_TYPE "SenseMonitor"

#define RETURN_POINT_CAL \
  calfuncs.responseFunction->sampleUnits = strainPerCount; \
  TRY( LALResponseConvert( status->statusPtr, \
        output, calfuncs.responseFunction ), status ); \
  if ( R0.data ) { \
    TRY( LALCDestroyVector( status->statusPtr, &R0.data ), status ); \
  } \
  if ( C0.data ) { \
    TRY( LALCDestroyVector( status->statusPtr, &C0.data ), status ); \
  } \
  TRY( LALDestroyFrCache( status->statusPtr, &calCache ), status );

#define OPEN_FAC \
      LALFrCacheOpen( status->statusPtr, &facStream, facCache ); \
      BEGINFAIL( status ) \
      { \
        TRY( LALDestroyFrCache( status->statusPtr, &facCache ), status ); \
        RETURN_POINT_CAL; \
      } \
      ENDFAIL( status ); \
      LALDestroyFrCache( status->statusPtr, &facCache ); \
      BEGINFAIL( status ) \
      { \
        TRY( LALFrClose( status->statusPtr, &facStream ), status ); \
        RETURN_POINT_CAL; \
      } \
      ENDFAIL( status );

#define GET_POS \
      LALFrSeek( status->statusPtr, &seekEpoch, facStream ); \
      BEGINFAIL( status ) \
      { \
        TRY( LALFrClose( status->statusPtr, &facStream ), status ); \
        RETURN_POINT_CAL; \
      } \
      ENDFAIL( status ); \
      LALFrGetPos( status->statusPtr, &facPos, facStream ); \
      BEGINFAIL( status ) \
      { \
        TRY( LALFrClose( status->statusPtr, &facStream ), status ); \
        RETURN_POINT_CAL; \
      } \
      ENDFAIL( status );

/* <lalVerbatim file="FrameCalibrationCP"> */
void
LALExtractFrameResponse(
    LALStatus               *status,
    COMPLEX8FrequencySeries *output,
    const CHAR              *catalog,
    const CHAR              *ifo,
    LIGOTimeGPS		    *duration)

{ /* </lalVerbatim> */
  const LALUnit strainPerCount = {0,{0,0,0,0,0,1,-1},{0,0,0,0,0,0,0}};

  FrCache      *calCache  = NULL;
  FrCache      *refCache  = NULL;
  FrCache      *facCache  = NULL;
  FrStream     *refStream = NULL;
  FrStream     *facStream = NULL;
  FrCacheSieve  sieve;
  FrChanIn      frameChan;
  FrPos         facPos;
  
  CHAR          facDsc[LALNameLength];
  CHAR          channelName[LALNameLength];

  COMPLEX8FrequencySeries       R0;
  COMPLEX8FrequencySeries       C0;
  COMPLEX8TimeSeries            ab;
  COMPLEX8TimeSeries            a;
  COMPLEX8Vector                abVec;
  COMPLEX8Vector                aVec;
  COMPLEX8                      abData;
  COMPLEX8                      aData;
  CalibrationFunctions          calfuncs;
  CalibrationUpdateParams       calfacts;

  LIGOTimeGPS                   seekEpoch;
  UINT4				length;
  REAL8				duration_real;
  const REAL8                   fuzz = 0.1 / 16384.0;
    
  INITSTATUS( status, "LALFrameExtractResponse", FRAMECALIBRATIONC );
  ATTATCHSTATUSPTR( status );

  ASSERT( output, status, 
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );
  ASSERT( output->data, status, 
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );
  ASSERT( output->data->data, status, 
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );
  ASSERT( catalog, status, 
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );
  ASSERT( ifo, status, 
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );
  ASSERT( duration, status,
      FRAMECALIBRATIONH_ENULL, FRAMECALIBRATIONH_MSGENULL );


  /* 
   *
   * set up and clear the structures to hold the input data 
   *
   */


  memset( &R0, 0, sizeof(COMPLEX8FrequencySeries) );
  memset( &C0, 0, sizeof(COMPLEX8FrequencySeries) );
  memset( &ab, 0, sizeof(COMPLEX8TimeSeries) );
  memset( &a,  0, sizeof(COMPLEX8TimeSeries) );
  memset( &calfuncs, 0, sizeof(CalibrationFunctions) );
  memset( &calfacts, 0, sizeof(CalibrationUpdateParams) );
  
  calfuncs.responseFunction = &R0;
  calfuncs.sensingFunction  = &C0;
  calfacts.openLoopFactor   = &ab;
  calfacts.sensingFactor    = &a;
  calfacts.epoch = output->epoch;
  calfacts.duration = *duration;
  frameChan.name = channelName;

  
  /* 
   *
   * open the calibration cache file 
   *
   */


  LALFrCacheImport( status->statusPtr, &calCache, catalog );
  if ( status->statusPtr->statusCode )
  {
    ABORT( status, FRAMECALIBRATIONH_EMCHE, FRAMECALIBRATIONH_MSGEMCHE );
  }
  memset( &sieve, 0, sizeof(FrCacheSieve) );


  /*
   *
   * get the reference calibration and cavity gain frequency series
   *
   */
  

  /* sieve the calibration cache for the reference frame */
  sieve.dscRegEx = REF_TYPE;
  LALFrCacheSieve( status->statusPtr, &refCache, calCache, &sieve );
  if ( status->statusPtr->statusCode || ! refCache->numFrameFiles )
  {
    /* if we don't have a reference calibration, we can't do anything */
    if ( ! status->statusPtr->statusCode )
    {
      TRY( LALDestroyFrCache( status->statusPtr, &refCache ), status );
    }
    TRY( LALDestroyFrCache( status->statusPtr, &calCache ), status );
    ABORT( status, FRAMECALIBRATIONH_ECREF, FRAMECALIBRATIONH_MSGECREF );
  }

  /* open the reference calibration frame */
  LALFrCacheOpen( status->statusPtr, &refStream, refCache );
  if ( status->statusPtr->statusCode )
  {
    /* if we don't have a reference calibration, we can't do anything */
    TRY( LALDestroyFrCache( status->statusPtr, &calCache ), status );
    TRY( LALDestroyFrCache( status->statusPtr, &refCache ), status );
    ABORT( status, FRAMECALIBRATIONH_EOREF, FRAMECALIBRATIONH_MSGEOREF );
  }
  LALDestroyFrCache( status->statusPtr, &refCache );
  if ( status->statusPtr->statusCode )
  {
    TRY( LALFrClose( status->statusPtr, &refStream ), status );
    ABORT( status, FRAMECALIBRATIONH_EDCHE, FRAMECALIBRATIONH_MSGEDCHE );
  }
  
  /* read in the frequency series for the reference calbration */
  LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
      "%s:" RESPONSE_CHAN,  ifo );
  LALFrGetCOMPLEX8FrequencySeries( status->statusPtr, 
      &R0, &frameChan, refStream );
  if ( status->statusCode )
  {
    /* if we don't have a reference calibration, we can't do anything */
    TRY( LALDestroyFrCache( status->statusPtr, &refCache ), status );
    TRY( LALDestroyFrCache( status->statusPtr, &calCache ), status );
    ABORT( status, FRAMECALIBRATIONH_EREFR, FRAMECALIBRATIONH_MSGEREFR );
  }
  
  /* read in the reference cavity gain frequency series */
  LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
      "%s:" CAV_GAIN_CHAN,  ifo );
  LALFrGetCOMPLEX8FrequencySeries( status->statusPtr, 
      &C0, &frameChan, refStream );
  BEGINFAIL( status )
  {
    /* no cavity gain response to update point cal */
    TRY( LALDestroyFrCache( status->statusPtr, &refCache ), status );
    TRY( LALFrClose( status->statusPtr, &refStream ), status );
    RETURN_POINT_CAL;
  }
  ENDFAIL( status );
  
  LALFrClose( status->statusPtr, &refStream );
  BEGINFAIL( status )
  {
    RETURN_POINT_CAL;
  }
  ENDFAIL( status );


  /*
   *
   * get the factors necessary to update the reference calibration
   *
   */

  
  /* try and get some update factors. first we try to get a cache  */
  /* containing sensemon frames. if that fails, try the S1 type    */
  /* calibration data, otherwise just return the point calibration */
  sieve.dscRegEx = facDsc;
  do
  {
    /* try and get sensemon frames */
    LALSnprintf( facDsc, LALNameLength * sizeof(CHAR), SENSEMON_FAC_TYPE );
    LALFrCacheSieve( status->statusPtr, &facCache, calCache, &sieve );
    BEGINFAIL( status )
    {
      RETURN_POINT_CAL;
    }
    ENDFAIL( status );

    if ( facCache->numFrameFiles )
    { 
      /* sensemon stores fac times series as real_8 adc trend data */  
      REAL8TimeSeries     sensemonTS;
      REAL8               alphaDeltaT;
      INT4		  i;

      memset( &sensemonTS, 0, sizeof(REAL8TimeSeries) ); 
    
      OPEN_FAC;

      LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
          "%s:" CAV_FAC_CHAN ".mean" ,  ifo );

      /* get the sample rate of the alpha channel */
      LALFrGetREAL8TimeSeries( status->statusPtr, 
          &sensemonTS, &frameChan, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* determine number of calibration points required */
      TRY( LALGPStoFloat( status->statusPtr, &duration_real, duration), 
	  status );
      length = (UINT4) ceil( duration_real / sensemonTS.deltaT );
      ++length;

      /* make sure we get the first point before the requested cal time */
      alphaDeltaT = sensemonTS.deltaT;
      TRY( LALAddFloatToGPS( status->statusPtr, &seekEpoch, &(output->epoch),
            0.0 - ( sensemonTS.deltaT + fuzz ) ), status );
      sensemonTS.epoch = seekEpoch;

      GET_POS;
      
      /* create memory for the alpha values */
      LALDCreateVector( status->statusPtr, &(sensemonTS.data), length );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );
      
      /* get the alpha values */
      LALFrGetREAL8TimeSeries( status->statusPtr, 
          &sensemonTS, &frameChan, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );
      
      LALCCreateVector( status->statusPtr, &(a.data), length ); 
      BEGINFAIL( status )
      {
        TRY( LALDDestroyVector( status->statusPtr, &(sensemonTS.data) ), 
            status );
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      for ( i = 0; i < length; ++i )
      {
	a.data->data[i].re = (REAL4) sensemonTS.data->data[i];
	a.data->data[i].im = 0;
      }
      a.epoch  = sensemonTS.epoch;
      a.deltaT = sensemonTS.deltaT;
      strncpy( a.name, sensemonTS.name, LALNameLength );

      LALFrSetPos( status->statusPtr, &facPos, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );
      
      /* get the alpha*beta values */
      LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
          "%s:" OLOOP_FAC_CHAN ".mean",  ifo );
      LALFrGetREAL8TimeSeries( status->statusPtr, 
          &sensemonTS, &frameChan, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* check that alpha and alpha*beta have the same sample rate */
      if ( fabs( alphaDeltaT - sensemonTS.deltaT ) > LAL_REAL8_EPS )
      {
        TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status );
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
        ABORT( status, FRAMECALIBRATIONH_EDTMM, FRAMECALIBRATIONH_MSGEDTMM );
      }

      LALCCreateVector( status->statusPtr, &(ab.data), length ); 
      BEGINFAIL( status )
      {
        TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status);
        TRY( LALDDestroyVector( status->statusPtr, &(sensemonTS.data) ), 
            status );
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      for ( i = 0; i < length; ++i )
      {
	ab.data->data[i].re = (REAL4) sensemonTS.data->data[i];
	ab.data->data[i].im = 0;
      }
      ab.epoch  = sensemonTS.epoch;
      ab.deltaT = sensemonTS.deltaT;
      strncpy( ab.name, sensemonTS.name, LALNameLength );
     
      /* destroy the sensemonTS.data */
      LALDDestroyVector( status->statusPtr, &(sensemonTS.data) );
      CHECKSTATUSPTR( status ); 	
      break; 
    }

    /* destroy the empty frame cache and try again */
    LALDestroyFrCache( status->statusPtr, &facCache );
    BEGINFAIL( status )
    {
      RETURN_POINT_CAL;
    }
    ENDFAIL( status );

    /* try and get the the factors from lalapps_mkcalfac frames */
    sieve.dscRegEx = FAC_TYPE;
    LALFrCacheSieve( status->statusPtr, &facCache, calCache, &sieve );
    BEGINFAIL( status )
    {
      RETURN_POINT_CAL;
    }
    ENDFAIL( status );

    if ( facCache->numFrameFiles )
    {
      /* the lalapps frames are complex_8 proc data */
      OPEN_FAC;
     
      LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
          "%s:" CAV_FAC_CHAN,  ifo );

      /* get the sample rate of the alpha channel */
      LALFrGetCOMPLEX8TimeSeries( status->statusPtr, 
          &a, &frameChan, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* determine number of calibration points required */
      TRY( LALGPStoFloat( status->statusPtr, &duration_real, duration), 
          status);
      length = (UINT4) ceil( duration_real / a.deltaT );
      ++length;

      /* make sure we get the first point before the requested cal time */
      TRY( LALAddFloatToGPS( status->statusPtr, &seekEpoch, &(output->epoch),
            0.0 - ( a.deltaT + fuzz ) ), status );
      a.epoch = ab.epoch = seekEpoch;

      GET_POS;
      
      /* create storage for the alpha values */
      LALCCreateVector( status->statusPtr, &(a.data), length );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* get the alpha values */ 
      LALFrGetCOMPLEX8TimeSeries( status->statusPtr, 
          &a, &frameChan, facStream );
      BEGINFAIL( status )
      {
        TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status );
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );
      
      LALFrSetPos( status->statusPtr, &facPos, facStream );
      BEGINFAIL( status )
      {
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* create storage for the alpha*beta values */
      LALCCreateVector( status->statusPtr, &(ab.data), length ); 
      BEGINFAIL( status )
      {
	TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status);
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* get the alpha*beta values */
      LALSnprintf( channelName, LALNameLength * sizeof(CHAR), 
          "%s:" OLOOP_FAC_CHAN,  ifo );
      LALFrGetCOMPLEX8TimeSeries( status->statusPtr, 
          &ab, &frameChan, facStream );
      BEGINFAIL( status )
      {
	TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status);
	TRY( LALCDestroyVector( status->statusPtr, &(ab.data) ), status);
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
      }
      ENDFAIL( status );

      /* check that alpha and alpha*beta have the same sample rate */
      if ( fabs( a.deltaT - ab.deltaT ) > LAL_REAL8_EPS )
      {
        TRY( LALCDestroyVector( status->statusPtr, &(a.data) ), status );
        TRY( LALCDestroyVector( status->statusPtr, &(ab.data) ), status );
        TRY( LALFrClose( status->statusPtr, &facStream ), status );
        RETURN_POINT_CAL;
        ABORT( status, FRAMECALIBRATIONH_EDTMM, FRAMECALIBRATIONH_MSGEDTMM );
      }

      break;
    }

    /* destroy the empty frame cache and give up */
    LALDestroyFrCache( status->statusPtr, &facCache );
    BEGINFAIL( status )
    {
      RETURN_POINT_CAL;
    }
    ENDFAIL( status );

    /* no update factors available, so just return the point cal */
    RETURN_POINT_CAL;
    ABORT( status, FRAMECALIBRATIONH_ECFAC, FRAMECALIBRATIONH_MSGECFAC );

  } while ( 0 );

  /* close the update factor stream */
  LALFrClose( status->statusPtr, &facStream );
  BEGINFAIL( status )
  {
    RETURN_POINT_CAL;
  }
  ENDFAIL( status );

  /* should be able to update into the same functions... */
  calfuncs.responseFunction->sampleUnits = strainPerCount;
  LALUpdateCalibration( status->statusPtr, &calfuncs, &calfuncs, &calfacts );
  BEGINFAIL( status )
  {
    LALCDestroyVector( status->statusPtr, &(a.data) );
    CHECKSTATUSPTR( status );
    LALCDestroyVector( status->statusPtr, &(ab.data) );
    CHECKSTATUSPTR( status );
    RETURN_POINT_CAL;
  }
  ENDFAIL( status );

  /* now convert response to get output, hardwire units */
  LALResponseConvert( status->statusPtr, output, calfuncs.responseFunction );
  CHECKSTATUSPTR( status );

  /* free the allocated memory */
  LALDestroyFrCache( status->statusPtr, &calCache );
  CHECKSTATUSPTR( status );
  LALCDestroyVector( status->statusPtr, &R0.data );
  CHECKSTATUSPTR( status );
  LALCDestroyVector( status->statusPtr, &C0.data );
  CHECKSTATUSPTR( status );
  LALCDestroyVector( status->statusPtr, &(a.data) );
  CHECKSTATUSPTR( status );
  LALCDestroyVector( status->statusPtr, &(ab.data) );
  CHECKSTATUSPTR( status );
  
  DETATCHSTATUSPTR( status );
  RETURN( status );
}
