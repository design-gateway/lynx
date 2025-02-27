//--------+------------+-------------+---------------------------------------
//-- Filename     dglynx10g.c
//-- Title        General library for dglynx10g
//-- Company      Design Gateway
//--
//--  Ver |    Date    |  Author     | Remark
//--------+------------+-------------+---------------------------------------
//-- 1.0  | 2024/08/13 | I.Taksaporn | New Creation
//---------------------------------------------------------------------------

#include "dglynx10g.h"
#include <time.h>
#include <stdlib.h>

uint32_t regRead(uint32_t * mem_ptr, uint32_t * value){
    
    const char *econn = getenv("ECONN");
    if ( (econn!=NULL) && strcmp("10",econn)==0 ){
        int mem_fd;
        void *base;
        uint32_t *offset;
        long pagesize;
        
        off_t addr = (off_t)mem_ptr;
        
        // Get the page size
        pagesize = sysconf(_SC_PAGE_SIZE);
        if (pagesize <= 0) {
            perror("sysconf");
            return -1;
        }
        // Open /dev/mem
        mem_fd = open("/dev/mem", O_RDONLY);
        if (mem_fd < 0) {
            perror("open /dev/mem failed");
            return -1;
        }
        // Memory map the specified address
        base = mmap(NULL, pagesize, PROT_READ, MAP_SHARED, mem_fd, addr & ~(pagesize - 1));
        if (base == MAP_FAILED) {
            perror("mmap failed");
            close(mem_fd); // Close the file descriptor before returning
            return -1;
        }
        // Calculate the offset within the page
        offset = (uint32_t *)( (uint64_t)base + (addr & (pagesize - 1)));
        // read data
        *value = *(volatile uint32_t *)offset;
        // Unmap the memory
        if (munmap(base, pagesize)) {
            perror("munmap failed");
        }
        // Close the file descriptor
        if (close(mem_fd)) {
            perror("cannot close /dev/mem");
        }
    }
    return 0;
}

uint32_t regWrite(uint32_t * mem_ptr, uint32_t value)
{
    const char *econn = getenv("ECONN");
    if ( (econn!=NULL) && strcmp("10",econn)==0 ){
        int mem_fd;
        void *base;
        uint32_t *offset;
        long pagesize;
        
        off_t addr = (off_t)mem_ptr;
        
        // Get the page size
        pagesize = sysconf(_SC_PAGE_SIZE);
        if (pagesize <= 0) {
            perror("sysconf");
            return -1;
        }

        // Open /dev/mem for reading and writing
        mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (mem_fd < 0) {
            perror("open /dev/mem failed");
            return -1;
        }

        // Memory map the specified address with read and write permissions
        base = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr & ~(pagesize - 1));
        if (base == MAP_FAILED) {
            perror("mmap failed");
            close(mem_fd); // Close the file descriptor before returning
            return -1;
        }

        // Calculate the offset within the page
        offset = (uint32_t *)( (uint64_t)base + (addr & (pagesize - 1)));
        // Write the value
        *(volatile uint32_t *)offset = (uint32_t)value;
    
        // Unmap the memory
        if (munmap(base, pagesize)) {
            perror("munmap failed");
        }

        // Close the file descriptor
        if (close(mem_fd)) {
            perror("cannot close /dev/mem");
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
// initialize TOE by setting network parameters
// Return	: 0 -> initialization is completed
//			:-1 -> initialization is failed
uint32_t init_param(struct sockaddr_in *soc_in){
    uint32_t	temp32b;
    struct  timespec start_timestamp, current_timestamp;

    char line[100];               // Buffer to store each line
    int count = 0;                // Counter for IP addresses
    
    uint8_t	src_ip_set[4]	 = {192, 168, 11, 42};					// FPGA IP address (Client)
    uint8_t	mac_addr_set[6] = {0x80, 0x11, 0x22, 0x33, 0x44, 0x55};	// FPGA MAC address
    uint8_t	gateway_ip_set[4]= {192, 168, 11, 1};					// Gateway IP address
    uint8_t	gateway_port_set[2]	= {0x01, 0xBB};	
    
    
    // get FPGA IP address (Client) and gateway IP address from DG.cfg
    FILE *file = fopen("/home/ubuntu/tool/lynx/etc/DG.cfg", "r");
    if (file) {
            int temp[6];
            int valid = 1;
        // Read the file line by line
        while (fgets(line, sizeof(line), file) != NULL) {
            if (count >= 3) {  // Check for array bounds
                fprintf(stderr, "Error: Too many entries in the file. Only 3 are allowed.\n");
                break;
            }

            // Find the colon character
            char *colon = strchr(line, ':');
            if (colon == NULL) {
                fprintf(stderr, "Error: Invalid line format (missing ':') in line: %s", line);
                continue;
            }

            // Extract the name and IP address
            *colon = '\0';                          // Split the string into name and IP
            char *name = line;                      // Name is before the colon
            char *ipStr = colon + 1;                // IP is after the colon
            ipStr[strcspn(ipStr, "\r\n")] = '\0';   // Remove newline characters

            if (strcmp(name, "FPGA_MAC") == 0) {
                // Parse the MAC address into an array of uint8_t
                
                if (sscanf(ipStr, "%x.%x.%x.%x.%x.%x", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6) {
                    fprintf(stderr, "Error: Invalid MAC address format: %s\n", ipStr);
                    continue;
                }

                // Validate that each part of the MAC is within the range [0, 255]
                valid = 1;
                for (int i = 0; i < 6; i++) {
                    if (temp[i] < 0 || temp[i] > 255) {
                        valid = 0;
                        break;
                    }
                    mac_addr_set[i] = (uint8_t)temp[i];
                }

                if (!valid) {
                    fprintf(stderr, "Error: MAC address out of range: %s\n", ipStr);
                    continue;
                }
            } else {
                // Parse the IP address into an array of uint8_t
                if (sscanf(ipStr, "%d.%d.%d.%d", &temp[0], &temp[1], &temp[2], &temp[3]) != 4) {
                    fprintf(stderr, "Error: Invalid IP address format: %s\n", ipStr);
                    continue;
                }

                // Validate that each part of the IP is within the range [0, 255]
                valid = 1;
                for (int i = 0; i < 4; i++) {
                    if (temp[i] < 0 || temp[i] > 255) {
                        valid = 0;
                        break;
                    }

                    if (strcmp(name, "GATEWAY_IP") == 0) {
                        gateway_ip_set[i] = (uint8_t)temp[i];
                    } else if (strcmp(name, "FPGA_IP") == 0) {
                        src_ip_set[i] = (uint8_t)temp[i];
                    }
                }

                if (!valid) {
                    fprintf(stderr, "Error: IP address out of range: %s\n", ipStr);
                    continue;
                }
            }

            count++;
            
        }
        // Close the file
        fclose(file);

    } else {
        fprintf(stderr, "Error: Could not found DG.cfg\n");
        return -1;
    }

    // source network parameters
    uint8_t	src_port_set[2] = {0xC0, 0x00};							// FPGA port number=49153 (Client)
    srand(time(NULL)); 
    src_port_set[1] = (rand())&0xFF;

    // destination network parameters
    uint8_t	dst_ip_set[4];											// Target IP address (Server)
    dst_ip_set[0] = (int) *((unsigned char *) (&soc_in->sin_addr) + 0);
    dst_ip_set[1] = (int) *((unsigned char *) (&soc_in->sin_addr) + 1);
    dst_ip_set[2] = (int) *((unsigned char *) (&soc_in->sin_addr) + 2);
    dst_ip_set[3] = (int) *((unsigned char *) (&soc_in->sin_addr) + 3);
    uint8_t	dst_port_set[2]	= {0xEA, 0x61};							// Target port number=60001 (Server)
    dst_port_set[0] = (soc_in->sin_port)&0xFF;
    dst_port_set[1] = ((soc_in->sin_port)&0xFF00)>>8;

    uint32_t	gateway_mac_set[2];	//Gateway's MAC address

    // goto connect_to_target;
    regWrite(TOE_RST_INTREG, 1);	// TOEIP parameter initialization
    // 2-bit dst mac mode and 1-bit ARP ICMP Enable 
    // set enable ARP / ICMP reply
    regWrite(TOE_OPM_INTREG, (1<<16) );
    // FPGA param
    // 32-bit lower FPGA MAC address
    regWrite(TOE_SML_INTREG, (mac_addr_set[2]<<24) | (mac_addr_set[3]<<16) | (mac_addr_set[4]<<8) | mac_addr_set[5]);
    // 16-bit upper FPGA MAC address
    regWrite(TOE_SMH_INTREG, (mac_addr_set[0]<<8) | mac_addr_set[1]);
    // 32-bit FPGA IP address
    regWrite(TOE_SIP_INTREG, (src_ip_set[0]<<24) | (src_ip_set[1]<<16) | (src_ip_set[2]<<8) | src_ip_set[3]);
    // 16-bit FPGA port number
    regWrite(TOE_SPN_INTREG, (src_port_set[0]<<8) | src_port_set[1]);
    // 32-bit target IP address
    regWrite(TOE_DIP_INTREG, (dst_ip_set[0]<<24) | (dst_ip_set[1]<<16) | (dst_ip_set[2]<<8) | dst_ip_set[3]);
    // 16-bit target port number
    regWrite(TOE_DPN_INTREG, (dst_port_set[0]<<8) | dst_port_set[1]);
    // timeout set in firmware
    regWrite(TOE_TMO_INTREG, IPTMO_SET);
    // Release reset
    regWrite(TOE_RST_INTREG, 0);	// Release reset to start parameter initialize
    
    // Wait IP Initialization
    clock_gettime(CLOCK_REALTIME, &start_timestamp);

    // Force clear to register that latch the interrupt value 
    //to clear all previous interrupt before checking IP status
    regWrite(TOE_TIC_INTREG, 1);
        
    // Check InitFinish flag=1
    regRead(TOE_STS_INTREG, &temp32b);
    while ( (temp32b & TOE_STS_INITFIN) == 0 )	{
        // Print interrupt status when TCP-IP Interrupt is asserted
        regRead(TOE_STS_INTREG, &temp32b);
        if ( (temp32b & TOE_STS_INTERRUPT)!=0 ) {
            regRead(TOE_INT_INTREG, &temp32b);
            if ( (temp32b & TOE_NO_ARP_REPLY) != 0 ){
                goto connect_to_gateway;
            } else {
                // Clear interrupt flag stat
                regWrite(TOE_TIC_INTREG, 1);
                return -1;
            }
            // Clear interrupt flag stat
            regWrite(TOE_TIC_INTREG, 1);
        } else {
            // printf("\r\n");
        }
        clock_gettime(CLOCK_REALTIME, &current_timestamp);
        if ( current_timestamp.tv_sec-start_timestamp.tv_sec > TMO_TRANSFER_DATA){
            return -1;
        }
        // Check InitFinish flag=1
        regRead(TOE_STS_INTREG, &temp32b);
    }
    return 0;

    connect_to_gateway:
        //send APR to gateway
    
        regWrite(TOE_RST_INTREG, 1);	// TOEIP parameter initialization
        
        // 32-bit target IP address
        regWrite(TOE_DIP_INTREG, (gateway_ip_set[0]<<24) | (gateway_ip_set[1]<<16) | (gateway_ip_set[2]<<8) | gateway_ip_set[3]);
        // 16-bit target port number
        regWrite(TOE_DPN_INTREG, (gateway_port_set[0]<<8) | gateway_port_set[1]);
        // Release reset
        regWrite(TOE_RST_INTREG, 0);	// Release reset to start parameter initialize
        // Wait IP Initialization
        // Set timer
        clock_gettime(CLOCK_REALTIME, &start_timestamp);
        // Force clear to register that latch the interrupt value 
        //to clear all previous interrupt before checking IP status
        regWrite(TOE_TIC_INTREG, 1);		
        // Check busy flag
        regRead(TOE_STS_INTREG, &temp32b);
        // Check InitFinish flag=1
        regRead(TOE_STS_INTREG, &temp32b);
        while ( (temp32b & TOE_STS_INITFIN) == 0 )	{
            // Print interrupt status when TCP-IP Interrupt is asserted
            regRead(TOE_STS_INTREG, &temp32b);
            if ( (temp32b & TOE_STS_INTERRUPT)!=0 ) {
                printf("\r\nCannot reach gateway\r\n");
                // Clear interrupt flag stat
                regWrite(TOE_TIC_INTREG, 1);
                return -1;
            }

            // Clear interrupt flag stat
            regWrite(TOE_TIC_INTREG, 1);
            clock_gettime(CLOCK_REALTIME, &current_timestamp);
            if ( current_timestamp.tv_sec-start_timestamp.tv_sec > TMO_TRANSFER_DATA){
                return 0;
            }

            // Check InitFinish flag=1
            regRead(TOE_STS_INTREG, &temp32b);
        }
    
    connect_to_target:
        // get gateway's mac address
        regRead(TOE_DMOH_INTREG, &gateway_mac_set[0]);
        regRead(TOE_DMOL_INTREG, &gateway_mac_set[1]);

        regWrite(TOE_RST_INTREG, 1);	// TOEIP parameter initialization
        // 2-bit dst mac mode and 1-bit ARP ICMP Enable 
        // set enable ARP / ICMP reply and set fix MAC mode
        regWrite(TOE_OPM_INTREG, (1<<16 | 2) );
        // FPGA param
        // 32-bit lower FPGA MAC address
        regWrite(TOE_SML_INTREG, (mac_addr_set[2]<<24) | (mac_addr_set[3]<<16) | (mac_addr_set[4]<<8) | mac_addr_set[5]);
        // 16-bit upper FPGA MAC address
        regWrite(TOE_SMH_INTREG, (mac_addr_set[0]<<8) | mac_addr_set[1]);
        // 32-bit FPGA IP address
        regWrite(TOE_SIP_INTREG, (src_ip_set[0]<<24) | (src_ip_set[1]<<16) | (src_ip_set[2]<<8) | src_ip_set[3]);
        // 16-bit FPGA port number
        regWrite(TOE_SPN_INTREG, (src_port_set[0]<<8) | src_port_set[1]);
        // 32-bit target IP address
        regWrite(TOE_DIP_INTREG, (dst_ip_set[0]<<24) | (dst_ip_set[1]<<16) | (dst_ip_set[2]<<8) | dst_ip_set[3]);
        // 16-bit target port number
        regWrite(TOE_DPN_INTREG, (dst_port_set[0]<<8) | dst_port_set[1]);
        regWrite(TOE_DMIH_INTREG, gateway_mac_set[0]);
        regWrite(TOE_DMIL_INTREG, gateway_mac_set[1]);
        // timeout set in firmware
        regWrite(TOE_TMO_INTREG, IPTMO_SET);
        regWrite(TOE_RST_INTREG, 0);	// Release reset to start parameter initialize
        // Wait IP Initialization
        // Set timer
        clock_gettime(CLOCK_REALTIME, &start_timestamp);
        // Force clear to register that latch the interrupt value 
        //to clear all previous interrupt before checking IP status
        regWrite(TOE_TIC_INTREG, 1);

        // Check InitFinish flag=1
        regRead(TOE_STS_INTREG, &temp32b);
        while ( (temp32b & TOE_STS_INITFIN) == 0 )	{
            // Print interrupt status when TCP-IP Interrupt is asserted
            regRead(TOE_STS_INTREG, &temp32b);
            printf("TOE_STS_INTREG = %08x\r\n",temp32b);
            if ( (temp32b & TOE_STS_INTERRUPT)!=0 ) {
                // Clear interrupt flag stat
                regWrite(TOE_TIC_INTREG, 1);
                printf("No ARP reply\r\n");
                return -1;
            }
            clock_gettime(CLOCK_REALTIME, &current_timestamp);
            if ( current_timestamp.tv_sec-start_timestamp.tv_sec > TMO_TRANSFER_DATA){
                return 0;
            }
            // Check InitFinish flag=1
            regRead(TOE_STS_INTREG, &temp32b);
        }
    return 0;
}
//---------------------------------------------------------------------------
// Send TOE command to open or close a TCP connection
// Returns: 0  -> Command completed successfully
//         -1  -> Command failed
int exec_port(uint32_t port_ctl, uint32_t mode_active)
{
    uint32_t 	temp32b;
    struct  timespec start_timestamp, current_timestamp;
    
    // Set timer
    clock_gettime(CLOCK_REALTIME, &start_timestamp);

    do{
        clock_gettime(CLOCK_REALTIME, &current_timestamp);
        if ( current_timestamp.tv_sec-start_timestamp.tv_sec > TMO_TRANSFER_DATA) return -1;
        regRead(TOE_STS_INTREG, &temp32b);
    } while ( (temp32b & TOE_STS_BUSY) != 0 );
    
    // assign port to open or close
    if ( port_ctl==PORT_OPEN ) {				// Open port
        // Active open
        if ( mode_active==ACTIVE ) {
            regWrite(TOE_CMD_INTREG, CMD_ACT_OPEN);
        }
        // Passive open
        else {
            regWrite(TOE_CMD_INTREG, CMD_PAS_OPEN);
        }
    }
    // Close port
    else {										// Close port
        if ( mode_active==ACTIVE ) {	
            // Active Close
            regWrite(TOE_CMD_INTREG, CMD_ACT_CLOSE);
        }
    }
    //----------------------------------------------------------------	
    // Wait Open Connection
    clock_gettime(CLOCK_REALTIME, &start_timestamp);
    // Clear previous interrupt before close connection
    regWrite(TOE_TIC_INTREG, 1);

    // (ConnOn='0' for port closing, '1' for port opening)
    while ( (port_ctl==PORT_CLOSE && read_conon()==CONNON) ||
            (port_ctl==PORT_OPEN  && read_conon()==CONNOFF) ) {
        // Print interrupt status when TCP-IP Interrupt is asserted
        regRead(TOE_STS_INTREG, &temp32b);
        if ( (temp32b & TOE_STS_INTERRUPT)!=0 ) {
            // Show RAW value of Interrupt state
            // After TOE_INT_INTREG is read, it will auto clear TCP-IP Interrupt in TOE_STS_INTREG
            regRead(TOE_INT_INTREG, &temp32b);
            printf("\r\nTCP-IP Interrupt code : 0x%08x\r", temp32b);
            // Clear interrupt flag stat
            regWrite(TOE_TIC_INTREG, 1);
        }
        clock_gettime(CLOCK_REALTIME, &current_timestamp);
        if ( current_timestamp.tv_sec-start_timestamp.tv_sec > TMO_TRANSFER_DATA){
            printf("Timeout for waiting open connection\r\n");
            regWrite(TOE_RST_INTREG, 1);
            regWrite(TOE_RST_INTREG, 0);
            return -1;
        }
    }

    return(0);
}
//---------------------------------------------------------------------------
// Check connection
// Return 	: 0 -> Connection OFF
//			: 1 -> Connection ON
uint32_t read_conon(void) {
    uint32_t	temp32b;
    regRead(TOE_STS_INTREG, &temp32b);

    return (temp32b & TOE_STS_CONNON)>>1;
}
//---------------------------------------------------------------------------
// Check Ethernet link status
int check_ethlink(void)
{
    uint32_t	temp32b;
    regRead(EMAC_STS_INTREG, &temp32b);

    return temp32b&EMAC_LINKUP_STS ;
}