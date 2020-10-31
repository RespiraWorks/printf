///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2017-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief printf unit tests
//
///////////////////////////////////////////////////////////////////////////////

// use the 'catch' test framework
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstring>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

static constexpr double nan_double = std::numeric_limits<double>::quiet_NaN();
static constexpr float nan_float = std::numeric_limits<float>::quiet_NaN();



namespace test {

  // dummy putchar
  static char   printf_buffer[100];
  static size_t printf_idx = 0U;

  void _putchar(char character)
  {
    printf_buffer[printf_idx++] = character;
  }

  void _out_fct(char character, void* arg)
  {
    (void)arg;
    printf_buffer[printf_idx++] = character;
  }

  // use functions in own test namespace to avoid stdio conflicts
  #include "../printf.h"
  #include "../printf.cpp"

} // namespace test


/***********
* Utilities
***********/

auto adjust_sigfigs( const std::string &in, unsigned desired_sigfigs, unsigned desired_width ) -> std::string {
  std::string out(in);
  // Find positions of exponent and decimal point.
  size_t pos_exponent = out.find_first_of( "eEgG" );
  size_t pos_decimal = out.find( '.' );
  if( pos_exponent == std::string::npos ) {
    return in;
  }

  // Insert decimal point if needed.
  if( pos_decimal == std::string::npos ) {
    out.insert( pos_exponent, "." );
  }

  // Remove the leading spaces, if any.
  while( out.length() > 0 && out[0] == ' ' ) {
    out.erase(0,1);
  }

  // Find positions again.
  pos_exponent = out.find_first_of( "eEgG" );
  pos_decimal = out.find( '.' );
  size_t decimal_places_found = (pos_exponent < (pos_decimal + 1))? 0 : (pos_exponent - (pos_decimal + 1));
  size_t current_sigfigs = decimal_places_found + 1;
  //std::cout << "aft rm spaces::  desired_sigfigs=" << desired_sigfigs << ", current_sigfigs=" << current_sigfigs << ", decimal_places_found=" << decimal_places_found << ", pos_exponent=" << pos_exponent << ", pos_decimal=" << pos_decimal << std::endl;

  if( current_sigfigs > desired_sigfigs ) {
    size_t iz = 1;
    // Remove just enough 0's to achieve desired_sigfigs.
    while( out.length() > 0 && out[pos_exponent-iz] == '0' ) {
      if( iz > (current_sigfigs - desired_sigfigs) ) break;
      out.erase(pos_exponent-iz,1);
      iz++;
    }
  } else if( current_sigfigs < desired_sigfigs ) {
    // Insert just enough 0's to achieve desired_sigfigs.
    for( size_t j = 0; j < desired_sigfigs - current_sigfigs; j++ ) {
      out.insert( pos_exponent, "0" );
    }
  }

  // Find positions again.
  pos_exponent = out.find_first_of( "eEgG" );
  pos_decimal = out.find( '.' );
  decimal_places_found = (pos_exponent < (pos_decimal + 1))? 0 : (pos_exponent - (pos_decimal + 1));
  current_sigfigs = decimal_places_found + 1;
  //std::cout << "aft rm/ins 0s::  desired_sigfigs=" << desired_sigfigs << ", current_sigfigs=" << current_sigfigs << ", decimal_places_found=" << decimal_places_found << ", pos_exponent=" << pos_exponent << ", pos_decimal=" << pos_decimal << std::endl;

  // Remove decimal point, if there are now no decimal places.
  if( current_sigfigs == 1 ) {
    if( out.length() > 0 && out[pos_decimal] == '.' ) {
      out.erase(pos_decimal,1);
    }
  }

  // Insert just enough leading spaces to achieve desired_width.
  while( out.length() < desired_width ) {
    out.insert( 0, " " );
  }
  return out;
}



/***********
* Test Cases
***********/

TEST_CASE("printf", "[]" ) {
  test::printf_idx = 0U;
  memset(test::printf_buffer, 0xCC, 100U);
  REQUIRE(test::printf("% d", 4232) == 5);
  REQUIRE(test::printf_buffer[5] == (char)0xCC);
  test::printf_buffer[5] = 0;
  REQUIRE(!strcmp(test::printf_buffer, " 4232"));
}


TEST_CASE("fctprintf", "[]" ) {
  test::printf_idx = 0U;
  memset(test::printf_buffer, 0xCC, 100U);
  test::fctprintf(&test::_out_fct, nullptr, "This is a test of %X", 0x12EFU);
  REQUIRE(!strncmp(test::printf_buffer, "This is a test of 12EF", 22U));
  REQUIRE(test::printf_buffer[22] == (char)0xCC);
}


TEST_CASE("snprintf", "[]" ) {
  char buffer[100];

  test::snprintf(buffer, 100U, "%d", -1000);
  REQUIRE(!strcmp(buffer, "-1000"));

  test::snprintf(buffer, 3U, "%d", -1000);
  REQUIRE(!strcmp(buffer, "-1"));
}

static void vprintf_builder_1(char* buffer, ...)
{
  va_list args;
  va_start(args, buffer);
  test::vprintf("%d", args);
  va_end(args);
}

static void vsnprintf_builder_1(char* buffer, ...)
{
  va_list args;
  va_start(args, buffer);
  test::vsnprintf(buffer, 100U, "%d", args);
  va_end(args);
}

static void vsnprintf_builder_3(char* buffer, ...)
{
  va_list args;
  va_start(args, buffer);
  test::vsnprintf(buffer, 100U, "%d %d %s", args);
  va_end(args);
}


TEST_CASE("vprintf", "[]" ) {
  char buffer[100];
  test::printf_idx = 0U;
  memset(test::printf_buffer, 0xCC, 100U);
  vprintf_builder_1(buffer, 2345);
  REQUIRE(test::printf_buffer[4] == (char)0xCC);
  test::printf_buffer[4] = 0;
  REQUIRE(!strcmp(test::printf_buffer, "2345"));
}


TEST_CASE("vsnprintf", "[]" ) {
  char buffer[100];

  vsnprintf_builder_1(buffer, -1);
  REQUIRE(!strcmp(buffer, "-1"));

  vsnprintf_builder_3(buffer, 3, -1000, "test");
  REQUIRE(!strcmp(buffer, "3 -1000 test"));
}


TEST_CASE("float: various special cases, pt 1", "[]" ) {
  char buffer[100];

  // out of range for float: should switch to exp notation if supported, else empty
  test::sprintf(buffer, "%.1f", 1E20);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  REQUIRE(!strcmp(buffer, "1.0e+20"));
#else
  REQUIRE(!strcmp(buffer, ""));
#endif
}


TEST_CASE("float: various special cases, pt 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  const char * s = "";

  fail = false;
  {
    test::sprintf(buffer, "%0-15.3g", -0.042);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
    s = "-0.0420        ";
#else
    s = "g";
#endif
    CHECK( std::string( buffer ) == s );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << s << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;

    test::sprintf(buffer, "%0-15.4g", -0.042);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
    s = "-0.04200       ";
#else
    s = "g";
#endif
    CHECK( std::string( buffer ) == s );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << s << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
  }
  REQUIRE(!fail);
}


using CaseSpec = struct { const char *fmt; double stimulus; const char *shouldBe; };


TEST_CASE("various large exponents", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  {
    CaseSpec specs[] = {
      { "%9.3f", 1e+200, "1.000e+200" },
      { "%9.3f", 1e-200, "1.000e-200" },
      { "%9.3f", 1e+17, "1.000e+17" },
      { "%9.3f", 1e-17, "1.000e-17" },
      { "%9.3f", 1e+307, "1.000e+307" },
      { "%9.3f", 1e+257, "1.000e+257" },
      { "%9.3f", 1e+207, "1.000e+207" },
      { "%9.3f", 1e+157, "1.000e+157" },
      { "%9.3f", 1e+107, "1.000e+107" },
      { "%9.3f", 1e+87, "1.000e+87" },
      { "%9.3f", 1e+67, "1.000e+67" },
      { "%9.3f", 1e+57, "1.000e+57" },
      { "%9.3f", 1e+47, "1.000e+47" },
      { "%9.3f", 1e+37, "1.000e+37" },
      { "%9.3f", 1e+27, "1.000e+27" },
      { "%9.3f", 1e+17, "1.000e+17" },
      { "%9.3f", 1e-307, "1.000e-307" },
      { "%9.3f", 1e-257, "1.000e-257" },
      { "%9.3f", 1e-207, "1.000e-207" },
      { "%9.3f", 1e-157, "1.000e-157" },
      { "%9.3f", 1e-107, "1.000e-107" },
      { "%9.3f", 1e-87, "1.000e-87" },
      { "%9.3f", 1e-67, "1.000e-67" },
      { "%9.3f", 1e-57, "1.000e-57" },
      { "%9.3f", 1e-47, "1.000e-47" },
      { "%9.3f", 1e-37, "1.000e-37" },
      { "%9.3f", 1e-27, "1.000e-27" },
      { "%9.3f", 1e-17, "1.000e-17" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
}




TEST_CASE("various to start with", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  {
    CaseSpec specs[] = {
      { "%f", 42167.0, "42167.000000" },
      { "%10.3f", 42167.0, " 42167.000" },
      { "%10.3f", -42167.0, "-42167.000" },
      { "%e", 42167.0, "4.216700e+04" },
      { "%+10.3e", 42167.0, "+4.217e+04" },
      { "%10.3e", -42167.0, "-4.217e+04" },
      { "%g", 42167.0, "42167.0" },
      { "%+10.3g", 42167.0, " +4.22e+04" },
      { "%10.3g", -42167.0, " -4.22e+04" },
      { "%+012.4g", 0.00001234, "+001.234e-05" },
      { "%.3g", -1.2345e-308, "-1.23e-308" },
      { "%+.3E", 1.23e+308, "+1.230E+308" },
      { "%+10.4G", 0.001234, " +0.001234" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);

#endif

}


TEST_CASE("float, set 1", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  {
    CaseSpec specs[] = {

      // test special-case floats using std::numeric_limits.
      { "%8f", nan_double , "     nan" },
      { "%8f", static_cast<double>( nan_float ), "     nan" },
      { "%8f", std::numeric_limits<double>::infinity() /* INFINITY */ , "     inf" },
      { "%-8f", -std::numeric_limits<double>::infinity() /* -INFINITY */ , "-inf    " },
      { "%8e", nan_double , "     nan" },
      { "%8e", static_cast<double>( nan_float ), "     nan" },
      { "%+8e", std::numeric_limits<double>::infinity() /* INFINITY */ , "    +inf" },
      { "%-8e", -std::numeric_limits<double>::infinity() /* -INFINITY */ , "-inf    " },
      { "%.4f", 3.1415354, "3.1415" },
      { "%.3f", 30343.1415354, "30343.142" },
      { "%.0f", 34.1415354, "34" },
      { "%.0f", 1.3, "1" },
      { "%.0f", 1.55, "2" },
      { "%.1f", 1.64, "1.6" },
      { "%.2f", 42.8952, "42.90" },
      { "%.9f", 42.8952, "42.895200000" },
      { "%.10f", 42.895223, "42.8952230000" },
      // this testcase checks, that the precision is truncated to 9 digits.
      // a perfect working float should return the whole number
      { "%.12f", 42.89522312345678, "42.895223123000" },
      // this testcase checks, that the precision is truncated AND rounded to 9 digits.
      // a perfect working float should return the whole number
      { "%.12f", 42.89522387654321, "42.895223877000" },
      { "%6.2f", 42.8952, " 42.90" },
      { "%+6.2f", 42.8952, "+42.90" },
      { "%+5.1f", 42.9252, "+42.9" },
      { "%f", 42.5, "42.500000" },
      { "%.1f", 42.5, "42.5" },
      { "%f", 42167.0, "42167.000000" },
      { "%.9f", -12345.987654321, "-12345.987654321" },
      { "%.1f", 3.999, "4.0" },
      { "%.0f", 3.5, "4" },
      { "%.0f", 4.5, "4" },
      { "%.0f", 3.49, "3" },
      { "%.1f", 3.49, "3.5" },
      { "%.0F", 3.49, "3" },
      { "%.1F", 3.49, "3.5" },
      { "a%-5.1f", 0.5, "a0.5  " },
      { "a%-5.1fend", 0.5, "a0.5  end" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
#endif
}


TEST_CASE("float, set 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  fail = false;
  {
    CaseSpec specs[] = {

      { "%G", 12345.678, "12345.7" },
      { "%.4G", 12345.678, "1.235E+04" },
      { "%.5G", 12345.678, "12346" },
      { "%.6G", 12345.678, "12345.7" },
      { "%.7G", 12345.678, "12345.68" },
      { "%.5G", 123456789., "1.2346E+08" },
      { "%.6G", 12345., "12345.0" },
      { "%+12.4g", 123456789., "  +1.235e+08" },
      { "%.2G", 0.001234, "0.0012" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
#endif
}

TEST_CASE("float, set 3", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  fail = false;
  {
    CaseSpec specs[] = {
      { "%+012.4g", 0.00001234, "+001.234e-05" },
      { "%.3g", -1.2345e-308, "-1.23e-308" },
      { "%+.3E", 1.23e+308, "+1.230E+308" },
      { "%+10.4G", 0.001234, " +0.001234" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
#endif
}


#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float: %g: precision vs exponent, part 1", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

  fail = false;
  {
    CaseSpec specs[] = {

      { "%7.0g", static_cast<double>(8.34f), "      8" },
      { "%7.0g", static_cast<double>(8.34e1f), "  8e+01" },
      { "%7.0g", static_cast<double>(8.34e2f), "  8e+02" },
      { "%7.1g", static_cast<double>(8.34f), "      8" },
      { "%7.1g", static_cast<double>(8.34e1f), "  8e+01" },
      { "%7.1g", static_cast<double>(8.34e2f), "  8e+02" },
      { "%7.2g", static_cast<double>(8.34f), "    8.3" },
      { "%7.2g", static_cast<double>(8.34e1f), "     83" },
      { "%7.2g", static_cast<double>(8.34e2f), "8.3e+02" },
      { "%7.3g", static_cast<double>(8.34f), "   8.34" },
      { "%7.3g", static_cast<double>(8.34e1f), "   83.4" },
      { "%7.3g", static_cast<double>(8.34e2f), "    834" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
}
#endif

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float: %g: precision vs exponent, part 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

  fail = false;
  {
    CaseSpec specs[] = {

      { "%7.3g", static_cast<double>(8.34e9f), "8.34e+09" },
      { "%7.3g", static_cast<double>(8.34e3f), "8.34e+03" },
      { "%7.3g", static_cast<double>(8.34e-2f), " 0.0834" },
      { "%7.3g", static_cast<double>(8.34e-7f), "8.34e-07" },
      { "%10.7g", static_cast<double>(8.34e9f), "8.340000e+09" },
      { "%10.7g", static_cast<double>(8.34e3f), "  8340.000" },
      { "%10.7g", static_cast<double>(8.34e-2f), "0.08340000" },
      { "%10.7g", static_cast<double>(8.34e-7f), "8.340000e-07" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
}
#endif

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float: %g: precision vs exponent, part 3", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

  fail = false;
  {
    CaseSpec specs[] = {
      { "%7.3g", static_cast<double>(8.34e-1f), "  0.834" },
      { "%7.3g", static_cast<double>(8.34e-2f), " 0.0834" },
      { "%7.3g", static_cast<double>(8.34e-3f), "0.00834" },
      { "%7.4g", static_cast<double>(8.34e-1f), " 0.8340" },
      { "%7.4g", static_cast<double>(8.34e-2f), "0.08340" },
      { "%7.4g", static_cast<double>(8.34e-3f), "0.008340" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
}
#endif


#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float: %f-to-%e, case 1", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  std::stringstream sstr;

  fail = false;
  float f = -9.999999;
  for( int i=1; i<20; i++ ) {
    sstr.str("");
    sstr.unsetf(std::ios::floatfield);
    if( i >= 9 ) {
      sstr.precision(4);
      sstr.setf(std::ios::scientific);
    } else {
      sstr.precision(3);
      sstr.setf(std::ios::fixed);
    }
    test::sprintf(buffer, "%10.3f", static_cast<double>(f));
    sstr << std::setw(10) << f;
    std::string str2 = adjust_sigfigs( sstr.str(), 4, 10 );
    CHECK( std::string( buffer ) == str2 );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << str2.c_str() << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
    f *= 10.0f;
  }
  REQUIRE(!fail);
}
#endif


#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float, %f-to-%e, case 2b", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  std::stringstream sstr;

  // brute force exp
  fail = false;
  for (float f = -1e17f; f < +1e17f; f+= 0.9e15f) {
    test::sprintf(buffer, "%10.2f", static_cast<double>(f));
    sstr.str("");
    sstr.unsetf(std::ios::floatfield);
    sstr.precision(3);
    sstr << std::setw(10) << f;
    std::string str2 = adjust_sigfigs( sstr.str(), 3, 10 );
    CHECK( std::string( buffer ) == str2 );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << str2.c_str() << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
  }
  REQUIRE(!fail);
}
#endif


#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float: %g-to-%e, case 1", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  std::stringstream sstr;

  fail = false;
  float f = -999.9999;
  for( int i=3; i<20; i++ ) {
    sstr.str("");
    sstr.unsetf(std::ios::floatfield);
    sstr.precision(3);
    if( i >= 3 ) {
      sstr.setf(std::ios::scientific);
    } else {
      sstr.setf(std::ios::fixed);
    }
    test::sprintf(buffer, "%10.2g", static_cast<double>(f));
    sstr << std::setw(10) << f;
    std::string str2 = adjust_sigfigs( sstr.str(), 2, 10 );
    CHECK( std::string( buffer ) == str2 );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << str2.c_str() << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
    f *= 10.0f;
  }
  REQUIRE(!fail);
}
#endif


#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
TEST_CASE("float, %g-to-%e, case 2b", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  std::stringstream sstr;

  // brute force exp
  fail = false;
  for (float f = -1e17f; f < +1e17f; f+= 0.9e15f) {
    test::sprintf(buffer, "%10.3g", static_cast<double>(f));
    sstr.str("");
    sstr.unsetf(std::ios::floatfield);
    sstr.precision(3);
    sstr << std::setw(10) << f;
    std::string str2 = adjust_sigfigs( sstr.str(), 3, 10 );
    CHECK( std::string( buffer ) == str2 );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << str2.c_str() << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
  }
  REQUIRE(!fail);
}
#endif


TEST_CASE("space flag", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "% d", 42);
  REQUIRE(!strcmp(buffer, " 42"));

  test::sprintf(buffer, "% d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "% 5d", 42);
  REQUIRE(!strcmp(buffer, "   42"));

  test::sprintf(buffer, "% 5d", -42);
  REQUIRE(!strcmp(buffer, "  -42"));

  test::sprintf(buffer, "% 15d", 42);
  REQUIRE(!strcmp(buffer, "             42"));

  test::sprintf(buffer, "% 15d", -42);
  REQUIRE(!strcmp(buffer, "            -42"));

  test::sprintf(buffer, "% 15d", -42);
  REQUIRE(!strcmp(buffer, "            -42"));

  test::sprintf(buffer, "% 15.3f", -42.987);
  REQUIRE(!strcmp(buffer, "        -42.987"));

  test::sprintf(buffer, "% 15.3f", 42.987);
  REQUIRE(!strcmp(buffer, "         42.987"));

  test::sprintf(buffer, "% s", "Hello testing");
  REQUIRE(!strcmp(buffer, "Hello testing"));

  test::sprintf(buffer, "% d", 1024);
  REQUIRE(!strcmp(buffer, " 1024"));

  test::sprintf(buffer, "% d", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "% i", 1024);
  REQUIRE(!strcmp(buffer, " 1024"));

  test::sprintf(buffer, "% i", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "% u", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "% u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272"));

  test::sprintf(buffer, "% o", 511);
  REQUIRE(!strcmp(buffer, "777"));

  test::sprintf(buffer, "% o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001"));

  test::sprintf(buffer, "% x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd"));

  test::sprintf(buffer, "% x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433"));

  test::sprintf(buffer, "% X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD"));

  test::sprintf(buffer, "% X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433"));

  test::sprintf(buffer, "% c", 'x');
  REQUIRE(!strcmp(buffer, "x"));
}

TEST_CASE("+ flag", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%+d", 42);
  REQUIRE(!strcmp(buffer, "+42"));

  test::sprintf(buffer, "%+d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "%+5d", 42);
  REQUIRE(!strcmp(buffer, "  +42"));

  test::sprintf(buffer, "%+5d", -42);
  REQUIRE(!strcmp(buffer, "  -42"));

  test::sprintf(buffer, "%+15d", 42);
  REQUIRE(!strcmp(buffer, "            +42"));

  test::sprintf(buffer, "%+15d", -42);
  REQUIRE(!strcmp(buffer, "            -42"));

  test::sprintf(buffer, "%+s", "Hello testing");
  REQUIRE(!strcmp(buffer, "Hello testing"));

  test::sprintf(buffer, "%+d", 1024);
  REQUIRE(!strcmp(buffer, "+1024"));

  test::sprintf(buffer, "%+d", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%+i", 1024);
  REQUIRE(!strcmp(buffer, "+1024"));

  test::sprintf(buffer, "%+i", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%+u", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%+u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272"));

  test::sprintf(buffer, "%+o", 511);
  REQUIRE(!strcmp(buffer, "777"));

  test::sprintf(buffer, "%+o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001"));

  test::sprintf(buffer, "%+x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd"));

  test::sprintf(buffer, "%+x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433"));

  test::sprintf(buffer, "%+X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD"));

  test::sprintf(buffer, "%+X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433"));

  test::sprintf(buffer, "%+c", 'x');
  REQUIRE(!strcmp(buffer, "x"));

  test::sprintf(buffer, "%+.0d", 0);
  REQUIRE(!strcmp(buffer, "+"));
}


TEST_CASE("0 flag", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%0d", 42);
  REQUIRE(!strcmp(buffer, "42"));

  test::sprintf(buffer, "%0ld", 42L);
  REQUIRE(!strcmp(buffer, "42"));

  test::sprintf(buffer, "%0d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "%05d", 42);
  REQUIRE(!strcmp(buffer, "00042"));

  test::sprintf(buffer, "%05d", -42);
  REQUIRE(!strcmp(buffer, "-0042"));

  test::sprintf(buffer, "%015d", 42);
  REQUIRE(!strcmp(buffer, "000000000000042"));

  test::sprintf(buffer, "%015d", -42);
  REQUIRE(!strcmp(buffer, "-00000000000042"));

  test::sprintf(buffer, "%015.2f", 42.1234);
  REQUIRE(!strcmp(buffer, "000000000042.12"));

  test::sprintf(buffer, "%015.3f", 42.9876);
  REQUIRE(!strcmp(buffer, "00000000042.988"));

  test::sprintf(buffer, "%015.5f", -42.9876);
  REQUIRE(!strcmp(buffer, "-00000042.98760"));
}


TEST_CASE("- flag, part 1", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%-d", 42);
  REQUIRE(!strcmp(buffer, "42"));

  test::sprintf(buffer, "%-d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "%-5d", 42);
  REQUIRE(!strcmp(buffer, "42   "));

  test::sprintf(buffer, "%-5d", -42);
  REQUIRE(!strcmp(buffer, "-42  "));

  test::sprintf(buffer, "%-15d", 42);
  REQUIRE(!strcmp(buffer, "42             "));

  test::sprintf(buffer, "%-15d", -42);
  REQUIRE(!strcmp(buffer, "-42            "));

  test::sprintf(buffer, "%-0d", 42);
  REQUIRE(!strcmp(buffer, "42"));

  test::sprintf(buffer, "%-0d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "%-05d", 42);
  REQUIRE(!strcmp(buffer, "42   "));

  test::sprintf(buffer, "%-05d", -42);
  REQUIRE(!strcmp(buffer, "-42  "));

  test::sprintf(buffer, "%-015d", 42);
  REQUIRE(!strcmp(buffer, "42             "));

  test::sprintf(buffer, "%-015d", -42);
  REQUIRE(!strcmp(buffer, "-42            "));

  test::sprintf(buffer, "%0-d", 42);
  REQUIRE(!strcmp(buffer, "42"));

  test::sprintf(buffer, "%0-d", -42);
  REQUIRE(!strcmp(buffer, "-42"));

  test::sprintf(buffer, "%0-5d", 42);
  REQUIRE(!strcmp(buffer, "42   "));

  test::sprintf(buffer, "%0-5d", -42);
  REQUIRE(!strcmp(buffer, "-42  "));

  test::sprintf(buffer, "%0-15d", 42);
  REQUIRE(!strcmp(buffer, "42             "));

  test::sprintf(buffer, "%0-15d", -42);
  REQUIRE(!strcmp(buffer, "-42            "));

  test::sprintf(buffer, "%0-15.3e", -42.);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  REQUIRE(!strcmp(buffer, "-4.200e+01     "));
#else
  REQUIRE(!strcmp(buffer, "e"));
#endif
}


TEST_CASE("- flag, part 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;
  const char * s = "";

  fail = false;
  {
    test::sprintf(buffer, "%0-15.3g", -42.);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
    s = "-42.0          ";
#else
    s = "g";
#endif
    CHECK( std::string( buffer ) == s );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << s << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;

    test::sprintf(buffer, "%0-15.4g", -42.);
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
    s = "-42.00         ";
#else
    s = "g";
#endif
    CHECK( std::string( buffer ) == s );
    fail1 = false;
    //std::cout << "line " << __LINE__ << "... should-be:'" << s << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
    fail = fail || fail1;
  }
  REQUIRE(!fail);

}


TEST_CASE("# flag", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%#.0x", 0);
  REQUIRE(!strcmp(buffer, ""));
  test::sprintf(buffer, "%#.1x", 0);
  REQUIRE(!strcmp(buffer, "0"));
  test::sprintf(buffer, "%#.0llx", 0LL);
  REQUIRE(!strcmp(buffer, ""));
  test::sprintf(buffer, "%#.8x", 0x614e);
  REQUIRE(!strcmp(buffer, "0x0000614e"));
  test::sprintf(buffer,"%#b", 6);
  REQUIRE(!strcmp(buffer, "0b110"));
}


TEST_CASE("specifier", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "Hello testing");
  REQUIRE(!strcmp(buffer, "Hello testing"));

  test::sprintf(buffer, "%s", "Hello testing");
  REQUIRE(!strcmp(buffer, "Hello testing"));

  test::sprintf(buffer, "%d", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%d", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%i", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%i", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%u", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272"));

  test::sprintf(buffer, "%o", 511);
  REQUIRE(!strcmp(buffer, "777"));

  test::sprintf(buffer, "%o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001"));

  test::sprintf(buffer, "%x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd"));

  test::sprintf(buffer, "%x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433"));

  test::sprintf(buffer, "%X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD"));

  test::sprintf(buffer, "%X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433"));

  test::sprintf(buffer, "%%");
  REQUIRE(!strcmp(buffer, "%"));
}


TEST_CASE("width", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%1s", "Hello testing");
  REQUIRE(!strcmp(buffer, "Hello testing"));

  test::sprintf(buffer, "%1d", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%1d", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%1i", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%1i", -1024);
  REQUIRE(!strcmp(buffer, "-1024"));

  test::sprintf(buffer, "%1u", 1024);
  REQUIRE(!strcmp(buffer, "1024"));

  test::sprintf(buffer, "%1u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272"));

  test::sprintf(buffer, "%1o", 511);
  REQUIRE(!strcmp(buffer, "777"));

  test::sprintf(buffer, "%1o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001"));

  test::sprintf(buffer, "%1x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd"));

  test::sprintf(buffer, "%1x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433"));

  test::sprintf(buffer, "%1X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD"));

  test::sprintf(buffer, "%1X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433"));

  test::sprintf(buffer, "%1c", 'x');
  REQUIRE(!strcmp(buffer, "x"));
}


TEST_CASE("width 20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%20s", "Hello");
  REQUIRE(!strcmp(buffer, "               Hello"));

  test::sprintf(buffer, "%20d", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20d", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%20i", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20i", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%20u", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20u", 4294966272U);
  REQUIRE(!strcmp(buffer, "          4294966272"));

  test::sprintf(buffer, "%20o", 511);
  REQUIRE(!strcmp(buffer, "                 777"));

  test::sprintf(buffer, "%20o", 4294966785U);
  REQUIRE(!strcmp(buffer, "         37777777001"));

  test::sprintf(buffer, "%20x", 305441741);
  REQUIRE(!strcmp(buffer, "            1234abcd"));

  test::sprintf(buffer, "%20x", 3989525555U);
  REQUIRE(!strcmp(buffer, "            edcb5433"));

  test::sprintf(buffer, "%20X", 305441741);
  REQUIRE(!strcmp(buffer, "            1234ABCD"));

  test::sprintf(buffer, "%20X", 3989525555U);
  REQUIRE(!strcmp(buffer, "            EDCB5433"));

  test::sprintf(buffer, "%20c", 'x');
  REQUIRE(!strcmp(buffer, "                   x"));
}


TEST_CASE("width *20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%*s", 20, "Hello");
  REQUIRE(!strcmp(buffer, "               Hello"));

  test::sprintf(buffer, "%*d", 20, 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%*d", 20, -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%*i", 20, 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%*i", 20, -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%*u", 20, 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%*u", 20, 4294966272U);
  REQUIRE(!strcmp(buffer, "          4294966272"));

  test::sprintf(buffer, "%*o", 20, 511);
  REQUIRE(!strcmp(buffer, "                 777"));

  test::sprintf(buffer, "%*o", 20, 4294966785U);
  REQUIRE(!strcmp(buffer, "         37777777001"));

  test::sprintf(buffer, "%*x", 20, 305441741);
  REQUIRE(!strcmp(buffer, "            1234abcd"));

  test::sprintf(buffer, "%*x", 20, 3989525555U);
  REQUIRE(!strcmp(buffer, "            edcb5433"));

  test::sprintf(buffer, "%*X", 20, 305441741);
  REQUIRE(!strcmp(buffer, "            1234ABCD"));

  test::sprintf(buffer, "%*X", 20, 3989525555U);
  REQUIRE(!strcmp(buffer, "            EDCB5433"));

  test::sprintf(buffer, "%*c", 20,'x');
  REQUIRE(!strcmp(buffer, "                   x"));
}


TEST_CASE("width -20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%-20s", "Hello");
  REQUIRE(!strcmp(buffer, "Hello               "));

  test::sprintf(buffer, "%-20d", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%-20d", -1024);
  REQUIRE(!strcmp(buffer, "-1024               "));

  test::sprintf(buffer, "%-20i", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%-20i", -1024);
  REQUIRE(!strcmp(buffer, "-1024               "));

  test::sprintf(buffer, "%-20u", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%-20.4f", 1024.1234);
  REQUIRE(!strcmp(buffer, "1024.1234           "));

  test::sprintf(buffer, "%-20u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272          "));

  test::sprintf(buffer, "%-20o", 511);
  REQUIRE(!strcmp(buffer, "777                 "));

  test::sprintf(buffer, "%-20o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001         "));

  test::sprintf(buffer, "%-20x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd            "));

  test::sprintf(buffer, "%-20x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433            "));

  test::sprintf(buffer, "%-20X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD            "));

  test::sprintf(buffer, "%-20X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433            "));

  test::sprintf(buffer, "%-20c", 'x');
  REQUIRE(!strcmp(buffer, "x                   "));

  test::sprintf(buffer, "|%5d| |%-2d| |%5d|", 9, 9, 9);
  REQUIRE(!strcmp(buffer, "|    9| |9 | |    9|"));

  test::sprintf(buffer, "|%5d| |%-2d| |%5d|", 10, 10, 10);
  REQUIRE(!strcmp(buffer, "|   10| |10| |   10|"));

  test::sprintf(buffer, "|%5d| |%-12d| |%5d|", 9, 9, 9);
  REQUIRE(!strcmp(buffer, "|    9| |9           | |    9|"));

  test::sprintf(buffer, "|%5d| |%-12d| |%5d|", 10, 10, 10);
  REQUIRE(!strcmp(buffer, "|   10| |10          | |   10|"));
}


TEST_CASE("width 0-20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%0-20s", "Hello");
  REQUIRE(!strcmp(buffer, "Hello               "));

  test::sprintf(buffer, "%0-20d", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%0-20d", -1024);
  REQUIRE(!strcmp(buffer, "-1024               "));

  test::sprintf(buffer, "%0-20i", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%0-20i", -1024);
  REQUIRE(!strcmp(buffer, "-1024               "));

  test::sprintf(buffer, "%0-20u", 1024);
  REQUIRE(!strcmp(buffer, "1024                "));

  test::sprintf(buffer, "%0-20u", 4294966272U);
  REQUIRE(!strcmp(buffer, "4294966272          "));

  test::sprintf(buffer, "%0-20o", 511);
  REQUIRE(!strcmp(buffer, "777                 "));

  test::sprintf(buffer, "%0-20o", 4294966785U);
  REQUIRE(!strcmp(buffer, "37777777001         "));

  test::sprintf(buffer, "%0-20x", 305441741);
  REQUIRE(!strcmp(buffer, "1234abcd            "));

  test::sprintf(buffer, "%0-20x", 3989525555U);
  REQUIRE(!strcmp(buffer, "edcb5433            "));

  test::sprintf(buffer, "%0-20X", 305441741);
  REQUIRE(!strcmp(buffer, "1234ABCD            "));

  test::sprintf(buffer, "%0-20X", 3989525555U);
  REQUIRE(!strcmp(buffer, "EDCB5433            "));

  test::sprintf(buffer, "%0-20c", 'x');
  REQUIRE(!strcmp(buffer, "x                   "));
}


TEST_CASE("padding 20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%020d", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%020d", -1024);
  REQUIRE(!strcmp(buffer, "-0000000000000001024"));

  test::sprintf(buffer, "%020i", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%020i", -1024);
  REQUIRE(!strcmp(buffer, "-0000000000000001024"));

  test::sprintf(buffer, "%020u", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%020u", 4294966272U);
  REQUIRE(!strcmp(buffer, "00000000004294966272"));

  test::sprintf(buffer, "%020o", 511);
  REQUIRE(!strcmp(buffer, "00000000000000000777"));

  test::sprintf(buffer, "%020o", 4294966785U);
  REQUIRE(!strcmp(buffer, "00000000037777777001"));

  test::sprintf(buffer, "%020x", 305441741);
  REQUIRE(!strcmp(buffer, "0000000000001234abcd"));

  test::sprintf(buffer, "%020x", 3989525555U);
  REQUIRE(!strcmp(buffer, "000000000000edcb5433"));

  test::sprintf(buffer, "%020X", 305441741);
  REQUIRE(!strcmp(buffer, "0000000000001234ABCD"));

  test::sprintf(buffer, "%020X", 3989525555U);
  REQUIRE(!strcmp(buffer, "000000000000EDCB5433"));
}


TEST_CASE("padding .20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%.20d", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%.20d", -1024);
  REQUIRE(!strcmp(buffer, "-00000000000000001024"));

  test::sprintf(buffer, "%.20i", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%.20i", -1024);
  REQUIRE(!strcmp(buffer, "-00000000000000001024"));

  test::sprintf(buffer, "%.20u", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%.20u", 4294966272U);
  REQUIRE(!strcmp(buffer, "00000000004294966272"));

  test::sprintf(buffer, "%.20o", 511);
  REQUIRE(!strcmp(buffer, "00000000000000000777"));

  test::sprintf(buffer, "%.20o", 4294966785U);
  REQUIRE(!strcmp(buffer, "00000000037777777001"));

  test::sprintf(buffer, "%.20x", 305441741);
  REQUIRE(!strcmp(buffer, "0000000000001234abcd"));

  test::sprintf(buffer, "%.20x", 3989525555U);
  REQUIRE(!strcmp(buffer, "000000000000edcb5433"));

  test::sprintf(buffer, "%.20X", 305441741);
  REQUIRE(!strcmp(buffer, "0000000000001234ABCD"));

  test::sprintf(buffer, "%.20X", 3989525555U);
  REQUIRE(!strcmp(buffer, "000000000000EDCB5433"));
}


TEST_CASE("padding #020", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%#020d", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%#020d", -1024);
  REQUIRE(!strcmp(buffer, "-0000000000000001024"));

  test::sprintf(buffer, "%#020i", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%#020i", -1024);
  REQUIRE(!strcmp(buffer, "-0000000000000001024"));

  test::sprintf(buffer, "%#020u", 1024);
  REQUIRE(!strcmp(buffer, "00000000000000001024"));

  test::sprintf(buffer, "%#020u", 4294966272U);
  REQUIRE(!strcmp(buffer, "00000000004294966272"));

  test::sprintf(buffer, "%#020o", 511);
  REQUIRE(!strcmp(buffer, "00000000000000000777"));

  test::sprintf(buffer, "%#020o", 4294966785U);
  REQUIRE(!strcmp(buffer, "00000000037777777001"));

  test::sprintf(buffer, "%#020x", 305441741);
  REQUIRE(!strcmp(buffer, "0x00000000001234abcd"));

  test::sprintf(buffer, "%#020x", 3989525555U);
  REQUIRE(!strcmp(buffer, "0x0000000000edcb5433"));

  test::sprintf(buffer, "%#020X", 305441741);
  REQUIRE(!strcmp(buffer, "0X00000000001234ABCD"));

  test::sprintf(buffer, "%#020X", 3989525555U);
  REQUIRE(!strcmp(buffer, "0X0000000000EDCB5433"));
}


TEST_CASE("padding #20", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%#20d", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%#20d", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%#20i", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%#20i", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%#20u", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%#20u", 4294966272U);
  REQUIRE(!strcmp(buffer, "          4294966272"));

  test::sprintf(buffer, "%#20o", 511);
  REQUIRE(!strcmp(buffer, "                0777"));

  test::sprintf(buffer, "%#20o", 4294966785U);
  REQUIRE(!strcmp(buffer, "        037777777001"));

  test::sprintf(buffer, "%#20x", 305441741);
  REQUIRE(!strcmp(buffer, "          0x1234abcd"));

  test::sprintf(buffer, "%#20x", 3989525555U);
  REQUIRE(!strcmp(buffer, "          0xedcb5433"));

  test::sprintf(buffer, "%#20X", 305441741);
  REQUIRE(!strcmp(buffer, "          0X1234ABCD"));

  test::sprintf(buffer, "%#20X", 3989525555U);
  REQUIRE(!strcmp(buffer, "          0XEDCB5433"));
}


TEST_CASE("padding 20.5", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%20.5d", 1024);
  REQUIRE(!strcmp(buffer, "               01024"));

  test::sprintf(buffer, "%20.5d", -1024);
  REQUIRE(!strcmp(buffer, "              -01024"));

  test::sprintf(buffer, "%20.5i", 1024);
  REQUIRE(!strcmp(buffer, "               01024"));

  test::sprintf(buffer, "%20.5i", -1024);
  REQUIRE(!strcmp(buffer, "              -01024"));

  test::sprintf(buffer, "%20.5u", 1024);
  REQUIRE(!strcmp(buffer, "               01024"));

  test::sprintf(buffer, "%20.5u", 4294966272U);
  REQUIRE(!strcmp(buffer, "          4294966272"));

  test::sprintf(buffer, "%20.5o", 511);
  REQUIRE(!strcmp(buffer, "               00777"));

  test::sprintf(buffer, "%20.5o", 4294966785U);
  REQUIRE(!strcmp(buffer, "         37777777001"));

  test::sprintf(buffer, "%20.5x", 305441741);
  REQUIRE(!strcmp(buffer, "            1234abcd"));

  test::sprintf(buffer, "%20.10x", 3989525555U);
  REQUIRE(!strcmp(buffer, "          00edcb5433"));

  test::sprintf(buffer, "%20.5X", 305441741);
  REQUIRE(!strcmp(buffer, "            1234ABCD"));

  test::sprintf(buffer, "%20.10X", 3989525555U);
  REQUIRE(!strcmp(buffer, "          00EDCB5433"));
}


TEST_CASE("padding neg numbers", "[]" ) {
  char buffer[100];

  // space padding
  test::sprintf(buffer, "% 1d", -5);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "% 2d", -5);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "% 3d", -5);
  REQUIRE(!strcmp(buffer, " -5"));

  test::sprintf(buffer, "% 4d", -5);
  REQUIRE(!strcmp(buffer, "  -5"));

  // zero padding
  test::sprintf(buffer, "%01d", -5);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "%02d", -5);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "%03d", -5);
  REQUIRE(!strcmp(buffer, "-05"));

  test::sprintf(buffer, "%04d", -5);
  REQUIRE(!strcmp(buffer, "-005"));
}


TEST_CASE("float padding neg numbers, part 1", "[]" ) {
  char buffer[100];

  // space padding
  test::sprintf(buffer, "% 3.1f", -5.);
  REQUIRE(!strcmp(buffer, "-5.0"));

  test::sprintf(buffer, "% 4.1f", -5.);
  REQUIRE(!strcmp(buffer, "-5.0"));

  test::sprintf(buffer, "% 5.1f", -5.);
  REQUIRE(!strcmp(buffer, " -5.0"));

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL

  test::sprintf(buffer, "% 6.1e", -5.);
  REQUIRE(!strcmp(buffer, "-5.0e+00"));

  test::sprintf(buffer, "% 10.1e", -5.);
  REQUIRE(!strcmp(buffer, "  -5.0e+00"));
#endif

  // zero padding
  test::sprintf(buffer, "%03.1f", -5.);
  REQUIRE(!strcmp(buffer, "-5.0"));

  test::sprintf(buffer, "%04.1f", -5.);
  REQUIRE(!strcmp(buffer, "-5.0"));

  test::sprintf(buffer, "%05.1f", -5.);
  REQUIRE(!strcmp(buffer, "-05.0"));

  // zero padding no decimal point
  test::sprintf(buffer, "%01.0f", -5.);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "%02.0f", -5.);
  REQUIRE(!strcmp(buffer, "-5"));

  test::sprintf(buffer, "%03.0f", -5.);
  REQUIRE(!strcmp(buffer, "-05"));

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  test::sprintf(buffer, "%010.1e", -5.);
  REQUIRE(!strcmp(buffer, "-005.0e+00"));

  test::sprintf(buffer, "%07.0E", -5.);
  REQUIRE(!strcmp(buffer, "-05E+00"));
#endif
}


TEST_CASE("float padding neg numbers, part 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  fail = false;
  {
    CaseSpec specs[] = {
      { "% 6.1g", -5., "    -5" },
      { "%03.0g", -5., "-05" },
    };

    for( CaseSpec spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.stimulus);
      CHECK( std::string( buffer ) == spec.shouldBe );
      fail1 = false;
      //std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
#endif
}


TEST_CASE("length", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%.0s", "Hello testing");
  REQUIRE(!strcmp(buffer, ""));

  test::sprintf(buffer, "%20.0s", "Hello testing");
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%.s", "Hello testing");
  REQUIRE(!strcmp(buffer, ""));

  test::sprintf(buffer, "%20.s", "Hello testing");
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.0d", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20.0d", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%20.d", 0);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.0i", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20.i", -1024);
  REQUIRE(!strcmp(buffer, "               -1024"));

  test::sprintf(buffer, "%20.i", 0);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.u", 1024);
  REQUIRE(!strcmp(buffer, "                1024"));

  test::sprintf(buffer, "%20.0u", 4294966272U);
  REQUIRE(!strcmp(buffer, "          4294966272"));

  test::sprintf(buffer, "%20.u", 0U);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.o", 511);
  REQUIRE(!strcmp(buffer, "                 777"));

  test::sprintf(buffer, "%20.0o", 4294966785U);
  REQUIRE(!strcmp(buffer, "         37777777001"));

  test::sprintf(buffer, "%20.o", 0U);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.x", 305441741);
  REQUIRE(!strcmp(buffer, "            1234abcd"));

  test::sprintf(buffer, "%50.x", 305441741);
  REQUIRE(!strcmp(buffer, "                                          1234abcd"));

  test::sprintf(buffer, "%50.x%10.u", 305441741, 12345);
  REQUIRE(!strcmp(buffer, "                                          1234abcd     12345"));

  test::sprintf(buffer, "%20.0x", 3989525555U);
  REQUIRE(!strcmp(buffer, "            edcb5433"));

  test::sprintf(buffer, "%20.x", 0U);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%20.X", 305441741);
  REQUIRE(!strcmp(buffer, "            1234ABCD"));

  test::sprintf(buffer, "%20.0X", 3989525555U);
  REQUIRE(!strcmp(buffer, "            EDCB5433"));

  test::sprintf(buffer, "%20.X", 0U);
  REQUIRE(!strcmp(buffer, "                    "));

  test::sprintf(buffer, "%02.0u", 0U);
  REQUIRE(!strcmp(buffer, "  "));

  test::sprintf(buffer, "%02.0d", 0);
  REQUIRE(!strcmp(buffer, "  "));
}


TEST_CASE("types", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%i", 0);
  REQUIRE(!strcmp(buffer, "0"));

  test::sprintf(buffer, "%i", 1234);
  REQUIRE(!strcmp(buffer, "1234"));

  test::sprintf(buffer, "%i", 32767);
  REQUIRE(!strcmp(buffer, "32767"));

  test::sprintf(buffer, "%i", -32767);
  REQUIRE(!strcmp(buffer, "-32767"));

  test::sprintf(buffer, "%li", 30L);
  REQUIRE(!strcmp(buffer, "30"));

  test::sprintf(buffer, "%li", -2147483647L);
  REQUIRE(!strcmp(buffer, "-2147483647"));

  test::sprintf(buffer, "%li", 2147483647L);
  REQUIRE(!strcmp(buffer, "2147483647"));

  test::sprintf(buffer, "%lli", 30LL);
  REQUIRE(!strcmp(buffer, "30"));

  test::sprintf(buffer, "%lli", -9223372036854775807LL);
  REQUIRE(!strcmp(buffer, "-9223372036854775807"));

  test::sprintf(buffer, "%lli", 9223372036854775807LL);
  REQUIRE(!strcmp(buffer, "9223372036854775807"));

  test::sprintf(buffer, "%lu", 100000L);
  REQUIRE(!strcmp(buffer, "100000"));

  test::sprintf(buffer, "%lu", 0xFFFFFFFFL);
  REQUIRE(!strcmp(buffer, "4294967295"));

  test::sprintf(buffer, "%llu", 281474976710656LLU);
  REQUIRE(!strcmp(buffer, "281474976710656"));

  test::sprintf(buffer, "%llu", 18446744073709551615LLU);
  REQUIRE(!strcmp(buffer, "18446744073709551615"));

  test::sprintf(buffer, "%zu", 2147483647UL);
  REQUIRE(!strcmp(buffer, "2147483647"));

  test::sprintf(buffer, "%zd", 2147483647UL);
  REQUIRE(!strcmp(buffer, "2147483647"));

  if (sizeof(size_t) == sizeof(long)) {
    test::sprintf(buffer, "%zi", -2147483647L);
    REQUIRE(!strcmp(buffer, "-2147483647"));
  }
  else {
    test::sprintf(buffer, "%zi", -2147483647LL);
    REQUIRE(!strcmp(buffer, "-2147483647"));
  }

  test::sprintf(buffer, "%b", 60000);
  REQUIRE(!strcmp(buffer, "1110101001100000"));

  test::sprintf(buffer, "%lb", 12345678L);
  REQUIRE(!strcmp(buffer, "101111000110000101001110"));

  test::sprintf(buffer, "%o", 60000);
  REQUIRE(!strcmp(buffer, "165140"));

  test::sprintf(buffer, "%lo", 12345678L);
  REQUIRE(!strcmp(buffer, "57060516"));

  test::sprintf(buffer, "%lx", 0x12345678L);
  REQUIRE(!strcmp(buffer, "12345678"));

  test::sprintf(buffer, "%llx", 0x1234567891234567LLU);
  REQUIRE(!strcmp(buffer, "1234567891234567"));

  test::sprintf(buffer, "%lx", 0xabcdefabL);
  REQUIRE(!strcmp(buffer, "abcdefab"));

  test::sprintf(buffer, "%lX", 0xabcdefabL);
  REQUIRE(!strcmp(buffer, "ABCDEFAB"));

  test::sprintf(buffer, "%c", 'v');
  REQUIRE(!strcmp(buffer, "v"));

  test::sprintf(buffer, "%cv", 'w');
  REQUIRE(!strcmp(buffer, "wv"));

  test::sprintf(buffer, "%s", "A Test");
  REQUIRE(!strcmp(buffer, "A Test"));

  test::sprintf(buffer, "%hhu", 0xFFFFUL);
  REQUIRE(!strcmp(buffer, "255"));

  test::sprintf(buffer, "%hu", 0x123456UL);
  REQUIRE(!strcmp(buffer, "13398"));

  test::sprintf(buffer, "%s%hhi %hu", "Test", 10000, 0xFFFFFFFF);
  REQUIRE(!strcmp(buffer, "Test16 65535"));

  test::sprintf(buffer, "%tx", &buffer[10] - &buffer[0]);
  REQUIRE(!strcmp(buffer, "a"));

// TBD
  if (sizeof(intmax_t) == sizeof(long)) {
    test::sprintf(buffer, "%ji", -2147483647L);
    REQUIRE(!strcmp(buffer, "-2147483647"));
  }
  else {
    test::sprintf(buffer, "%ji", -2147483647LL);
    REQUIRE(!strcmp(buffer, "-2147483647"));
  }
}


TEST_CASE("pointer", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%p", (void*)0x1234U);
  if (sizeof(void*) == 4U) {
    REQUIRE(!strcmp(buffer, "00001234"));
  }
  else {
    REQUIRE(!strcmp(buffer, "0000000000001234"));
  }

  test::sprintf(buffer, "%p", (void*)0x12345678U);
  if (sizeof(void*) == 4U) {
    REQUIRE(!strcmp(buffer, "12345678"));
  }
  else {
    REQUIRE(!strcmp(buffer, "0000000012345678"));
  }

  test::sprintf(buffer, "%p-%p", (void*)0x12345678U, (void*)0x7EDCBA98U);
  if (sizeof(void*) == 4U) {
    REQUIRE(!strcmp(buffer, "12345678-7EDCBA98"));
  }
  else {
    REQUIRE(!strcmp(buffer, "0000000012345678-000000007EDCBA98"));
  }

  if (sizeof(uintptr_t) == sizeof(uint64_t)) {
    test::sprintf(buffer, "%p", (void*)(uintptr_t)0xFFFFFFFFU);
    REQUIRE(!strcmp(buffer, "00000000FFFFFFFF"));
  }
  else {
    test::sprintf(buffer, "%p", (void*)(uintptr_t)0xFFFFFFFFU);
    REQUIRE(!strcmp(buffer, "FFFFFFFF"));
  }
}


TEST_CASE("unknown flag", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%kmarco", 42, 37);
  REQUIRE(!strcmp(buffer, "kmarco"));
}


TEST_CASE("string length", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%.4s", "This is a test");
  REQUIRE(!strcmp(buffer, "This"));

  test::sprintf(buffer, "%.4s", "test");
  REQUIRE(!strcmp(buffer, "test"));

  test::sprintf(buffer, "%.7s", "123");
  REQUIRE(!strcmp(buffer, "123"));

  test::sprintf(buffer, "%.7s", "");
  REQUIRE(!strcmp(buffer, ""));

  test::sprintf(buffer, "%.4s%.2s", "123456", "abcdef");
  REQUIRE(!strcmp(buffer, "1234ab"));

  test::sprintf(buffer, "%.4.2s", "123456");
  REQUIRE(!strcmp(buffer, ".2s"));

  test::sprintf(buffer, "%.*s", 3, "123456");
  REQUIRE(!strcmp(buffer, "123"));
}


TEST_CASE("buffer length", "[]" ) {
  char buffer[100];
  int ret;

  ret = test::snprintf(nullptr, 10, "%s", "Test");
  REQUIRE(ret == 4);
  ret = test::snprintf(nullptr, 0, "%s", "Test");
  REQUIRE(ret == 4);

  buffer[0] = (char)0xA5;
  ret = test::snprintf(buffer, 0, "%s", "Test");
  REQUIRE(buffer[0] == (char)0xA5);
  REQUIRE(ret == 4);

  buffer[0] = (char)0xCC;
  test::snprintf(buffer, 1, "%s", "Test");
  REQUIRE(buffer[0] == '\0');

  test::snprintf(buffer, 2, "%s", "Hello");
  REQUIRE(!strcmp(buffer, "H"));
}


TEST_CASE("ret value", "[]" ) {
  char buffer[100] ;
  int ret;

  ret = test::snprintf(buffer, 6, "0%s", "1234");
  REQUIRE(!strcmp(buffer, "01234"));
  REQUIRE(ret == 5);

  ret = test::snprintf(buffer, 6, "0%s", "12345");
  REQUIRE(!strcmp(buffer, "01234"));
  REQUIRE(ret == 6);  // '5' is truncated

  ret = test::snprintf(buffer, 6, "0%s", "1234567");
  REQUIRE(!strcmp(buffer, "01234"));
  REQUIRE(ret == 8);  // '567' are truncated

  ret = test::snprintf(buffer, 10, "hello, world");
  REQUIRE(ret == 12);

  ret = test::snprintf(buffer, 3, "%d", 10000);
  REQUIRE(ret == 5);
  REQUIRE(strlen(buffer) == 2U);
  REQUIRE(buffer[0] == '1');
  REQUIRE(buffer[1] == '0');
  REQUIRE(buffer[2] == '\0');
}


TEST_CASE("misc, part 1", "[]" ) {
  char buffer[100];

  test::sprintf(buffer, "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit");
  REQUIRE(!strcmp(buffer, "53000atest-20 bit"));

  test::sprintf(buffer, "%.*d", -1, 1);
  REQUIRE(!strcmp(buffer, "1"));

  test::sprintf(buffer, "%.3s", "foobar");
  REQUIRE(!strcmp(buffer, "foo"));

  test::sprintf(buffer, "% .0d", 0);
  REQUIRE(!strcmp(buffer, " "));

  test::sprintf(buffer, "%10.5d", 4);
  REQUIRE(!strcmp(buffer, "     00004"));

  test::sprintf(buffer, "%*sx", -3, "hi");
  REQUIRE(!strcmp(buffer, "hi x"));
}


using CaseSpec2 = struct { const char *fmt; int parm; double stimulus; const char *shouldBe; };

TEST_CASE("misc, part 2", "[]" ) {
  char buffer[100];
  bool fail = false;
  bool fail1 = false;

#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
  fail = false;
  {
    CaseSpec2 specs[] = {
      { "%.*f", 2, 0.33333333, "0.33" },
      { "%.*g", 2, 0.33333333, "0.33" },
      { "%.*e", 2, 0.33333333, "3.33e-01" },
    };

    for( CaseSpec2 spec : specs ) {
      test::sprintf(buffer, spec.fmt, spec.parm, spec.stimulus);
      // Perhaps use this model, setting fail1 first, on all CHECKs.  Otherwise, fail1 doesn't ever tell us whether the check failed.
      fail1 = !( std::string( buffer ) == spec.shouldBe );
      CHECK( !fail1 );
      if( fail1 ) {
        std::cout << "line " << __LINE__ << "... should-be:'" << spec.shouldBe << "'" << " code-said:'" << buffer << "' " << (fail1? "MISMATCH" : "SAME" ) << std::endl;
      }
      fail = fail || fail1;
    }
  }
  REQUIRE(!fail);
#endif
}
