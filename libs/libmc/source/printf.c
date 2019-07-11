///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
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
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources. These routines are thread
//        safe and reentrant!
//        Use this instead of the bloated standard/newlib printf cause these use
//        malloc for printf (and may not be thread safe).
//
///////////////////////////////////////////////////////////////////////////////


#include <mc/stdarg.h>
#include <mc/stdio.h>


// define this globally (e.g. gcc -DPRINTF_INCLUDE_CONFIG_H ...) to include the
// printf_config.h header file
// default: undefined
#ifdef PRINTF_INCLUDE_CONFIG_H
#include "printf_config.h"
#endif


// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
// default: 32 byte
#ifndef PRINTF_NTOA_BUFFER_SIZE
#define PRINTF_NTOA_BUFFER_SIZE    32U
#endif

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack)
// default: 32 byte
#ifndef PRINTF_FTOA_BUFFER_SIZE
#define PRINTF_FTOA_BUFFER_SIZE    32U
#endif

// support for the floating point type (%f)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_SUPPORT_FLOAT
#endif

// support for exponential floating point notation (%e/%g)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_SUPPORT_EXPONENTIAL
#endif

// define the default floating point precision
// default: 6 digits
#ifndef PRINTF_DEFAULT_FLOAT_PRECISION
#define PRINTF_DEFAULT_FLOAT_PRECISION  6U
#endif

// define the largest float suitable to print with %f
// default: 1e9
#ifndef PRINTF_MAX_FLOAT
#define PRINTF_MAX_FLOAT  1e9
#endif

// support for the long long types (%llu or %p)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_LONG_LONG
#endif

// support for the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define PRINTF_SUPPORT_PTRDIFF_T
#endif

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)


// import float.h for DBL_MAX
#if defined(PRINTF_SUPPORT_FLOAT)
#include <mc/float.h>
#endif


struct printf_wrapper;

// output function type
typedef void (*out_fct_type)(struct printf_wrapper *wrapper, CHAR_TYPE character);


// wrapper (used as buffer) for output function type
struct printf_wrapper
{
  out_fct_type out;
  cprintf_callback_t callback;
  void* arg;
  CHAR_TYPE *buffer;
  size_t idx;
  size_t maxlen;
};


// internal buffer output
static inline void _out_buffer(struct printf_wrapper *wrapper, CHAR_TYPE character)
{
  if (wrapper->idx < wrapper->maxlen)
    wrapper->buffer[wrapper->idx] = character;
  else
  if (character == 0)
    wrapper->buffer[wrapper->maxlen-1] = 0;
  wrapper->idx++;
}


// internal null output
static inline void _out_null(struct printf_wrapper *wrapper, CHAR_TYPE character)
{
  (void)character;
  wrapper->idx++;
}


// internal _putchar wrapper
static inline void _out_char(struct printf_wrapper *wrapper, CHAR_TYPE character)
{
  (void) wrapper;
  if (character) _putchar(character);
  wrapper->idx++;
}


// internal output function wrapper
static inline void _out_fct(struct printf_wrapper *wrapper, CHAR_TYPE character)
{
  if (character) wrapper->callback(character, wrapper->arg);
  wrapper->idx++;
}


// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
static inline unsigned int _strnlen_s(const CHAR_TYPE* str, size_t maxsize)
{
  const CHAR_TYPE* s;
  for (s = str; *s && maxsize--; ++s);
  return (unsigned int)(s - str);
}


// internal test if CHAR_TYPE is a digit (0-9)
// \return true if CHAR_TYPE is a digit
static inline bool _is_digit(CHAR_TYPE ch)
{
  return (ch >= '0') && (ch <= '9');
}


// internal ASCII string to unsigned int conversion
static unsigned int _atoi(const CHAR_TYPE** str)
{
  unsigned int i = 0U;
  while (_is_digit(**str)) {
    i = i * 10U + (unsigned int)(*((*str)++) - '0');
  }
  return i;
}


// output the specified string in reverse, taking care of any zero-padding
static void _out_rev(struct printf_wrapper *wrapper, const CHAR_TYPE* buf, size_t len, unsigned int width, unsigned int flags)
{
  const size_t start_idx = wrapper->idx;

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++) {
      wrapper->out(wrapper, ' ');
    }
  }

  // reverse string
  while (len) {
    wrapper->out(wrapper, buf[--len]);
  }

  // append pad spaces up to given width
  if (flags & FLAGS_LEFT) {
    while (wrapper->idx - start_idx < width) {
      wrapper->out(wrapper, ' ');
    }
  }
}


// internal itoa format
static void _ntoa_format(struct printf_wrapper *wrapper, CHAR_TYPE* buf, size_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
    while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width))) {
      len--;
      if (len && (base == 16U)) {
        len--;
      }
    }
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'x';
    }
    else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'X';
    }
    else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'b';
    }
    if (len < PRINTF_NTOA_BUFFER_SIZE) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_NTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  _out_rev(wrapper, buf, len, width, flags);
}


// internal itoa for 'long' type
static void _ntoa_long(struct printf_wrapper *wrapper, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  CHAR_TYPE buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const CHAR_TYPE digit = (CHAR_TYPE)(value % base);
      buf[len++] = (char16_t) ((digit < 10) ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10);
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  _ntoa_format(wrapper, buf, len, negative, (unsigned int)base, prec, width, flags);
}


// internal itoa for 'long long' type
#if defined(PRINTF_SUPPORT_LONG_LONG)
static void _ntoa_long_long(struct printf_wrapper *wrapper, unsigned long long value, bool negative, unsigned long long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  CHAR_TYPE buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const CHAR_TYPE digit = (CHAR_TYPE)(value % base);
      buf[len++] = (char16_t)  ((digit < 10) ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10);
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  _ntoa_format(wrapper, buf, len, negative, (unsigned int)base, prec, width, flags);
}
#endif  // PRINTF_SUPPORT_LONG_LONG


#if defined(PRINTF_SUPPORT_FLOAT)

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// forward declaration so that _ftoa can switch to exp notation for values > PRINTF_MAX_FLOAT
static void _etoa(struct printf_wrapper *wrapper, double value, unsigned int prec, unsigned int width, unsigned int flags);
#endif


// internal ftoa for fixed decimal floating point
static void _ftoa(struct printf_wrapper *wrapper, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
  CHAR_TYPE buf[PRINTF_FTOA_BUFFER_SIZE];
  size_t len  = 0U;
  double diff = 0.0;

  // powers of 10
  static const double pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

  // test for special values
  if (value != value)
  {
    _out_rev(wrapper, u"nan", 3, width, flags);
    return;
  }
  if (value < -DBL_MAX)
  {
    _out_rev(wrapper, u"fni-", 4, width, flags);
    return;
  }
  if (value > DBL_MAX)
  {
    _out_rev(wrapper, (flags & FLAGS_PLUS) ? u"fni+" : u"fni", (flags & FLAGS_PLUS) ? 4U : 3U, width, flags);
    return;
  }

  // test for very large values
  // standard printf behavior is to print EVERY whole number digit -- which could be 100s of characters overflowing your buffers == bad
  if ((value > PRINTF_MAX_FLOAT) || (value < -PRINTF_MAX_FLOAT)) {
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
    _etoa(wrapper, value, prec, width, flags);
    return;
#else
    return;
#endif
  }

  // test for negative
  bool negative = false;
  if (value < 0) {
    negative = true;
    value = 0 - value;
  }

  // set default precision, if not set explicitly
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }
  // limit precision to 9, cause a prec >= 10 can lead to overflow errors
  while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U)) {
    buf[len++] = '0';
    prec--;
  }

  int whole = (int)value;
  double tmp = (value - whole) * pow10[prec];
  unsigned long frac = (unsigned long)tmp;
  diff = tmp - frac;

  if (diff > 0.5) {
    ++frac;
    // handle rollover, e.g. case 0.99 with prec 1 is 1.0
    if (frac >= pow10[prec]) {
      frac = 0;
      ++whole;
    }
  }
  else if (diff < 0.5) {
  }
  else if ((frac == 0U) || (frac & 1U)) {
    // if halfway, round up if odd OR if last digit is 0
    ++frac;
  }

  if (prec == 0U) {
    diff = value - (double)whole;
    if ((!(diff < 0.5) || (diff > 0.5)) && (whole & 1)) {
      // exactly 0.5 and ODD, then round up
      // 1.5 -> 2, but 2.5 -> 2
      ++whole;
    }
  }
  else {
    unsigned int count = prec;
    // now do fractional part, as an unsigned number
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
      --count;
      buf[len++] = (CHAR_TYPE)(48U + (frac % 10U));
      if (!(frac /= 10U)) {
        break;
      }
    }
    // add extra 0s
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U)) {
      buf[len++] = '0';
    }
    if (len < PRINTF_FTOA_BUFFER_SIZE) {
      // add decimal
      buf[len++] = '.';
    }
  }

  // do whole part, number is reversed
  while (len < PRINTF_FTOA_BUFFER_SIZE) {
    buf[len++] = (CHAR_TYPE)(48 + (whole % 10));
    if (!(whole /= 10)) {
      break;
    }
  }

  // pad leading zeros
  if (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD)) {
    if (width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_FTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  _out_rev(wrapper, buf, len, width, flags);
}


#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// internal ftoa variant for exponential floating-point type, contributed by Martijn Jasperse <m.jasperse@gmail.com>
static void _etoa(struct printf_wrapper *wrapper, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
  // check for NaN and special values
  if ((value != value) || (value > DBL_MAX) || (value < -DBL_MAX)) {
    _ftoa(wrapper, value, prec, width, flags);
    return;
  }

  // determine the sign
  const bool negative = value < 0;
  if (negative) {
    value = -value;
  }

  // default precision
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }

  // determine the decimal exponent
  // based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
  union {
    uint64_t U;
    double   F;
  } conv;

  conv.F = value;
  int exp2 = (int)((conv.U >> 52U) & 0x07FFU) - 1023;           // effectively log2
  conv.U = (conv.U & ((1ULL << 52U) - 1U)) | (1023ULL << 52U);  // drop the exponent so conv.F is now in [1,2)
  // now approximate log10 from the log2 integer part and an expansion of ln around 1.5
  int expval = (int)(0.1760912590558 + exp2 * 0.301029995663981 + (conv.F - 1.5) * 0.289529654602168);
  // now we want to compute 10^expval but we want to be sure it won't overflow
  exp2 = (int)(expval * 3.321928094887362 + 0.5);
  const double z  = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
  const double z2 = z * z;
  conv.U = (uint64_t)(exp2 + 1023) << 52U;
  // compute exp(z) using continued fractions, see https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
  conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
  // correct for rounding errors
  if (value < conv.F) {
    expval--;
    conv.F /= 10;
  }

  // the exponent format is "%+03d" and largest value is "307", so set aside 4-5 characters
  unsigned int minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

  // in "%g" mode, "prec" is the number of *significant figures* not decimals
  if (flags & FLAGS_ADAPT_EXP) {
    // do we want to fall-back to "%f" mode?
    if ((value >= 1e-4) && (value < 1e6)) {
      if ((int)prec > expval) {
        prec = (unsigned)((int)prec - expval - 1);
      }
      else {
        prec = 0;
      }
      flags |= FLAGS_PRECISION;   // make sure _ftoa respects precision
      // no characters in exponent
      minwidth = 0U;
      expval   = 0;
    }
    else {
      // we use one sigfig for the whole part
      if ((prec > 0) && (flags & FLAGS_PRECISION)) {
        --prec;
      }
    }
  }

  // will everything fit?
  unsigned int fwidth = width;
  if (width > minwidth) {
    // we didn't fall-back so subtract the characters required for the exponent
    fwidth -= minwidth;
  } else {
    // not enough characters, so go back to default sizing
    fwidth = 0U;
  }
  if ((flags & FLAGS_LEFT) && minwidth) {
    // if we're padding on the right, DON'T pad the floating part
    fwidth = 0U;
  }

  // rescale the float value
  if (expval) {
    value /= conv.F;
  }

  // output the floating part
  const size_t start_idx = wrapper->idx;
  _ftoa(wrapper, negative ? -value : value, prec, fwidth, flags & ~FLAGS_ADAPT_EXP);

  // output the exponent part
  if (minwidth) {
    // output the exponential symbol
    wrapper->out(wrapper, (flags & FLAGS_UPPERCASE) ? 'E' : 'e');
    // output the exponent value
    _ntoa_long(wrapper, (unsigned long) ((expval < 0) ? -expval : expval), expval < 0, 10, 0, minwidth-1, FLAGS_ZEROPAD | FLAGS_PLUS);
    // might need to right-pad spaces
    if (flags & FLAGS_LEFT) {
      while (wrapper->idx - start_idx < width) wrapper->out(wrapper, ' ');
    }
  }
}
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT


// internal vsnprintf
static int _vsnprintf(/*out_fct_type out, CHAR_TYPE* buffer, const size_t maxlen*/struct printf_wrapper *wrapper, const CHAR_TYPE* format, va_list va)
{
  unsigned int flags, width, precision, n;
  //size_t idx = 0U;

  if (wrapper->buffer == NULL && wrapper->callback == NULL && wrapper->out != _out_char) wrapper->out = _out_null;

  while (*format)
  {
    // format specifier?  %[flags][width][.precision][length]
    if (*format != '%') {
      // no
      wrapper->out(wrapper, *format);
      format++;
      continue;
    }
    else {
      // yes, evaluate it
      format++;
    }

    // evaluate flags
    flags = 0U;
    do {
      switch (*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      }
    } while (n);

    // evaluate width field
    width = 0U;
    if (_is_digit(*format)) {
      width = _atoi(&format);
    }
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {
        flags |= FLAGS_LEFT;    // reverse padding
        width = (unsigned int)-w;
      }
      else {
        width = (unsigned int)w;
      }
      format++;
    }

    // evaluate precision field
    precision = 0U;
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format)) {
        precision = _atoi(&format);
      }
      else if (*format == '*') {
        const int prec = (int)va_arg(va, int);
        precision = prec > 0 ? (unsigned int)prec : 0U;
        format++;
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        flags |= FLAGS_LONG;
        format++;
        if (*format == 'l') {
          flags |= FLAGS_LONG_LONG;
          format++;
        }
        break;
      case 'h' :
        flags |= FLAGS_SHORT;
        format++;
        if (*format == 'h') {
          flags |= FLAGS_CHAR;
          format++;
        }
        break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
#endif
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    // evaluate specifier
    switch (*format) {
      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        unsigned int base;
        if (*format == 'x' || *format == 'X') {
          base = 16U;
        }
        else if (*format == 'o') {
          base =  8U;
        }
        else if (*format == 'b') {
          base =  2U;
        }
        else {
          base = 10U;
          flags &= ~FLAGS_HASH;   // no hash for dec format
        }
        // uppercase
        if (*format == 'X') {
          flags |= FLAGS_UPPERCASE;
        }

        // no plus or space flag for u, x, X, o, b
        if ((*format != 'i') && (*format != 'd')) {
          flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
        }

        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION) {
          flags &= ~FLAGS_ZEROPAD;
        }

        // convert the integer
        if ((*format == 'i') || (*format == 'd')) {
          // signed
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const long long value = va_arg(va, long long);
            _ntoa_long_long(wrapper, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            _ntoa_long(wrapper, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? (CHAR_TYPE)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
            _ntoa_long(wrapper, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {
          // unsigned
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            _ntoa_long_long(wrapper, va_arg(va, unsigned long long), false, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            _ntoa_long(wrapper, va_arg(va, unsigned long), false, base, precision, width, flags);
          }
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? (UCHAR_TYPE)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
            _ntoa_long(wrapper, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_FLOAT)
      case 'f' :
      case 'F' :
        if (*format == 'F') flags |= FLAGS_UPPERCASE;
        _ftoa(wrapper, va_arg(va, double), precision, width, flags);
        format++;
        break;
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        _etoa(wrapper, va_arg(va, double), precision, width, flags);
        format++;
        break;
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT
      case 'c' : {
        unsigned int l = 1U;
        // pre padding
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            wrapper->out(wrapper, ' ');
          }
        }
        // CHAR_TYPE output
        wrapper->out(wrapper, (CHAR_TYPE)va_arg(va, int));
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            wrapper->out(wrapper, ' ');
          }
        }
        format++;
        break;
      }

      case 's' : {
        const CHAR_TYPE* p = va_arg(va, CHAR_TYPE*);
        unsigned int l = _strnlen_s(p, precision ? precision : (size_t)-1);
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            wrapper->out(wrapper, ' ');
          }
        }
        // string output
        while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          wrapper->out(wrapper, *(p++));
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            wrapper->out(wrapper, ' ');
          }
        }
        format++;
        break;
      }

      case 'p' : {
        width = sizeof(void*) * 2U;
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
        if (is_ll) {
          _ntoa_long_long(wrapper, (uintptr_t)va_arg(va, void*), false, 16U, precision, width, flags);
        }
        else {
#endif
          _ntoa_long(wrapper, (unsigned long)((uintptr_t)va_arg(va, void*)), false, 16U, precision, width, flags);
#if defined(PRINTF_SUPPORT_LONG_LONG)
        }
#endif
        format++;
        break;
      }

      case '%' :
        wrapper->out(wrapper, '%');
        format++;
        break;

      default :
        wrapper->out(wrapper, *format);
        format++;
        break;
    }
  }

  // termination
  wrapper->out(wrapper, (CHAR_TYPE)0);

  // return written chars without terminating \0
  return (int)wrapper->idx - 1;
}


///////////////////////////////////////////////////////////////////////////////

int printf_(const CHAR_TYPE* format, ...)
{
  va_list va;
  va_start(va, format);
  struct printf_wrapper wrapper = { _out_char, NULL, NULL, NULL, 0, 0 };
  const int ret = _vsnprintf(&wrapper, format, va);
  va_end(va);
  return ret;
}


int sprintf_(CHAR_TYPE* buffer, const CHAR_TYPE* format, ...)
{
  va_list va;
  va_start(va, format);
  struct printf_wrapper wrapper = { _out_buffer, NULL, NULL, buffer, 0, (size_t)-1 };
  const int ret = _vsnprintf(&wrapper, format, va);
  va_end(va);
  return ret;
}


int snprintf_(CHAR_TYPE* buffer, size_t count, const CHAR_TYPE* format, ...)
{
  va_list va;
  va_start(va, format);
  struct printf_wrapper wrapper = { _out_buffer, NULL, NULL, buffer, 0, count };
  const int ret = _vsnprintf(&wrapper, format, va);
  va_end(va);
  return ret;
}


int sncatprintf_(CHAR_TYPE* buffer, size_t count, const CHAR_TYPE* format, ...)
{
  CHAR_TYPE *p = buffer;

  // move cursor to the end of the string
  while (*p != 0 && p < buffer + count) ++p;
  count = count - (size_t) (p - buffer);

  va_list va;
  va_start(va, format);
  struct printf_wrapper wrapper = { _out_buffer, NULL, NULL, p, 0, count };
  const int ret = _vsnprintf(&wrapper, format, va);
  va_end(va);
  return ret;
}


int vprintf_(const CHAR_TYPE* format, va_list va)
{
  struct printf_wrapper wrapper = { _out_char, NULL, NULL, NULL, 0, 0 };
  return _vsnprintf(&wrapper, format, va);
}


int vsnprintf_(CHAR_TYPE* buffer, size_t count, const CHAR_TYPE* format, va_list va)
{
  struct printf_wrapper wrapper = { _out_buffer, NULL, NULL, buffer, 0, count };
  return _vsnprintf(&wrapper, format, va);
}


int cprintf(cprintf_callback_t callback, void* arg, const CHAR_TYPE* format, ...)
{
  va_list va;
  va_start(va, format);
  struct printf_wrapper wrapper = { _out_fct, callback, arg, NULL, 0, 0 };
  const int ret = _vsnprintf(&wrapper, format, va);
  va_end(va);
  return ret;
}

