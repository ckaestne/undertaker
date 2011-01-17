#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>

#include <assert.h>
#include <typeinfo>
#include <check.h>
#include <string>
#include <iostream>

#include "SatChecker.h"
#include "SatChecker-grammar.t"

bool cnf_test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);

    if (error == 0)  {
        fail_unless(checker() == result,
                    "%s should evaluate to %d",
                    s.c_str(), (int) result);
        return false;
    } else {
        try {
            fail_unless(checker() == result,
                        "%s should evaluate to %d",
                        s.c_str(), (int) result);
            return false;
        } catch (std::runtime_error &e) {
            fail_unless(typeid(*error) == typeid(e),
                        "%s didn't throw the right exception should be %s, but is %s.",
                        typeid(*error).name(), typeid(e).name());
            return false;
        }
    }
    return true;
}

START_TEST(cnf_translator_test)
{
    SatCheckerError error("");

    cnf_test("A & B", true);
    cnf_test("A & !A", false);
    cnf_test("(A -> B) -> A -> A", true);
    cnf_test("(A <-> B) & (B <-> C) & (C <-> A)", true);
    cnf_test("B6 & \n(B5 <-> !S390) & (B6 <-> !B5) & !S390", false);
    cnf_test("a >,,asd/.sa,;.ljkxf;vnbmkzjghrarjkf.dvd,m.vjkb54y98g4tjoij", false, &error);
    cnf_test("B90 & \n( B90 <->  ! MODULE )\n& ( B92 <->  ( B90 ) \n & CONFIG_STMMAC_TIMER )\n", true);
    cnf_test("B9\n&\n(B1 <-> !_DRIVERS_MISC_SGIXP_XP_H)\n&\n(B3 <-> B1 & CONFIG_X86_UV | CONFIG_IA64_SGI_UV)\n&\n(B6 <-> B1 & !is_uv)\n&\n(B9 <-> B1 & CONFIG_IA64)\n&\n(B12 <-> B1 & !is_shub1)\n&\n(B15 <-> B1 & !is_shub2)\n&\n(B18 <-> B1 & !is_shub)\n&\n(B21 <-> B1 & USE_DBUG_ON)\n&\n(B23 <-> B1 & !B21)\n&\n(B26 <-> B1 & XPC_MAX_COMP_338)\n&\n(CONFIG_ACPI -> !CONFIG_IA64_HP_SIM & (CONFIG_IA64 | CONFIG_X86) & CONFIG_PCI & CONFIG_PM)\n&\n(CONFIG_CHOICE_6 -> !CONFIG_X86_ELAN)\n&\n(CONFIG_CHOICE_8 -> CONFIG_X86_32)\n&\n(CONFIG_HIGHMEM64G -> CONFIG_CHOICE_8 & !CONFIG_M386 & !CONFIG_M486)\n&\n(CONFIG_INTR_REMAP -> CONFIG_X86_64 & CONFIG_X86_IO_APIC & CONFIG_PCI_MSI & CONFIG_ACPI & CONFIG_EXPERIMENTAL)\n&\n(CONFIG_M386 -> CONFIG_CHOICE_6 & CONFIG_X86_32 & !CONFIG_UML)\n&\n(CONFIG_M486 -> CONFIG_CHOICE_6 & CONFIG_X86_32)\n&\n(CONFIG_NUMA -> CONFIG_SMP & (CONFIG_X86_64 | CONFIG_X86_32 & CONFIG_HIGHMEM64G & (CONFIG_X86_NUMAQ | CONFIG_X86_BIGSMP | CONFIG_X86_SUMMIT & CONFIG_ACPI) & CONFIG_EXPERIMENTAL))\n&\n(CONFIG_PCI_MSI -> CONFIG_PCI & CONFIG_ARCH_SUPPORTS_MSI)\n&\n(CONFIG_PM -> !CONFIG_IA64_HP_SIM)\n&\n(CONFIG_X86_32_NON_STANDARD -> CONFIG_X86_32 & CONFIG_SMP & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_BIGSMP -> CONFIG_X86_32 & CONFIG_SMP)\n&\n(CONFIG_X86_ELAN -> CONFIG_X86_32 & CONFIG_X86_EXTENDED_PLATFORM)\n&\n(CONFIG_X86_EXTENDED_PLATFORM -> CONFIG_X86_64)\n&\n(CONFIG_X86_IO_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_LOCAL_APIC -> CONFIG_X86_64 | CONFIG_SMP | CONFIG_X86_32_NON_STANDARD | CONFIG_X86_UP_APIC)\n&\n(CONFIG_X86_NUMAQ -> CONFIG_X86_32_NON_STANDARD & CONFIG_PCI)\n&\n(CONFIG_X86_SUMMIT -> CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UP_APIC -> CONFIG_X86_32 & !CONFIG_SMP & !CONFIG_X86_32_NON_STANDARD)\n&\n(CONFIG_X86_UV -> CONFIG_X86_64 & CONFIG_X86_EXTENDED_PLATFORM & CONFIG_NUMA & CONFIG_X86_X2APIC)\n&\n(CONFIG_X86_X2APIC -> CONFIG_X86_LOCAL_APIC & CONFIG_X86_64 & CONFIG_INTR_REMAP)\n&\n!(CONFIG_IA64 | CONFIG_IA64_HP_SIM | CONFIG_IA64_SGI_UV | CONFIG_UML)\n", false);

}
END_TEST


void parse_test(std::string input, bool good) {
    static bool_grammar e;

    tree_parse_info<> info = pt_parse(input.c_str(), e, space_p);

    fail_unless((info.full ? true : false) == good,
                "%s: %d", input.c_str(), info.full);

    if (!info.full) {
        try {
            fail_if(SatChecker::check(input));
            fail();
        } catch (SatCheckerError &e) {/*IGNORE*/}
    }
}


START_TEST(bool_parser_test)
{
    parse_test("A", true);
    parse_test("! A", true);
    parse_test("--0--", false);
    parse_test("A & B", true);
    parse_test("A  |   B", true);
    parse_test("A &", false);
    parse_test("(A & B) | C", true);
    parse_test("A & B & C & D", true);
    parse_test("A | C & B", true);
    parse_test("C & B | A", true);
    parse_test("! ( ! (A))", true);
    parse_test("!!!!!A", true);

    parse_test("A -> B", true);
    parse_test(" -> B", false);
    parse_test("(A -> B) -> A -> A", true);
    parse_test("(A <-> ! B) | ( B <-> ! A)", true);
    parse_test("A -> B -> C -> (D -> C)", true);

    parse_test("A & !A | B & !B", true);
    parse_test("A -> B -> C", true);
    parse_test("A <-> B", true);
}
END_TEST

START_TEST(pprinter_test)
{
    fail_unless(SatChecker("B72 & ( B67 <->  ! CONFIG_MMU ) "
                           "& ( B69 <->  ( B67 )  "
                           "&  ! CONFIG_MMU ) "
                           "& ( B72 <->  ( B67 )  & CONFIG_MMU )").pprint()
                ==
                "B72 & ( B67 <->  ! CONFIG_MMU ) "
                "& ( B69 <->  ( B67 )  "
                "&  ! CONFIG_MMU ) "
                "& ( B72 <->  ( B67 )  & CONFIG_MMU )\n\n"
                "\n"
                "- and\n"
                "  - B72\n"
                "  - <->\n"
                "    - B67\n"
                "    - not\n"
                "      - CONFIG_MMU\n"
                "  - <->\n"
                "    - B69\n"
                "    - and\n"
                "      - B67\n"
                "      - not\n"
                "        - CONFIG_MMU\n"
                "  - <->\n"
                "    - B72\n"
                "    - and\n"
                "      - B67\n"
                "      - CONFIG_MMU\n");
}
END_TEST

START_TEST(format_config_items_simple)
{
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", true));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_HURZ", true));

    std::stringstream ss;
    int c = SatChecker::formatConfigItems(m, ss, dummy);
    ck_assert_int_eq(3, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=y\n"
                     "CONFIG_HURZ=y\n");
}
END_TEST

START_TEST(format_config_items_module)
{
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", false));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_FOO_MODULE", true));

    std::stringstream ss;
    int c = SatChecker::formatConfigItems(m, ss, dummy);
    ck_assert_int_eq(2, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=m\n");
}
END_TEST

START_TEST(format_config_items_module_not_valid_in_kconfig)
{
    MissingSet dummy;
    SatChecker::AssignmentMap m;
    m.insert(std::make_pair("CONFIG_FOO", true));
    m.insert(std::make_pair("CONFIG_BAR", false));
    m.insert(std::make_pair("CONFIG_FOO_MODULE", true));

    std::stringstream ss;
    int c = SatChecker::formatConfigItems(m, ss, dummy);
    ck_assert_int_eq(2, c);
    ck_assert_str_eq(ss.str().c_str(),
                     "CONFIG_BAR=n\n"
                     "CONFIG_FOO=m\n");
}
END_TEST

Suite *
satchecker_suite(void) {
    Suite *s  = suite_create("SatChecker");
    TCase *tc = tcase_create("SatChecker");
    tcase_add_test(tc, cnf_translator_test);
    tcase_add_test(tc, pprinter_test);
    tcase_add_test(tc, bool_parser_test);
    tcase_add_test(tc, format_config_items_simple);
    tcase_add_test(tc, format_config_items_module);
    tcase_add_test(tc, format_config_items_module_not_valid_in_kconfig);

    suite_add_tcase(s, tc);

    return s;
}

int main() {
    Suite *s = satchecker_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
