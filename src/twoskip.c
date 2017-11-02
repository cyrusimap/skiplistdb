/*
 * twoskip
 *
 *
 * twoskip is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 */

#include "twoskip.h"

#include "cstring.h"
#include "util.h"
#include "mappedfile.h"
#include "htable.h"

#include <stdio.h>
#include <stdint.h>

/*
 * TS_MALXLEVEL: Number of skiplist levels - 31.
 * This gives us binary search for 2^32 records. Limited to 255 by file format.
 */
#define TS_MAXLEVEL 31

/**
 *  Twoskip DB Header
 **/
enum {
        OFFSET_HEADER        = 0,
        OFFSET_VERSION       = 20,
        OFFSET_GENERATION    = 24,
        OFFSET_NUM_RECORDS   = 32,
        OFFSET_REPACK_SIZE   = 40,
        OFFSET_CURRENT_SIZE  = 48,
        OFFSET_FLAGS         = 56,
        OFFSET_CRC32         = 60,
};

typedef enum rectype {
        DUMMY  = '=',
        ADD    = '+',
        DELETE = '-',
        COMMIT = '$',
} RecType;

struct db_header {
        uint32_t version;
        uint32_t flags;
        uint64_t generation;
        uint64_t num_records;
        size_t   repack_size;
        size_t   current_size;
};

static const char *TS_HEADER_MAGIC = "\241\002\213\015twoskip file\0\0\0\0";
static const int   TS_HEADER_MAGIC_SIZE = 20;
#define TS_HEADER_SIZE 64
#define DUMMY_OFFSET TS_HEADER_SIZE
#define MAXRECORDHEAD ((TS_MAXLEVEL + 5) * 8)

/**
 * Trasaction structure
 **/
struct txn {
        int num;
};

/**
 * The structure of each record in Twoskip DB.
 **/
struct tsrec {
        /* location on disk (not part of the on-disk format) */
        size_t offset;
        size_t len;

        /* Header fields */
        RecType type;
        uint8_t level;
        size_t keylen;
        size_t vallen;

        /* Levels */
        size_t nextlevel[TS_MAXLEVEL + 1];

        /* Integrity checks */
        uint32_t crc_head;
        uint32_t crc_tail;

        /* Key and Value */
        size_t keyoffset;
        size_t valoffset;
};

/**
 * A structure that describes location in the Twoskip DB file.
 **/
struct tsloc {
        /* Requested data */
        cstring keybuf;
        int is_exactmatch;

        /* current or next record */
        struct tsrec record;

        size_t backloc[TS_MAXLEVEL + 1];
        size_t forwardloc[TS_MAXLEVEL + 1];

        /* generation to ensure that the location is still valid */
        uint64_t generation;
        size_t end;
};

struct tsdb_priv {
        struct mappedfile *mf;
        HashTable *ht;
        struct db_header header;
        struct tsloc loc;

        /* tracking info */
        int is_open;
        size_t end;
        int txn_num;
        struct txn *current_txn;

        /* compare function for sorting */
        int open_flags;
        int (*compare)(unsigned char *s1, int l1, unsigned char *s2, int l2);
};

struct tsdb_list {
        struct tsdb_engine *engine;
        struct tsdb_list *next;
        int refcount;
};


static int ts_init(DBType type __attribute__((unused)),
                   struct skiplistdb **db __attribute__((unused)),
                   struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_final(struct skiplistdb *db __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_open(const char *fname __attribute__((unused)),
                   struct skiplistdb *db __attribute__((unused)),
                   int flags __attribute__((unused)),
                   struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_close(struct skiplistdb *db __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_sync(struct skiplistdb *db __attribute__((unused)))
{
        if (db->op->sync)
                return db->op->sync(db);
        else
                return SDB_NOTIMPLEMENTED;
}

static int ts_archive(struct skiplistdb *db __attribute__((unused)),
                      const struct str_array *fnames __attribute__((unused)),
                      const char *dirname __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_unlink(struct skiplistdb *db __attribute__((unused)),
                     const char *fname __attribute__((unused)),
                     int flags __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_fetch(struct skiplistdb *db __attribute__((unused)),
                    unsigned char *key __attribute__((unused)),
                    size_t keylen __attribute__((unused)),
                    unsigned char **data __attribute__((unused)),
                    size_t *datalen __attribute__((unused)),
                    struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_fetchlock(struct skiplistdb *db __attribute__((unused)),
                        unsigned char *key __attribute__((unused)),
                        size_t keylen __attribute__((unused)),
                        unsigned char **data __attribute__((unused)),
                        size_t *datalen __attribute__((unused)),
                        struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_fetchnext(struct skiplistdb *db __attribute__((unused)),
                        unsigned char *key __attribute__((unused)),
                        size_t keylen __attribute__((unused)),
                        unsigned char **foundkey __attribute__((unused)),
                        size_t *foundkeylen __attribute__((unused)),
                        unsigned char **data __attribute__((unused)),
                        size_t *datalen __attribute__((unused)),
                        struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_foreach(struct skiplistdb *db __attribute__((unused)),
                      unsigned char *prefix __attribute__((unused)),
                      size_t prefixlen __attribute__((unused)),
                      foreach_p *p __attribute__((unused)),
                      foreach_cb *cb __attribute__((unused)),
                      void *rock __attribute__((unused)),
                      struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_add(struct skiplistdb *db __attribute__((unused)),
                  unsigned char *key __attribute__((unused)),
                  size_t keylen __attribute__((unused)),
                  unsigned char *data __attribute__((unused)),
                  size_t datalen __attribute__((unused)),
                  struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_remove(struct skiplistdb *db __attribute__((unused)),
                     unsigned char *key __attribute__((unused)),
                     size_t keylen __attribute__((unused)),
                     struct txn **tid __attribute__((unused)),
                     int force __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_store(struct skiplistdb *db __attribute__((unused)),
                    unsigned char *key __attribute__((unused)),
                    size_t keylen __attribute__((unused)),
                    unsigned char *data __attribute__((unused)),
                    size_t datalen __attribute__((unused)),
                    struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_commit(struct skiplistdb *db __attribute__((unused)),
                     struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_abort(struct skiplistdb *db __attribute__((unused)),
                    struct txn **tid __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_dump(struct skiplistdb *db __attribute__((unused)),
                   DBDumpLevel level __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_consistent(struct skiplistdb *db __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_repack(struct skiplistdb *db __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

static int ts_cmp(struct skiplistdb *db __attribute__((unused)),
                  unsigned char *s1 __attribute__((unused)),
                  int l1 __attribute__((unused)),
                  unsigned char *s2 __attribute__((unused)),
                  int l2 __attribute__((unused)))
{
        return SDB_NOTIMPLEMENTED;
}

/* Hash table */
unsigned int hash_fn(const void *keyp)
{
        unsigned long key = (unsigned long)keyp;

        key = murmur3_hash_32(&key, sizeof(key));

        key += ~(key << 15);
        key ^=  (key >> 10);
        key +=  (key << 3);
        key ^=  (key >> 6);
        key += ~(key << 11);
        key ^=  (key >> 16);

        return key;
}

int key_cmp_fn(const void *key1, const void *key2)
{
        unsigned long k1 = (unsigned long)key1;
        unsigned long k2 = (unsigned long)key2;

        return k1 == k2;
}

HashProps hash_props_long = {
        hash_fn,
        key_cmp_fn,
        NULL,
        NULL,
        NULL,
        NULL
};
/* The operations structure */
static const struct skiplistdb_operations twoskip_ops = {
        .init         = ts_init,
        .final        = ts_final,
        .open         = ts_open,
        .close        = ts_close,
        .sync         = ts_sync,
        .archive      = ts_archive,
        .unlink       = ts_unlink,
        .fetch        = ts_fetch,
        .fetchlock    = ts_fetchlock,
        .fetchnext    = ts_fetchnext,
        .foreach      = ts_foreach,
        .add          = ts_add,
        .remove       = ts_remove,
        .store        = ts_store,
        .commit       = ts_commit,
        .abort        = ts_abort,
        .dump         = ts_dump,
        .consistent   = ts_consistent,
        .repack       = ts_repack,
        .cmp          = ts_cmp,
};


struct skiplistdb * twoskip_new(const char *path __attribute__((unused)))
{
        struct skiplistdb *db = NULL;
        struct tsdb_priv *priv = NULL;

        db = xcalloc(1, sizeof(struct skiplistdb));
        if (!db) {
                fprintf(stderr, "Error allocating memory\n");
                goto done;
        }

        db->name = "twoskip";
        db->type = TWO_SKIP;
        db->op = &twoskip_ops;

        /* Allocate the private data structure */
        priv = xcalloc(1, sizeof(struct tsdb_priv));
        if (!priv) {
                fprintf(stderr, "Error allocating memory for private data\n");
                xfree(db);
                goto done;
        }

        /* Setup hashtable for tracking open db files */
        priv->ht = ht_new(&hash_props_long);

        db->priv = priv;
done:
        return db;
}


void twoskip_free(struct skiplistdb *db)
{
        if (db && db->priv) {
                xfree(db->priv);
                xfree(db);
        }

        return;
}
