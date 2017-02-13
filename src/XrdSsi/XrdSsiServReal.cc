/******************************************************************************/
/*                                                                            */
/*                     X r d S s i S e r v R e a l . c c                      */
/*                                                                            */
/* (c) 2013 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/*                                                                            */
/* This file is part of the XRootD software suite.                            */
/*                                                                            */
/* XRootD is free software: you can redistribute it and/or modify it under    */
/* the terms of the GNU Lesser General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or (at your     */
/* option) any later version.                                                 */
/*                                                                            */
/* XRootD is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public       */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU Lesser General Public License   */
/* along with XRootD in a file called COPYING.LESSER (LGPL license) and file  */
/* COPYING (GPL license).  If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                            */
/* The copyright holder's institutional names and contributor's names may not */
/* be used to endorse or promote products derived from this software without  */
/* specific prior written permission of the institution or contributor.       */
/******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>
  
#include "XrdSsi/XrdSsiReqAgent.hh"
#include "XrdSsi/XrdSsiResource.hh"
#include "XrdSsi/XrdSsiScale.hh"
#include "XrdSsi/XrdSsiServReal.hh"
#include "XrdSsi/XrdSsiSessReal.hh"
#include "XrdSsi/XrdSsiTrace.hh"
#include "XrdSsi/XrdSsiUtils.hh"
  
/******************************************************************************/
/*                     S t a t i c s   &   G l o b a l s                      */
/******************************************************************************/
  
namespace XrdSsi
{
       XrdSsiScale   sidScale;
}

using namespace XrdSsi;

/******************************************************************************/
/*                            D e s t r u c t o r                             */
/******************************************************************************/
  
XrdSsiServReal::~XrdSsiServReal()
{
   XrdSsiSessReal *sP;

// Free pointer to the manager node
//
   if (manNode) {free(manNode); manNode = 0;}

// Delete all free session objects
//
   while((sP = freeSes))
        {freeSes = sP->nextSess;
         delete sP;
        }
}

/******************************************************************************/
/* Private:                        A l l o c                                  */
/******************************************************************************/

XrdSsiSessReal *XrdSsiServReal::Alloc(const char *sName, int uent, bool hold)
{
   XrdSsiSessReal *sP;

// Reuse or allocate a new session object and return it
//
   myMutex.Lock();
   actvSes++;
   if ((sP = freeSes))
      {freeCnt--;
       freeSes = sP->nextSess;
       myMutex.UnLock();
       sP->InitSession(this, sName, uent, hold);
      } else {
       myMutex.UnLock();
       if (!(sP = new XrdSsiSessReal(this, sName, uent, hold)))
          {myMutex.Lock(); actvSes--; myMutex.UnLock();}
      }
   return sP;
}
  
/******************************************************************************/
/* Private:                       G e n U R L                                 */
/******************************************************************************/
  
bool XrdSsiServReal::GenURL(XrdSsiResource *rP, char *buff, int blen, int uEnt)
{
   static const char affTab[] = "\0\0n\0w\0s\0S";
   const char *xUsr, *xAt,  *iSep, *iVal, *tVar, *tVal, *uVar, *uVal;
   const char *aVar, *aVal, *qVal = "";
   char uBuff[8];
   int n;

// Preprocess avoid list, if any
//
   if (rP->hAvoid.length() == 0) tVar = tVal = "";
      else {tVar = "&tried=";
            tVal = rP->hAvoid.c_str();
            qVal = "?";
           }

// Preprocess affinity
//
   if (!(rP->affinity)) aVar = aVal = "";
      else {aVar = "&cms.aff=";
            aVal = &affTab[rP->affinity*2];
            qVal = "?";
           }

// Check if we need to add a user name
//
   if (rP->rUser.length() == 0) uVar = uVal = "";
      else {uVar = "&ssi.user=";
            uVal = rP->rUser.c_str();
            qVal = "?";
           }

// Preprocess the cgi information
//
   if (rP->rInfo.length() == 0) iSep = iVal = "";
      else {iVal = rP->rInfo.c_str();
            iSep = "&ssi.cgi=";
            qVal = "?";
           }

// Check if we need to qualify the host with a user index
//
   if (uEnt == 0) xUsr = xAt = "";
      else {snprintf(uBuff, sizeof(uBuff), "%d", uEnt);
            uVal = uBuff;
            xAt = "@";
           }

// Generate appropriate url
//                                             ? t   a   u   i
   n = snprintf(buff, blen, "xroot://%s%s%s/%s%s%s%s%s%s%s%s%s%s",
                             xUsr, xAt, manNode, rP->rName.c_str(), qVal,
                             tVar, tVal, aVar, aVal,
                             uVar, uVal, iSep, iVal);

// Return overflow or not
//
   return n < blen;
}

/******************************************************************************/
/*                        P r o c e s s R e q u e s t                         */
/******************************************************************************/

void XrdSsiServReal::ProcessRequest(XrdSsiRequest  &reqRef,
                                    XrdSsiResource &resRef)
{
   XrdSsiSessReal *sObj;
   int  uEnt;
   char epURL[4096];

// Validate the resource name
//
   if (resRef.rName.length() == 0)
      {XrdSsiUtils::RetErr(reqRef, "Resource name missing.", EINVAL);
       return;
      }

// Get a sid entry number
//
   if ((uEnt = sidScale.getEnt()) < 0)
      {XrdSsiUtils::RetErr(reqRef, "Out of stream resources.", ENOSR);
       return;
      }

// Construct url
//
   if (!GenURL(&resRef, epURL, sizeof(epURL), uEnt))
      {XrdSsiUtils::RetErr(reqRef, "Resource url is too long.", ENAMETOOLONG);
       sidScale.retEnt(uEnt);
       return;
      }

// Obtain a new session object (note the first request uses the session mutex)
//
   if (!(sObj = Alloc(resRef.rName.c_str(), uEnt)))
      {XrdSsiUtils::RetErr(reqRef, "Insufficient memory.", ENOMEM);
       sidScale.retEnt(uEnt);
       return;
      } else XrdSsiReqAgent::SetMutex(&reqRef, sObj->MutexP());

// Now just provision this resource which will execute the request should it
// be successful.
//
   if (!(sObj->Provision(&reqRef, epURL))) Recycle(sObj);
}

/******************************************************************************/
/*                               R e c y c l e                                */
/******************************************************************************/
  
void XrdSsiServReal::Recycle(XrdSsiSessReal *sObj)
{
   EPNAME("Recycle");
   static const char *tident = "ServReal";

// Add to queue unless we have too many of these
//
   myMutex.Lock();
   actvSes--;
   DEBUG("Sessions: free=" <<freeCnt <<" active=" <<actvSes);
   if (freeCnt >= freeMax) {myMutex.UnLock(); delete sObj;}
      else {sObj->ClrEvent();
            sObj->nextSess = freeSes;
            freeSes = sObj;
            freeCnt++;
            myMutex.UnLock();
           }
}

/******************************************************************************/
/*                                  S t o p                                   */
/******************************************************************************/
  
bool XrdSsiServReal::Stop()
{
// Make sure we are clean
//
   myMutex.Lock();
   if (actvSes) {myMutex.UnLock(); return false;}
   myMutex.UnLock();
   delete this;
   return true;
}