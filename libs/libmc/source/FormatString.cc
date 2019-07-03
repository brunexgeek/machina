/*
 *    Copyright 2016 Bruno Ribeiro
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <sys/types.h>
#include <mc/stdarg.h>
#include <mc/string.h>


/**
 * @brief Left-justify within the given field width.
 *
 * Right justification is the default (see width sub-specifier).
 */
#define FLAG_LEFT            (0x10U)

/**
 * @brief Forces to preceed the result with a plus or minus sign (+ or -)
 * even for positive numbers.
 *
 * By default, only negative numbers are preceded with a - sign.
 */
#define FLAG_SIGNAL          (0x04U)

/**
 * @brief If no sign is going to be written, a blank space is inserted before the value.
 */
#define FLAG_SPACE           (0x08U)

/**
 * @brief Format-specific customization.
 */
#define FLAG_CUSTOM          (0x20U)

/**
 * @brief Left-pads the number with zeroes (0) instead of spaces when padding
 * is specified (see width sub-specifier).
 */
#define FLAG_ZERO            (0x01)

#define FLAG_SIGNED          (0x02)

#define FLAG_UPPER           (0x40)

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static const char16_t *LOWER_CHARACTERS = u"0123456789abcdefghijklmnopqrstuvwxyz";
static const char16_t *UPPER_CHARACTERS = u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


struct FormatInfo
{
	uint32_t flags;
	int32_t precision;
	int32_t width;
	char16_t specifier;
};


static const char16_t *stringToNumber(
	const char16_t *text,
	int32_t &value )
{
	value = 0;
	while (is_digit(*text))
		value = value * 10 + *(text++) - (char16_t) '0';
	return text;
}

static char16_t *numberToString(
	char16_t *output,
	size_t value,
	int base,
	int size,
	int precision,
	int type)
{
	char16_t sign = u'\0';
	const char16_t *digits = LOWER_CHARACTERS;

	if (type & FLAG_UPPER)
		digits = UPPER_CHARACTERS;
	if (type & FLAG_LEFT)
		type &= ~FLAG_ZERO;
	if (base < 2 || base > 36)
		return nullptr;

	// detects the filler character
	char16_t filler = (type & FLAG_ZERO) ? u'0' : u' ';
	// checks if we need to print the sign
	if (type & FLAG_SIGNED)
	{
		if ( (ssize_t) value < 0)
		{
			sign = u'-';
			value =  (size_t) -( (ssize_t) value );
			size--;
		}
		else if (type & FLAG_SIGNAL)
		{
			sign = u'+';
			size--;
		}
		else if (type & FLAG_SPACE)
		{
			sign = ' ';
			size--;
		}
	}
	// checks if we have some customization
	if (type & FLAG_CUSTOM)
	{
		if (base == 16)
		{
			// we need room for the "0x"
			size -= 2;
		}
		else if (base == 8)
		{
			// we need room for the "0"
			size--;
		}
	}

	// converts the value to string
	int i = 0;
	char16_t tmp[24] = { u'0' };
	if (value > 0)
	{
		while (value != 0)
		{
			tmp[i++] = digits[ value % base ];
			value = value / base;
		}
	}
	else
		++i;

	if (i > precision) precision = i;
	size -= precision;

	// checks if we need to insert leading zeros
	if (!(type & FLAG_ZERO) && !(type & FLAG_LEFT))
		while (size-- > 0) *output++ = ' ';
	// insert the sign
	if (sign != 0) *output++ = sign;
	// checks if we need to insert the custom prefix
	if (type & FLAG_CUSTOM)
	{
		if (base == 8)
		{
			*output++ = '0';
		}
		else if (base == 16)
		{
			*output++ = '0';
			*output++ = digits[33];
		}
	}

	// inserts filler characters
	if ((type & FLAG_LEFT) == 0)
		for ( ; size-- > 0; *output++ = filler );
	// put precision zeros
	while (i < precision--) *output++ = '0';
	// put the converted number
	while (i-- > 0) *output++ = tmp[i];
	// put the remaining spaces
	while (size-- > 0) *output++ = ' ';

	return output;
}


const char16_t *parseFormat(
	const char16_t *format,
	va_list args,
	FormatInfo &info )
{
	if (*format == '%') ++format;

	info.flags = 0;
	info.width = -1;
	info.precision = -1;
	info.specifier = 0;

loop:
	switch (*format++)
	{
		case '-':
			info.flags |= FLAG_LEFT;
			goto loop;
		case '+':
			info.flags |= FLAG_SIGNAL;
			goto loop;
		case ' ':
			info.flags |= FLAG_SPACE;
			goto loop;
		case '#':
			info.flags |= FLAG_CUSTOM;
			goto loop;
		case '0':
			info.flags |= FLAG_ZERO;
			goto loop;
		default:
			--format;
			break;
	}

	// parse the output width
	if (is_digit(*format))
		format = stringToNumber(format, info.width);
	else
	if (*format == '*')
	{
		format++;
		// get the width from the argument list (one integer)
		info.width = va_arg(args, int);
		if (info.width < 0)
		{
			// if the width is a negative value, the output is
			// supposed to be left aligned
			info.width = -(info.width);
			info.flags |= FLAG_LEFT;
		}
		if (info.width > 0x3F000000)
			info.width = 0x3F000000;
	}

	// Note: We only support integers, so precision specifies the minimum number
	//       of digits to be written. If the value to be written is shorter than
	//       this number, the result is padded with leading zeros. The value is
	//       not truncated even if the result is longer. A precision of 0 means
	//       that no character is written for the value 0.

	// parse the precision
	if (*format == '.')
	{
		++format;
		if (is_digit(*format))
			format = stringToNumber(format, info.precision);
		else
		if (*format == '*')
		{
			++format;
			// get the width from the argument list (one integer)
			info.precision = va_arg(args, int);
		}
		// the precision can not be negative
		if (info.precision < 0)
			info.precision = 0;
		else
		if (info.precision > 0x3F000000)
			info.precision = 0x3F000000;
	}

	// Note: we do not support 'length' sub-specifier (which would modify the
	//       length of the data type).

	// get the specifier (e.g. 'd', 'x', 's')
	info.specifier = *format++;

	return format;
}


int FormatStringEx(
	char16_t *output,
	size_t outputSize,
	const char16_t *format,
	va_list args )
{
	if (output == nullptr || outputSize <= 1)
		return 0;

	int base;
	char16_t *current;

	for (current = output; *format && outputSize;)
	{
		// copy ordinary characters
		if (*format != '%')
		{
			*current++ = *format++;
			--outputSize;
			continue;
		}
		else
			++format;

		// parse the format string
		FormatInfo info;
		format = parseFormat(format, args, info);

		if (info.specifier == 'c')
		{
			--info.width;

			if (info.flags & FLAG_LEFT)
				for (; info.width-- > 0 && outputSize; *current++ = ' ', outputSize--);

			*current++ = (char16_t) va_arg(args, int);

			for (; info.width-- > 0 && outputSize; *current++ = ' ', outputSize--);
		}
		else
		if (info.specifier == 's')
		{
			const char16_t *text = va_arg(args, char16_t *);
			if (!text)
				text = u"<NULL>";

			int32_t length = (int32_t) StringLengthEx(text, (size_t) info.precision);
			info.width -= length;

			if ((info.flags & FLAG_LEFT) == 0)
				for (; info.width-- > 0 && outputSize; *current++ = ' ', outputSize--);

			for (int32_t i = 0; i < length; ++i)
				*current++ = *text++;

			for (; info.width-- > 0 && outputSize; *current++ = ' ', outputSize--);
		}
		else
		{
			base = 10;

			switch (info.specifier)
			{
				case 'p':
					if (info.width == -1)
					{
						info.width = 2 * sizeof(void *);
						info.flags |= FLAG_ZERO;
					}
					current = numberToString(current, (size_t) va_arg(args, void *), 16, info.width, info.precision, info.flags);
					continue;

				case 'o':
					base = 8;
					break;

				case 'X':
					info.flags |= FLAG_UPPER;
					// fall through

				case 'x':
					base = 16;
					break;

				case 'd':
				case 'i':
					info.flags |= FLAG_SIGNAL;
					// fall through

				case 'u':
					break;

				default:
					if (*format == '%')
						*current++ = '%';
					else
					if (*format)
						*current++ = *format;
					else
						--format;
					continue;
			}

			size_t value;
			if (info.flags & FLAG_SIGNAL)
				value = va_arg(args, ssize_t);
			else
				value = va_arg(args, size_t);

			current = numberToString(current, value, base, info.width, info.precision, info.flags);
		}
	}

	*current = '\0';
	return current - output;
}


int FormatString(
	char16_t *buffer,
	size_t size,
	const char16_t *format,
	... )
{
	va_list args;

	va_start(args, format);
	int count = FormatStringEx(buffer, size, format, args);
	va_end(args);

	return count;
}