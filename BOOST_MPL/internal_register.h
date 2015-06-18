//This file MUST NOT have include guards

#ifndef CHECKER_TO_REGISTER
#error You must define CLASS_TO_REGISTER before executing REGISTER_CLASS()
#endif

typedef boost::mpl::push_back<
    CHECKERS_LIST(),
    CHECKER_TO_REGISTER
>::type BOOST_PP_CAT(checkers, BOOST_PP_INC(BOOST_PP_SLOT(1)));

#undef CHECKER_TO_REGISTER

#define BOOST_PP_VALUE BOOST_PP_SLOT(1) + 1
#include BOOST_PP_ASSIGN_SLOT(1)