// ============================================================================
//
//     Copyright (c) Marvell Corporation 2000-2015  -  All rights reserved
//
//  This computer program contains confidential and proprietary information,
//  and  may NOT  be reproduced or transmitted, in whole or in part,  in any
//  form,  or by any means electronic, mechanical, photo-optical, or  other-
//  wise, and  may NOT  be translated  into  another  language  without  the
//  express written permission from Marvell Corporation.
//
// ============================================================================
// =      C O M P A N Y      P R O P R I E T A R Y      M A T E R I A L       =
// ============================================================================
#ifndef __MV_COM_REQUEST_HEADER_H__
#define __MV_COM_REQUEST_HEADER_H__

#define NO_MORE_DATA                            0xFFFF

#pragma pack(8)

/****************************************/
/* Start of variable size data request structures */
/***************************************/
#define REQUEST_BY_RANGE	1	/*  get a range of data by a given starting index and total number desired. */
#define REQUEST_BY_ID		2	/*  get specific data by a given device ID. */

typedef struct _RequestHeader
{
    MV_U8  version;			     /* Request header version; 0 for now. */
    MV_U8  requestType;		     /* REQUEST_BY_ID or REQUEST_BY_RANGE */
    MV_U16 startingIndexOrId;	     /* Starting index (in driver) of the data to be retrieved if requestType is REQUEST_BY_RANGE; 
								* otherwise this is the device ID of which its data is to be retrieved.  */
    MV_U16 numRequested;		     /* Max number of data entries application expected driver to return starting from 
								* startingIndexOrId, Application based on this value to allocate data space. 
								* If requestType is REQUEST_BY_ID, numRequested should set to 1. */
    MV_U16 numReturned;		     /* Actual number of data entries returned by driver. API might reduce this number 
								* by filtering out un-wanted entries. */
    MV_U16 nextStartingIndex;	     /* Driver suggested next starting index.  If requestType is REQUEST_BY_RANGE and  
								* if there is no more data available after this one, set it to NO_MORE_DATA. 
								* If requestType is REQUEST_BY_ID, always set it to NO_MORE_DATA. */
    MV_U8  reserved1[6];
} RequestHeader, *PRequestHeader;

typedef struct _Info_Request
{
    RequestHeader header;
    MV_U8	      data[1];
} Info_Request, *PInfo_Request;

#define FillRequestHeader( pReq,  m_requestType,  m_startingIndexOrId,  m_numRequested)  \
do \
{  \
	memset((pReq), 0,sizeof(RequestHeader));  \
	(pReq)->requestType = (m_requestType);   \
	(pReq)->startingIndexOrId= (m_startingIndexOrId);  \
	(pReq)->numRequested = (m_numRequested);  \
}while(0)

#pragma pack()

#endif
