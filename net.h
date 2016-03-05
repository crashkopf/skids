
#define PKT_CMD (0x00)
#define PKT_VAL (0x02)

struct packet {
	uint16_t command;
	int32_t value;
};

int get_socket (char *, char *);
void pack (unsigned char *, uint16_t, int32_t);
void unpack (unsigned char *b, uint16_t *c, int32_t *v);