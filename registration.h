typedef boost::mpl::vector<>::type checkers0;

#define BOOST_PP_VALUE 0
#include BOOST_PP_ASSIGN_SLOT(1)

#define REGISTER_CHECKER() "internal_register.h"

#define CHECKERS_LIST() BOOST_PP_CAT(checkers, BOOST_PP_SLOT(1))