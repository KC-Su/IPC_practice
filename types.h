#ifndef TYPES_H
#define TYPES_H

typedef		unsigned char			boolean_t;
/*
typedef		unsigned char			uint8_t;
typedef		unsigned short			uint16_t;
typedef		unsigned int			uint32_t;
typedef		unsigned long long		uint64_t;
typedef		char					int8_t;
typedef		short					int16_t;
typedef		int						int32_t;
typedef		long long				int64_t;
*/
typedef		unsigned char			u8;
typedef		unsigned short			u16;
typedef		unsigned short			sum16;
typedef		unsigned int			u32;
typedef		unsigned long long		u64;

typedef		unsigned short			le16;
typedef		unsigned short			be16;
typedef		unsigned int			le32;
typedef		unsigned int			be32;
typedef		unsigned long long		le64;
typedef		unsigned long long		be64;

typedef struct {
	u8		version;			// 4 for IPv4, 6 for IPv6
	u8		address[16];		// pad zero for IPv4
} ipAddress_t;

typedef struct {
    u8    version;      // 4 for IPv4, 6 for IPv6
    u8    length;       // prefix length
    u8    address[16];      // pad zero for IPv4
} ipPrefix_t;

typedef struct {
  u8    mcc2:4;
  u8    mcc1:4;
  u8    mnc1:4;
  u8    mcc3:4;
  u8    mnc3:4;
  u8    mnc2:4;
  u16   lac;
  u8    rac;
} __attribute__((__packed__)) rai_t;

typedef struct {
  u8    mcc2:4;
  u8    mcc1:4;
  u8    mnc1:4;
  u8    mcc3:4;
  u8    mnc3:4;
  u8    mnc2:4;
  u16   lac;
  u16   cgi;
} __attribute__((__packed__)) loc_t;

//Phil_Task2266
typedef struct {
  u16  sid;
  u16  nid;
  u16  cell;
} __attribute__((__packed__)) bsid_t;

extern void meta_dtype_output(int dtype_id, char *buf, void *value);
extern int meta_dtype_check_input(int dtype_id, char *buf);
extern int meta_ie_element_size(int index);

#endif
