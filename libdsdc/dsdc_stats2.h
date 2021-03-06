// -*-c++-*-
/* $Id: dsdc_stats.h 105205 2008-11-18 20:59:38Z max $ */

#ifndef __DSDC_STATS2_H__
#define __DSDC_STATS2_H__

#include "dsdc_stats.h"

namespace dsdc {

namespace stats {

    //------------------------------------------------------------

    struct dist_v2_t {
        dist_v2_t();
        void clear();
        void insert(dsdc_statval_t v);
        void output_to_log(strbuf& b);

        dsdc_statval_t _min;
        dsdc_statval_t _max;
        dsdc_big_statval_t _sum;
        dsdc_big_statval_t _sum2;
        dsdc_statval_t _n;

        bool _hit;
    };

    //------------------------------------------------------------

    struct stats_v2_t {
        stats_v2_t();
        void clear();
        void output_to_log(strbuf& b);

        dist_v2_t _dist_insert_sz; // distribution of objs inserted
        dist_v2_t _dist_obj_sz;    // distribution of live objs

        dsdc_statval_t _n_get_hit;
        dsdc_statval_t _n_get_notfound;
        dsdc_statval_t _n_get_timeout;

        dsdc_statval_t _n_rm_explicit;
        dsdc_statval_t _n_rm_replace;
        dsdc_statval_t _n_rm_pushout;
        dsdc_statval_t _n_rm_timeout;
        dsdc_statval_t _n_rm_clean;
        dsdc_statval_t _n_rm_miss;
    };

    //------------------------------------------------------------
};

namespace annotation {

    //------------------------------------------------------------

    class v2_t : public base_t {
      public:
        v2_t() {}
        ~v2_t() {}
        void mark_get_attempt(action_code_t a);
        void
        collect(int g, int gie, int l, size_t os, bool del, action_code_t t);
        void elem_create(size_t sz);
        void missed_remove();
        void missed_get();
        void prepare_sweep();
        void output_to_log(strbuf& b, time_t start, int len);
        void clear_stats2();

        bool output(dsdc_statistics_t* out, const dsdc_dataset_params_t& p);

      protected:
        virtual void output_type_to_log(strbuf& b) = 0;

        stats::stats_v2_t _stats;
    };

    //------------------------------------------------------------

    class str2_t : public v2_t {
      public:
        str2_t(const str& v) : v2_t(), _val(v) {}
        void output_type_to_log(strbuf& b);
        bool to_xdr(dsdc_annotation_t* out) const;
        dsdc_annotation_type_t get_type() const;
        str _val;
        ihash_entry<str2_t> _hlnk;
    };

    //------------------------------------------------------------

    class int2_t : public v2_t {
      public:
        int2_t(dsdc_id_t& i) : v2_t(), _val(i) {}
        void output_type_to_log(strbuf& b);
        bool to_xdr(dsdc_annotation_t* out) const;
        dsdc_annotation_type_t get_type() const;
        dsdc_id_t _val;
        ihash_entry<int2_t> _hlnk;
    };

    //------------------------------------------------------------
};

namespace stats {

    //------------------------------------------------------------

    class collector2_t;

    //------------------------------------------------------------

    class str2_factory_t {
      public:
        typedef annotation::str2_t typ;
        typ* alloc(const str& s, collector_base_t* c, bool newobj = true);

      private:
        ihash<str, typ, &typ::_val, &typ::_hlnk> _tab;
    };

    //------------------------------------------------------------

    class int2_factory_t {
      public:
        typedef annotation::int2_t typ;
        typ* alloc(dsdc_id_t i, collector_base_t* c, bool newobj = true);

      private:
        ihash<dsdc_id_t, typ, &typ::_val, &typ::_hlnk> _tab;
    };

    //------------------------------------------------------------

    class collector2_t : public collector_base_t {
      public:
        collector2_t() : _start(sfs_get_tsnow()) {}
        ~collector2_t() {}

        dsdc_res_t
        output(dsdc_statistics_t* sz, const dsdc_dataset_params_t& p);

        annotation::base_t*
        alloc(const dsdc_annotation_t& a, bool newobj = true);

        bool
        is_collector2() const {
            return true;
        }

        void output_to_log(strbuf& b);
        void
        start_interval() {
            _start = sfs_get_tsnow();
        }

        obj_t* int_alloc(dsdc_id_t i);
        obj_t* str_alloc(const str& s);

      private:
        int2_factory_t _int_factory;
        str2_factory_t _str_factory;
        struct timespec _start;
    };

    //------------------------------------------------------------

    extern collector2_t allocator;
};
};

#endif /* __DSDC_STATS2_H__ */
