// -*-c++-*-
/* $Id$ */

#ifndef _DSDC_DSDC_H
#define _DSDC_DSDC_H

#include "dsdc_prot.h"

//
// a glossary:
//
//   dsdc  - Dirt Simple Distributed Cache
//   dsdcm - A master process
//   dsdcs - A slave process
//   dsdck - a key
//   dsdcn - A node in the global consistent hash ring.
//

hash_t hash_hash (const dsdc_key_t &k);

inline int dsdck_cmp (const dsdc_key_t &a, const dsdc_key_t &b)
{ return memcmp (a.base (), b.base (), a.size ()); }

class dsdck_compare_t {
public:
  dsdck_compare_t () {}
  int operator() (const dsdc_key_t &a, const dsdc_key_t &b) const
  { return dsdck_cmp (a, b); }
};

class dsdck_hashfn_t {
public:
  dsdck_hashfn_t () {}
  hash_t operator () (const dsdc_key_t &k) const { return hash_hash (k); }
};

class dsdck_equals_t {
public:
  dsdck_equals_t () {}
  bool operator() (const dsdc_key_t &a, const dsdc_key_t &b) const
  { return dsdck_cmp (a,b) == 0; }
};

class dsdc_app_t {
public:
  dsdc_app_t () : _daemonize (false) {}
  virtual bool init () = 0;
  bool daemonize () const { return _daemonize; }
  void set_daemon_mode (bool m) { _daemonize = m; }
  virtual str startup_msg () const { return NULL; }
  virtual str progname (const str &in) const ;
  virtual str progname_xtra () const { return NULL; }
private:
  bool _daemonize;
  
};

typedef enum { DSDC_MODE_NONE = 0,
	       DSDC_MODE_MASTER = 1,
	       DSDC_MODE_SLAVE = 2 } dsdc_mode_t;

void set_hostname (const str &s);
extern str dsdc_hostname;
void set_debug (int lev);
bool show_debug (int lev);
str key_to_str (const dsdc_key_t &k);
bool parse_hn (const str &in, str *host, int *port);

#endif /* _DSDC_DSDC_H */