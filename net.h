
#define PKT_CMD (0x00)
#define PKT_VAL (0x02)

struct packet {
	uint16_t command;
	int32_t value;
};

static inline
void pack (unsigned char *b, uint16_t c, int32_t v) {
	c = htons(c);
	memcpy(&b[PKT_CMD], &c, sizeof(c));
	v = htonl(v);
	memcpy(&b[PKT_VAL], &v, sizeof(v));
}

static inline
void unpack (unsigned char *b, uint16_t *c, int32_t *v) {
	memcpy(c, &b[PKT_CMD], sizeof(uint16_t));
	*c = ntohs(*c);
	memcpy(v, &b[PKT_VAL], sizeof(int32_t));
	*v = ntohl(*v);
}

int get_socket (char *, char *);
//void pack (unsigned char *, uint16_t, int32_t);
//void unpack (unsigned char *b, uint16_t *c, int32_t *v);