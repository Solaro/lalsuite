/*  <lalVerbatim file="LALEtaTau04CV">
Author: Sathyaprakash, B. S.
$Id$
</lalVerbatim>  */

/*  <lalLaTeX>

\subsection{Module \texttt{LALEtaTau04.c}}
Given $\tau_0$ and $\tau_4$ solve for the mass ratio $\eta.$
\subsubsection*{Prototypes}
\vspace{0.1in}
\input{LALEtaTau04CP}
\idx{LALEtaTau04()}

\subsubsection*{Description}
Given $\tau_0$ and $\tau_4$ one can determine $\eta$ by solving 
\begin{equation}
-\eta^{4/5} \tau_4 + A_4 \left ( \frac {\tau_0}{A_0} \right )^{1/5}  
\left (1 + B_4\eta + C_4 \eta^2 \right )  = 0,
\end{equation}
where $A_0 = 5/[256 (\pi f_{s} )^{8/3}],$ 
$A_4 = 5 \times 3058673/ [128 \times 1016064  (\pi f_s)^{4/3}],$
$B_4 = 5429 \times 1016064 /(1008 \times 3058673),$ and $C_4 = 617 \times
1016064/(144 \times 3058673).$
This function returns the LHS of the above
equation in \texttt{x} for a given \texttt{eta}.


\subsubsection*{Algorithm}
None.

\subsubsection*{Uses}
None.

\subsubsection*{Notes}
The {\tt void pointer} {\tt *p} should point to a {\tt struct}
of type {\tt EtaTau04In}\\[10pt]
{\tt 
void *p;\\
EtaTau04In q;\\[5pt]
$\ldots$\\
p = (void *) \&q;\\
}
</lalLaTeX> */

#include <lal/LALInspiral.h>

NRCSID (LALETATAU04C, "$Id$");
/*  <lalVerbatim file="LALEtaTau04CP"> */
void 
LALEtaTau04(
   LALStatus *status, 
   REAL8     *x, 
   REAL8     eta, 
   void      *p
   ) 
{ /* </lalVerbatim> */
   EtaTau04In *q;
   INITSTATUS(status, "LALEtaTau04", LALETATAU04C);
   ATTATCHSTATUSPTR(status);
   ASSERT (p,  status, LALINSPIRALH_ENULL, LALINSPIRALH_MSGENULL);
   ASSERT(eta > 0, status, LALINSPIRALH_ESIZE, LALINSPIRALH_MSGESIZE);

   q = (EtaTau04In *) p;
   *x = -q->t4 + q->A4/pow(eta,0.8) * (1. + q->B4*eta + q->C4*eta*eta);
   DETATCHSTATUSPTR(status);
   RETURN(status);
}
