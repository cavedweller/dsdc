
// -*-c++-*-

#ifndef _DSDC_FSCACHE_H_
#define _DSDC_FSCACHE_H_

#include "async.h"
#include "tame.h"
#include "aiod.h"
#include "crypt.h"

namespace fscache {

typedef enum {
    BACKEND_SIMPLE = 0,
    BACKEND_AIOD = 1
} backend_typ_t;

class cfg_t {
    public:
        cfg_t ();

        backend_typ_t backend () const { return _backend; }
        int n_levels () const { return _n_levels; }
        str root () const { return _root; }
        int n_aiods () const { return _n_aiods; }
        size_t shmsize () const { return _shmsize; }
        size_t maxbuf () const { return _maxbuf; }
        int file_mode () const { return _file_mode; }
        size_t blocksz () const { return _blocksz; }

        backend_typ_t _backend;
        int _n_levels;
        str _root;
        int _n_aiods;
        size_t _shmsize, _maxbuf, _blocksz;
        int _file_mode;
};

typedef callback<void,int,time_t,str>::ref cbits_t;
typedef callback<void,int,str>::ref cbis_t;

class file_id_t {
    public:
        file_id_t (const str &name, u_int32_t index)
            : _name (name), _index (index) {}
        str name () const { return _name; }
        str fullpath (int lev) const;
    private:
        const str _name;
        const u_int32_t _index;
};

class backend_t {
    public:
        backend_t () {}
        virtual ~backend_t () {}
        virtual void file2str (str fn, cbis_t cb) = 0;
        virtual void str2file (str f, str s, int mode, cbi cb) = 0;
        virtual void remove (str f, cbi cb) = 0;
};

class engine_t {
    public:
        engine_t (const cfg_t *c);
        ~engine_t ();

        void load (file_id_t id, cbits_t cb, CLOSURE);
        void store (file_id_t id, time_t tm, str data, cbi cb, CLOSURE);
        void remove (file_id_t id, cbi cb, CLOSURE);

        str filename (file_id_t id) const;

    private:
        const cfg_t *_cfg;
        backend_t *_backend;
};

class simple_backend_t : public backend_t {
    public:
        simple_backend_t () : backend_t () {}
        void file2str (str fn, cbis_t cb);
        void str2file (str f, str s, int mode, cbi cb);
        void remove (str f, cbi cb);
};

class aiod_backend_t : public backend_t {
    public:
        aiod_backend_t (const cfg_t *c);
        ~aiod_backend_t ();

        void file2str (str fn, cbis_t cb) { file2str_T (fn, cb); }
        void str2file (str f, str s, int m, cbi cb)
        { str2file_T (f, s, m, cb); }

        void remove (str f, cbi cb) { remove_T (f, cb); }

    private:
        void file2str_T (str fn, cbis_t cb, CLOSURE);
        void str2file_T (str f, str s, int mode, cbi cb, CLOSURE);
        void remove_T (str f, cbi cb, CLOSURE);

        const cfg_t *_cfg;
        aiod *_aiod;
};


template<class C>
class iface_t {
    public:
        iface_t (engine_t *e) : _engine (e) {}

        typedef typename callback<void, int, time_t, ptr<C> >::ref load_cb_t;

        void load (file_id_t id, load_cb_t cb)
        {
            _engine->load (id, wrap (this, &iface_t<C>::load_cb, id, cb));
        }

        void store (file_id_t id, time_t tm, const C &obj, cbi cb)
        {
            str s;
            str fn = filename (id);
            int ret;
            if (!(s = xdr2str (obj))) {
                warn << "Failed to marshal data struct: " << fn << "\n";
                ret = -EINVAL;
                (*cb) (ret);
            } else {
                _engine->store (id, tm, s, cb);
            }
        }

        void remove (file_id_t id, cbi cb) { _engine->remove (id, cb); }
        str filename (file_id_t id) const
        { return _engine->filename (id); }

    private:

        void load_cb (file_id_t id, load_cb_t cb, int rc, time_t tm, str s)
        {
            ptr<C> ret;
            if (rc == 0) {
                ret = New refcounted<C> ();
                if (!str2xdr (*ret, s)) {
                    str fn = filename (id);
                    warn << "Failed to demarshall data struct: " << fn << "\n";
                    rc = -EINVAL;
                }
            }
            (*cb) (rc, tm, ret);
        }


        engine_t *_engine;

};


};

#endif /* _DSDC_FSCACHE_H_ */