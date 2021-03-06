/*
 * $Id$
 */

#ifndef DSDC_NO_CUPID
// For question data loaded into match.
%#define MATCHD_FRONTD_FROBBER	0
// For user information loaded into match
%#define MATCHD_FRONTD_USERCACHE_FROBBER	1
%#define UBER_USER_FROBBER	2
%#define PROFILE_STALKER_FROBBER	3
// For cached match results.
%#define MATCHD_FRONTD_MATCHCACHE_FROBBER	4
// For groups.
%#define GROUP_INFO_FROBBER	5

// For generic test scores.
%#define GTEST_SCORE_FROBBER	6
// For psych test scores.
%#define PTEST_SCORE_FROBBER	7
// For member test scores.
%#define MTEST_SCORE_FROBBER	8
// For cupid test scores.
%#define CUPID_TEST_SCORE_FROBBER	9

// For generic test sessions.
%#define GTEST_SESSION_FROBBER	10
// For psych test sessions.
%#define PTEST_SESSION_FROBBER	11
// For member test sessions.
%#define MTEST_SESSION_FROBBER	12

// For member test metadata
%#define MTEST_METADATA_FROBBER	13
// For member test stats
%#define MTEST_STATS_FROBBER	14


/* %#include "userid_prot.h" */

/* 64 bit key */
struct dsdc_key64_t {
	int frobber;
	u_int64_t key64;	/**< user id */
};

/**
 * Identifier for most uber user structs.
 */
struct uber_key_t {
    int frobber;
    u_int64_t userid;
    unsigned int load_type;
};

%#define MATCHD_NULL_QUESTION_ID	0

/*
 * Matchd question.  This matches the qanswers table in our database.
 */
struct matchd_qanswer_row_t {
	int questionid;
	unsigned int data;      /**< bits: 0-1 answer, 2-5 answer mask,
				  6-8 importance */
};

typedef matchd_qanswer_row_t matchd_qanswer_rows_t<>;

/*
 * This structure is passed to the DSDC backend for computing matches.
 * it contains the questions for the user doing the comparison and the
 * userids on the backend server to compare against.
 */
struct matchd_frontd_dcdc_arg_t {
	matchd_qanswer_rows_t user_questions;
	u_int64_t userids<>;
};

/*
 * This is the per-user match result.
 */
struct matchd_frontd_match_datum_t {
	u_int64_t userid;	/**< user id */
	bool match_found;	/**< is median/deviation valid? */
	int mpercent;		/**< match percentage. */
	int fpercent;		/**< friend percentage. */
	int epercent;		/**< enemy percentage. */
};

/*
 * structure containing per-user match results.
 */
struct match_frontd_match_results_t {
    int cache_misses;
	matchd_frontd_match_datum_t results<>;
};

#endif /* !DSDC_NO_CUPID */


%#define DSDC_KEYSIZE 20
%#define DSDC_DEFAULT_PORT 30002

typedef opaque dsdc_key_t[DSDC_KEYSIZE];

typedef dsdc_key_t dsdc_keyset_t<>;


struct dsdc_req_t {
    dsdc_key_t key;
    int time_to_expire;
};

typedef opaque dsdc_custom_t<>;

struct dsdc_key_template_t {
	unsigned id;
	unsigned pid;
	unsigned port;
	string hostname<>;
};

enum dsdc_res_t {
	DSDC_OK = 0,
	DSDC_REPLACED = 1,
	DSDC_INSERTED = 2,
	DSDC_NOTFOUND = 3,
	DSDC_NONODE = 4,
	DSDC_ALREADY_REGISTERED = 5,
	DSDC_RPC_ERROR = 6,
	DSDC_DEAD = 7,
	DSDC_LOCKED = 8,
	DSDC_TIMEOUT = 9
};

typedef opaque dsdc_obj_t<>;

struct dsdc_put_arg_t {
	dsdc_key_t key;
	dsdc_obj_t obj;
};

union dsdc_get_res_t switch (dsdc_res_t status) {
case DSDC_OK:
  dsdc_obj_t obj;
case DSDC_RPC_ERROR:
  unsigned err;
default:
  void;
};


/**
 * a series of name/value pairs for a multi-get (mget)
 */
struct dsdc_mget_1res_t {
  dsdc_key_t key;
  dsdc_get_res_t res;
};

typedef dsdc_mget_1res_t dsdc_mget_res_t<>;
typedef dsdc_key_t       dsdc_mget_arg_t<>;
typedef dsdc_req_t       dsdc_mget2_arg_t<>;


struct dsdcx_slave_t {
 	dsdc_keyset_t keys;
	string hostname<>;
	int port;
};

struct dsdcx_state_t {
	dsdcx_slave_t slaves<>;
	dsdcx_slave_t *lock_server;
};

struct dsdc_register_arg_t {
 	dsdcx_slave_t slave;
	bool primary;
	bool lock_server;
};


union dsdc_getstate_res_t switch (bool needupdate) {
case true:
	dsdcx_state_t state;
case false:
	void;
};

union dsdc_lock_acquire_res_t switch (dsdc_res_t status) {
case DSDC_OK:
	unsigned hyper lockid;
case DSDC_RPC_ERROR:
	unsigned err;
default:
	void;
};

struct dsdc_lock_acquire_arg_t {
	dsdc_key_t key;           // a key into a lock-specific namespace
        bool writer;              // if shared lock, if needed for writing
	bool block;               // whether to block or just fail
	unsigned timeout;         // how long the lock is held for
};

struct dsdc_lock_release_arg_t {
	dsdc_key_t key;	          // original key that was locked
	unsigned hyper lockid;    // provide the lock-ID to catch bugs

};


program DSDC_PROG
{
	version DSDC_VERS {

		void
		DSDC_NULL (void) = 0;

/*
 * these are the only 4 calls that clients should use.  they should
 * issue them to the master nodes, who will deal with them:
 *
 *  PUT / REMOVE / GET / MGET
 *
 */
		dsdc_res_t
		DSDC_PUT (dsdc_put_arg_t) = 1;

		dsdc_res_t
		DSDC_REMOVE (dsdc_key_t) = 2;

		dsdc_get_res_t
		DSDC_GET (dsdc_key_t) = 3;

		dsdc_mget_res_t
		DSDC_MGET (dsdc_mget_arg_t) = 4;


/*
 * the following 4 calls are for internal management, that dsdc
 * uses for itself:
 *
 *   REGISTER / HEARTBEAT / NEWNODE / GETSTATE
 */

		/*
   		 * a slave node should register with all master nodes
		 * using this call.  It should register as many keys
		 * in the keyspace as it wants to service.  The more
		 * keys, the more of the load it will see. 
		 * Should set the master flag in the arg structure
		 * only once; the master will broadcast the insertion
		 * to the other nodes in the ring.
		 */
		dsdc_res_t	
		DSDC_REGISTER (dsdc_register_arg_t) = 6;

		/*
 		 * heartbeat;  a slave must send a periodic heartbeat
	  	 * message, otherwise, the master will think it's dead.
		 */
		dsdc_res_t
		DSDC_HEARTBEAT (void) = 7;

		/*
		 * when a new node is inserted, the master broadcasts
		 * all of the other nodes, alerting them to clean out
		 * their caches.  this is also when we would add
		 * data movement protocols.
		 */
		dsdc_res_t
		DSDC_NEWNODE (dsdcx_slave_t) = 8;

		/*
		 * nodes should periodically get the complete system
		 * state and clean out their caches accordingly.
		 */
		dsdc_getstate_res_t	
		DSDC_GETSTATE (dsdc_key_t) = 9;


/*
 * Simple locking primitives for doing synchronization via dsdc
 */

		/*
		 * Acquire a lock.
		 */
		dsdc_lock_acquire_res_t
		DSDC_LOCK_ACQUIRE (dsdc_lock_acquire_arg_t) = 10;


		/*
		 * Relase a lock that was granted.
 		 */
		dsdc_res_t
		DSDC_LOCK_RELEASE (dsdc_lock_release_arg_t) = 11;

        /*
         * get with expiry times
         */
		dsdc_get_res_t
		DSDC_GET2 (dsdc_req_t) = 12;

        /*
         * multi-get with expiry times
         */
		dsdc_mget_res_t
		DSDC_MGET2 (dsdc_mget2_arg_t) = 13;

#ifndef DSDC_NO_CUPID
/*
 *-----------------------------------------------------------------------
 * Below are custom RPCs for matching and okcupid-related functions
 * in particular (with procno >= 100...)
 *
 */
		match_frontd_match_results_t
                DSDC_COMPUTE_MATCHES(matchd_frontd_dcdc_arg_t) = 100;
#endif

	} = 1;
} = 30002;
