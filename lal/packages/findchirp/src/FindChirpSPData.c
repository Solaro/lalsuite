/*----------------------------------------------------------------------- 
 * 
 * File Name: FindChirpSPData.c
 *
 * Author: Brown D. A.
 * 
 * Revision: $Id$
 * 
 *-----------------------------------------------------------------------
 */

#if 0 
<lalVerbatim file="FindChirpSPDataCV">
Author: Brown, D. A.
$Id$
</lalVerbatim> 

<lalLaTeX>
\subsection{Module \texttt{FindChirpSPData.c}}
\label{ss:FindChirpSPData.c}

\input{FindChirpSPDataCDoc}

\vfill{\footnotesize\input{FindChirpSPDataCV}}
</lalLaTeX> 
#endif

#include <lal/LALStdlib.h>
#include <lal/LALConstants.h>
#include <lal/AVFactories.h>
#include <lal/DataBuffer.h>
#include <lal/LALInspiral.h>
#include <lal/FindChirp.h>
#include <lal/FindChirpSP.h>


NRCSID (FINDCHIRPSPDATAC, "$Id$");

/* <lalVerbatim file="FindChirpSPDataCP"> */
void
LALFindChirpSPDataInit (
    LALStatus                  *status,
    FindChirpSPDataParams     **output,
    FindChirpInitParams        *params
    )
/* </lalVerbatim> */
{
  FindChirpSPDataParams        *dataParamPtr;
  REAL4                        *amp;
  const REAL4                   exponent = -7.0/6.0;
  UINT4                         k;

  INITSTATUS( status, "LALFindChirpSPDataInit", FINDCHIRPSPDATAC );
  ATTATCHSTATUSPTR( status );


  /*
   *
   * check that the arguments are reasonable
   *
   */


  /* make sure the output handle exists, but points to a null pointer */
  ASSERT( output, status, FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( !*output, status, FINDCHIRPSPH_ENNUL, FINDCHIRPSPH_MSGENNUL );

  /* make sure that the parameter structure exists */
  ASSERT (params, status, FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL);

  /* make sure that the number of points in a segment is positive */
  ASSERT (params->numPoints > 0, status, 
      FINDCHIRPSPH_ENUMZ, FINDCHIRPSPH_MSGENUMZ);


  /*
   *
   * allocate memory for the FindChirpSPDataParams
   *
   */


  /* create the output structure */
  dataParamPtr = *output = (FindChirpSPDataParams *)
    LALCalloc( 1, sizeof(FindChirpSPDataParams) );
  if ( ! dataParamPtr )
  {
    ABORT( status, FINDCHIRPSPH_EALOC, FINDCHIRPSPH_MSGEALOC );
  }


  /*
   *
   * allocate and fill vector for exponent of amplitude
   *
   */


  LALCreateVector( status->statusPtr, &dataParamPtr->ampVec, 
        (params->numPoints)/2 + 1 );
  BEGINFAIL( status )
  {
    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );

  amp = dataParamPtr->ampVec->data;
  amp[0] = 0.0;

  for ( k = 1; k < dataParamPtr->ampVec->length; ++k )
    amp[k] = pow( ((REAL4) k / (REAL4)params->numPoints), exponent );


  /*
   *
   * create fft plans and workspace vectors
   *
   */


  /* foward fft plan */
  LALCreateForwardRealFFTPlan( status->statusPtr, &dataParamPtr->fwdPlan, 
        params->numPoints, 0 );
  BEGINFAIL( status )
  {
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->ampVec ), status );

    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );

  /* inverse fft plan */
  LALCreateReverseRealFFTPlan( status->statusPtr, &dataParamPtr->invPlan, 
        params->numPoints, 0 );
  BEGINFAIL( status )
  {
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->fwdPlan ), status );
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->ampVec ), status );

    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );

  /* workspace vector w: time domain */
  LALCreateVector( status->statusPtr, &dataParamPtr->wVec, 
      params->numPoints );
  BEGINFAIL( status )
  {
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->invPlan ), status ); 
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->fwdPlan ), status );
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->ampVec ), status );

    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );

  /* workspace vector w: freq domain */
  LALCCreateVector( status->statusPtr, &dataParamPtr->wtildeVec, 
      params->numPoints/2 + 1 );
  BEGINFAIL( status )
  {
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->wVec ), status ); 
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->invPlan ), status ); 
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->fwdPlan ), status );
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->ampVec ), status );

    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );
  CHECKSTATUSPTR (status);
  
  /* template power vector */
  LALCreateVector( status->statusPtr, &dataParamPtr->tmpltPowerVec, 
      params->numPoints/2 + 1 );
  BEGINFAIL( status )
  {
    TRY( LALCDestroyVector( status->statusPtr, &dataParamPtr->wtildeVec), status );
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->wVec ), status ); 
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->invPlan ), status ); 
    TRY( LALDestroyRealFFTPlan( status->statusPtr, &dataParamPtr->fwdPlan ), status );
    TRY( LALDestroyVector( status->statusPtr, &dataParamPtr->ampVec ), status );

    LALFree( dataParamPtr );
    *output = NULL;
  }
  ENDFAIL( status );
  

  /* normal exit */
  DETATCHSTATUSPTR( status );
  RETURN( status );
}



/* <lalVerbatim file="FindChirpSPDataCP"> */
void
LALFindChirpSPDataFinalize (
    LALStatus                  *status,
    FindChirpSPDataParams     **output
    )
/* </lalVerbatim> */
{
  FindChirpSPDataParams        *dataParamPtr;

  INITSTATUS( status, "LALFindChirpSPDataFinalize", FINDCHIRPSPDATAC );
  ATTATCHSTATUSPTR( status );


  /*
   *
   * check that the arguments are reasonable
   *
   */
  

  /* make sure handle is non-null and points to a non-null pointer */
  ASSERT( output, status, FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( *output, status, FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  
  /*
   *
   * destroy fft plans and workspace vectors
   *
   */


  /* local pointer to structure */
  dataParamPtr = *output;
  
  LALDestroyRealFFTPlan (status->statusPtr, &dataParamPtr->fwdPlan);
  CHECKSTATUSPTR (status);

  LALDestroyRealFFTPlan (status->statusPtr, &dataParamPtr->invPlan);
  CHECKSTATUSPTR (status);

  LALDestroyVector (status->statusPtr, &dataParamPtr->wVec);
  CHECKSTATUSPTR (status);

  LALCDestroyVector (status->statusPtr, &dataParamPtr->wtildeVec);
  CHECKSTATUSPTR (status);

  LALDestroyVector (status->statusPtr, &dataParamPtr->tmpltPowerVec);
  CHECKSTATUSPTR (status);


  /*
   *
   * destroy vector for exponent of amplitude
   *
   */


  LALDestroyVector (status->statusPtr, &dataParamPtr->ampVec);
  CHECKSTATUSPTR (status);


  /*
   *
   * free memory for the FindChirpSPDataParams
   *
   */


  LALFree (dataParamPtr);
  *output = NULL;


  DETATCHSTATUSPTR (status);
  RETURN (status);
}


/* <lalVerbatim file="FindChirpSPDataCP"> */
void
LALFindChirpSPData (
    LALStatus                  *status,
    FindChirpSegmentVector     *fcSegVec,
    DataSegmentVector          *dataSegVec,
    FindChirpSPDataParams      *params
    )
/* </lalVerbatim> */
{
  UINT4                 i, k; 
  UINT4                 cut;

  REAL4                *w;
  REAL4                *amp;
  COMPLEX8             *wtilde;
  REAL4                *tmpltPower;
  
  REAL4Vector          *dataVec;
  REAL4                *spec;
  COMPLEX8             *resp;
  
  COMPLEX8             *outputData;

  UINT4                *chisqBin = NULL;
  UINT4                 numChisqBins;
  UINT4                 chisqPt;
  REAL4                 increment;
  REAL4                 nextBin;
  REAL4                 partSum;
  
  FindChirpSegment     *fcSeg;
  DataSegment          *dataSeg;

  INITSTATUS( status, "LALFindChirpSPData", FINDCHIRPSPDATAC );
  ATTATCHSTATUSPTR( status );

  
  /*
   *
   * make sure that the arguments are reasonable
   *
   */


  /* check that the output exists */
  ASSERT( fcSegVec, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": fcSegVec" );
  ASSERT( fcSegVec->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": fcSegVec->data" );
  ASSERT( fcSegVec->data->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": fcSegVec->data->dat" );
  ASSERT( fcSegVec->data->data->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": fcSegVec->data->data->data" );

  /* check that the parameter structure exists */
  ASSERT( params, status, FINDCHIRPSPH_ENULL, 
      FINDCHIRPSPH_MSGENULL ": params" );
  
  /* check that the workspace vectors exist */
  ASSERT( params->ampVec, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->ampVec->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->wVec, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->wVec->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->wtildeVec, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->wtildeVec->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->tmpltPowerVec, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->tmpltPowerVec->data, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  /* check that the fft plans exist */
  ASSERT( params->fwdPlan, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->invPlan, status, 
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  /* check that the parameter values are reasonable */
  ASSERT( params->deltaT > 0, status,
      FINDCHIRPSPH_EDELT, FINDCHIRPSPH_MSGEDELT );
  ASSERT( params->fLow >= 0, status,
      FINDCHIRPSPH_EFLOW, FINDCHIRPSPH_MSGEFLOW );
  ASSERT( params->dynRange > 0, status,
      FINDCHIRPSPH_EDYNR, FINDCHIRPSPH_MSGEDYNR );
  
  /* check that the input exists */
  ASSERT( dataSegVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": dataSegVec" );
  ASSERT( dataSegVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": dataSegVec->data" );
  ASSERT( dataSegVec->data->chan, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": dataSegVec->data->chan" );
  ASSERT( dataSegVec->data->chan->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL 
      ": dataSegVec->data->chan->data" );


  /*
   *
   * set up local segment independent pointers
   *
   */


  w          = params->wVec->data;
  amp        = params->ampVec->data;
  wtilde     = params->wtildeVec->data;
  tmpltPower = params->tmpltPowerVec->data;


  /*
   *
   * loop over data segments
   *
   */


  for ( i = 0; i < dataSegVec->length; ++i )
  {


    /*
     *
     * set up segment dependent pointers
     *
     */


    dataSeg      = &(dataSegVec->data[i]);
    fcSeg        = &(fcSegVec->data[i]);

    dataVec      = dataSeg->chan->data;
    spec         = dataSeg->spec->data->data;
    resp         = dataSeg->resp->data->data;

    outputData   = fcSeg->data->data->data;

    if ( fcSeg->chisqBinVec->length )
    {
      chisqBin     = fcSeg->chisqBinVec->data;
      numChisqBins = fcSeg->chisqBinVec->length - 1;
    }
    else
    {
      numChisqBins = 0;
    }

    ASSERT( params->wtildeVec->length == fcSeg->data->data->length, status,
        FINDCHIRPSPH_EMISM, FINDCHIRPSPH_MSGEMISM );


    /*
     *
     * compute htilde and store in fcSeg
     *
     */


    LALForwardRealFFT( status->statusPtr, fcSeg->data->data, 
        dataVec, params->fwdPlan );
    CHECKSTATUSPTR( status );

    /* compute strain */
    for ( k = 0; k < fcSeg->data->data->length; ++k )
    {
      REAL4 p = outputData[k].re;
      REAL4 q = outputData[k].im;
      REAL4 x = resp[k].re * params->dynRange;
      REAL4 y = resp[k].im * params->dynRange;

      outputData[k].re =  p*x - q*y;
      outputData[k].im =  p*y + q*x;
    }


    /*
     *
     * compute inverse power spectrum
     *
     */


    /* set low frequency cutoff inverse power spectrum */
    cut = params->fLow / dataSeg->spec->deltaF > 1 ? 
      params->fLow / dataSeg->spec->deltaF : 1;

    /* set inverse power spectrum to zero */
    memset( wtilde, 0, params->wtildeVec->length * sizeof(COMPLEX8) );

    /* compute inverse of S_v */
    for ( k = cut; k < params->wtildeVec->length; ++k )
    {
      if ( spec[k] == 0 )
      {
        ABORT( status, FINDCHIRPSPH_EDIVZ, FINDCHIRPSPH_MSGEDIVZ );
      }
      wtilde[k].re = 1.0 / spec[k];
    }


    /*
     *
     * truncate inverse power spectrum in time domain if required
     *
     */


    if ( params->invSpecTrunc )
    {
      /* compute square root of inverse power spectrum */
      for ( k = cut; k < params->wtildeVec->length; ++k )
      {
        wtilde[k].re = sqrt( wtilde[k].re );
      }

      /* set nyquist and dc to zero */
      wtilde[params->wtildeVec->length - 1].re = 0.0;
      wtilde[0].re                             = 0.0;

      /* transform to time domain */
      LALReverseRealFFT( status->statusPtr, params->wVec, params->wtildeVec, 
          params->invPlan );
      CHECKSTATUSPTR (status);

      /* truncate in time domain */
      memset( w + params->invSpecTrunc/2, 0, 
          (params->wVec->length - params->invSpecTrunc) * sizeof(REAL4) );

      /* transform to frequency domain */
      LALForwardRealFFT( status->statusPtr, params->wtildeVec, params->wVec, 
          params->fwdPlan );
      CHECKSTATUSPTR (status);

      /* normalise fourier transform and square */
      {
        REAL4 norm = 1.0 / (REAL4) params->wVec->length;
        for ( k = cut; k < params->wtildeVec->length; ++k )
        {
          wtilde[k].re *= norm;
          wtilde[k].re *= wtilde[k].re;
          wtilde[k].im = 0.0;
        }
      }

      /* set nyquist and dc to zero */
      wtilde[params->wtildeVec->length - 1].re = 0.0;
      wtilde[0].re                             = 0.0;
    }

    /* set inverse power spectrum below cut to zero */
    memset( wtilde, 0, cut * sizeof(COMPLEX8) );

    /* convert from S_v to S_h */
    for ( k = cut; k < params->wtildeVec->length; ++k )
    {
      REAL4 respRe = resp[k].re * params->dynRange;
      REAL4 respIm = resp[k].im * params->dynRange;
      REAL4 modsqResp = (respRe * respRe + respIm * respIm);
      REAL4 invmodsqResp;
      if ( modsqResp == 0 )
      {
        ABORT( status, FINDCHIRPSPH_EDIVZ, FINDCHIRPSPH_MSGEDIVZ );
      }
      invmodsqResp = 1.0 / modsqResp;
      wtilde[k].re *= invmodsqResp;
    }


    /*
     *
     * compute segment normalisation, outputData, point fcSeg at data segment
     *
     */


    fcSeg->segNorm = 0.0;

    for ( k = 0; k < cut; ++k )
    {
      outputData[k].re = 0.0;
      outputData[k].im = 0.0;
    }

    memset( tmpltPower, 0, params->tmpltPowerVec->length * sizeof(REAL4) );

    for ( k = 1; k < fcSeg->data->data->length; ++k )
    {
      tmpltPower[k]   = amp[k] * amp[k] * wtilde[k].re;
      fcSeg->segNorm += tmpltPower[k];
    }

    for ( k = cut; k < fcSeg->data->data->length; ++k )
    {
      outputData[k].re  *= wtilde[k].re * amp[k];
      outputData[k].im  *= wtilde[k].re * amp[k];
    }

    /* set output frequency series parameters */
    strncpy( fcSeg->data->name, dataSeg->chan->name, LALNameLength );

    fcSeg->data->epoch.gpsSeconds      = dataSeg->chan->epoch.gpsSeconds;
    fcSeg->data->epoch.gpsNanoSeconds  = dataSeg->chan->epoch.gpsNanoSeconds;

    fcSeg->data->f0     = dataSeg->chan->f0;
    fcSeg->data->deltaF = 1.0 / 
      ( (REAL8) dataSeg->chan->data->length * dataSeg->chan->deltaT ) ;

    fcSeg->deltaT       = dataSeg->chan->deltaT;
    fcSeg->number       = dataSeg->number;

    /* store low frequency cutoff and invSpecTrunc in segment */
    fcSeg->fLow         = params->fLow;
    fcSeg->invSpecTrunc = params->invSpecTrunc;


    /*
     *
     * calculate the chisq bins for the segment and template
     *
     */


    if ( numChisqBins )
    {
      increment = fcSeg->segNorm / (REAL4) numChisqBins;
      nextBin   = increment;
      chisqPt   = 0;
      partSum   = 0.0;

      /* calculate the frequencies of the chi-squared bin boundaries */
      chisqBin[chisqPt++] = 0;

      for ( k = 1; k < fcSeg->data->data->length; ++k ) 
      {
        partSum += tmpltPower[k];
        if ( partSum >= nextBin ) 
        {
          chisqBin[chisqPt++] = k;
          nextBin += increment;
          if ( chisqPt == numChisqBins ) break;
        }
      }
      chisqBin[numChisqBins] = fcSeg->data->data->length;
    }

  } /* end loop over data segments */


  /* normal exit */
  DETATCHSTATUSPTR( status );
  RETURN( status );
}


/* <lalVerbatim file="FindChirpBCVDataCP"> */
void
LALFindChirpBCVData (
    LALStatus                  *status,
    FindChirpSegmentVector     *fcSegVec,
    DataSegmentVector          *dataSegVec,
    FindChirpSPDataParams      *params
    )
/* </lalVerbatim> */
{
  UINT4                 i, k;
  UINT4                 cut;

  REAL4                *w;
  REAL4                *amp;
  COMPLEX8             *wtilde;
  REAL4                *tmpltPower;
  REAL4		       *tmpltPower1;
  REAL4		       *tmpltPower2;

  REAL4Vector          *dataVec;
  REAL4                *spec;
  COMPLEX8             *resp;

  COMPLEX8             *outputData;

 UINT4                *chisqBin = NULL;
  UINT4                 numChisqBins;
  UINT4                 chisqPt;
  REAL4                 increment;
  REAL4                 nextBin;
  REAL4                 partSum;

  FindChirpSegment     *fcSeg;
  DataSegment          *dataSeg;


  INITSTATUS( status, "LALFindChirpBCVData", FINDCHIRPSPDATAC );
  ATTATCHSTATUSPTR( status );


  /*
   *
   * make sure that the arguments are reasonable
   *
   */


  /* check that the output exists */
  ASSERT( fcSegVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": fcSegVec" );
  ASSERT( fcSegVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": fcSegVec->data" );
  ASSERT( fcSegVec->data->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": fcSegVec->data->dat" );
  ASSERT( fcSegVec->data->data->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": fcSegVec->data->data->data" );

  /* check that the parameter structure exists */
  ASSERT( params, status, FINDCHIRPSPH_ENULL,
      FINDCHIRPSPH_MSGENULL ": params" );
/* check that the workspace vectors exist */
  ASSERT( params->ampVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->ampVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->wVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->wVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->wtildeVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->wtildeVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  ASSERT( params->tmpltPowerVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->tmpltPowerVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  /* check that the fft plans exist */
  ASSERT( params->fwdPlan, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );
  ASSERT( params->invPlan, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL );

  /* check that the parameter values are reasonable */
  ASSERT( params->deltaT > 0, status,
      FINDCHIRPSPH_EDELT, FINDCHIRPSPH_MSGEDELT );
  ASSERT( params->fLow >= 0, status,
      FINDCHIRPSPH_EFLOW, FINDCHIRPSPH_MSGEFLOW );
  ASSERT( params->dynRange > 0, status,
      FINDCHIRPSPH_EDYNR, FINDCHIRPSPH_MSGEDYNR );

  /* check that the input exists */
  ASSERT( dataSegVec, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": dataSegVec" );
  ASSERT( dataSegVec->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": dataSegVec->data" );
  ASSERT( dataSegVec->data->chan, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": dataSegVec->data->chan" );
  ASSERT( dataSegVec->data->chan->data, status,
      FINDCHIRPSPH_ENULL, FINDCHIRPSPH_MSGENULL
      ": dataSegVec->data->chan->data" );


 /*
   *
   * set up local segment independent pointers
   *
   */


  w          = params->wVec->data;
  amp        = params->ampVec->data;
  wtilde     = params->wtildeVec->data;
  tmpltPower = params->tmpltPowerVec->data;


  /*
   *
   * loop over data segments
   *
   */


  for ( i = 0; i < dataSegVec->length; ++i )
  {


    /*
     *
     * set up segment dependent pointers
     *
     */


    dataSeg      = &(dataSegVec->data[i]);
    fcSeg        = &(fcSegVec->data[i]);

    dataVec      = dataSeg->chan->data;
    spec         = dataSeg->spec->data->data;
    resp         = dataSeg->resp->data->data;

    outputData   = fcSeg->data->data->data;

    if ( fcSeg->chisqBinVec->length )
    {
      chisqBin     = fcSeg->chisqBinVec->data;
      numChisqBins = fcSeg->chisqBinVec->length - 1;
    }
    else
    {
      numChisqBins = 0;
    }

    ASSERT( params->wtildeVec->length == fcSeg->data->data->length, status,
        FINDCHIRPSPH_EMISM, FINDCHIRPSPH_MSGEMISM );

    /*
     *
     * compute htilde and store in fcSeg
     *
     */


    LALForwardRealFFT( status->statusPtr, fcSeg->data->data,
        dataVec, params->fwdPlan );
    CHECKSTATUSPTR( status );

    /* compute strain */
    for ( k = 0; k < fcSeg->data->data->length; ++k )
    {
      REAL4 p = outputData[k].re;
      REAL4 q = outputData[k].im;
      REAL4 x = resp[k].re * params->dynRange;
      REAL4 y = resp[k].im * params->dynRange;

      outputData[k].re =  p*x - q*y;
      outputData[k].im =  p*y + q*x;
    }


    /*
     *
     * compute inverse power spectrum
     *
     */


    /* set low frequency cutoff inverse power spectrum */
    cut = params->fLow / dataSeg->spec->deltaF > 1 ?
      params->fLow / dataSeg->spec->deltaF : 1;

    /* set inverse power spectrum to zero */
    memset( wtilde, 0, params->wtildeVec->length * sizeof(COMPLEX8) );

    /* compute inverse of S_v */
    for ( k = cut; k < params->wtildeVec->length; ++k )
    {
      if ( spec[k] == 0 )
      {
        ABORT( status, FINDCHIRPSPH_EDIVZ, FINDCHIRPSPH_MSGEDIVZ );
      }
      wtilde[k].re = 1.0 / spec[k];
    }

    /*
     *
     * truncate inverse power spectrum in time domain if required
     *
     */


    if ( params->invSpecTrunc )
    {
      /* compute square root of inverse power spectrum */
      for ( k = cut; k < params->wtildeVec->length; ++k )
      {
        wtilde[k].re = sqrt( wtilde[k].re );
      }

      /* set nyquist and dc to zero */
      wtilde[params->wtildeVec->length - 1].re = 0.0;
      wtilde[0].re                             = 0.0;

      /* transform to time domain */
      LALReverseRealFFT( status->statusPtr, params->wVec, params->wtildeVec,
          params->invPlan );
      CHECKSTATUSPTR (status);

      /* truncate in time domain */
      memset( w + params->invSpecTrunc/2, 0,
          (params->wVec->length - params->invSpecTrunc) * sizeof(REAL4) );

      /* transform to frequency domain */
      LALForwardRealFFT( status->statusPtr, params->wtildeVec, params->wVec,
          params->fwdPlan );
      CHECKSTATUSPTR (status);

      /* normalise fourier transform and square */
      {
        REAL4 norm = 1.0 / (REAL4) params->wVec->length;
        for ( k = cut; k < params->wtildeVec->length; ++k )
        {
          wtilde[k].re *= norm;
          wtilde[k].re *= wtilde[k].re;
          wtilde[k].im = 0.0;
        }
      }

      /* set nyquist and dc to zero */
      wtilde[params->wtildeVec->length - 1].re = 0.0;
      wtilde[0].re                             = 0.0;
    }

    /* set inverse power spectrum below cut to zero */
    memset( wtilde, 0, cut * sizeof(COMPLEX8) );

    /* convert from S_v to S_h */
    for ( k = cut; k < params->wtildeVec->length; ++k )
    {
      REAL4 respRe = resp[k].re * params->dynRange;
      REAL4 respIm = resp[k].im * params->dynRange;
      REAL4 modsqResp = (respRe * respRe + respIm * respIm);
      REAL4 invmodsqResp;
      if ( modsqResp == 0 )
      {
        ABORT( status, FINDCHIRPSPH_EDIVZ, FINDCHIRPSPH_MSGEDIVZ );
      }
      invmodsqResp = 1.0 / modsqResp;
      wtilde[k].re *= invmodsqResp;
    }


    /*
     *
     * compute segment normalisation, outputData, point fcSeg at data segment
     *
     */


    fcSeg->segNorm = 0.0;
    fcSeg->b1 = 0.0;
    fcSeg->a2 = 0.0;

    for ( k = 0; k < cut; ++k )
    {
      outputData[k].re = 0.0;
      outputData[k].im = 0.0;
    }

    memset( tmpltPower, 0, params->tmpltPowerVec->length * sizeof(REAL4) );

    for ( k = 1; k < fcSeg->data->data->length; ++k )
    {
      tmpltPower1[k]   = amp[k] * amp[k] * wtilde[k].re;
      tmpltPower2[k]   = tmpltPower1[k] *
	pow( (k/(fcSeg->data->data->length)), 4.0/3.0 );
      fcSeg->segNorm += tmpltPower[k]; /* probably not necessary for BCV */
      fcSeg->b1 += tmpltPower1[k];
      fcSeg->a2 += tmpltPower2[k];
    }

    fcSeg->b1 = 1.0 / 2.0 / sqrt( fcSeg->b1 ) ;
    fcSeg->a2 = 1.0 / 2.0 / sqrt( fcSeg->a2 ) ;

    for ( k = cut; k < fcSeg->data->data->length; ++k )
    {
      outputData[k].re  *= wtilde[k].re * amp[k];
      outputData[k].im  *= wtilde[k].re * amp[k];
    }

    /* set output frequency series parameters */
    strncpy( fcSeg->data->name, dataSeg->chan->name, LALNameLength );

    fcSeg->data->epoch.gpsSeconds      = dataSeg->chan->epoch.gpsSeconds;
    fcSeg->data->epoch.gpsNanoSeconds  = dataSeg->chan->epoch.gpsNanoSeconds;

    fcSeg->data->f0     = dataSeg->chan->f0;
    fcSeg->data->deltaF = 1.0 /
      ( (REAL8) dataSeg->chan->data->length * dataSeg->chan->deltaT ) ;

    fcSeg->deltaT       = dataSeg->chan->deltaT;
    fcSeg->number       = dataSeg->number;

    /* store low frequency cutoff and invSpecTrunc in segment */
    fcSeg->fLow         = params->fLow;
    fcSeg->invSpecTrunc = params->invSpecTrunc;


    /*
     *
     * calculate the chisq bins for the segment and template
     *
     */


    if ( numChisqBins )
    {
      increment = (fcSeg->b1 + fcSeg->a2) / (REAL4) numChisqBins;
      nextBin   = increment;
      chisqPt   = 0;
      partSum   = 0.0;

      /* calculate the frequencies of the chi-squared bin boundaries */
      chisqBin[chisqPt++] = 0;

      for ( k = 1; k < fcSeg->data->data->length; ++k )
      {
        partSum += ( tmpltPower1[k] + tmpltPower2[k] ); 
        if ( partSum >= nextBin )
        {
          chisqBin[chisqPt++] = k;
          nextBin += increment;
          if ( chisqPt == numChisqBins ) break;
        }
      }
      chisqBin[numChisqBins] = fcSeg->data->data->length;
    }

    /* final values of normalization constants b1 and a2 */
    fcSeg->b1 = 1.0 / 2.0 / sqrt( fcSeg->b1 ) ;
    fcSeg->a2 = 1.0 / 2.0 / sqrt( fcSeg->a2 ) ;


  } /* end loop over data segments */


  /* normal exit */
  DETATCHSTATUSPTR( status );
  RETURN( status );
}

