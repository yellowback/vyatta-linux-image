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
#include "core_header.h"
#include "spi_hal.h"
#include "core_spi.h"
#include "core_util.h"

static MV_U8           SPICmd[16];

MV_U8   ATMEL_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x52, 0x62, 0x15
};
MV_U8   MXIC_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x90
};
MV_U8   WINBOND_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0xD8, 0xC7, 0xAB
};

MV_U8   ATMEL_SPI_CMD_41A_021[16] = 
{
/* 0 		1	2	 3	   4      5 	    6	    7   	8      9      10       11*/
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0xD8, 0x60, 0x9F, 0x36, 0x39, 0x3C
};

MV_U8	EON_F20_SPI_CMD[16] = 
{
	0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x90
};

MV_U8   SST_SPI_CMD[16] = 
{
/* 0 		1	2	 3	   4      5 	    6	    7   	8      9      10       11 */
    0x06, 0x04, 0x05, 0x01, 0x03, 0xAD,  0xD8 , 0xC7, 0xAB, 0x01, 0x50, 0x01 
};

#if defined( SPIRegR32 )
#undef SPIRegR32
#endif
#if defined( SPIRegW32 )
#undef SPIRegW32
#endif

#define SPIRegR32       FMemR32
#define SPIRegW32       FMemW32

/**********************************************************************************/
MV_U32
SPI_Cmd
(
    MV_U8   cmd
)
{
    if( cmd >= sizeof( SPICmd ) )
        return -1;
    return SPICmd[cmd];
}
/**********************************************************************************/
int
OdinSPI_WaitDataReady
(
    AdapterInfo *pAI,
    MV_U32         timeout
)
{

    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;

    for( i=0; i<timeout; i++ ) {
	dwTmp = SPIRegR32( BarAdr, ODIN_SPI_CTRL_REG );
       if( !( dwTmp & SPI_CTRL_SpiStart ) ) {
            return 0;
       }
       DelayUSec( 10 );
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );

    return -1;
}




int
OdinSPI_WaitCmdIssued
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;

    for( i=0; i<3000; i++ ) {
		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_CTRL_REG );
		if( !( dwTmp & SPI_CTRL_SpiStart ) )
        {
            return 0;
        }
        DelayUSec( 20 );
    }

    return -1;
}

#define OdinSPI_BuildCmd SPI_BuildCmd

int
SPI_BuildCmd
(
	MV_PVOID	BarAdr,
    MV_U32      *dwCmd,
    MV_U8       cmd,
    MV_U8       read,
    MV_U8       length,
    MV_U32      addr
)

{
    MV_U32  dwTmp;

    SPIRegW32( BarAdr, ODIN_SPI_CTRL_REG, 0 );
    dwTmp = ( ( MV_U32 )cmd << 8 )|( (MV_U32)length << 4 );
    if( read )
    {
        dwTmp|= SPI_CTRL_READ;
    }
    if (addr != MV_MAX_U32)
	{
		SPIRegW32( BarAdr, ODIN_SPI_ADDR_REG, ( addr & 0x0003FFFF ) );
		dwTmp|=SPI_CTRL_AddrValid;
	}

	*dwCmd = dwTmp;
    return 0;
}

int
OdinSPI_IssueCmd
(
    AdapterInfo *pAI,
    MV_U32      cmd
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    int     retry;

    for( retry=0; retry<1; retry++ )
    {
        SPIRegW32( BarAdr, ODIN_SPI_CTRL_REG, cmd | SPI_CTRL_SpiStart );
	}    
        
    return 0;
}
int
OdinSPI_RDSR
(
    AdapterInfo *pAI,
    MV_U8       *sr
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
				(MV_U8)SPI_Cmd( SPI_INS_RDSR ), 
                  1, 
                  1, 
                  -1 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

        dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		*sr = (MV_U8)dwTmp;
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}


int
OdinSPI_EWSR
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];


		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_UPTSEC ), 
                  0, 
                  0, 
                  -1 );

    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 ==OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        	return 0;
    }
    SPI_DBG( ( "line: %d\n", __LINE__ ) );
    return -1;
}


int
OdinSPI_WRSR
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32 dwTmp;
    MV_U8 status=0xFF;
   
    if (0!=OdinSPI_RDSR( pAI, &status ))
        return -1;
    
    if((status & 0x1C) == 0){	
		return 0;
    }

    if (0  !=OdinSPI_EWSR( pAI )){
	SPI_DBG(("Can not enable write satatus.\n"));
        return -1;
    }

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WRSR), 
                  0, 
                  1, 
                  -1);

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, (MV_U32)(status & (~ 0x1C)));

    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}





int
OdinSPI_PollSR
(
    AdapterInfo *pAI,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout
)
{
    MV_U32  i;
    MV_U8   sr;
    
    for( i=0; i<timeout; i++ ) {
        if( 0 == OdinSPI_RDSR( pAI, &sr ) )
        {
            if( ( sr & mask )==bit )
                return 0;
        }
        else
            SPI_DBG( ( "%d\n", __LINE__ ) );
        DelayUSec( 20 );
    }

    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_WREN
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WREN ), 
                  0, 
                  0, 
                  -1 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0x02, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_WRDI
(
    AdapterInfo *pAI
)
{
    MV_U32 dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WRDI ), 
                  0, 
                  0, 
                  -1 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    OdinSPI_WaitDataReady( pAI, 10000 );
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_RDPT
(
    AdapterInfo *pAI,
    MV_U32      addr,
    MV_U8       *Data
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32   dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_RDPT ), 
                  1, 
                  1, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );
		*Data = (MV_U8)dwTmp;
        return 0;
    }
    else
    {
        SPI_DBG( ( " SPI_RDPT timeout\n"  ) );
    }
    return -1;
}



int
OdinSPI_SectUnprotect
(
    AdapterInfo *pAI,
    MV_U32      addr
)
{
    MV_U32 dwTmp;
    MV_U8 protect_sect=0xFF;	
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

    if(-1 ==OdinSPI_RDPT(pAI, addr, &protect_sect))
    {
	return -1;
    }

    if(protect_sect==0)
    	return 0;
    
    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_UPTSEC), 
                  0, 
                  0, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "error SPI_SectUnprotect \n" ) );
    return -1;
}

int
OdinSPI_SectErase
(
    AdapterInfo *pAI,
    MV_U32      addr
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
	
    if(pAI->FlashID==SST25VF040B )
    {
      if(-1 == OdinSPI_WRSR(pAI))
      {
        SPI_DBG(("SPI_WRSR error.\n"));
        return -1;
      }
    }

    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

   if((pAI->FlashID==AT25DF041A) || (pAI->FlashID==AT25DF021)) {

	   if(-1 == OdinSPI_SectUnprotect(pAI, addr))
	   {
			SPI_DBG(("Un protect error.\n"));
			return -1;
	    }
   }

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
	                  (MV_U8)SPI_Cmd( SPI_INS_SERASE ), 
                  0, 
                  0, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "error OdinSPI_SectErase\n" ) );
    return -1;
}

int
OdinSPI_ChipErase
(
    AdapterInfo *pAI
)
{
    MV_U32 dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
	
    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_CERASE ), 
                  0, 
                  0, 
                  -1 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}
int
OdinSPI_Write_autoinc
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U16      Data, 
    MV_U32      Count
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;
   
    if (Count>4)
        Count = 4;

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, Data );

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_PROG ), 
                  0, 
                  (MV_U8)Count, 
                 Addr );

    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0 != OdinSPI_WaitDataReady( pAI, 10000  ) )
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }


    if( 0 != OdinSPI_PollSR( pAI, 0x01, 0, 5000 ) )
    {
       SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
  
    return 0;
}


int
OdinSPI_Write_SST
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U8*      Data, 
    MV_U32      Count
)
{
	MV_U32 i=0;
	MV_U16 dwTmp;

    	if (0  !=OdinSPI_WREN( pAI ))
        		return -1;

	MV_CopyMemory( &dwTmp, &Data[0], 2);
	if(OdinSPI_Write_autoinc(pAI, Addr, dwTmp, 2)){
	       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
		return -1;
	}

	for(i=2; i<Count; i+=2){
		MV_CopyMemory( &dwTmp, &Data[i], 2);
		if(OdinSPI_Write_autoinc(pAI, -1L, dwTmp, 2)){
		       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
			return -1;
		}
	}
	
	if(0 != OdinSPI_WRDI(pAI)){
	       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
		return -1;
	}

	return 0;

	
}

int
OdinSPI_Write
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      Data
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;
    if(pAI->FlashID	==	SST25VF040B ){
	return OdinSPI_Write_SST(pAI, Addr, (MV_U8 *)(&Data), 4);
    }

    OdinSPI_WREN( pAI );

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, Data );

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_PROG ), 
                  0, 
                  4, 
                  Addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 != OdinSPI_WaitDataReady( pAI, 10000  ) )
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x01, 0, 5000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_WriteBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
)
{
    MV_U32  i;

    for( i=0; i<Count; i+=4 )
    {
        if( -1 == OdinSPI_Write( pAI, Addr + i, *(MV_U32*)&Data[i] ) )
        {
            SPI_DBG( ( "Write failed at %5.5x\n", Addr+i ) );
            return -1;
        }
    }
    return 0;
}

int
OdinSPI_Read
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U8       Size
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;

    if( Size > 4 )
        Size = 4;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_READ ), 
                  1, 
                  Size, 
                  Addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		for( i=0; i<Size; i++ )
        {
            Data[i] = ((MV_U8*)&dwTmp)[i];
        }
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}

int
OdinSPI_ReadBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
)
{
    MV_U32      i, j;
    MV_U32      tmpAddr, tmpData, AddrEnd;
    MV_U8       *u8 = (MV_U8*)Data;

    AddrEnd = Addr + Count;
    tmpAddr = ALIGN( Addr, 4 );
    j = ( Addr & ( MV_BIT( 2 ) - 1 ) );
    if( j > 0 )
    {
        OdinSPI_Read( pAI, tmpAddr, (MV_U8*)&tmpData, 4 );

        for( i=j; i<4; i++ )
        {
            *u8++ = ((MV_U8*)&tmpData)[i];
        }

        tmpAddr+= 4;
    }
    j = ALIGN( AddrEnd, 4 );    
    for(; tmpAddr < j; tmpAddr+=4 )
    {
        if (OdinSPI_Read(pAI, tmpAddr, (MV_U8*)&tmpData, 4) == -1) {
            SPI_DBG( ( "Read failed at %5.5x\n", tmpAddr ) );
            return -1;
        }
        *((MV_U32*)u8) = tmpData;
        u8+= 4;
    }
    if( tmpAddr < AddrEnd )
    {
        OdinSPI_Read( pAI, tmpAddr, (MV_U8*)&tmpData, 4 );
        Count = AddrEnd - tmpAddr;
        for( i=0; i<Count; i++ )
        {
            *u8++ = ( (MV_U8*)&tmpData )[i];
        }
    }
    
    return 0;
}

int
OdinSPI_RMWBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count,
    MV_U8       *Buf
)
{
MV_U32  tmpAddr;
MV_U32  SecSize;
MV_U32  i, StartOfSec;

    SecSize = pAI->FlashSectSize;
    tmpAddr = ALIGN( Addr, SecSize );
    while( Count > 0 )
    {
        StartOfSec = ( Addr - tmpAddr );
        if( ( StartOfSec>0 ) || ( Count<SecSize ) )
            OdinSPI_ReadBuf( pAI, tmpAddr, Buf, SecSize );
        if( 0!=OdinSPI_SectErase( pAI, tmpAddr ) )
            return -1;
        for( i=StartOfSec; Count>0 && i<SecSize; i++ )
        {
            Buf[i] = *Data++;
            Count--;
        }
        if( 0!=OdinSPI_WriteBuf( pAI, tmpAddr, Buf, SecSize ) )
            return -1;
        Addr+= (SecSize - StartOfSec);
        tmpAddr+= SecSize;
    }    

    return 0;
}

int
OdinSPI_SSTIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  SST_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

        switch( dwTmp )
        {
            case 0x8DBF:
               pAI->FlashID = SST25VF040B;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}


int
OdinSPI_AtmelIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  ATMEL_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x631f:
                pAI->FlashID = AT25F2048;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}

int
OdinSPI_AtmelIdentify_41A_021
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)ATMEL_SPI_CMD_41A_021[SPI_INS_RDID], 
                  1, 
                  2, 
                  -1 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
       {
            case 0x441f:
                pAI->FlashID = AT25DF041A;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
		    return 0;	
            case 0x431f:
                pAI->FlashID = AT25DF021;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
				return 0;
 	    	case 0x9d7f:
	    		pAI->FlashID = PM25LD010;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}

int
OdinSPI_WinbondIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  WINBOND_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x1212:
                pAI->FlashID = W25X40;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}

int
OdinSPI_MxicIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  MXIC_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );
		switch( dwTmp )
        {
            case 0x11C2:
                pAI->FlashID = MX25L2005;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 4L * 1024;
                return 0;
        }
    }

    return -1;
}

int
OdinSPI_EONIdentify_F20
(
	AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  EON_F20_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );
    
    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x111C:
                pAI->FlashID = EN25F20;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 4L * 1024;
                return 0;
        }
    }

    return -1;

}

int
OdinSPI_Init
(
    AdapterInfo *pAI
)
{
    MV_U32  i;
    MV_U8   *pSPIVendor;

    pSPIVendor = NULL;
    if( 0 == OdinSPI_AtmelIdentify( pAI ) )
    {
        pSPIVendor = ATMEL_SPI_CMD;
    }
    else if( 0 == OdinSPI_MxicIdentify( pAI ) )
    {
        pSPIVendor = MXIC_SPI_CMD;
    }
    	else if ( 0 == OdinSPI_AtmelIdentify_41A_021( pAI) ) {
		  pSPIVendor = ATMEL_SPI_CMD_41A_021;
	}
	else if( 0 == OdinSPI_WinbondIdentify( pAI ) )
	{
		pSPIVendor = WINBOND_SPI_CMD;
	} 
	else if( 0 == OdinSPI_SSTIdentify( pAI ) )
	{
		pSPIVendor = SST_SPI_CMD;
	}
	else if( 0 == OdinSPI_EONIdentify_F20( pAI ) )
	{
		pSPIVendor = EON_F20_SPI_CMD;
	}
	else
	{
		pSPIVendor = ATMEL_SPI_CMD;
	}

    if( pSPIVendor )
    {
        for( i=0; i<sizeof( SPICmd ); i++ )
        {
            SPICmd[i] = pSPIVendor[i];
        }    
        return 0;
    }        

    return -1;
}

int
InitHBAInfo
(
    HBA_Info_Main *pHBAInfo
)
{
    int i;
    MV_U8 *pU8;

    pU8 = (MV_U8*)pHBAInfo;
    for( i=0; i<sizeof( HBA_Info_Main ); i++ )
        pU8[i] = 0xFF;
    MV_CopyMemory( pHBAInfo->signature, "MRVL", 4 );

    return 0;
}

int
GetHBAInfoChksum
(
    HBA_Info_Main *pHBAInfo
)
{
    MV_U32  i;
    MV_U8   *pU8, cs;

    cs = 0;

    pU8 = (MV_U8*)pHBAInfo;
    for( i=0; i<sizeof( HBA_Info_Main ); i++ )
    {
        cs+= pU8[i];
    }
    return cs;
}

int
LoadHBAInfo
(
    AdapterInfo *pAI,
    HBA_Info_Main *pHBAInfo
)
{
    if( 0!=OdinSPI_ReadBuf( pAI, 
                            pAI->FlashSize-sizeof( HBA_Info_Main ), 
                            (MV_U8*)pHBAInfo, 
                            sizeof( HBA_Info_Main ) ) )
    {
        return -1;
    }
    if( 0!=GetHBAInfoChksum( pHBAInfo ) )
    {
        InitHBAInfo( pHBAInfo );
        return -1;
    }
    return 0;
}

