/*
	sabertooth.h
*/

#define ST_START (0xAA) // Start byte must be 10101010b
#define ST_ADDR_OFFSET (0x80) // Device addresses start at 128
#define ST_CHKSUM_MASK (0x7F)
#define ST_DRIVE_MASK (0x7F)
#define ST_FWD_A (0x00)
#define ST_REV_A (0x01)
#define ST_FWD_B (0x04)
#define ST_REV_B (0x05)
#define ST_DRV_FWD (0x08)
#define ST_DRV_REV (0x09)
#define ST_TRN_RHT (0x0a)
#define ST_TRN_LFT (0x0b)

typedef struct {
	uint8_t addr;
	uint8_t cmd;
	uint8_t data;
	uint8_t chksum;
} st_packet;

/* 
	This function builds a command packet
	Use static inline function instead of a macro to guarantee type
*/
static inline
int st_command (void * b, uint8_t addr, uint8_t cmd, uint8_t data) {
	((st_packet *) b)->addr = addr | ST_ADDR_OFFSET;
	((st_packet *) b)->cmd = cmd;
	((st_packet *) b)->data = data & ST_DRIVE_MASK;
	((st_packet *) b)->chksum = (addr + cmd + data) & ST_CHKSUM_MASK;
	
	return sizeof(st_packet);
}