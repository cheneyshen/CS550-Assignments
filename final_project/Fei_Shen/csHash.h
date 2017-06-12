/*
    Improved from jwHash
*/

#define HASHTHREADED  1
//#define HASHTEST      1
#define HASHDEBUG     1

#ifndef csHash_h
#define csHash_h

// needed for size_t
#include <stddef.h>

#ifdef HASHDEBUG
#define HASH_DEBUG(fmt,args...) printf(fmt, ## args)
#else
#define HASH_DEBUG(fmt,args...) do {} while (0);
#endif

#define NUMTHREADS      6
#define HASHCOUNT       100000

// resuts codes
typedef enum
{
	HASHOK,
	HASHADDED,
	HASHREPLACEDVALUE,
	HASHALREADYADDED,
	HASHDELETED,
	HASHNOTFOUND,
} HASHRESULT;

typedef enum
{
	HASHPTR,
	HASHNUMERIC,
	HASHSTRING,
} HASHVALTAG;


typedef struct csHashEntry csHashEntry;
struct csHashEntry
{
	union
	{
		char  *strValue;
		double dblValue;
		int	   intValue;
	} key;
	HASHVALTAG valtag;
	union
	{
		char  *strValue;
		double dblValue;
		int	   intValue;
		void  *ptrValue;
	} value;
	csHashEntry *next;
};

typedef struct csHashTable csHashTable;
struct csHashTable
{
	csHashEntry **bucket;			// pointer to array of buckets
	size_t buckets;
	size_t bucketsinitial;			// if we resize, may need to hash multiple times
	HASHRESULT lastError;
#ifdef HASHTHREADED
	volatile int *locks;			// array of locks
	volatile int lock;				// lock for entire table
#endif
};

// Create/delete hash table
csHashTable *create_hash( size_t buckets );
// not finished
void *delete_hash( csHashTable *table );		// clean up all memory


// Add to table - keyed by string
HASHRESULT add_str_by_str( csHashTable*, char *key, char *value );
HASHRESULT add_dbl_by_str( csHashTable*, char *key, double value );
HASHRESULT add_int_by_str( csHashTable*, char *key, long int value );
HASHRESULT add_ptr_by_str( csHashTable*, char *key, void *value );

// Delete by string
HASHRESULT del_by_str( csHashTable*, char *key );

// Get by string
HASHRESULT get_str_by_str( csHashTable *table, char *key, char **value );
HASHRESULT get_int_by_str( csHashTable *table, char *key, int *i );
HASHRESULT get_dbl_by_str( csHashTable *table, char *key, double *val );
HASHRESULT get_set_by_str( csHashTable *table, unsigned int start, char *key, char **value );
// Add to table - keyed by int
HASHRESULT add_str_by_int( csHashTable*, long int key, char *value );
HASHRESULT add_dbl_by_int( csHashTable*, long int key, double value );
HASHRESULT add_int_by_int( csHashTable*, long int key, long int value );
// not finished
HASHRESULT add_ptr_by_int( csHashTable*, long int key, void *value );

// Delete by int
HASHRESULT del_by_int( csHashTable*, long int key );

// Get by int
HASHRESULT get_str_by_int( csHashTable *table, long int key, char **value );
// not finished
HASHRESULT get_int_by_int( csHashTable *table, long int key, int *i );
// not finished
HASHRESULT get_dbl_by_int( csHashTable *table, long int key, double *val );

#endif



