/*
 * zeroskip-priv.h
 *
 * This file is part of skiplistdb.
 *
 * skiplistdb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 */

#ifndef _ZEROSKIP_PRIV_H_
#define _ZEROSKIP_PRIV_H_

#include "cstring.h"
#include "mappedfile.h"
#include "util.h"

#include <uuid/uuid.h>

/*
 * Zeroskip db files have the following file naming scheme:
 *   zeroskip-$(UUID)-$(index)                     - for an unpacked file
 *   zeroskip-$(UUID)-$(startindex)-$(endindex)    - for a packed filed
 *
 * The UUID, startindex and endindex values are in the header of each file.
 * The index starts with a 0, for a completely new Zeroskip DB. And is
 * incremented every time a file is finalsed(packed).
 */
#define ZS_FNAME_PREFIX       "zeroskip-"
#define ZS_FNAME_PREFIX_LEN   9
#define ZS_SIGNATURE          0x5a45524f534b4950 /* "ZEROSKIP" */
#define ZS_VERSION            1

/* This is the size of the unparssed uuid string */
#define UUID_STRLEN  36


/*
 *  Zeroskip on-disk file format:
 *
 *  [Header]([Key|Value]+[Commit])+[Pointers][Commit]
 */

/**
 * zeroskip header.
 */
/* Header offsets */
enum {
        ZS_HDR      = 0,
        ZS_VER      = 8,
        ZS_UUID     = 12,
        ZS_SIDX     = 28,
        ZS_EIDX     = 32,
        ZS_CRC      = 36,
};

struct zs_header {
        uint64_t signature;         /* Signature */
        uint32_t version;           /* Version Number */
        uuid_t   uuid;              /* UUID of DB - 128 bits: unsigned char uuid_t[16];*/
        uint32_t startidx;          /* Start Index of DB range */
        uint32_t endidx;            /* End Index of DB range */
        uint32_t crc32;             /* CRC32 of rest of header */
};

#define ZS_HDR_SIZE      sizeof(struct zs_header)


/**
 * Zeroskip .zsdb
 */
struct dotzsdb {
        uint64_t signature;
        uint32_t curidx;
        char uuidstr[36];
};                              /* A total of 48 bytes */
#define DOTZSDB_FNAME ".zsdb"
#define DOTZSDB_SIZE  sizeof(struct dotzsdb)



/* Types of files in the DB */
enum db_ftype_t {
        DB_FTYPE_ACTIVE,
        DB_FTYPE_FINALISED,
        DB_FTYPE_PACKED,
        DB_FTYPE_UNKNOWN,
};

/**
 * Zeroskip record[key|value|commit]
 */
enum record_t {
        REC_TYPE_UNUSED              =  0,
        REC_TYPE_KEY                 =  1,
        REC_TYPE_VALUE               =  2,
        REC_TYPE_COMMIT              =  4,
        REC_TYPE_2ND_HALF_COMMIT     =  8,
        REC_TYPE_FINAL               = 16,
        REC_TYPE_LONG                = 32,
        REC_TYPE_DELETED             = 64,
        REC_TYPE_LONG_KEY            = REC_TYPE_KEY | REC_TYPE_LONG,
        REC_TYPE_LONG_VALUE          = REC_TYPE_VALUE | REC_TYPE_LONG,
        REC_TYPE_LONG_COMMIT         = REC_TYPE_COMMIT | REC_TYPE_LONG,
        REC_TYPE_LONG_FINAL          = REC_TYPE_FINAL | REC_TYPE_LONG,
};


#define ZS_KEY_BASE_REC_SIZE 24
#define ZS_VAL_BASE_REC_SIZE 16

struct zs_key {
        uint8_t type;
        uint64_t length;
        uint64_t val_offset;
        uint8_t *data;
};

struct zs_val {
        uint8_t type;
        uint64_t length;
        uint8_t *data;
};

struct zs_short_key {
        uint8_t  type;
        uint16_t length;
        uint64_t val_offset;
        uint8_t  *data;
};

struct zs_long_key {
        uint8_t  type;
        uint64_t length;
        uint64_t val_offset;
        uint8_t  *data;
};

struct zs_short_val {
        uint8_t  type;
        uint32_t length;
        uint8_t  *data;
};

struct zs_long_val {
        uint8_t  type;
        uint64_t length;
        uint8_t  *data;
};

struct zs_short_commit {
        uint8_t type;
        uint32_t length;
        uint32_t crc32;
};

struct zs_long_commit {
        uint8_t type1;
        uint8_t  padding1[7];
        uint64_t length;
        uint8_t type2;
        uint8_t  padding2[3];
        uint32_t crc32;
};

#define MAX_SHORT_KEY_LEN 65535
#define MAX_SHORT_VAL_LEN 16777215

struct zs_rec {
        uint8_t type;
        union {
                struct zs_short_key    skey;
                struct zs_long_key     lkey;
                struct zs_short_val    sval;
                struct zs_long_val     lval;
                struct zs_short_commit scommit;
                struct zs_long_commit  lcommit;
        } rec;
};

/**
 * Pointers
 */
struct zs_pointer {
        uint64_t      num_ptrs;
        uint64_t      num_shadowed_recs;
        uint64_t      num_shadowed_bytes;
        struct zs_rec *key_ptr;
};

/**
 * Trasaction structure
 **/
struct txn {
        int num;
};

/**
 * Mapped Zeroskip db files
 */
struct zsdb_file {
        enum db_ftype_t ftype;
        cstring fname;
        int is_open;
        struct mappedfile *mf;
        struct zs_header header;
};

struct zsdb_files {
        struct zsdb_file **fdata;
        int count;
};

/*
 * zeroskip private data
 */
struct zsdb_priv {
        uuid_t uuid;
        struct dotzsdb dotzsdb;

        struct zsdb_file factive; /* The currently active file */
        struct zsdb_file fpacked; /* The packed file */
        struct zsdb_file *ffinalised;

        struct btree *btree;

        cstring dbdir;

        unsigned int is_open:1;
        unsigned int valid:1;

        size_t end;
};

/* zeroskip-header.c */
extern int zs_header_write(struct zsdb_file *f);
extern int zs_header_validate(struct zsdb_file *f);

/* zeroskip-dotzsdb.c */
extern int zs_dotzsdb_create(struct zsdb_priv *priv);
extern int zs_dotzsdb_validate(struct zsdb_priv *priv);

#endif  /* _ZEROSKIP_PRIV_H_ */
