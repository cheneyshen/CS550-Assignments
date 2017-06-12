/*
    Improved from jwHash
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csHash.h"

#ifdef HASHTEST
#include <sys/time.h>
#endif

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// STATIC HELPER FUNCTIONS

// Spin-locking
// http://stackoverflow.com/questions/1383363/is-my-spin-lock-implementation-correct-and-optimal

// http://stackoverflow.com/a/12996028
// hash function for int keys
static inline long int hashInt(long int x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x);
	return x;
}

// http://www.cse.yorku.ca/~oz/hash.html
// hash function for string keys djb2
static inline long int hashString(char * str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

// helper for copying string keys and values
static inline char * copystring(char * value)
{
	char * copy = (char *)malloc(strlen(value)+1);
	if (!copy) {
		printf("Unable to allocate string value %s\n",value);
		abort();
	}
	strcpy(copy,value);
	return copy;
}

////////////////////////////////////////////////////////////////////////////////
// CREATING A NEW HASH TABLE

// Create hash table
csHashTable *create_hash( size_t buckets )
{
	// allocate space
	csHashTable *table= (csHashTable *)malloc(sizeof(csHashTable));
	if (!table) {
		// unable to allocate
		return NULL;
	}
	// locks
#ifdef HASHTHREADED
	table->lock = 0;
	table->locks = (int *)malloc(buckets * sizeof(int));
	if ( !table->locks ) {
		free(table);
		return NULL;
	}
	memset((int *)&table->locks[0],0,buckets*sizeof(int));
#endif
	// setup
	table->bucket = (csHashEntry **)malloc(buckets*sizeof(void*));
	if ( !table->bucket ) {
		free(table);
		return NULL;
	}
	memset(table->bucket,0,buckets*sizeof(void*));
	table->buckets = table->bucketsinitial = buckets;
	HASH_DEBUG("table: %p bucket: %p\n",table,table->bucket);
	return table;
}

////////////////////////////////////////////////////////////////////////////////
// ADDING / DELETING / GETTING BY STRING KEY

// Add str to table - keyed by string
HASHRESULT add_str_by_str( csHashTable *table, char *key, char *value )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("adding %s -> %s hash: %ld\n",key,value,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (0==strcmp(entry->key.strValue,key) && 0==strcmp(value,entry->value.strValue))
			return HASHALREADYADDED;
		// check for replacing entry
		if (0==strcmp(entry->key.strValue,key) && 0!=strcmp(value,entry->value.strValue))
		{
			free(entry->value.strValue);
			entry->value.strValue = copystring(value);
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

#ifdef HASHTHREADED
	// lock this bucket against changes
	// Refer to Multithreaded simple data type access and atomic variables
	while (__sync_lock_test_and_set(&table->locks[hash], 1)) {
		printf(".");
		// Do nothing. This GCC builtin instruction
		// ensures memory barrier.
	  }
#endif
	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.strValue = copystring(key);
	entry->valtag = HASHSTRING;
	entry->value.strValue = copystring(value);
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
unlock:
#ifdef HASHTHREADED
	__sync_synchronize(); // memory barrier
	table->locks[hash] = 0;
#endif
	return HASHOK;
}

HASHRESULT add_dbl_by_str( csHashTable *table, char *key, double value )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("adding %s -> %f hash: %ld\n",key,value,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (0==strcmp(entry->key.strValue,key) && value==entry->value.dblValue)
			return HASHALREADYADDED;
		// check for replacing entry
		if (0==strcmp(entry->key.strValue,key) && value!=entry->value.dblValue)
		{
			entry->value.dblValue = value;
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.strValue = copystring(key);
	entry->valtag = HASHNUMERIC;
	entry->value.dblValue = value;
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
	return HASHOK;
}

HASHRESULT add_int_by_str( csHashTable *table, char *key, long int value )
{
	// compute hash on key
	size_t hash = hashString(key);
	hash %= table->buckets;
	HASH_DEBUG("adding %s -> %ld hash: %ld\n",key,value,hash);

#ifdef HASHTHREADED
	// lock this bucket against changes
	// Refer to Multithreaded simple data type access and atomic variables
	while (__sync_lock_test_and_set(&table->locks[hash], 1)) {
		//printf(".");
		// Do nothing. This GCC builtin instruction
		// ensures memory barrier.
	  }
#endif

	// check entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (0==strcmp(entry->key.strValue,key) && value==entry->value.intValue)
			return HASHALREADYADDED;
		// check for replacing entry
		if (0==strcmp(entry->key.strValue,key) && value!=entry->value.intValue)
		{
			entry->value.intValue = value;
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.strValue = copystring(key);
	entry->valtag = HASHNUMERIC;
	entry->value.intValue = value;
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
unlock:
#ifdef HASHTHREADED
	__sync_synchronize(); // memory barrier
	table->locks[hash] = 0;
#endif
	return HASHOK;
}

HASHRESULT add_ptr_by_str( csHashTable *table, char *key, void *ptr )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("adding %s -> %p hash: %ld\n",key,ptr,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (0==strcmp(entry->key.strValue,key) && ptr==entry->value.ptrValue)
			return HASHALREADYADDED;
		// check for replacing entry
		if (0==strcmp(entry->key.strValue,key) && ptr!=entry->value.ptrValue)
		{
			entry->value.ptrValue = ptr;
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.strValue = copystring(key);
	entry->valtag = HASHPTR;
	entry->value.ptrValue = ptr;
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
	return HASHOK;
}

// Delete by string
HASHRESULT del_by_str( csHashTable *table, char *key )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("deleting: %s hash: %ld\n",key,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];
	csHashEntry *previous = NULL;

	// found an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (0==strcmp(entry->key.strValue,key))
		{
			// skip first record, or one in the chain
			if (!previous)
				table->bucket[hash] = entry->next;
			else
				previous->next = entry->next;
			// delete string value if needed
			if ( entry->valtag==HASHSTRING )
				free(entry->value.strValue);
			free(entry->key.strValue);
			free(entry);
			return HASHDELETED;
		}
		// move to next entry
		previous = entry;
		entry = entry->next;
	}
	return HASHNOTFOUND;
}

// Lookup str - keyed by str
HASHRESULT get_str_by_str( csHashTable *table, char *key, char **value )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("fetching %s -> ?? hash: %ld\n",key,hash);

	// get entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	while(entry)
	{
		// check for key
		//HASH_DEBUG("found entry key: %s value: %s\n",entry->key.strValue,entry->value.strValue);
		if (0==strcmp(entry->key.strValue,key)) {
			*value =  entry->value.strValue;
			return HASHOK;
		}
		// move to next entry
		entry = entry->next;
	}

	// not found
	return HASHNOTFOUND;
}

// Traverse hash table - keyed by str
HASHRESULT get_set_by_str( csHashTable *table, unsigned int start, char *key, char **value )
{
	//HASH_DEBUG("searching for %s \n",key);
	unsigned int  hash; int result=0;
	unsigned int  segment = table->buckets / NUMTHREADS;
	unsigned int  end = (table->buckets < (start + 1) *segment) ?  table->buckets :  (start + 1) *segment;
	for (hash = start*segment; hash < end; hash++)
	{
		// get entry
		csHashEntry *entry = table->bucket[hash];
		// already an entry
		while(entry)
		{
			// check for key
			char skey[256] = {0};
			strcpy(skey, entry->key.strValue);
			if (0 != strstr(entry->key.strValue, key)) {
				//HASH_DEBUG("found entry key: %s value: %s\n",entry->key.strValue,entry->value.strValue);
				strcat(*value, entry->value.strValue);
				strncat(*value, "$", 1);
				strcat(*value, entry->key.strValue);
				strncat(*value, "^", 1);
				result = 1;
			}
			// move to next entry
			entry = entry->next;
		}
	}
	if (result)
		return HASHOK;
	// not found
	return HASHNOTFOUND;
}

// Lookup int - keyed by str
HASHRESULT get_int_by_str( csHashTable *table, char *key, int *i )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("fetching %s -> ?? hash: %ld\n",key,hash);

	// get entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	while(entry)
	{
		// check for key
		//HASH_DEBUG("found entry key: %s value: %d\n",entry->key.strValue,entry->value.intValue);
		if (0==strcmp(entry->key.strValue,key)) {
			*i = entry->value.intValue;
			return HASHOK;
		}
		// move to next entry
		entry = entry->next;
	}

	// not found
	return HASHNOTFOUND;
}

// Lookup dbl - keyed by str
HASHRESULT get_dbl_by_str( csHashTable *table, char *key, double *val )
{
	// compute hash on key
	size_t hash = hashString(key) % table->buckets;
	HASH_DEBUG("fetching %s -> ?? hash: %ld\n",key,hash);

	// get entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	while(entry)
	{
		// check for key
		//HASH_DEBUG("found entry key: %s value: %f\n",entry->key.strValue,entry->value.dblValue);
		if (0==strcmp(entry->key.strValue,key)) {
			*val = entry->value.dblValue;
			return HASHOK;
		}
		// move to next entry
		entry = entry->next;
	}

	// not found
	return HASHNOTFOUND;
}

////////////////////////////////////////////////////////////////////////////////
// ADDING / DELETING / GETTING BY LONG INT KEY

// Add to table - keyed by int
HASHRESULT add_str_by_int( csHashTable *table, long int key, char *value )
{
	// compute hash on key
	size_t hash = hashInt(key) % table->buckets;
	HASH_DEBUG("adding %ld -> %s hash: %ld\n",key,value,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (entry->key.intValue==key && 0==strcmp(value,entry->value.strValue))
			return HASHALREADYADDED;
		// check for replacing entry
		if (entry->key.intValue==key && 0!=strcmp(value,entry->value.strValue))
		{
			free(entry->value.strValue);
			entry->value.strValue = copystring(value);
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

#ifdef HASHTHREADED
	// lock this bucket against changes
	// Refer to Multithreaded simple data type access and atomic variables
	while (__sync_lock_test_and_set(&table->locks[hash], 1)) {
		printf(".");
		// Do nothing. This GCC builtin instruction
		// ensures memory barrier.
	  }
#endif
	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.intValue = key;
	entry->valtag = HASHSTRING;
	entry->value.strValue = copystring(value);
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
unlock:
#ifdef HASHTHREADED
	__sync_synchronize(); // memory barrier
	table->locks[hash] = 0;
#endif
	return HASHOK;
}

// Add dbl to table - keyed by int
HASHRESULT add_dbl_by_int( csHashTable* table, long int key, double value )
{
	// compute hash on key
	size_t hash = hashInt(key) % table->buckets;
	HASH_DEBUG("adding %ld -> %f hash: %ld\n",key,value,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (entry->key.intValue==key && value==entry->value.dblValue)
			return HASHALREADYADDED;
		// check for replacing entry
		if (entry->key.intValue==key && value!=entry->value.dblValue)
		{
			entry->value.dblValue = value;
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.intValue = key;
	entry->valtag = HASHNUMERIC;
	entry->value.dblValue = value;
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
	return HASHOK;
}

HASHRESULT add_int_by_int( csHashTable* table, long int key, long int value )
{
	// compute hash on key
	size_t hash = hashInt(key) % table->buckets;
	HASH_DEBUG("adding %ld -> %ld hash: %ld\n",key,value,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for already indexed
		if (entry->key.intValue==key && value==entry->value.intValue)
			return HASHALREADYADDED;
		// check for replacing entry
		if (entry->key.intValue==key && value!=entry->value.intValue)
		{
			entry->value.intValue = value;
			return HASHREPLACEDVALUE;
		}
		// move to next entry
		entry = entry->next;
	}

	// create a new entry and add at head of bucket
	HASH_DEBUG("creating new entry\n");
	entry = (csHashEntry *)malloc(sizeof(csHashEntry));
	HASH_DEBUG("new entry: %p\n",entry);
	entry->key.intValue = key;
	entry->valtag = HASHNUMERIC;
	entry->value.intValue = value;
	entry->next = table->bucket[hash];
	table->bucket[hash] = entry;
	HASH_DEBUG("added entry\n");
	return HASHOK;
}


// Delete by int
HASHRESULT del_by_int( csHashTable* table, long int key )
{
	// compute hash on key
	size_t hash = hashInt(key) % table->buckets;
	HASH_DEBUG("deleting: %ld hash: %ld\n",key,hash);

	// add entry
	csHashEntry *entry = table->bucket[hash];
	csHashEntry *prev = NULL;

	// found an entry
	HASH_DEBUG("entry: %p\n",entry);
	while(entry!=0)
	{
		HASH_DEBUG("checking entry: %p\n",entry);
		// check for entry to delete
		if (entry->key.intValue==key)
		{
            #ifdef HASHTHREADED
            // lock this bucket against changes
            // Refer to Multithreaded simple data type access and atomic variables
            while (__sync_lock_test_and_set(&table->locks[hash], 1)) {
                printf(".");
                // Do nothing. This GCC builtin instruction
                // ensures memory barrier.
              }
            #endif
			// skip first record, or one in the chain
			if (!prev)
				table->bucket[hash] = entry->next;
			else
				prev->next = entry->next;
            // delete string value if needed
			if ( entry->valtag==HASHSTRING )
				free(entry->value.strValue);
			free(entry);
            unlock:
            #ifdef HASHTHREADED
                __sync_synchronize(); // memory barrier
                table->locks[hash] = 0;
            #endif
			return HASHDELETED;
		}
		// move to next entry
		prev = entry;
		entry = entry->next;
	}
	return HASHNOTFOUND;
}

// Lookup str - keyed by int
HASHRESULT get_str_by_int( csHashTable *table, long int key, char **value )
{
	// compute hash on key
	size_t hash = hashInt(key) % table->buckets;
	HASH_DEBUG("fetching %ld -> ?? hash: %ld\n",key,hash);

	// get entry
	csHashEntry *entry = table->bucket[hash];

	// already an entry
	while(entry)
	{
		// check for key
		//HASH_DEBUG("found entry key: %d value: %s\n",entry->key.intValue,entry->value.strValue);
		if (entry->key.intValue==key) {
			*value = entry->value.strValue;
			return HASHOK;
		}
		// move to next entry
		entry = entry->next;
	}

	// not found
	return HASHNOTFOUND;
}






