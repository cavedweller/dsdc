#ifndef DSDC_NO_CUPID
/*
 * $Id$
 *
 * Contains the logic for computing match results.
 */

#include <math.h>
#include "xdrmisc.h"
#include "dsdc.h"
#include "dsdc_slave.h"
#include "dsdc_const.h"
#include "dsdc_prot.h"
#include "dsdc_util.h"
#include "dsdc_match.h"
#include "qanswer_aux.h"
#include "crypt.h"

// scale results by this much, should be in sync with MATCH_T_PERC_MULT
#define DSDC_MATCH_T_PERC_MULT 100

// dump a variable macro D(f) -> << " f: " << f << "\n"
#define D(f)  << " " #f ": " << f << "\n"

/*
 * XXX: this should be configurable.
 */
static inline int
getImportance(const matchd_qanswer_row_t &answer)
{
    int result = 0;
    switch (qa_importance_get(answer)) {
    case 1:
        result = 250;
        break;
    case 2:
        result = 50;
        break;
    case 3:
        result = 10;
        break;
    case 4:
        result = 1;
        break;
    case 5:
        result = 0;
        break;
    }
    return result;
}

/*
 * @brief return the bit set equal to the user's answer + 1
 * Basically, 1 = 2, 2 = 4, 3 = 8... etc
 */
static inline int
getMatchAnswer(matchd_qanswer_row_t &answer)
{

    return (1 << qa_answer_get(answer));
}

/*
 * @brief return true if the user answered the question at all.
 */
static inline bool
getMatchAnswered(matchd_qanswer_row_t &answer)
{

    return (true);
}

#if !SFSLITE_AT_VERSION(1,2,8,3)
inline const strbuf &
strbuf_cat (const strbuf &b, double n)
{
    char buf[100];

    sprintf(buf, "%f", n);
    b << buf;
    return b;
}
#endif /* SFSLITE_AT_VERSION */

static double
calcMatchAvg(
    int common,
    int u1possible, int u2possible,
    int u1actual, int u2actual,
    bool lowerconfidence = true)
{
    double result(0.0);
    double u1percent(0.0);
    double u2percent(0.0);

    if (u1possible > 0) {
        u1percent = (double)u1actual / (double)u1possible;
    }
    if (u2possible > 0) {
        u2percent = (double)u2actual / (double)u2possible;
    }


    result = sqrt(u1percent * u2percent);

    double delta(0.0);
    if (common > 0) {
        delta = 1 / sqrt(common);
        // If we're not using lower confidence, then we're
        // doing an enemy match, so adjust the delta even more.
    }

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << __func__ << " :\n"
        D(u1percent)
        D(u2percent)
        D(u1actual)
        D(u2actual)
        D(u1possible)
        D(u2possible)
        D(delta)
        D(result)
        << "\n";
    }

    if (lowerconfidence) {
        result -= delta;
        if (result < 0.0) {
            result = 0.0;
        }
    } else {
        result += delta;
        if (result > 1.0) {
            result = 1.0;
        }
    }
    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << __func__ << " :\n"
        D(result)
        << "\n";
    }
    return result;
}

void
compute_match(
    matchd_qanswer_rows_t &q1,
    matchd_qanswer_rows_t &q2,
    matchd_frontd_match_datum_t &datum)
{
    unsigned int i = 0, j = 0;

    int common = 0;
    int u1possible = 0;
    int u2possible = 0;
    int friend_u1actual = 0;
    int friend_u2actual = 0;
    int match_u1actual = 0;
    int match_u2actual = 0;

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "two arrays, size1: "
        << q1.size() << " size2: " << q2.size() << "\n";
    }

    for (i = 0; i < q1.size(); i++) {
        /*
         * both lists should be sorted by question id.
         * So first traverse q2 until we hit a question that matches.
         */
        while (j < q2.size() && q1[i].questionid > q2[j].questionid) {
            j++;
        }
        /* if we exhausted the list then we're done. */
        if (j == q2.size())
            break;
        // convenient accessors.
        matchd_qanswer_row_t &q1r = q1[i];
        matchd_qanswer_row_t &q2r = q2[j];
        if (show_debug(DSDC_DBG_MATCH_HIGH)) {
            warn << "\n";
            warn << "1: id: " << q1r.questionid
            << ", answer " << qa_answer_get(q1r)
            << ", match_answer " << qa_matchanswer_get(q1r)
            << ", importance " << qa_importance_get(q1r)
            << "\n";
            warn << "2: id: " << q2r.questionid
            << ", answer " << qa_answer_get(q2r)
            << ", match_answer " << qa_matchanswer_get(q2r)
            << ", importance " << qa_importance_get(q2r)
            << "\n";
        }
        /*
         * if we're now greater but not the same question, then loop
         * to advance our cursor into the first list.
         */
        if (q1[i].questionid < q2[j].questionid) {
            continue;
        }

        if (show_debug(DSDC_DBG_MATCH_HIGH)) {
            warn
            << i << " : " << q1[i].questionid
            << " <=> "
            << j << " : " << q2[j].questionid << "\n";
        }
        /*
         * ok, we got matching question ids!
         */

        // both need to answer or we skip it.
        if (!getMatchAnswered(q1r) || !getMatchAnswered(q2r)) {
            continue;
        }

        int points1 = getImportance(q1r);
        int points2 = getImportance(q2r);
        int matchanswer1 = getMatchAnswer(q1r);
        int matchanswer2 = getMatchAnswer(q2r);

        common++;
        u1possible += points1;
        u2possible += points2;
        if (show_debug(DSDC_DBG_MATCH_HIGH)) {
            warn << "Points: "
            << points1 << " : " << qa_importance_get(q1r) << " <=> "
            << points2 << " : " << qa_importance_get(q2r) << "\n"
            << "Answer: "
            << matchanswer1 << " <=> " << matchanswer2 << "\n"
            << "Answer x: "
            << qa_answer_get(q1r) << " <=> " << qa_answer_get(q2r) << "\n"
            << "Mask x: "
            << qa_matchanswer_get(q1r) << " <=> "
            << qa_matchanswer_get(q2r) << "\n";
        }
        /*
         * If they had the same answer, then give them
             * actual friend points.
             */
        if (matchanswer1 == matchanswer2) {
            friend_u1actual += points1;
            friend_u2actual += points2;
            if (show_debug(DSDC_DBG_MATCH_HIGH)) {
                warn << "Friend match!\n";
            }
        }

        /*
         * Ok now do actual match points.
         * If they fit each other's expectations, then give
         * each other actual points.
             */
        if ((matchanswer1 & qa_matchanswer_get(q2r)) != 0) {
            if (show_debug(DSDC_DBG_MATCH_HIGH)) {
                warn << "Match 1!\n";
            }
            match_u2actual += points2;
        }
        if ((matchanswer2 & qa_matchanswer_get(q1r)) != 0) {
            if (show_debug(DSDC_DBG_MATCH_HIGH)) {
                warn << "Match 2!\n";
            }
            match_u1actual += points1;
        }
        if (show_debug(DSDC_DBG_MATCH_HIGH)) {
            warn
            D(common)
            D(u1possible)
            D(u2possible)
            D(friend_u1actual)
            D(friend_u2actual)
            D(match_u1actual)
            D(match_u2actual)
            ;
        }
    }

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "MATCH ***************************************\n";
    }
    double match_avg = calcMatchAvg(common,
                                    u1possible, u2possible,
                                    match_u1actual, match_u2actual);

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "FRIEND ***************************************\n";
    }
    double friend_avg = calcMatchAvg(common,
                                     u1possible, u2possible,
                                     friend_u1actual, friend_u2actual);

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "ENEMY ***************************************\n";
    }
    double enemy_avg = 1.0 - calcMatchAvg(common,
                                          u1possible, u2possible,
                                          match_u1actual, match_u2actual, false);


    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "ADJUSTING ***************************************\n";
    }
    if (common > 0) {
        /*
         * This boosts the friend average a little.
             */
        friend_avg = sqrt(friend_avg);
    }

    datum.mpercent = (int)(match_avg * 100.0 * DSDC_MATCH_T_PERC_MULT);
    datum.fpercent = (int)(friend_avg * 100.0 * DSDC_MATCH_T_PERC_MULT);

    // In theory no one should score an enemy percentage of 100%
    // If they do, that means they just do not have any intersecting
    // questions, so make them go to 0.
    if (enemy_avg > 0.99) {
        enemy_avg = 0.0;
    }
    datum.epercent = (int)(enemy_avg * 100.0 * DSDC_MATCH_T_PERC_MULT);

    if (show_debug(DSDC_DBG_MATCH_HIGH)) {
        warn << "two arrays, size1: "
        << q1.size() << " size2: " << q2.size() << "\n"
        D(common)
        D(u1possible)
        D(u2possible)
        D(friend_u1actual)
        D(friend_u2actual)
        D(match_u1actual)
        D(match_u2actual)
        << "Mpercent: " << datum.mpercent
        << " Fpercent: " << datum.fpercent
        << " Epercent: " << datum.epercent
        << "\n"
        << "DONE ***************************************\n";
    }
#undef D
}

void
dsdc_slave_t::fill_datum(
    u_int64_t userid,
    matchd_qanswer_rows_t *user_questions,
    matchd_frontd_match_datum_t &datum)
{
    datum.userid = userid;
    datum.mpercent = 0;
    datum.fpercent = 0;
    datum.epercent = 0;

    // lookup the user's questions by creating the key for them.
    dsdc_key64_t key;
    key.frobber = MATCHD_FRONTD_FROBBER;
    key.key64 = userid;
    ptr<dsdc_key_t> k = mkkey_ptr(key);

    // do the lookup.
    dsdc_obj_t *o = lru_lookup(*k);

    if (o == NULL) {
        if (show_debug (DSDC_DBG_MATCH)) {
            warn << "userid: " << userid << "\n";
        }
        datum.match_found = false;
        return;
    }

    matchd_qanswer_rows_t questions;
    bytes2xdr(questions, *o);
    datum.match_found = true;
    if (show_debug(DSDC_DBG_MATCH)) {
        warn << "calling compute_match(), userid: " << userid << "\n";
    }
    compute_match(*user_questions, questions, datum);
    if (show_debug(DSDC_DBG_MATCH)) {
        warn << "userid: " << userid
        << " found datum: match: " << datum.mpercent
        << " friend: " << datum.fpercent
        << " enemy: " << datum.epercent
        << "\n";
    }
}

void
dsdc_slave_t::handle_compute_matches (svccb *sbp)
{
    matchd_frontd_dcdc_arg_t *a =
        sbp->Xtmpl getarg<matchd_frontd_dcdc_arg_t>();
    matchd_qanswer_rows_t *user_questions = &a->user_questions;
    ptr<match_frontd_match_results_t> res =
        New refcounted<match_frontd_match_results_t>();

    res->cache_misses = 0;

    if (show_debug (DSDC_DBG_MATCH)) {
        warn << __func__ << ": user count: " << a->userids.size() << "\n";
    }
    for (unsigned int i = 0; i < a->userids.size(); i++) {
        u_int64_t userid = a->userids[i];
        matchd_frontd_match_datum_t datum;

        fill_datum(userid, user_questions, datum);
        if (!datum.match_found)
            res->cache_misses++;
        res->results.push_back(datum);
    }
    sbp->reply(res);
}

#endif /* !DSDC_NO_CUPID */

