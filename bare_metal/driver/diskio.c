/*-----------------------------------------------------------------------*/
/* MMCv3/SDv1/SDv2/SDHC (in SD mode) control module                     */
/*-----------------------------------------------------------------------*/
/*
 *  Copyright (C) 2014, ChaN, all right reserved.
 *
 * * This software is a free software and there is NO WARRANTY.
 * * No restriction on use. You can use, modify and redistribute it for
 *   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 * * Redistributions of source code must retain the above copyright notice.
 *
 * Copyright (c) 2015, University of Cambridge.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Cambridge nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * IN NO EVENT SHALL UNIVERSITY OF CAMBRIDGE BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * UNIVERSITY OF CAMBRIDGE SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY,
 * PROVIDED HEREUNDER IS PROVIDED "AS IS". UNIVERSITY OF CAMBRIDGE HAS NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 */
/*------------------------------------------------------------------------*/

#include "diskio.h"
#include "minion_lib.h"

/*--------------------------------------------------------------------------

  Module Private Functions

  ---------------------------------------------------------------------------*/

static volatile
DSTATUS Stat = STA_NOINIT;  /* Disk status */

static
uint8_t CardType;          /* Card type flags */


/*-----------------------------------------------------------------------*/
/* Power Control  (Platform dependent)                                   */
/*-----------------------------------------------------------------------*/
/* When the target system does not support socket power control, there   */
/* is nothing to do in these functions and chk_power always returns 1.   */

static
void power_on (void)
{

}

static
void power_off (void)
{

}



/*--------------------------------------------------------------------------

  Public Functions

  ---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
                         uint8_t pdrv       /* Physical drive nmuber (0) */
                         )
{
  uint8_t n, cmd, ty, ocr[4];
  uint32_t timeout;

  if (pdrv) return STA_NOINIT;        /* Supports only single drive */
  init_sd();

  ty = CT_SD2|CT_BLOCK;
  CardType = ty;
  sd_deselect();

  if (ty) {              /* Initialization succeded */
    Stat &= ~STA_NOINIT; /* Clear STA_NOINIT */
  } else {               /* Initialization failed */
    power_off();
  }

  return Stat;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
                     uint8_t pdrv       /* Physical drive nmuber (0) */
                     )
{
  if (pdrv) return STA_NOINIT;    /* Supports only single drive */
  return Stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
                   uint8_t pdrv,          /* Physical drive nmuber (0) */
                   uint8_t *buff,         /* Pointer to the data buffer to store read data */
                   uint32_t sector,       /* Start sector number (LBA) */
                   uint32_t count          /* Sector count (1..128) */
                   )
{
  uint8_t cmd;


  if (pdrv || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  if (!(CardType & CT_BLOCK)) sector *= 512;  /* Convert to byte address if needed */

    do {
    sd_read_sector(sector, buff, 512);
      buff += 512;
    sector++;
    } while (--count);
  sd_deselect();

  return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
                    uint8_t pdrv,          /* Physical drive nmuber (0) */
                    const uint8_t *buff,   /* Pointer to the data to be written */
                    uint32_t sector,       /* Start sector number (LBA) */
                    uint32_t count          /* Sector count (1..128) */
                    )
{
  if (pdrv || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;
  if (Stat & STA_PROTECT) return RES_WRPRT;

  if (!(CardType & CT_BLOCK)) sector *= 512;  /* Convert to byte address if needed */

  if (count == 1) {   /* Single block write */
    if ((send_cmd(CMD24, sector, 0) == 0)  /* WRITE_BLOCK */
        && xmit_datablock(buff, 0xFE))
      count = 0;
  }
  else {              /* Multiple block write */
    if (CardType & CT_SDC) send_cmd(ACMD23, count, 0);
    if (send_cmd(CMD25, sector, 0) == 0) { /* WRITE_MULTIPLE_BLOCK */
      do {
        if (!xmit_datablock(buff, 0xFC)) break;
        buff += 512;
      } while (--count);
      if (!xmit_datablock(0, 0xFD))   /* STOP_TRAN token */
        count = 1;
    }
  }
  sd_deselect();

  return count ? RES_ERROR : RES_OK;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
                    uint8_t pdrv,      /* Physical drive nmuber (0) */
                    uint8_t cmd,       /* Control code */
                    void *buff      /* Buffer to send/receive control data */
                    )
{
  DRESULT res;
  uint8_t n, csd[16], *ptr = buff;
  uint32_t csize;


  if (pdrv) return RES_PARERR;

  res = RES_ERROR;

  if (Stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd) {
  case CTRL_SYNC :        /* Make sure that no pending write process. Do not remove this or written sector might not left updated. */
    if (sd_select()) res = RES_OK;
    break;

  case GET_SECTOR_COUNT : /* Get number of sectors on the disk (uint32_t) */
    if ((send_cmd(CMD9, 0, 0) == 0) && rcvr_datablock(csd, 16)) {
      if ((csd[0] >> 6) == 1) {   /* SDC ver 2.00 */
        csize = csd[9] + ((uint16_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
        *(uint32_t*)buff = csize << 10;
      } else {                    /* SDC ver 1.XX or MMC*/
        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
        *(uint32_t*)buff = csize << (n - 9);
      }
      res = RES_OK;
    }
    break;

  case GET_BLOCK_SIZE :   /* Get erase block size in unit of sector (uint32_t) */
    if (CardType & CT_SD2) {    /* SDv2? */
      if (send_cmd(ACMD13, 0, 0) == 0) { /* Read SD status */
        if (rcvr_datablock(csd, 16)) {              /* Read partial block */
          queue_block_read1();
          *(uint32_t*)buff = 16UL << (csd[10] >> 4);
          res = RES_OK;
        }
      }
    } else {                    /* SDv1 or MMCv3 */
      if ((send_cmd(CMD9, 0, 0) == 0) && rcvr_datablock(csd, 16)) {  /* Read CSD */
        if (CardType & CT_SD1) {    /* SDv1 */
          *(uint32_t*)buff = (((csd[10] & 63) << 1) + ((uint16_t)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
        } else {                    /* MMCv3 */
          *(uint32_t*)buff = ((uint16_t)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
        }
        res = RES_OK;
      }
    }
    break;

    /* Following commands are never used by FatFs module */

  case MMC_GET_TYPE :     /* Get card type flags (1 byte) */
    *ptr = CardType;
    res = RES_OK;
    break;

  case MMC_GET_CSD :      /* Receive CSD as a data block (16 bytes) */
    if (send_cmd(CMD9, 0, 0) == 0      /* READ_CSD */
        && rcvr_datablock(ptr, 16))
      res = RES_OK;
    break;

  case MMC_GET_CID :      /* Receive CID as a data block (16 bytes) */
    if (send_cmd(CMD10, 0, 0) == 0     /* READ_CID */
        && rcvr_datablock(ptr, 16))
      res = RES_OK;
    break;

  case MMC_GET_OCR :      /* Receive OCR as an R3 resp (4 bytes) */
    if (send_cmd(CMD58, 0, 0) == 0) {  /* READ_OCR */
      unsigned rslt = sd_resp(0);
      for (n = 0; n < 4; n++) *ptr++ = rslt >> (24-n*8);
      res = RES_OK;
    }
    break;

  case MMC_GET_SDSTAT :   /* Receive SD statsu as a data block (64 bytes) */
    if (send_cmd(ACMD13, 0, 0) == 0) { /* SD_STATUS */
      if (rcvr_datablock(ptr, 64))
        res = RES_OK;
    }
    break;

  case CTRL_POWER_OFF :   /* Power off */
    power_off();
    Stat |= STA_NOINIT;
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
  }

  sd_deselect();

  return res;
}
#endif
