

#include <stdio.h>
#include <string>

#include <boost/test/minimal.hpp>

#include "trie.hpp"

int test_main(int /*argc*/, char ** /*argv*/)
{
  std::string src = "bazar this is some foo, bar, and baz BazBarFooFoo bazbazarbaz end bazar ąČęĖįŠųŪž";
	printf("Searching in '%s':\n", src.c_str());

  gnote::TrieTree<std::string> trie(false);
  trie.add_keyword ("foo", "foo");
  trie.add_keyword ("bar", "bar");
  trie.add_keyword ("baz", "baz");
  trie.add_keyword ("bazar", "bazar");
  trie.add_keyword ("ąčęėįšųūž", "ąčęėįšųūž");

  printf ("Starting search...\n");
  gnote::TrieTree<std::string>::HitListPtr matches(trie.find_matches (src));
  BOOST_CHECK( matches.get() );

  BOOST_CHECK( matches->size() == 16 );
  gnote::TrieTree<std::string>::HitList::const_iterator iter = matches->begin();

  BOOST_CHECK( *iter );
  BOOST_CHECK( (*iter)->key == "baz" );
  BOOST_CHECK( (*iter)->start == 0 );
  BOOST_CHECK( (*iter)->end == 3 );
  ++iter;
  BOOST_CHECK( *iter );
  BOOST_CHECK( (*iter)->key == "bazar" );
  BOOST_CHECK( (*iter)->start == 0 );
  BOOST_CHECK( (*iter)->end == 5 );

  for(gnote::TrieTree<std::string>::HitList::const_iterator hit_iter = matches->begin();
      hit_iter != matches->end(); ++hit_iter) {
    gnote::TrieHit<std::string> *hit(*hit_iter);
    printf ("*** Match: '%s' at %d-%d\n",
            hit->key.c_str(), hit->start, hit->end);
  }
  printf ("Search finished!\n");
  return 0;
}
