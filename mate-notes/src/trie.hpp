/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */




#ifndef __TRIE_HPP_
#define __TRIE_HPP_

#include <ctype.h>

#include <string>
#include <tr1/memory>

#include "sharp/string.hpp"

#include "note.hpp"
#include "triehit.hpp"

namespace gnote {


  template<class value_t>
  class TrieTree 
  {
    class TrieMatch;
    typedef std::tr1::shared_ptr<TrieMatch> TrieMatchPtr;
    class TrieState;
    typedef std::tr1::shared_ptr<TrieState> TrieStatePtr;

    class TrieState
    {
    public:
      TrieState()
        : final(0)
        {
        }
      TrieStatePtr next;
      TrieStatePtr fail;
      TrieMatchPtr first_match;
      int         final;
      value_t     id;
    };

    class TrieMatch
    {
    public:
      TrieMatch()
        : value(0)
        {
        }
      TrieMatchPtr next;
      TrieStatePtr state;
      gunichar     value;
    };

  public:
    typedef TrieHit<value_t>  triehit_t;
    
    typedef typename triehit_t::List     HitList;
    typedef typename triehit_t::ListPtr  HitListPtr;

    TrieTree(bool case_sensitive)
      : m_root(new TrieState())
      , m_fail_states(8)
      , m_case_sensitive(case_sensitive)
      , m_max_length(0)
      {
      }

    ~TrieTree()
      {

      }

  private:
    TrieStatePtr insert_match_at_state(size_t depth, const TrieStatePtr & q, gunichar c)
      {
        // Create a new state with a failure at %root
        TrieStatePtr new_q(new TrieState ());
        new_q->fail = m_root;

        // Insert/Replace into fail_states at %depth
        if (depth < m_fail_states.size()) {
          new_q->next = m_fail_states[depth];
          m_fail_states[depth] = new_q;
        } 
        else {
          m_fail_states.resize(depth + 1);
          m_fail_states[depth] = new_q;
        }

        // New match points to the newly created state for value %c
        TrieMatchPtr m(new TrieMatch ());
        m->next = q->first_match;
        m->state = new_q;
        m->value = c;

        // Insert the new match into existin state's match list
        q->first_match = m;

        return new_q;
      }

    // Iterate the matches at state %s looking for the first match
    // containing %c.
    TrieMatchPtr find_match_at_state (const TrieStatePtr & s, gunichar c) const
      {
        TrieMatchPtr m = s->first_match;

        while (m && (m->value != c)) {
          m = m->next;
        }

        return m;
      }
  public:
    /*
     * final = empty set
     * FOR p = 1 TO #pat
     *   q = root
     *   FOR j = 1 TO m[p]
     *     IF g(q, pat[p][j]) == null
     *       insert(q, pat[p][j])
     *     ENDIF
     *     q = g(q, pat[p][j])
     *   ENDFOR
     *   final = union(final, q)
     * ENDFOR
     */
    void add_keyword(const Glib::ustring & needle, const value_t & pattern_id)
      {
        TrieStatePtr q = m_root;
        int depth = 0;

        // Step 1: add the pattern to the trie...

        for (size_t idx = 0; idx < needle.size(); idx++) {
          gunichar c = needle [idx];
          if (!m_case_sensitive)
            c = g_unichar_tolower(c);

          TrieMatchPtr m = find_match_at_state (q, c);
          if (m == NULL) {
            q = insert_match_at_state (depth, q, c);
          }
          else {
            q = m->state;
          }

          depth++;
        }
        q->final = depth;
        q->id = pattern_id;

        // Step 2: compute failure graph...

        for (size_t idx = 0; idx < m_fail_states.size(); idx++) {
          q = m_fail_states[idx];

          while (q) {
            TrieMatchPtr m = q->first_match;

            while (m) {
              TrieStatePtr q1 = m->state;
              TrieStatePtr r = q->fail;
              TrieMatchPtr n;

              while (r) {
                n = find_match_at_state (r, m->value);
                if (n == NULL) {
                  r = r->fail;
                }
                else {
                  break;
                }
              }

              if (r && n) {
                q1->fail = n->state;

                if (q1->fail->final > q1->final)
                  q1->final = q1->fail->final;
              } 
              else {
                n = find_match_at_state (m_root, m->value);
                if (n == NULL)
                  q1->fail = m_root;
                else
                  q1->fail = n->state;
              }

              m = m->next;
            }

            q = q->next;
          }
        }
        // Update max_length
        m_max_length = std::max(m_max_length, needle.size());
      }

    /*
     * Aho-Corasick
     *
     * q = root
     * FOR i = 1 TO n
     *   WHILE q != fail AND g(q, text[i]) == fail
     *     q = h(q)
     *   ENDWHILE
     *   IF q == fail
     *     q = root
     *   ELSE
     *     q = g(q, text[i])
     *   ENDIF
     *   IF isElement(q, final)
     *     RETURN TRUE
     *   ENDIF
     * ENDFOR
     * RETURN FALSE
     */
    HitListPtr find_matches(const Glib::ustring & haystack)
      {
        HitListPtr matches(new HitList());
        TrieStatePtr q = m_root;
        TrieMatchPtr m;
        size_t idx = 0, start_idx = 0, last_idx = 0;
        while (idx < haystack.size()) {
          gunichar c = haystack [idx++];
          if (!m_case_sensitive)
            c = g_unichar_tolower(c);

          while (q) {
            m = find_match_at_state (q, c);
            if (m == NULL) {
              q = q->fail;
            }
            else {
              break;
            }
          }

          if (q == m_root) {
            start_idx = last_idx;
          }

          if (q == NULL) {
            q = m_root;
            start_idx = idx;
          } 
          else if (m) {
            q = m->state;

            // Got a match!
            if (q->final != 0) {
              std::string key = sharp::string_substring (haystack, start_idx,
                                                         idx - start_idx);
              matches->push_back(new triehit_t (start_idx, idx, key, q->id));
            }
          }

          last_idx = idx;
        }
        return matches;
      }

    value_t lookup (const std::string & key)
      {
        HitListPtr matches(find_matches (key));
        typename HitList::const_iterator iter;
        for (iter = matches->begin(); iter != matches->end(); ++iter) {
          if ((*iter)->key.size() == key.size()) {
            return (*iter)->value;
          }
        }
        return NULL;
      }

    size_t max_length() const
      {
        return m_max_length;
      }
  private:
    TrieStatePtr            m_root;
    std::vector<TrieStatePtr> m_fail_states;
    bool                    m_case_sensitive;
    size_t                  m_max_length;

  };

}

#endif
