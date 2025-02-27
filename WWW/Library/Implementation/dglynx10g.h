//--------+------------+-------------+---------------------------------------
//-- Filename     dglynx10g.h
//-- Title        General library for dglynx10g
//-- Company      Design Gateway
//--
//--  Ver |    Date    |  Author     | Remark
//--------+------------+-------------+---------------------------------------
//-- 1.0  | 2024/08/13 | I.Taksaporn | New Creation
//---------------------------------------------------------------------------

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include "time.h"
#include <stdlib.h>
#include <HTTCP.h>

#define	IPTMO_SET               322265625   // Timeout 1 sec (@322.26 MHz)
#define	ACTIVE                  1           // Active open/Close port
#define	PASSIVE                 0           // Passive open/Close port
#define	PORT_OPEN               1           // Open port
#define	PORT_CLOSE              0           // Close port
#define	CONNON                  1           // Connection on
#define	CONNOFF                 0           // Connection off

#define CMD_ACT_OPEN            0x00000001
#define CMD_PAS_OPEN            0x00000002
#define CMD_ACT_CLOSE           0x00000000

#define TOE_STS_INITFIN         0x00000001
#define TOE_STS_CONNON          0x00000002
#define TOE_STS_INTERRUPT       0x00000004
#define TOE_STS_BUSY            0x00000008
#define TOE_NO_ARP_REPLY        0x00000001
#define EMAC_LINKUP_STS         0x00000001

#define TMO_TRANSFER_DATA       10*TM_1SEC
#define TM_1SEC                 1
#define TM_1uS                  1

#define BASE_ADDR               0xA0000000
#define TOE_BASE_ADDR           0xA0080000

#define TOE_RST_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0000)
#define TOE_OPM_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0004)
#define TOE_SML_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0008)
#define TOE_SMH_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x000C)
#define TOE_DMIL_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x0010)
#define TOE_DMIH_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x0014)
#define TOE_SIP_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0018)
#define TOE_DIP_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x001C)
#define TOE_TMO_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0020)
#define TOE_TIC_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0024)
#define TOE_CMD_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0030)
#define TOE_SPN_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0034)
#define TOE_DPN_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0038)
#define TOE_VER_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0040)
#define TOE_STS_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0044)
#define TOE_INT_INTREG          (uint32_t*)(TOE_BASE_ADDR+0x0048)
#define TOE_DMOL_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x004C)
#define TOE_DMOH_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x0050)
#define EMAC_VER_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x0060)
#define EMAC_STS_INTREG         (uint32_t*)(TOE_BASE_ADDR+0x0064)
#define HW_ACCESS_REG           (uint32_t*)(TOE_BASE_ADDR+0x0070)

// Function to read from a memory-mapped address
uint32_t regRead(uint32_t * mem_ptr, uint32_t * value);
// Function to write to a memory-mapped address
uint32_t regWrite(uint32_t * mem_ptr, uint32_t value);

uint32_t init_param(struct sockaddr_in *soc_in);
uint32_t read_conon(void);
int check_ethlink(void);
int exec_port(uint32_t port_ctl, uint32_t mode_active);

extern BOOLEAN hasHwAccess;